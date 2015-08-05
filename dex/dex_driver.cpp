/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#include "anablock.h"
#include "anatypes.h"
#include "anaopcode.h"
#include "ananode.h"
#include "anadot.h" //for debug
#include "anaglobal.h"
#include "anamethod.h"
#include "anamem.h"
#include "anautility.h"
#include "anaprofile.h"
#include "anainterface.h"

#include "libdex/DexFile.h"
#include "libdex/DexClass.h"
#include "liropcode.h"
#include "drAlloc.h"
#include "d2d_comm.h"

#include "../opt/cominc.h"

#include "tcomf.h"
#include "anacomf.h"
#include "dx_mgr.h"
#include "aoc_dx_mgr.h"

#include "../opt/prdf.h"
#include "dex.h"
#include "gra.h"

#include "dex_util.h"
#include "dex2ir.h"
#include "ir2dex.h"
#include "d2d_l2d.h"
#include "dex_driver.h"

//Ensure RETURN IR at the end of function if its return-type is VOID.
static void add_ret(IR * irs, REGION * ru)
{
	IR * last = get_last(irs);
	if (last != NULL && IR_type(last) != IR_RETURN) {
		IR * ret = ru->build_return(NULL);
		if (irs == NULL) {
			irs = ret;
		} else {
			insertafter_one(&last, ret);
		}
	}
}


static void update_fu(LIRCode * fu, DEX2IR & d2ir, IR2DEX & ir2d)
{
	TRY_INFO * ti = d2ir.get_try_info();
	LAB2UINT * lab2idx = ir2d.get_lab2idx();
	if (ti != NULL) {
		IS_TRUE0(fu->trys);
		UINT i = 0;
		for (; ti != NULL; ti = ti->next, i++) {
			bool find;
			UINT idx = lab2idx->get(ti->try_start, &find);
			IS_TRUE0(find);
			LIROpcodeTry * each_try = fu->trys + i;
			each_try->start_pc = idx;

			idx = lab2idx->get(ti->try_end, &find);
			IS_TRUE0(find);

			/*
			The means of value of try end position is not
			same as try start position.
			e.g:
				0th: iget
				1th: add
				2th: move
				iget and add belong to try block, and move is not.
				try-start is 0th, try-end is 2th.
				So the last try-end position (if has) without lir
				corresponding to.
			*/
			each_try->end_pc = idx;
			//IS_TRUE0(each_try->end_pc > each_try->start_pc);

			UINT j = 0;
			for (CATCH_INFO * ci = ti->catch_list;
				 ci != NULL; ci = ci->next, j++) {
				IS_TRUE0(each_try->catches);
				LIROpcodeCatch * each_catch = each_try->catches + j;

				UINT idx = lab2idx->get(ci->catch_start, &find);
				IS_TRUE0(find);
				each_catch->handler_pc = idx;
				each_catch->class_type = ci->kind;
			}
			if (ti->catch_list != NULL) {
				IS_TRUE0(j == each_try->catchSize);
			}
		}
		IS_TRUE0(i == fu->triesSize);
	}
}


static IR * do_opt_only_test(IR * ir_list, DEX_REGION * func_ru,
				   DEX2IR & d2ir, OUT PRNO2UINT & prno2v,
				   TYIDR & tr, UINT numargs, UINT maxvars)
{
	LOG("\t\tdo opt test: '%s'", func_ru->get_ru_name());
	prno2v.clean();
	prno2v.copy(*d2ir.get_pr2v_map());
	return ir_list;
}



static IR * do_opt(IR * ir_list, DEX_REGION * func_ru,
				   DEX2IR & d2ir, PRNO2UINT & prno2v,
				   TYIDR & tr, UINT numargs, UINT maxvars)
{
	if (ir_list == NULL) { return NULL; }

	//dump_irs(ir_list, func_ru->get_dm());

	//It is worth to do.
	//If the RETURN is redundant, the cfs-opt module would remove it.
	//add_ret(ir_list, func_ru);

	bool change;

	RF_CTX rf;

	//Do not insert cvt for DEX code to avoid smash the code sanity.
	RC_insert_cvt(rf) = false;

	//ir_list = func_ru->refine_ir_list(ir_list, change, rf);

	verify_irs(ir_list, NULL, func_ru->get_dm());

	func_ru->add_to_ir_list(ir_list);

	#if 0
	func_ru->process(prno2v, numargs, maxvars,
					 d2ir.get_v2pr_map(), d2ir.get_pr2v_map(), &tr);
	#else
	func_ru->process_simply(prno2v, numargs, maxvars, d2ir,
					 		d2ir.get_v2pr_map(), d2ir.get_pr2v_map(), &tr);
	#endif
	ir_list = func_ru->construct_ir_list(true);

	return ir_list;
}


static bool handle_region(IN DEX_REGION * func_ru, IN DexFile * df,
						  IN LIRCode * fu, D2Dpool * fupool,
						  IN DexMethod const* dexm)
{
	TYIDR tr;
	DT_MGR * dm = func_ru->get_dm();
	tr.i8 = dm->get_simplex_tyid(D_I8);
	tr.u8 = dm->get_simplex_tyid(D_U8);
	tr.i16 = dm->get_simplex_tyid(D_I16);
	tr.u16 = dm->get_simplex_tyid(D_U16);
	tr.i32 = dm->get_simplex_tyid(D_I32);
	tr.u32 = dm->get_simplex_tyid(D_U32);
	tr.i64 = dm->get_simplex_tyid(D_I64);
	tr.u64 = dm->get_simplex_tyid(D_U64);
	tr.f32 = dm->get_simplex_tyid(D_F32);
	tr.f64 = dm->get_simplex_tyid(D_F64);
	tr.b = dm->get_simplex_tyid(D_B);
	tr.ptr = dm->get_ptr_tyid(OBJ_MC_SIZE);

	DEX2IR d2ir(func_ru, df, fu, &tr);
	bool succ;
	IR * ir_list = d2ir.convert(&succ);
	if (!succ) {
		return false;
	}

	if (d2ir.has_catch()) {
		return false;
	}

	PRNO2UINT prno2v(get_nearest_power_of_2(
			d2ir.get_pr2v_map()->get_elem_count() + 1));
	#if 1
	ir_list = do_opt(ir_list, func_ru, d2ir, prno2v, tr, fu->numArgs, fu->maxVars);
	#else
	ir_list = do_opt_only_test(ir_list, func_ru, d2ir, prno2v, tr, fu->numArgs, fu->maxVars);
	#endif

	IR2DEX ir2dex(func_ru, df, &d2ir, &tr, &prno2v);
	LIST<LIR*> newlirs;
	ir2dex.convert(ir_list, newlirs, prno2v);

	//to LIRCode.
	UINT u = newlirs.get_elem_count();
	fu->instrCount = u;
	fu->lirList = (LIR**)LIRMALLOC(u * sizeof(LIR*));
	fu->maxVars = prno2v.maxreg + 1;
	IS_TRUE0(fu->numArgs == prno2v.paramnum);
	memset(fu->lirList, 0, u * sizeof(LIR*));
	UINT i = 0;
	for (LIR * l = newlirs.get_head(); l != NULL; l = newlirs.get_next()) {
		IS_TRUE0(l);
		fu->lirList[i++] = l;
	}
	update_fu(fu, d2ir, ir2dex);
	DexCode const* dexcode = dexGetCode(df, dexm);

	//to dex code buffer.
	DexCode x;
	memset(&x, 0, sizeof(DexCode));
	CBSHandle cbs = transformCode(fu, &x);
	DexCode * newone = writeCodeItem(fupool, cbs,
									 x.registersSize, x.insSize,
									 dexcode->outsSize, x.triesSize,
									 dexcode->debugInfoOff, x.insnsSize);
	AOC_DX_MGR adm(df);
	DX_INFO dxinfo;
	adm.extract_dxinfo(dxinfo,
					   newone->insns,
					   newone->insnsSize,
					   NULL, &dexm->methodIdx);
	adm.dump_method(dxinfo, g_tfile);
	return true;
}


int debug2()
{
	return 1;
}


static int pcount = 0;
int xdebug()
{
	pcount++;
	return 1;
}


void logd_fu_param(CHAR const* runame, LIRCode * fu)
{
	CHAR * buf = (CHAR*)ALLOCA(fu->numArgs * 10 + 100 + strlen(runame));
	sprintf(buf, "\nxoc copmile:%s, count=%d maxreg:%d ", runame, pcount, fu->maxVars - 1);
	CHAR * p = buf + strlen(buf);
	if (fu->maxVars > 0) {
		strcat(p, "(");
		p += strlen(p);
		for (INT i = fu->maxVars - fu->numArgs; i < (INT)fu->maxVars; i++) {
			IS_TRUE0(i >= 0);
			if (i != (INT)fu->maxVars - 1) {
				sprintf(p, "v%d,", i);
			} else {
				sprintf(p, "v%d", i);
			}
			p += strlen(p);
		}
		strcat(p, ")");
	}
	LOG("%s ", buf);
}


bool g_dd = false;

static void dump_all_method_name(CHAR const* runame)
{
	FILE * h = fopen("method.log", "a+");
	fprintf(h, "\n%s", runame);
	fclose(h);
}


//Return true if compilation is successful.
bool compile_func(D2Dpool * fupool, LIRCode * fu,
				  DexFile * df, DexMethod const* dexm)
{
	CHAR tmp[256];
	CHAR * runame = NULL;
	UINT len = strlen(get_class_name(df, dexm)) +
			   strlen(get_func_name(df, dexm)) + 10;
	if (len < 256) { runame = tmp; }
	else {
		runame = (CHAR*)ALLOCA(len);
		IS_TRUE0(runame);
	}

	sprintf(runame, "%s::%s",
			get_class_name(df, dexm), get_func_name(df, dexm));

	g_do_ssa = false;

	#ifdef _VMWARE_DEBUG_
	//initdump("a.tmp", true);
	#endif

	g_opt_level = OPT_LEVEL3;

	IS_TRUE0(df && dexm);

	//dump_all_class_and_field(df);
	//dump_all_lir(fu, df, dexm);

	DEX_REGION_MGR * rm = new DEX_REGION_MGR();

	rm->init_var_mgr();

	//Generate Program region.
	DEX_REGION * func_ru = (DEX_REGION*)rm->new_region(RU_FUNC);

	func_ru->set_ru_var(rm->get_var_mgr()->register_var(
						runame, D_MC, D_UNDEF,
						0, 0, 0, VAR_GLOBAL|VAR_FAKE));

	handle_region(func_ru, df, fu, fupool, dexm);

	//dump_all_lir(fu, df, dexm);

	delete func_ru;
	delete rm;

	#ifdef _DEBUG_
	finidump();
	#endif

	return true;
}
