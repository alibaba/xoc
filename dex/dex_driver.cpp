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
#include "libdex/DexFile.h"
#include "libdex/DexClass.h"
#include "liropcode.h"
#include "drAlloc.h"
#include "d2d_comm.h"

#include "../opt/cominc.h"

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
static void add_ret(IR * irs, Region * ru)
{
	IR * last = get_last(irs);
	if (last != NULL && IR_type(last) != IR_RETURN) {
		IR * ret = ru->buildReturn(NULL);
		if (irs == NULL) {
			irs = ret;
		} else {
			insertafter_one(&last, ret);
		}
	}
}


//Upate try-catch info if any.
static void updateLIRCode(LIRCode * lircode, Dex2IR & d2ir, IR2Dex & ir2d)
{
	TRY_INFO * ti = d2ir.get_try_info();
	LAB2UINT * lab2idx = ir2d.get_lab2idx();
	if (ti != NULL) {
		ASSERT0(lircode->trys);
		UINT i = 0;
		for (; ti != NULL; ti = ti->next, i++) {
			bool find;
			UINT idx = lab2idx->get(ti->try_start, &find);
			ASSERT0(find);
			LIROpcodeTry * each_try = lircode->trys + i;
			each_try->start_pc = idx;

			idx = lab2idx->get(ti->try_end, &find);
			ASSERT0(find);

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
			//ASSERT0(each_try->end_pc > each_try->start_pc);

			UINT j = 0;
			for (CATCH_INFO * ci = ti->catch_list;
				 ci != NULL; ci = ci->next, j++) {
				ASSERT0(each_try->catches);
				LIROpcodeCatch * each_catch = each_try->catches + j;

				UINT idx = lab2idx->get(ci->catch_start, &find);
				ASSERT0(find);
				each_catch->handler_pc = idx;
				each_catch->class_type = ci->kind;
			}

			if (ti->catch_list != NULL) {
				ASSERT0(j == each_try->catchSize);
			}
		}
		ASSERT0(i == lircode->triesSize);
	}
}


static IR * do_opt_only_test(IR * ir_list, DexRegion * func_ru,
				   Dex2IR & d2ir, OUT Prno2UINT & prno2v,
				   TypeIndexRep & tr, UINT numargs, UINT maxvars)
{
	LOG("\t\tdo pass test: '%s'", func_ru->get_ru_name());
	prno2v.clean();
	prno2v.copy(*d2ir.get_pr2v_map());
	return ir_list;
}



static IR * do_opt(IR * ir_list, DexRegion * func_ru,
				   Dex2IR & d2ir, Prno2UINT & prno2v,
				   TypeIndexRep & tr, UINT numargs, UINT maxvars)
{
	if (ir_list == NULL) { return NULL; }

	//dump_irs(ir_list, func_ru->get_dm());

	//It is worth to do.
	//If the RETURN is redundant, the cfs-pass module would remove it.
	//add_ret(ir_list, func_ru);

	bool change;

	RefineCTX rf;

	//Do not insert cvt for DEX code to avoid smash the code sanity.
	RC_insert_cvt(rf) = false;

	//ir_list = func_ru->refineIRlist(ir_list, change, rf);

	verify_irs(ir_list, NULL, func_ru);

	func_ru->addToIRlist(ir_list);

	#if 0
	func_ru->process(prno2v, numargs, maxvars,
					 d2ir.get_v2pr_map(), d2ir.get_pr2v_map(), &tr);
	#else
	func_ru->processSimply(prno2v, numargs, maxvars, d2ir,
					 		d2ir.get_v2pr_map(), d2ir.get_pr2v_map(), &tr);
	#endif
	ir_list = func_ru->constructIRlist(true);

	return ir_list;
}


/* Convert LIR code to DEX code and store it to
fupool code buffer.

df: dex file handler.
dexm: record the dex-method information.
fu: record the LIR code buffer and related information.
fupool: record the transformed DEX code. */
static void convertLIR2Dex(IN DexFile * df,
							IN DexMethod const* dexm,
							IN LIRCode * lircode,
							OUT D2Dpool * fupool)
{
	DexCode const* dexcode = dexGetCode(df, dexm);
	DexCode x; //only for local used.
	memset(&x, 0, sizeof(DexCode));

	//Transform LIR to DEX.
	CBSHandle transformed_dex_code = transformCode(lircode, &x);

	//Record in fupool.
	DexCode * newone = writeCodeItem(fupool,
									transformed_dex_code,
									x.registersSize,
									x.insSize,
									dexcode->outsSize,
									x.triesSize,
									dexcode->debugInfoOff,
									x.insnsSize);

	#if 0
	//#ifdef _DEBUG_
	//For dump purpose.
	AocDxMgr adm(df);
	DX_INFO dxinfo;
	adm.extract_dxinfo(dxinfo,
					   newone->insns,
					   newone->insnsSize,
					   NULL, &dexm->methodIdx);
	adm.dump_method(dxinfo, g_tfile);
	#endif
}


//Convert IR to LIR code. Store the generated IR list
//at lircode buffer.
void convertIR2LIR(IN IR * ir_list,
					IN DexRegion * func_ru,
					IN DexFile * df,
					IN Dex2IR & d2ir,
					IN TypeIndexRep & tr,
					IN Prno2UINT & prno2v,
					OUT LIRCode * lircode)
{
	IR2Dex ir2dex(func_ru, df, &d2ir, &tr, &prno2v);
	List<LIR*> newlirs;
	ir2dex.convert(ir_list, newlirs, prno2v);

	//to LIRCode.
	UINT u = newlirs.get_elem_count();
	lircode->instrCount = u;
	lircode->lirList = (LIR**)LIRMALLOC(u * sizeof(LIR*));
	lircode->maxVars = prno2v.maxreg + 1;
	ASSERT0(lircode->numArgs == prno2v.paramnum);
	memset(lircode->lirList, 0, u * sizeof(LIR*));
	UINT i = 0;
	for (LIR * l = newlirs.get_head(); l != NULL; l = newlirs.get_next()) {
		ASSERT0(l);
		lircode->lirList[i++] = l;
	}
	updateLIRCode(lircode, d2ir, ir2dex);
}


static bool handleRegion(IN DexRegion * func_ru,
						  IN DexFile * df,
						  IN OUT LIRCode * lircode,
						  D2Dpool * fupool,
						  IN DexMethod const* dexm)
{
	TypeIndexRep tr;
	TypeMgr * dm = func_ru->get_dm();
	tr.i8 = dm->getSimplexType(D_I8);
	tr.u8 = dm->getSimplexType(D_U8);
	tr.i16 = dm->getSimplexType(D_I16);
	tr.u16 = dm->getSimplexType(D_U16);
	tr.i32 = dm->getSimplexType(D_I32);
	tr.u32 = dm->getSimplexType(D_U32);
	tr.i64 = dm->getSimplexType(D_I64);
	tr.u64 = dm->getSimplexType(D_U64);
	tr.f32 = dm->getSimplexType(D_F32);
	tr.f64 = dm->getSimplexType(D_F64);
	tr.b = dm->getSimplexType(D_B);
	tr.ptr = dm->getPointerType(OBJ_MC_SIZE);
	tr.array = dm->getPointerType(ARRAY_MC_SIZE);

	Dex2IR d2ir(func_ru, df, lircode, &tr);
	bool succ;
	IR * ir_list = d2ir.convert(&succ);
	if (!succ) {
		return false;
	}

	if (d2ir.hasCatch()) {
		return false;
	}

	Prno2UINT prno2v(getNearestPowerOf2(
			d2ir.get_pr2v_map()->get_elem_count() + 1));
	//dump_irs(ir_list, func_ru->get_dm());
	#if 0
	ir_list = do_opt(ir_list, func_ru, d2ir, prno2v,
					 tr, lircode->numArgs, lircode->maxVars);
	#else
	ir_list = do_opt_only_test(ir_list, func_ru, d2ir, prno2v,
					tr, lircode->numArgs, lircode->maxVars);
	#endif

	convertIR2LIR(ir_list, func_ru, df, d2ir, tr, prno2v, lircode);
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
			ASSERT0(i >= 0);
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
bool is_compile(CHAR const* runame, LIRCode * fu)
{
	xdebug();
	logd_fu_param(runame, fu);
	g_dd = 0; //set to 1 to open dex2ir.log ir2dex.log
	return true;
}


static void dump_all_method_name(CHAR const* runame)
{
	FILE * h = fopen("method.log", "a+");
	fprintf(h, "\n%s", runame);
	fclose(h);
}


//Optimizer for LIR.
//Return true if compilation is successful.
bool compileFunc(OUT D2Dpool * fupool, IN LIRCode * lircode,
				 IN DexFile * df, DexMethod const* dexm)
{
	CHAR tmp[256];
	CHAR * runame = NULL;
	UINT len = strlen(get_class_name(df, dexm)) +
			   strlen(get_func_name(df, dexm)) + 10;
	if (len < 256) { runame = tmp; }
	else {
		runame = (CHAR*)ALLOCA(len);
		ASSERT0(runame);
	}

	sprintf(runame, "%s::%s",
			get_class_name(df, dexm), get_func_name(df, dexm));

	g_do_ssa = false;

	#ifdef _VMWARE_DEBUG_
	//initdump("a.tmp", true);
	#endif

	g_opt_level = OPT_LEVEL3;

	ASSERT0(df && dexm);

	//dump_all_class_and_field(df);
	//dump_all_lir(lircode, df, dexm);

	DexRegionMgr * rm = new DexRegionMgr();

	rm->initVarMgr();

	//Generate Program region.
	DexRegion * func_ru = (DexRegion*)rm->newRegion(RU_FUNC);

	func_ru->set_ru_var(rm->get_var_mgr()->registerVar(
						runame,
						rm->get_dm()->getMCType(0),
						0, VAR_GLOBAL|VAR_FAKE));

	handleRegion(func_ru, df, lircode, fupool, dexm);

	delete func_ru;
	delete rm;

	//Convert to DEX code and store it to code buffer.
	//convertLIR2Dex(df, dexm, lircode, fupool);

	//dump_all_lir(lircode, df, dexm);

	#ifdef _DEBUG_
	finidump();
	#endif

	return true;
}
