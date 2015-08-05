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
#include "cominc.h"
#include "comopt.h"

//
//START ANA_INS
//
ANA_INS::ANA_INS(REGION * ru) : m_mds_hash(&m_mds_mgr)
{
	m_ru = ru;
	m_ru_mgr = NULL;
	m_inner_region_lst = NULL;
	m_call_list = NULL;

	//Do not use '0' as prno.
	m_pr_count = 1; //counter of IR_PR

	m_ir_list = NULL;
	m_ir_cfg = NULL;
	m_ir_aa = NULL;
	m_ir_du_mgr = NULL;

	m_pool = smpool_create_handle(256, MEM_COMM);
	m_du_pool = smpool_create_handle(sizeof(DU) * 4, MEM_CONST_SIZE);

	memset(m_free_tab, 0, sizeof(m_free_tab));
}


ANA_INS::~ANA_INS()
{
	if (m_ir_cfg != NULL) {
		delete m_ir_cfg;
		m_ir_cfg = NULL;
	}

	//Free IR_AI's internal structure.
	//The vector of IR_AI must be destroyed explicitly.
	INT l = m_ir_vector.get_last_idx();
	for (INT i = 1; i <= l; i++) {
		IR * ir = m_ir_vector.get(i);
		IS_TRUE0(ir);
		if (IR_ai(ir) != NULL) {
			IR_ai(ir)->destroy_vec();
		}
		ir->free_duset(m_sbs_mgr);
	}

	if (m_ir_du_mgr != NULL) {
		delete m_ir_du_mgr;
		m_ir_du_mgr = NULL;
	}

	if (m_ir_aa != NULL) {
		delete m_ir_aa;
		m_ir_aa = NULL;
	}

	//Destroy inner regions.
	if (m_inner_region_lst != NULL) {
		for (REGION * ru = m_inner_region_lst->get_head();
			 ru != NULL; ru = m_inner_region_lst->get_next()) {
			delete ru;
		}
		//Delete it later.
		//m_inner_region_lst->clean();
	}

	//Free md's id and local-var's id back to MD_SYS and VAR_MGR.
	//The index of MD and VAR is important resource if there
	//are a lot of REGIONs in REGION_MGR.
	VAR_MGR * vm = m_ru->get_var_mgr();
	MD_SYS * ms = m_ru->get_md_sys();
	VTAB_ITER c;
	for (VAR * v = m_ru_var_tab.get_first(c);
		 v != NULL; v = m_ru_var_tab.get_next(c)) {
		IS_TRUE0(verify_var(vm, v));
		ms->remove_var(v);
		vm->remove_var(v);
	}

	//Destroy all IR.
	smpool_free_handle(m_pool);
	m_pool = NULL;

	//Destroy all DU structure.
	smpool_free_handle(m_du_pool);
	m_du_pool = NULL;

	m_ir_list = NULL;

	if (m_inner_region_lst != NULL) {
	 	delete m_inner_region_lst;
		m_inner_region_lst = NULL;
	}

	//Destory CALL list.
	if (m_call_list != NULL) {
		delete m_call_list;
		m_call_list = NULL;
	}
}


bool ANA_INS::verify_var(VAR_MGR * vm, VAR * v)
{
	IS_TRUE0(vm->get_var(VAR_id(v)) != NULL);

	if (RU_type(m_ru) == RU_FUNC || RU_type(m_ru) == RU_EH ||
		RU_type(m_ru) == RU_SUB) {
		//If var is global but unallocable, it often be
		//used as placeholder or auxilary var.

		//For these kind of regions, there are only local variable or
		//unablable global variable is legal.
		IS_TRUE0(VAR_is_local(v) || !VAR_allocable(v));
	} else if (RU_type(m_ru) == RU_PROGRAM) {
		//For program region, only global variable is legal.
		IS_TRUE0(VAR_is_global(v));
	} else {
		IS_TRUE(0, ("unsupport variable type."));
	}
	return true;
}


UINT ANA_INS::count_mem()
{
	UINT count = 0;
	if (m_ir_cfg != NULL) {
		count += m_ir_cfg->count_mem();
	}

	if (m_ir_aa != NULL) {
		count += m_ir_aa->count_mem();
	}

	if (m_ir_du_mgr != NULL) {
		count += m_ir_du_mgr->count_mem();
	}

	if (m_inner_region_lst != NULL) {
		for (REGION * ru = m_inner_region_lst->get_head();
			 ru != NULL; ru = m_inner_region_lst->get_next()) {
			count += ru->count_mem();
		}
	}

	if (m_call_list != NULL) {
		count += m_call_list->count_mem();
	}

	count += smpool_get_pool_size_handle(m_pool);
	count += smpool_get_pool_size_handle(m_du_pool);

	count += m_ru_var_tab.count_mem();
	count += m_ir_bb_mgr.count_mem();
	count += m_prno2var.count_mem();
	count += m_bs_mgr.count_mem();
	count += m_sbs_mgr.count_mem();
	count += m_mds_mgr.count_mem();
	count += m_mds_hash.count_mem();
	count += m_ir_vector.count_mem();
	count += m_ir_bb_list.count_mem();
	return count;
}
//END ANA_INS


//
//START REGION
//
//Make sure IR_ICALL is the largest ir.
static bool ck_max_ir_size()
{
	//Change MAX_OFFSET_AT_FREE_TABLE if IR_ICALL is not the largest.
	for (UINT i = IR_UNDEF; i < IR_TYPE_NUM; i++) {
		IS_TRUE0(IRTSIZE(i) <= IRTSIZE(IR_ICALL));
	}
	return true;
}


void REGION::init(REGION_TYPE rt, REGION_MGR * rm)
{
	m_ru_type = rt;
	m_var = NULL;
	id = 0;
	m_parent = NULL;
	m_ref_info = NULL;
	RU_ana(this) = NULL;
	m_u2.s1b1 = 0;
	if (m_ru_type == RU_PROGRAM ||
		m_ru_type == RU_FUNC ||
		m_ru_type == RU_EH ||
		m_ru_type == RU_SUB) {
		//All these REGION involve ir list.
		RU_ana(this) = new ANA_INS(this);
		if (m_ru_type == RU_PROGRAM || m_ru_type == RU_FUNC) {
			//All these REGION involve ir list.
			IS_TRUE0(rm);
			ANA_INS_ru_mgr(RU_ana(this)) = rm;
		}
	}
	IS_TRUE0(ck_max_ir_size());
}


void REGION::destroy()
{
	if ((RU_type(this) == RU_SUB ||
		RU_type(this) == RU_FUNC ||
		RU_type(this) == RU_EH ||
		RU_type(this) == RU_PROGRAM) &&
		RU_ana(this) != NULL) {
		delete RU_ana(this);
	}
	RU_ana(this) = NULL;

	m_var = NULL;

	//MDSET would be freed by MD_SET_MGR.
	m_ref_info = NULL;
	id = 0;
	m_parent = NULL;
	m_ru_type = RU_UNDEF;
}


UINT REGION::count_mem()
{
	UINT count = 0;
	if ((RU_type(this) == RU_SUB ||
		RU_type(this) == RU_FUNC ||
		RU_type(this) == RU_EH ||
		RU_type(this) == RU_PROGRAM) &&
		RU_ana(this) != NULL) {
		count += RU_ana(this)->count_mem();
		count += sizeof(*RU_ana(this));
	}

	if (m_ref_info != NULL) {
		count += m_ref_info->count_mem();
	}
	return count;
}


IR_BB * REGION::new_bb()
{
	IR_BB * bb = get_bb_mgr()->new_bb();
	IR_BB_ru(bb) = this;
	return bb;
}


IR * REGION::build_pr_dedicated(UINT prno, UINT tyid)
{
	IR * ir = new_ir(IR_PR);
	PR_no(ir) = prno;
	IS_TRUE0(get_dm()->get_dtd(tyid));
	IR_dt(ir) = tyid;
	return ir;
}


//Build pseudo-register.
IR * REGION::build_pr(UINT tyid)
{
	IR * ir = new_ir(IR_PR);
	PR_no(ir) = RU_ana(this)->m_pr_count++;
	IS_TRUE0(get_dm()->get_dtd(tyid));
	IR_dt(ir) = tyid;
	return ir;
}


//Generate the number of pseudo-register.
UINT REGION::build_prno(UINT tyid)
{
	IS_TRUE0(get_dm()->get_dtd(tyid));
	return RU_ana(this)->m_pr_count++;
}


IR * REGION::build_logical_not(IR * opnd0)
{
	return build_unary_op(IR_LNOT, D_B, opnd0);
}


//Build op for logical,equality,relation LAND|LOR|XOR.
//NOTICE: opnd1 of LNOT, BNOT should be NULL.
IR * REGION::build_logical_op(IR_TYPE irt, IR * opnd0, IR * opnd1)
{
	IR * ir = new_ir(irt);
	IS_TRUE0(opnd0 && opnd1);
	IS_TRUE0(irt == IR_LAND || irt == IR_LOR || irt == IR_XOR);
	BIN_opnd0(ir) = opnd0;
	BIN_opnd1(ir) = opnd1;
	IR_parent(opnd0) = ir;
	IR_parent(opnd1) = ir;
	IR_dt(ir) = get_dm()->get_simplex_tyid_ex(D_B);
	return ir;
}


IR * REGION::build_id(IN VAR * var)
{
	IS_TRUE0(var);
	IR * ir = new_ir(IR_ID);
	IS_TRUE0(var != NULL);
	ID_info(ir) = var;
	if (VAR_is_pointer(var)) {
		IS_TRUE0(VAR_pointer_base_size(var) > 0);
		IR_dt(ir) = get_dm()->get_ptr_tyid(VAR_pointer_base_size(var));
	} else if (VAR_data_type(var) == D_MC) {
		IR_dt(ir) = get_dm()->get_mc_tyid(VAR_data_size(var),
										  VAR_elem_type(var));
	} else if (IS_SIMPLEX(VAR_data_type(var))) {
		IR_dt(ir) = get_dm()->get_simplex_tyid_ex(VAR_data_type(var));
	} else {
		IS_TRUE0(0);
	}
	return ir;
}


IR * REGION::build_lda(IR * lda_base)
{
	IS_TRUE(lda_base &&
			(IR_type(lda_base) == IR_ID ||
			 IR_type(lda_base) == IR_ARRAY ||
			 IR_type(lda_base) == IR_ILD ||
			 lda_base->is_str(get_dm())),
			("cannot get base of PR"));
	IR * ir = new_ir(IR_LDA);
	LDA_base(ir) = lda_base;
	DT_MGR * dm = get_dm();
	IR_dt(ir) = dm->get_ptr_tyid(lda_base->get_dt_size(dm));
	IR_parent(lda_base) = ir;
	return ir;
}


IR * REGION::build_string(SYM * strtab)
{
	IS_TRUE0(strtab);
	IR * str = new_ir(IR_CONST);
	CONST_str_val(str) = strtab;
	IR_dt(str) = get_dm()->get_simplex_tyid_ex(D_STR);
	return str;
}


//Conditionally selected expression.
//e.g: x = a > b ? 10 : 100
IR * REGION::build_select(IR * det, IR * true_exp, IR * false_exp, UINT tyid)
{
	IS_TRUE0(det && true_exp && false_exp);
	IS_TRUE0(det->is_judge());
	IS_TRUE0(true_exp->is_type_equal(false_exp));
	IR * ir = new_ir(IR_SELECT);
	IR_dt(ir) = tyid;
	SELECT_det(ir) = det;
	SELECT_trueexp(ir) = true_exp;
	SELECT_falseexp(ir) = false_exp;

	IR_parent(det) = ir;
	IR_parent(true_exp) = ir;
	IR_parent(false_exp) = ir;
	return ir;
}


IR * REGION::build_ilabel()
{
	IR * ir = new_ir(IR_LABEL);
	LAB_lab(ir) = gen_ilab();
	return ir;
}


/* This function is just generate conservative memory reference
information of REGION as a default setting. User should supply
more accurate referrence information via various analysis, since
the information will affect optimizations. */
void REGION::gen_default_ref_info()
{
	init_ref_info();

	//For conservative, current region may modify all outer regions variables.
	REGION * t = this;
	MD_ITER iter;
	MD_SYS * md_sys = get_md_sys();
	MD_SET_MGR * md_set_mgr = get_mds_mgr();
	MD_SET * tmp = md_set_mgr->create();
	while (t != NULL) {
		VAR_TAB * vt = t->get_var_tab();
		VTAB_ITER c;
		for (VAR * v = vt->get_first(c); v != NULL; v = vt->get_next(c)) {
			if (VAR_is_fake(v)) { continue; }

			MD_TAB * mdt = md_sys->get_md_tab(v);
			IS_TRUE0(mdt != NULL);

			MD * x = mdt->get_effect_md();
			if (x != NULL) {
				tmp->bunion(x);
			}
			OFST_TAB * ofstab = mdt->get_ofst_tab();
			IS_TRUE0(ofstab);
			if (ofstab->get_elem_count() > 0) {
				iter.clean();
				for (MD * x = ofstab->get_first(iter, NULL);
					 x != NULL; x = ofstab->get_next(iter, NULL)) {
					tmp->bunion(x);
				}
			}
		}
		t = RU_parent(t);
	}

	if (!tmp->is_empty()) {
		REF_INFO_mayuse(RU_ref_info(this))->copy(*tmp);
		REF_INFO_maydef(RU_ref_info(this))->copy(*tmp);
	} else {
		REF_INFO_mayuse(RU_ref_info(this))->clean();
		REF_INFO_maydef(RU_ref_info(this))->clean();
	}
	REF_INFO_mustdef(RU_ref_info(this))->clean();
	md_set_mgr->free(tmp);
}



LABEL_INFO * REGION::gen_ilab()
{
	LABEL_INFO * li = gen_ilab(RU_MGR_label_count(get_ru_mgr()));
	RU_MGR_label_count(get_ru_mgr())++;
	return li;
}


//Generate MD corresponding to PR load or write.
MD * REGION::gen_md_for_pr(IR const* ir)
{
	IS_TRUE0(ir->is_write_pr() || ir->is_read_pr() || ir->is_call());
	return gen_md_for_pr(ir->get_prno(), IR_dt(ir));
}


//Generate MD corresponding to PR load or write.
MD * REGION::gen_md_for_pr(UINT prno, UINT tyid)
{
	DTD const* d = get_dm()->get_dtd(tyid);
	IS_TRUE0(d);
	VAR * pr_var = map_pr2var(prno);
	if (pr_var == NULL) {
		//Create a new PR VAR.
		CHAR name[64];
		sprintf(name, "pr%d", prno);
		IS_TRUE0(strlen(name) < 64);
		UINT flag = VAR_LOCAL;
		SET_FLAG(flag, VAR_IS_PR);
		pr_var = get_var_mgr()->register_var(name, DTD_rty(d),
										D_UNDEF, DTD_ptr_base_sz(d),
										get_dm()->get_dtd_bytesize(d), 0, flag);
		set_map_pr2var(prno, pr_var);

		/* Set the pr-var to be unallocable, means do NOT add
		pr-var immediately as a memory-variable.
		For now, it is only be regarded as a pseduo-register.
		And set it to allocable if the PR is in essence need to be
		allocated in memory. */
		VAR_allocable(pr_var) = false;
		add_to_var_tab(pr_var);
	}

	MD md;
	MD_base(&md) = pr_var; //correspond to VAR
	MD_ofst(&md) = 0;

	if (RU_is_pr_unique_for_same_no(this)) {
		MD_size(&md) = get_dm()->get_simplex_tyid_ex(D_I32);
	} else {
		MD_size(&md) = get_dm()->get_dtd_bytesize(d);
	}

	MD_ty(&md) = MD_EXACT;
	MD * e = get_md_sys()->register_md(md);
	IS_TRUE0(MD_id(e) > 0);
	return e;
}


//Generate MD corresponding to 'ir'.
MD * REGION::gen_md_for_var(IR const* ir)
{
	IS_TRUE0(IR_type(ir) == IR_LD || IR_type(ir) == IR_ID);
	MD md;
	if (IR_type(ir) == IR_LD) { MD_base(&md) = LD_idinfo(ir); }
	else { MD_base(&md) = ID_info(ir); }

	//MD_ofst(&md) = ir->get_ofst();
	MD_size(&md) = ir->get_dt_size(get_dm());
	MD_ty(&md) = MD_EXACT;
	MD * e = get_md_sys()->register_md(md);
	IS_TRUE0(MD_id(e) > 0);
	return e;
}


//Generate MD corresponding to 'ir'.
MD * REGION::gen_md_for_st(IR const* ir)
{
	IS_TRUE0(ir->is_stid());
	MD md;
	MD_base(&md) = ST_idinfo(ir);

	//MD_ofst(&md) = ir->get_ofst();
	MD_size(&md) = ir->get_dt_size(get_dm());
	MD_ty(&md) = MD_EXACT;
	MD * e = get_md_sys()->register_md(md);
	IS_TRUE0(MD_id(e) > 0);
	return e;
}


LABEL_INFO * REGION::gen_ilab(UINT labid)
{
	IS_TRUE0(labid <= RU_MGR_label_count(get_ru_mgr()));
	LABEL_INFO * li = new_ilabel(get_pool());
	LABEL_INFO_num(li) = labid;
	return li;
}


IR * REGION::build_label(LABEL_INFO * li)
{
	IS_TRUE0(li);
	IS_TRUE0(LABEL_INFO_type(li) == L_ILABEL ||
			 LABEL_INFO_type(li) == L_CLABEL);
	IR * ir = new_ir(IR_LABEL);
	LAB_lab(ir) = li;
	return ir;
}


IR * REGION::build_cvt(IR * exp, UINT tgt_tyid)
{
	IS_TRUE0(exp);
	IR * ir = new_ir(IR_CVT);
	CVT_exp(ir) = exp;
	IR_dt(ir) = tgt_tyid;
	IR_parent(exp) = ir;
	return ir;
}


//'res': result pr of PHI.
IR * REGION::build_phi(UINT prno, UINT tyid, UINT num_opnd)
{
	IS_TRUE0(prno > 0);
	IR * ir = new_ir(IR_PHI);
	PHI_prno(ir) = prno;
	IR_dt(ir) = tyid;

	IR * last = NULL;
	for (UINT i = 0; i < num_opnd; i++) {
		IR * x = build_pr_dedicated(prno, tyid);
		PR_ssainfo(x) = NULL;
		add_next(&PHI_opnd_list(ir), &last, x);
		IR_parent(x) = ir;
	}
	return ir;
}


/* 'res_list': reture value list.
'result_prno': indicate the result PR which hold the return value.
	0 means the call does not have a return value.
'tyid': result PR data type.
	0 means the call does not have a return value */
IR * REGION::build_call(VAR * callee, IR * param_list,
						UINT result_prno, UINT tyid)
{
	IS_TRUE0(callee);
	IR * ir = new_ir(IR_CALL);
	CALL_param_list(ir) = param_list;
	CALL_prno(ir) = result_prno;
	CALL_idinfo(ir) = callee;
	IR_dt(ir) = tyid;
	while (param_list != NULL) {
		IR_parent(param_list) = ir;
		param_list = IR_next(param_list);
	}
	return ir;
}


/* 'res_list': reture value list.
'result_prno': indicate the result PR which hold the return value.
	0 means the call does not have a return value.
'tyid': result PR data type.
	0 means the call does not have a return value. */
IR * REGION::build_icall(IR * callee, IR * param_list,
						 UINT result_prno, UINT tyid)
{
	IS_TRUE0(callee);
	IR * ir = new_ir(IR_ICALL);
	IS_TRUE0(IR_type(callee) != IR_ID);
	CALL_param_list(ir) = param_list;
	CALL_prno(ir) = result_prno;
	ICALL_callee(ir) = callee;
	IR_dt(ir) = tyid;

	IR_parent(callee) = ir;
	while (param_list != NULL) {
		IR_parent(param_list) = ir;
		param_list = IR_next(param_list);
	}
	return ir;
}


IR * REGION::build_region(REGION_TYPE rt, VAR * ru_var)
{
	IS_TRUE(get_ru_mgr(), ("Only build a region inside a region of RU_FUNC."));
	IR * ir = new_ir(IR_REGION);
	REGION * ru = get_ru_mgr()->new_region(rt);
	get_inner_region_list()->append_tail(ru);
	REGION_ru(ir) = ru;
	RU_parent(ru) = this;
	ru->set_ru_var(ru_var);
	return ir;
}


/* Build IGOTOH unconditional multi-branch stmt.
'vexp': expression to determine which case entry will be target.
'case_list': case entry list. case entry is consist of expression and label.
*/
IR * REGION::build_igoto(IR * vexp, IR * case_list)
{
	IS_TRUE0(vexp && vexp->is_exp());
	IS_TRUE0(case_list);

	IR * ir = new_ir(IR_IGOTO);
	IGOTO_vexp(ir) = vexp;
	IGOTO_case_list(ir) = case_list;
	IR_parent(vexp) = ir;

	IR * c = case_list;
	while (c != NULL) {
		IS_TRUE0(IR_type(c) == IR_CASE);
		IR_parent(c) = ir;
		c = IR_next(c);
	}
	return ir;
}


IR * REGION::build_goto(LABEL_INFO * li)
{
	IS_TRUE0(li);
	IR * ir = new_ir(IR_GOTO);
	IS_TRUE0(li != NULL);
	GOTO_lab(ir) = li;
	return ir;
}


IR * REGION::build_load(IN VAR * var)
{
	IS_TRUE0(var != NULL);
	UINT tyid = 0;
	if (VAR_is_pointer(var)) {
		IS_TRUE0(VAR_pointer_base_size(var) > 0);
		tyid = get_dm()->get_ptr_tyid(VAR_pointer_base_size(var));
	} else if (VAR_data_type(var) == D_MC) {
		tyid = get_dm()->get_mc_tyid(VAR_data_size(var), VAR_elem_type(var));
	} else if (IS_SIMPLEX(VAR_data_type(var))) {
		tyid = get_dm()->get_simplex_tyid_ex(VAR_data_type(var));
	} else {
		IS_TRUE0(0);
	}
	return build_load(var, tyid);
}


//Load value from variable with type 'tyid'.
//'tyid': result value type.
IR * REGION::build_load(VAR * var, UINT tyid)
{
	IS_TRUE0(var);
	IR * ir = new_ir(IR_LD);
	LD_idinfo(ir) = var;
	IR_dt(ir) = tyid;
	if (g_is_hoist_type) {
		//Hoisting I16/U16/I8/U8 to I32, to utilize whole register.
		DATA_TYPE dt = ir->get_rty(get_dm());
		if (IS_SIMPLEX(dt)) {
			//Hoist data-type from less than INT to INT.
			IR_dt(ir) =
				get_dm()->get_simplex_tyid_ex(get_dm()->hoist_dtype(dt));
		}
	}
	return ir;
}


/*
Result is either register or memory chunk, and the size of ILD
result equals to 'pointer_base_size' of 'addr'.

'base': memory address of ILD.
'ptbase_or_mc_size': if result of ILD is pointer, this parameter records
	pointer_base_size; or if result is memory chunk, it records
	the size of memory chunk.

NOTICE:
	The ofst of ILD requires to maintain when after return.
*/
IR * REGION::build_iload(IR * base, UINT tyid)
{
	IS_TRUE(base && base->is_ptr(get_dm()), ("mem-address of ILD must be pointer"));
	IR * ir = new_ir(IR_ILD);
	IS_TRUE0(get_dm()->get_dtd(tyid));
	IR_dt(ir) = tyid;
	ILD_base(ir) = base;
	IR_parent(base) = ir;
	return ir;
}


IR * REGION::build_iload(IR * base, UINT ofst, UINT tyid)
{
	IR * ir = build_iload(base, tyid);
	ILD_ofst(ir) = ofst;
	return ir;
}


//Build store operation to store 'rhs' to new pr with tyid and prno.
//'prno': target prno.
//'tyid': data type of targe pr.
//'rhs: value expected to store.
IR * REGION::build_store_pr(UINT prno, UINT tyid, IR * rhs)
{
	IS_TRUE0(prno > 0 && tyid > 0 && rhs);
	IR * ir = new_ir(IR_STPR);
	STPR_no(ir) = prno;
	STPR_rhs(ir) = rhs;
	IR_dt(ir) = tyid;
	IR_parent(rhs) = ir;
	return ir;
}


//Build store operation to store 'rhs' to new pr with tyid.
//'tyid': data type of targe pr.
//'rhs: value expected to store.
IR * REGION::build_store_pr(UINT tyid, IR * rhs)
{
	IS_TRUE0(rhs && tyid > 0);
	IS_TRUE0(get_dm()->get_dtd(tyid));
	IR * ir = new_ir(IR_STPR);
	STPR_no(ir) = RU_ana(this)->m_pr_count++;
	STPR_rhs(ir) = rhs;
	IR_dt(ir) = tyid;
	IR_parent(rhs) = ir;
	return ir;
}


//'lhs': memory variable, described target memory location.
//'rhs: value expected to store.
IR * REGION::build_store(VAR * lhs, IR * rhs)
{
	IS_TRUE0(lhs && rhs);
	UINT tyid = 0;
	if (VAR_is_pointer(lhs)) {
		IS_TRUE0(VAR_pointer_base_size(lhs) > 0);
		tyid = get_dm()->get_ptr_tyid(VAR_pointer_base_size(lhs));
	} else if (VAR_data_type(lhs) == D_MC) {
		tyid = get_dm()->get_mc_tyid(VAR_data_size(lhs), VAR_elem_type(lhs));
	} else if (IS_SIMPLEX(VAR_data_type(lhs))) {
		tyid = get_dm()->get_simplex_tyid_ex(VAR_data_type(lhs));
	} else {
		IS_TRUE0(0);
	}
	return build_store(lhs, tyid, rhs);
}


//'lhs': target memory location.
//'tyid: result data type.
//'rhs: value expected to store.
IR * REGION::build_store(VAR * lhs, UINT tyid, IR * rhs)
{
	IS_TRUE0(lhs != NULL && rhs != NULL);
	IR * ir = new_ir(IR_ST);
	ST_idinfo(ir) = lhs;
	ST_rhs(ir) = rhs;
	IR_dt(ir) = tyid;
	IR_parent(rhs) = ir;
	return ir;
}


//'lhs': target memory location.
//'tyid: result data type.
//'ofst': memory byte offset relative to lhs.
//'rhs: value expected to store.
IR * REGION::build_store(VAR * lhs, UINT tyid, UINT ofst, IR * rhs)
{
	IR * ir = build_store(lhs, tyid, rhs);
	ST_ofst(ir) = ofst;
	return ir;
}


IR * REGION::build_istore(IR * lhs, IR * rhs, UINT ofst, UINT tyid)
{
	IR * ir = build_istore(lhs, rhs, tyid);
	IST_ofst(ir) = ofst;
	return ir;
}


//'lhs': target memory location pointer.
//'rhs: value expected to store.
//'tyid': result type of indirect memory operation, note tyid is not the
//data type of lhs.
IR * REGION::build_istore(IR * lhs, IR * rhs, UINT tyid)
{
	IS_TRUE(lhs && rhs &&
			(lhs->is_ptr(get_dm()) || IR_type(lhs) == IR_ARRAY),
			("must be pointer"));
	IR * ir = new_ir(IR_IST);
	IS_TRUE0(get_dm()->get_dtd(tyid));
	IR_dt(ir) = tyid;
	IST_base(ir) = lhs;
	IST_rhs(ir) = rhs;
	IR_parent(lhs) = ir;
	IR_parent(rhs) = ir;
	return ir;
}


/* 'base': base of array operation, it is either LDA or pointer.
'sublist': subscript expression list.
'tyid': result type of array operator.
	Note that tyid may NOT be equal to elem_tyid, accroding to
	ARR_ofst(). If ARR_ofst() is not zero, that means array
	elem is MC, or VECTOR, and tyid should be type of member
	to MC/VECTOR.
	e.g: struct S{ int a,b,c,d;}
		struct S pa[100];
		If youe do access pa[1].c
		tyid should be int rather than struct S.
		and elem_tyid should be struct S.

'elem_tyid': record element-data-type.
	e.g:vector<int,8> g[100];
		elem_size is sizeof(vector<int,8>) = 32
		elem_type is vector.
	e.g1: struct S{ int a,b,c,d;}
		struct S * pa[100];
		elem_size is sizeof(struct S *)
		elem_type is PTR.
	e.g2:
		struct S pa[100];
		elem_size is sizeof(struct S)
		elem_type is struct S

'dims': indicate the array dimension.
'elem_num': point to an integer array that indicate
	the number of element for each dimension. The length of the integer
	array should be equal to 'dims'.
	e.g: int g[12][24];
		elem_num points to an array with 2 value, [12, 24].
		the 1th dimension has 12 elements, and the 2th dimension has 24
		elements, which element type is D_I32.
*/
IR * REGION::build_array(IR * base, IR * sublist, UINT tyid,
						 UINT elem_tyid, UINT dims, TMWORD const* elem_num)
{
	IS_TRUE0(base && sublist && tyid != 0 && elem_tyid != 0);
	CARRAY * ir = (CARRAY*)new_ir(IR_ARRAY);
	IS_TRUE0(get_dm()->get_dtd(tyid));
	IR_dt(ir) = tyid;
	ARR_base(ir) = base;
	IR_parent(base) = ir;
	ARR_sub_list(ir) = sublist;
	UINT n = 0;
	for (IR * p = sublist; p != NULL; p = IR_next(p)) {
		IR_parent(p) = ir;
		n++;
	}
	IS_TRUE0(n == dims);
	ARR_elem_ty(ir) = elem_tyid;

	UINT l = sizeof(TMWORD) * dims;
	TMWORD * ebuf = (TMWORD*)xmalloc(l);
	memcpy(ebuf, elem_num, l);
	ARR_elem_num_buf(ir) = ebuf;
	return ir;
}


IR * REGION::build_return(IR * retexp)
{
	IR * ir = new_ir(IR_RETURN);
	RET_exp(ir) = retexp;
	if (retexp != NULL) {
		IS_TRUE0(retexp->is_exp());
		IS_TRUE0(IR_next(retexp) == NULL);
		IS_TRUE0(IR_prev(retexp) == NULL);

		IR_parent(retexp) = ir;
	}
	return ir;
}


IR * REGION::build_continue()
{
	return new_ir(IR_CONTINUE);
}


IR * REGION::build_break()
{
	return new_ir(IR_BREAK);
}


IR * REGION::build_case(IR * casev_exp, LABEL_INFO * jump_lab)
{
	IS_TRUE0(casev_exp && jump_lab);
	IR * ir = new_ir(IR_CASE);
	CASE_lab(ir) = jump_lab;
	CASE_vexp(ir) = casev_exp;
	IR_parent(casev_exp) = ir;
	return ir;
}


/* Build Do Loop stmt.
'det': determinate expression.
'loop_body': stmt list.
'init': record the stmt that initialize iv.
'step': record the stmt that update iv. */
IR * REGION::build_do_loop(IR * det, IR * init, IR * step, IR * loop_body)
{
	IS_TRUE0(det && (IR_type(det) == IR_LT ||
					 IR_type(det) == IR_LE ||
					 IR_type(det) == IR_GT ||
			    	 IR_type(det) == IR_GE));
	IS_TRUE0(init->is_stid() || init->is_stpr());
	IS_TRUE0(step->is_stid() || step->is_stpr());
	IS_TRUE0(IR_type(step->get_rhs()) == IR_ADD);

	IR * ir = new_ir(IR_DO_LOOP);
	LOOP_det(ir) = det;
	IR_parent(det) = ir;

	IS_TRUE0(init && init->is_stmt() && step && step->is_stmt());

	LOOP_init(ir) = init;
	LOOP_step(ir) = step;

	IR_parent(init) = ir;
	IR_parent(step) = ir;

	LOOP_body(ir) = loop_body;
	IR * c = loop_body;
	while (c != NULL) {
		IR_parent(c) = ir;
		//Do not check if ir is stmt, it will be canonicalized later.
		c = IR_next(c);
	}
	return ir;
}


/* Build Do While stmt.
'det': determinate expression.
'loop_body': stmt list. */
IR * REGION::build_do_while(IR * det, IR * loop_body)
{
	IS_TRUE0(det && det->is_judge());

	IR * ir = new_ir(IR_DO_WHILE);
	LOOP_det(ir) = det;
	IR_parent(det) = ir;

	LOOP_body(ir) = loop_body;
	IR * c = loop_body;
	while (c != NULL) {
		IR_parent(c) = ir;
		//Do not check if ir is stmt, it will be canonicalized later.
		c = IR_next(c);
	}
	return ir;
}


/* Build While Do stmt.
'det': determinate expression.
'loop_body': stmt list. */
IR * REGION::build_while_do(IR * det, IR * loop_body)
{
	IS_TRUE0(det && det->is_judge());

	IR * ir = new_ir(IR_WHILE_DO);
	LOOP_det(ir) = det;
	IR_parent(det) = ir;

	LOOP_body(ir) = loop_body;
	IR * c = loop_body;
	while (c != NULL) {
		IR_parent(c) = ir;
		//Do not check if ir is stmt, it will be canonicalized later.
		c = IR_next(c);
	}
	return ir;
}


/* Build IF stmt.
'det': determinate expression.
'true_body': stmt list.
'false_body': stmt list. */
IR * REGION::build_if(IR * det, IR * true_body, IR * false_body)
{
	IS_TRUE0(det && det->is_judge());

	IR * ir = new_ir(IR_IF);
	IF_det(ir) = det;
	IR_parent(det) = ir;

	IF_truebody(ir) = true_body;
	IR * c = true_body;
	while (c != NULL) {
		IR_parent(c) = ir;
		//Do not check if ir is stmt, it will be canonicalized later.
		c = IR_next(c);
	}

	IF_falsebody(ir) = false_body;
	c = false_body;
	while (c != NULL) {
		IR_parent(c) = ir;
		//Do not check if ir is stmt, it will be canonicalized later.
		c = IR_next(c);
	}
	return ir;
}


/* Build SWITCH multi-select stmt.
'vexp': expression to determine which case entry will be target.
'case_list': case entry list. case entry is consist of expression and label.
	Note that case list is optional.
'body': stmt list.
'default_lab': label indicates the default choice, the label is optional.

NOTE: Do not set parent for stmt in 'body'. */
IR * REGION::build_switch(IR * vexp, IR * case_list, IR * body,
						  LABEL_INFO * default_lab)
{
	IS_TRUE0(vexp && vexp->is_exp());
	IR * ir = new_ir(IR_SWITCH);
	SWITCH_vexp(ir) = vexp;
	SWITCH_case_list(ir) = case_list;
	SWITCH_body(ir) = body;
	SWITCH_deflab(ir) = default_lab;
	IR_parent(vexp) = ir;

	IR * c = case_list;
	while (c != NULL) {
		IS_TRUE0(IR_type(c) == IR_CASE);
		IR_parent(c) = ir;
		c = IR_next(c);
	}

	c = body;
	while (c != NULL) {
		IR_parent(c) = ir;
		//Do not check if ir is stmt, it will be canonicalized later.
		c = IR_next(c);
	}
	return ir;
}


IR * REGION::build_branch(bool is_true_br, IR * det, LABEL_INFO * lab)
{
	IS_TRUE0(lab && det && det->is_judge());
	IR * ir;
	if (is_true_br) {
		ir = new_ir(IR_TRUEBR);
	} else {
		ir = new_ir(IR_FALSEBR);
	}
	BR_det(ir) = det;
	BR_lab(ir) = lab;
	IR_parent(det) = ir;
	return ir;
}


//Float point number.
IR * REGION::build_imm_fp(HOST_FP fp, UINT tyid)
{
	IR * imm = new_ir(IR_CONST);
	//Convert string to hex value , that is in order to generate
	//single load instruction to load float point value in Code
	//Generator.
	IS_TRUE0(get_dm()->is_fp(tyid));
	CONST_fp_val(imm) = fp;
	IR_dt(imm) = tyid;
	return imm;
}


//'v': value of integer.
//'tyid': integer type.
IR * REGION::build_imm_int(HOST_INT v, UINT tyid)
{
	IS_TRUE(DTD_rty(get_dm()->get_dtd(tyid)) >= D_B &&
			DTD_rty(get_dm()->get_dtd(tyid)) <= D_U128,
			("expect integer type"));
	IR * imm = new_ir(IR_CONST);
    CONST_int_val(imm) = v;
	IR_dt(imm) = tyid;
	return imm;
}


IR * REGION::build_pointer_op(IR_TYPE irt, IR * lchild, IR * rchild)
{
	IS_TRUE0(lchild && rchild);
	DT_MGR * dm = get_dm();
	if (!lchild->is_ptr(dm) && rchild->is_ptr(dm)) {
		IS_TRUE0(irt == IR_ADD ||
				 irt == IR_MUL ||
				 irt == IR_XOR ||
				 irt == IR_BAND ||
				 irt == IR_BOR ||
				 irt == IR_EQ ||
				 irt == IR_NE);
		return build_pointer_op(irt, rchild, lchild);
	}
	DTD const* d0 = lchild->get_dtd(dm);
	DTD const* d1 = rchild->get_dtd(dm);
	if (lchild->is_ptr(dm) && rchild->is_ptr(dm)) {
		//CASE: Pointer substraction.
		//	char *p, *q;
		//	p-q => t1=p-q, t2=t1/4, return t2
		switch (irt) {
		case IR_SUB:
			{
				DT_MGR * dm = get_dm();
				//Result is not pointer type.
				IS_TRUE0(DTD_ptr_base_sz(d0) > 0);
				IS_TRUE0(DTD_ptr_base_sz(d0) == DTD_ptr_base_sz(d1));
				IR * ret = new_ir(IR_SUB);
				BIN_opnd0(ret) = lchild;
				BIN_opnd1(ret) = rchild;
				IR_dt(ret) = dm->get_simplex_tyid_ex(
									dm->get_dtype(WORD_BITSIZE, true));
				if (DTD_ptr_base_sz(d0) > BYTE_PER_CHAR) {
					IR * div = new_ir(IR_DIV);
					BIN_opnd0(div) = ret;
					BIN_opnd1(div) = build_imm_int(DTD_ptr_base_sz(d0),
												  IR_dt(ret));
					IR_dt(div) = dm->get_simplex_tyid_ex(
										dm->get_dtype(WORD_BITSIZE, true));
					ret = div;
				}

				//Avoid too much boring pointer operations.
				ret->set_parent_pointer(true);
				return ret;
			}
		case IR_LT:
		case IR_LE:
		case IR_GT:
		case IR_GE:
		case IR_EQ:
		case IR_NE:
			{
				//Result is not pointer type.
				IR * ret = new_ir(irt);
				BIN_opnd0(ret) = lchild;
				BIN_opnd1(ret) = rchild;
				IR_dt(ret) = dm->get_simplex_tyid_ex(D_B);
				IR_parent(lchild) = ret;
				IR_parent(rchild) = ret;
				return ret;
			}
		default: IS_TRUE(0, ("illegal pointers operation"));
		}
		IS_TRUE(0, ("can not get here."));
	} else if (lchild->is_ptr(dm) && !rchild->is_ptr(dm)) {
		/*
		Result is pointer type.
		CASE:
			int * p;
			p + 4 => t1 = p + (4 * sizeof(BASE_TYPE_OF(p)))
			p - 4 => t1 = p - (4 * sizeof(BASE_TYPE_OF(p)))
		*/
		switch (irt) {
		case IR_ADD:
		case IR_SUB:
			{
				IR * addend = new_ir(IR_MUL);
				BIN_opnd0(addend) = rchild;
				BIN_opnd1(addend) = build_imm_int(DTD_ptr_base_sz(d0),
												 IR_dt(rchild));
				IR_dt(addend) = IR_dt(rchild);

  				IR * ret = new_ir(irt); //ADD or SUB
				BIN_opnd0(ret) = lchild; //lchild is pointer.
				BIN_opnd1(ret) = addend; //addend is not pointer.

				/*
				CASE: 'p = p + 1'
				so the result type of '+' should still be pointer type.
				*/
				ret->set_pointer_type(DTD_ptr_base_sz(d0), dm);

				//Avoid too much boring pointer operations.
				ret->set_parent_pointer(true);
				return ret;
			}
		default:
			{
				IS_TRUE0(irt == IR_LT || irt == IR_LE ||
						 irt == IR_GT || irt == IR_GE ||
						 irt == IR_EQ || irt == IR_NE);
				//Pointer operation will incur undefined behavior.
  				IR * ret = new_ir(irt);
				BIN_opnd0(ret) = lchild;
				BIN_opnd1(ret) = rchild;
				IR_dt(ret) = dm->get_simplex_tyid_ex(D_B);

				IR_parent(lchild) = ret;
				IR_parent(rchild) = ret;
				return ret;
			}
		}
	}
	IS_TRUE(0, ("can not get here."));
	return NULL; //just ceases warning.
}


/* Helper function.
Det-expression should be a relation-operation,
so we create a node comparing with ZERO by NE node. */
IR * REGION::build_judge(IR * exp)
{
	IS_TRUE0(!exp->is_judge());
	UINT tyid = IR_dt(exp);
	DT_MGR * dm = get_dm();
	if (exp->is_ptr(dm)) {
		tyid = dm->get_simplex_tyid_ex(dm->get_pointer_size_int_dtype());
	}
	IR * x = build_cmp(IR_NE, exp, build_imm_int(0, tyid));
	return x;
}


//Build comparision operation.
IR * REGION::build_cmp(IR_TYPE irt, IR * lchild, IR * rchild)
{
	IS_TRUE0(irt == IR_LAND || irt == IR_LOR ||
			 irt == IR_LT || irt == IR_LE ||
			 irt == IR_GT || irt == IR_GE ||
			 irt == IR_NE ||	irt == IR_EQ);
	IS_TRUE0(lchild && rchild);
	if (IR_is_const(lchild) &&
		!IR_is_const(rchild) &&
		(irt == IR_EQ ||
		 irt == IR_NE)) {
		return build_cmp(irt, rchild, lchild);
	}
	IR * ir = new_ir(irt);
	BIN_opnd0(ir) = lchild;
	BIN_opnd1(ir) = rchild;
	IR_dt(ir) = get_dm()->get_simplex_tyid_ex(D_B);
	IR_parent(lchild) = ir;
	IR_parent(rchild) = ir;
	return ir;
}


IR * REGION::build_unary_op(IR_TYPE irt, UINT tyid, IN IR * opnd)
{
	IS_TRUE0(is_una_irt(irt));
	IS_TRUE0(opnd);
	IR * ir = new_ir(irt);
	UNA_opnd0(ir) = opnd;
	IR_dt(ir) = tyid;
	IR_parent(opnd) = ir;
	return ir;

}


//Build binary op without considering pointer arith.
IR * REGION::build_binary_op_simp(IR_TYPE irt, UINT tyid,
								  IR * lchild, IR * rchild)
{
	IS_TRUE0(lchild && rchild);
	IR * ir = new_ir(irt);
	BIN_opnd0(ir) = lchild;
	BIN_opnd1(ir) = rchild;
	IR_parent(lchild) = ir;
	IR_parent(rchild) = ir;
	IR_dt(ir) = tyid;
	return ir;
}


//Binary operation
//'mc_size': record the memory-chunk size if rtype is D_MC, or else is 0.
IR * REGION::build_binary_op(IR_TYPE irt, UINT tyid,
							 IR * lchild, IR * rchild)
{
	IS_TRUE0(lchild && rchild);
	DT_MGR * dm = get_dm();
	if (lchild->is_ptr(dm) || rchild->is_ptr(dm)) {
		return build_pointer_op(irt, lchild, rchild);
	}
	if (IR_is_const(lchild) &&
		!IR_is_const(rchild) &&
		(irt == IR_ADD ||
		 irt == IR_MUL ||
		 irt == IR_XOR ||
		 irt == IR_BAND ||
		 irt == IR_BOR ||
		 irt == IR_EQ ||
		 irt == IR_NE)) {
		return build_binary_op(irt, tyid, rchild, lchild);
	}

	//Both lchild and rchild are NOT pointer.
	//Generic binary operation.
	DTD const* d = dm->get_dtd(tyid);
	IS_TRUE0(d);
	UINT mc_size = DTD_mc_sz(d);
	if (DTD_rty(d) == D_MC) {
		IS_TRUE0(mc_size == lchild->get_dt_size(dm) &&
				 mc_size == rchild->get_dt_size(dm));
	}
	IS_TRUE0((DTD_rty(d) == D_MC) ^ (mc_size == 0));
	return build_binary_op_simp(irt, tyid, lchild, rchild);
}


/* Split list of ir into basic block.
'irs': a list of ir.
'bbl': a list of bb.
'ctbb': marker current bb container. */
C<IR_BB*> * REGION::split_irs(IR * irs, IR_BB_LIST * bbl, C<IR_BB*> * ctbb)
{
	IR_CFG * cfg = get_cfg();
	IR_BB * newbb = new_bb();
	cfg->add_bb(newbb);
	ctbb = bbl->insert_after(newbb, ctbb);
	LAB2BB * lab2bb = cfg->get_lab2bb_map();

	while (irs != NULL) {
		IR * ir = removehead(&irs);
		if (newbb->is_bb_down_boundary(ir)) {
			IR_BB_ir_list(newbb).append_tail(ir);
			newbb = new_bb();
			cfg->add_bb(newbb);
			ctbb = bbl->insert_after(newbb, ctbb);
		} else if (newbb->is_bb_up_boundary(ir)) {
			IS_TRUE0(IR_type(ir) == IR_LABEL);

			newbb = new_bb();
			cfg->add_bb(newbb);
			ctbb = bbl->insert_after(newbb, ctbb);

			//Regard label-info as add-on info that attached on newbb, and
			//'ir' will be dropped off.
			LABEL_INFO * li = ir->get_label();
			newbb->add_label(li);
			lab2bb->set(li, newbb);
			if (!LABEL_INFO_is_try_start(li) && !LABEL_INFO_is_pragma(li)) {
				IR_BB_is_target(newbb) = true;
			}
			free_irs(ir); //free label ir.
		} else {
			IR_BB_ir_list(newbb).append_tail(ir);
		}
	}
	return ctbb;
}


/* Find the boundary IR generated in BB to update bb-list incremently.
e.g: Given BB1 has one stmt:
	BB1:
	a = b||c
after some simplification, we get:
	truebr(b != 0), L1
	truebr(c != 0), L1
	pr = 0
	goto L2
	L1:
	pr = 1
	L2:
	a = pr

where IR list should be splitted into :
	BB1:
	truebr(b != 0), L1
	BB2:
	truebr(c != 0), L1
	BB3:
	pr = 0
	goto L2
	BB4:
	L1:
	pr = 1
	BB5:
	L2:
	a = pr */
bool REGION::reconstruct_ir_bb_list(OPT_CTX & oc)
{
	START_TIMER("Reconstruct IR_BB list");
	bool change = false;

	C<IR_BB*> * ctbb;

	IR_CFG * cfg = get_cfg();

	IR_BB_LIST * bbl = get_bb_list();

	//Map label-info to related BB.
	LAB2BB * lab2bb = cfg->get_lab2bb_map();

	for (bbl->get_head(&ctbb); ctbb != NULL; bbl->get_next(&ctbb)) {
		IR_BB * bb = C_val(ctbb);
		C<IR*> * ctir;
		BBIR_LIST * irlst = &IR_BB_ir_list(bb);

		IR * tail = irlst->get_tail();
		for (irlst->get_head(&ctir); ctir != NULL; irlst->get_next(&ctir)) {
			IR * ir = C_val(ctir);

			if (bb->is_bb_down_boundary(ir) && ir != tail) {
				change = true;

				IR * restirs = NULL; //record rest part in bb list after 'ir'.
				IR * last = NULL;
				irlst->get_next(&ctir);

				for (C<IR*> * next_ctir = ctir;
					 ctir != NULL; ctir = next_ctir) {
					irlst->get_next(&next_ctir);
					irlst->remove(ctir);
					add_next(&restirs, &last, C_val(ctir));
				}

				ctbb = split_irs(restirs, bbl, ctbb);

				break;
			} else if (bb->is_bb_up_boundary(ir)) {
				IS_TRUE0(IR_type(ir) == IR_LABEL);

				change = true;
				IR_BB_is_fallthrough(bb) = true;

				IR * restirs = NULL; //record rest part in bb list after 'ir'.
				IR * last = NULL;

				for (C<IR*> * next_ctir = ctir;
					 ctir != NULL; ctir = next_ctir) {
					irlst->get_next(&next_ctir);
					irlst->remove(ctir);
					add_next(&restirs, &last, C_val(ctir));
				}

				ctbb = split_irs(restirs, bbl, ctbb);

				break;
			}
		}
	}

	END_TIMER();

	if (change) {
		//Must rebuild CFG and all other structures which are
		//closely related to CFG.
		oc.set_flag_if_cfg_changed();
	}
	return change;
}


//Construct IR list from IR_BB list.
//clean_ir_list: clean bb's ir list if it is true.
IR * REGION::construct_ir_list(bool clean_ir_list)
{
	START_TIMER("Construct IR_BB list");
	IR * ret_list = NULL;
	IR * last = NULL;
	for (IR_BB * bb = get_bb_list()->get_head(); bb != NULL;
		 bb = get_bb_list()->get_next()) {
		for (LABEL_INFO * li = IR_BB_lab_list(bb).get_head();
			 li != NULL; li = IR_BB_lab_list(bb).get_next()) {
			//insertbefore_one(&ret_list, ret_list, build_label(li));
			add_next(&ret_list, &last, build_label(li));
		}
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			//insertbefore_one(&ret_list, ret_list, ir);
			add_next(&ret_list, &last, ir);
			if (clean_ir_list) {
				ir->set_bb(NULL);
			}
		}
		if (clean_ir_list) {
			IR_BB_ir_list(bb).clean();
		}
	}
	//ret_list = reverse_list(ret_list);
	END_TIMER();
	return ret_list;
}


//1. Split list of IRs into basic-block list.
//2. Set BB propeties. e.g: entry-bb, exit-bb.
void REGION::construct_ir_bb_list()
{
	if (get_ir_list() == NULL) { return; }
	START_TIMER("Construct IR_BB list");
	IR * start_ir = get_ir_list();
	IR_BB * cur_bb = NULL;
	IR * pointer = start_ir;
	while (pointer != NULL) {
		if (cur_bb == NULL) {
			cur_bb = new_bb();
		}

		//Insert IR into individual BB.
		IS_TRUE0(pointer->is_stmt_in_bb() || pointer->is_lab());
		IR * cur_ir = pointer;
		pointer = IR_next(pointer);
		remove(&start_ir, cur_ir);
		if (cur_bb->is_bb_down_boundary(cur_ir)) {
			IR_BB_ir_list(cur_bb).append_tail(cur_ir);
			switch (IR_type(cur_ir)) {
			case IR_CALL:
			case IR_ICALL: //indirective call
			case IR_TRUEBR:
			case IR_FALSEBR:
			case IR_SWITCH:
				IR_BB_is_fallthrough(cur_bb) = true;
				break;
			case IR_IGOTO:
			case IR_GOTO:
				/* We have no knowledge about whether target BB of GOTO/IGOTO
				will be followed subsequently on current BB.
				Leave this problem to CFG builder, and the related
				attribute should be set at that time. */
				break;
			case IR_RETURN:
				//Succeed stmt of 'ir' may be DEAD code
				//IR_BB_is_func_exit(cur_bb) = true;
				IR_BB_is_fallthrough(cur_bb) = true;
				break;
			default: IS_TRUE(0, ("invalid bb down-boundary IR"));
			} //end switch

			//Generate new BB.
			get_bb_list()->append_tail(cur_bb);
			cur_bb = new_bb();
		} else if (cur_bb->is_bb_up_boundary(cur_ir)) {
			IR_BB_is_fallthrough(cur_bb) = true;
			get_bb_list()->append_tail(cur_bb);

			//Generate new BB.
			cur_bb = new_bb();

			//label info be seen as add-on info attached on bb, and
			//'ir' be dropped off.
			if (IR_type(cur_ir) == IR_LABEL) {
				cur_bb->add_label(LAB_lab(cur_ir));
				free_ir(cur_ir);
			} else {
				IR_BB_ir_list(cur_bb).append_tail(cur_ir);
			}
			IR_BB_is_target(cur_bb) = true;
		} else {
			IR_BB_ir_list(cur_bb).append_tail(cur_ir);
		}
	} //end while

	IS_TRUE0(cur_bb != NULL);
	get_bb_list()->append_tail(cur_bb);

	//cur_bb is the last bb, it is also the exit bb.
	//IR_BB_is_func_exit(cur_bb) = true;
	END_TIMER();
}


/* Do general check for safe optimizing.
	CASE 1:
		Do NOT optimize the code if meet the VIOLATE variable!
		e.g1:
			volatile int x, y;
			int a[SIZE];
			void f(void)
			{
			  for (int i = 0; i < SIZE; i++)
			    a[i] = x + y;
			}

			Some compilers will hoist the expression (x + y)
			out of the loop as shown below, which is an
			incorrect optimization.
		e.g2:
			## Incorrect Definiation! x,y are NOT const.
			const violate int x,y; */
bool REGION::is_safe_to_optimize(IR const* ir)
{
	if (ir->is_volatile()) {
		return false;
	}

	//Check kids.
	for(INT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid != NULL) {
			if (!is_safe_to_optimize(kid)) {
				return false;
			}
		}
	}
	return true;
}


//Get function unit.
REGION * REGION::get_func_unit()
{
	REGION * ru = this;
	while (RU_type(ru) != RU_FUNC) {
		ru = RU_parent(ru);
	}
	IS_TRUE(ru != NULL, ("Not in func unit"));
	return ru;
}


CHAR const* REGION::get_ru_name() const
{
	return SYM_name(VAR_name(m_var));
}


/* Free ir, ir's sibling, and all its kids.
We can only utilizing the function to free the IR
which allocated by 'new_ir'.

NOTICE: If ir's sibling is not NULL, that means the IR is
a high level type. IR_BB consists of only middle/low level IR. */
void REGION::free_irs_list(IR * ir)
{
	if (ir == NULL) return;
	IR * kid = NULL;
	IR * head = ir, * next = NULL;
	while (ir != NULL) {
		next = IR_next(ir);
		remove(&head, ir);
		free_irs(ir);
		ir = next;
	}
}


//Free ir, and all its kids.
//We can only utilizing the function to free the IR which allocated by 'new_ir'.
void REGION::free_irs_list(IR_LIST & irs)
{
	IR * next;
	for (IR * ir = irs.get_head(); ir != NULL; ir = next) {
		next = irs.get_next();
		IS_TRUE(IR_next(ir) == NULL && IR_prev(ir) == NULL,
				("do not allow sibling node, need to simplify"));
		irs.remove(ir);
		free_irs(ir);
	}
}


/* Free ir and all its kids, except its sibling node.
We can only utilizing the function to free the
IR which allocated by 'new_ir'. */
void REGION::free_irs(IR * ir)
{
	if (ir == NULL) { return; }
	IS_TRUE(IR_next(ir) == NULL && IR_prev(ir) == NULL,
			("chain list should be cut off"));
	for (INT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid != NULL) {
			free_irs_list(kid);
		}
	}
	free_ir(ir);
}


//Format label name string in 'buf'.
CHAR * REGION::format_label_name(IN LABEL_INFO * lab, OUT CHAR * buf)
{
	CHAR const* prefix = NULL;
	CHAR * org_buf = buf;
	prefix = SYM_name(VAR_name(get_ru_var()));
	sprintf(buf, "%s_", prefix);
	buf += strlen(buf);
	if (LABEL_INFO_type(lab) == L_ILABEL) {
		sprintf(buf, ILABEL_STR_FORMAT, ILABEL_CONT(lab));
	} else if (LABEL_INFO_type(lab) == L_CLABEL) {
		sprintf(buf, CLABEL_STR_FORMAT, CLABEL_CONT(lab));
	} else {
		IS_TRUE(0, ("unknown label type"));
	}
	return org_buf;
}


void REGION::dump_free_tab()
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n==-- DUMP REGION Free Table --==");
	for (UINT i = 0; i <= MAX_OFFSET_AT_FREE_TABLE; i++) {
		IR * lst = RU_ana(this)->m_free_tab[i];
		if (lst == NULL) { continue; }

		UINT sz = i + sizeof(IR);

		UINT count = 0;
		for (IR * ir = lst; ir != NULL; ir = IR_next(ir)) {
			count++;
		}

		fprintf(g_tfile, "\nirsize(%d), num(%d):", sz, count);

		for (IR * ir = lst; ir != NULL; ir = IR_next(ir)) {
			IS_TRUE0(get_irt_size(ir) == sz);
			fprintf(g_tfile, "ir(%d),", IR_id(ir));
		}
	}
	fflush(g_tfile);
}


//Generate IR, invoke free_ir() or free_irs() if it is useless.
//NOTE: Do NOT invoke ::free() to free IR, because all
//	IR are allocated in the pool.
IR * REGION::new_ir(IR_TYPE irt)
{
	IR * ir = NULL;
	UINT idx = IRTSIZE(irt) - sizeof(IR);
	bool lookup = false; //lookup freetab will save more memory, but slower.

	#ifndef CONST_IRT_SZ
	//If one is going to lookup freetab, IR_irt_size() must be defined.
	IS_TRUE0(!lookup);
	#endif

	if (lookup) {
		for (; idx <= MAX_OFFSET_AT_FREE_TABLE; idx++) {
			ir = RU_ana(this)->m_free_tab[idx];
			if (ir == NULL) { continue; }

			RU_ana(this)->m_free_tab[idx] = IR_next(ir);
			if (IR_next(ir) != NULL) {
				IR_prev(IR_next(ir)) = NULL;
			}
			break;
		}
	} else {
		ir = RU_ana(this)->m_free_tab[idx];
		if (ir != NULL) {
			RU_ana(this)->m_free_tab[idx] = IR_next(ir);
			if (IR_next(ir) != NULL) {
				IR_prev(IR_next(ir)) = NULL;
			}
		}
	}

	if (ir == NULL) {
		ir = (IR*)xmalloc(IRTSIZE(irt));
		INT v = MAX(get_ir_vec()->get_last_idx(), 0);
		IR_id(ir) = (UINT)(v+1);
		get_ir_vec()->set(IR_id(ir), ir);
		set_irt_size(ir, IRTSIZE(irt));
	} else {
		IS_TRUE0(IR_prev(ir) == NULL);
		IR_next(ir) = NULL;
		#ifdef _DEBUG_
		RU_ana(this)->m_has_been_freed_irs.diff(IR_id(ir));
		#endif
	}
	IR_type(ir) = irt;
	return ir;
}


//Just append freed 'ir' into free_list.
//Do NOT free its kids and siblings.
void REGION::free_ir(IR * ir)
{
	IS_TRUE0(ir);
	IS_TRUE(IR_next(ir) == NULL && IR_prev(ir) == NULL,
			("chain list should be cut off"));
	#ifdef _DEBUG_
	IS_TRUE0(!RU_ana(this)->m_has_been_freed_irs.is_contain(IR_id(ir)));
	RU_ana(this)->m_has_been_freed_irs.bunion(IR_id(ir));
	#endif

	IS_TRUE0(get_sbs_mgr());
	ir->free_duset(*get_sbs_mgr());

	if (IR_ai(ir) != NULL) {
		IR_ai(ir)->destroy();
	}

	DU * du = ir->clean_du();
	if (du != NULL) {
		DU_md(du) = NULL;
		DU_mds(du) = NULL;
		IS_TRUE0(du->has_clean());
		RU_ana(this)->m_free_du_list.append_head(du);
	}

	UINT res_id = IR_id(ir);
	IR_AI * res_ai = IR_ai(ir);
	UINT res_irt_sz = get_irt_size(ir);
	memset(ir, 0, res_irt_sz);
	IR_id(ir) = res_id;
	IR_ai(ir) = res_ai;
	set_irt_size(ir, res_irt_sz);

	UINT idx = res_irt_sz - sizeof(IR);
	IR * head = RU_ana(this)->m_free_tab[idx];
	if (head != NULL) {
		IR_next(ir) = head;
		IR_prev(head) = ir;
	}
	RU_ana(this)->m_free_tab[idx] = ir;
}


void REGION::remove_inner_region(REGION * ru)
{
	IS_TRUE0(RU_ana(this)->m_inner_region_lst);
	RU_ana(this)->m_inner_region_lst->remove(ru);
	delete ru;
}


/* Duplication 'ir' and kids, and its sibling,
Return list of new ir.

NOTICE: Only duplicate irs start from 'ir' to
all its 'next' node. */
IR * REGION::dup_irs_list(IR const* ir)
{
	IR * new_list = NULL;
	while (ir != NULL) {
		IR * newir = dup_irs(ir);
		add_next(&new_list, newir);
		ir = IR_next(ir);
	}
	return new_list;
}

//Duplicate 'ir' and its kids, but without ir's sibiling node.
IR * REGION::dup_irs(IR const* ir)
{
	if (ir == NULL) { return NULL; }
	IR * newir = dup_ir(ir);
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid != NULL) {
			IR * newkid_list = dup_irs_list(kid);
			newir->set_kid(i, newkid_list);
		} else { IS_TRUE0(newir->get_kid(i) == NULL); }
	}
	return newir;
}


//Duplication all contents of 'src', except the AI info, kids and siblings IR.
//Since src may be come from other region, we do not copy AI info.
IR * REGION::dup_ir(IR const* src)
{
	if (src == NULL) { return NULL; }
	IR_TYPE irt = IR_get_type(src);
	IR * res = new_ir(irt);
	IS_TRUE(res != NULL && src != NULL, ("res/src is NULL"));

	UINT res_id = IR_id(res);
	IR_AI * res_ai = IR_ai(res);
	UINT res_irt_sz = get_irt_size(res);
	memcpy(res, src, IRTSIZE(irt));
	IR_id(res) = res_id;
	IR_ai(res) = res_ai;
	set_irt_size(res, res_irt_sz);
	IR_next(res) = IR_prev(res) = IR_parent(res) = NULL;
	res->clean_du(); //Do not copy DU struct.
	if (IR_ai(src) != NULL) {
		if (IR_ai(res) == NULL) {
			IR_ai(res) = new_ai();
		}
		IR_ai(res)->copy(IR_ai(src));
	}
	return res;
}


LIST<IR const*> * REGION::scan_call_list()
{
	LIST<IR const*> * cl = get_call_list();
	IS_TRUE0(cl);
	cl->clean();
	if (get_ir_list() != NULL) {
		for (IR const* t = get_ir_list(); t != NULL; t = IR_next(t)) {
			if (IR_type(t) == IR_CALL || IR_type(t) == IR_ICALL) {
				cl->append_tail(t);
			}
		}
	} else {
		for (IR_BB * bb = get_bb_list()->get_head();
			 bb != NULL; bb = get_bb_list()->get_next()) {
			IR * t = IR_BB_last_ir(bb);
			if (t != NULL &&
				(IR_type(t) == IR_CALL || IR_type(t) == IR_ICALL)) {
				cl->append_tail(t);
			}
		}
	}
	return cl;
}


//Prepare informations for analysis phase, such as record
//which variables have been taken address for both
//global and local variable.
void REGION::prescan(IR const* ir)
{
	while (ir != NULL) {
		switch (IR_type(ir)) {
		case IR_ST:
			prescan(ST_rhs(ir));
			break;
		case IR_LDA:
			{
				IR * base = LDA_base(ir);
				if (IR_type(base) == IR_ID) {
					VAR * v = ID_info(base);
					IS_TRUE0(v && IR_parent(ir));
					if (VAR_is_str(v) &&
						get_ru_mgr()->get_dedicated_str_md() != NULL) {
						return;
					}
					if (IR_type(IR_parent(ir)) != IR_ARRAY) {
						//If LDA is the base of ARRAY, say (&a)[..], its
						//address does not need to mark as address taken.
						VAR_is_addr_taken(v) = true;
					}

					// ...=&x.a, address of 'x.a' is taken.
					MD md;
					MD_base(&md) = v; //correspond to VAR
					MD_ofst(&md) = LDA_ofst(ir);
					MD_size(&md) = ir->get_dt_size(get_dm());
					MD_ty(&md) = MD_EXACT;
					get_md_sys()->register_md(md);
				} else if (IR_type(base) == IR_ARRAY) {
					//... = &a[x]; address of 'a' is taken
					IR * array_base = ARR_base(base);
					IS_TRUE0(array_base != NULL);
					if (IR_type(array_base) == IR_LDA) {
						prescan(array_base);
					}
				} else if (base->is_str(get_dm())) {
					VAR * v = get_var_mgr()->
								register_str(NULL, CONST_str_val(base), 1);
					IS_TRUE0(v);
					VAR_is_addr_taken(v) = true;
					VAR_allocable(v) = true;
				} else if (IR_type(base) == IR_ILD) {
					//We do not known where to be taken.
					IS_TRUE(0, ("ILD should not appeared here, "
								"one should do refining at first."));
				} else {
					IS_TRUE(0, ("Illegal LDA base."));
				}
			}
			break;
		case IR_ID:
			/* If array base must not be ID.
			In C, array base could be assgined to other variable directly.
			Its address should be marked as taken. And it's parent must be LDA.
			e.g: Address of 'a' is taken.
				int a[10];
				int * p;
				p = a; */
			IS_TRUE0(0);
			//IS_TRUE0(ID_info(ir) && VAR_is_array(ID_info(ir)));
			break;
		case IR_CONST:
		case IR_LD:
		case IR_GOTO:
		case IR_LABEL:
		case IR_PR:
		case IR_BREAK:
		case IR_CONTINUE:
		case IR_PHI:
		case IR_REGION:
			break;
		default:
			for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
				IR * k = ir->get_kid(i);
				if (k != NULL) {
					IS_TRUE0(IR_parent(k) == ir);
					prescan(k);
				}
			}
		}
		ir = IR_next(ir);
	}
}


void REGION::dump_bb_usage(FILE * h)
{
	if (h == NULL) { return; }
	IR_BB_LIST * bbl = get_bb_list();
	if (bbl == NULL) { return; }
	fprintf(h, "\nTotal BB num: %d", bbl->get_elem_count());
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		fprintf(h, "\nBB%d: irnum%d", IR_BB_id(bb), IR_BB_ir_num(bb));
	}
	fflush(h);
}


//Dump IR and memory usage.
void REGION::dump_mem_usage(FILE * h)
{
	if (h == NULL) { return; }
	fprintf(h, "\n\n'%s' use %dKB memory",
			get_ru_name(), count_mem()/1024);
	SVECTOR<IR*> * v = get_ir_vec();
	float nid = 0.0;
	float nld = 0.0;
	float nst = 0.0;
	float nlda = 0.0;
	float ncallee = 0.0;
	float nstpr = 0.0;
	float nist = 0.0;
	float nbin = 0.0;
	float nuna = 0.0;

	for (int i = 0; i <= v->get_last_idx(); i++) {
		IR * ir = v->get(i);
		if (ir == NULL) continue;
		if (IR_type(ir) == IR_ID) nid+=1.0;
		if (IR_type(ir) == IR_LD) nld+=1.0;
		if (IR_type(ir) == IR_ST) nst+=1.0;
		if (IR_type(ir) == IR_LDA) nlda+=1.0;
		if (IR_type(ir) == IR_CALL) ncallee+=1.0;
		if (IR_type(ir) == IR_STPR) nstpr+=1.0;
		if (IR_type(ir) == IR_IST) nist+=1.0;
		if (ir->is_bin_op()) nbin+=1.0;
		if (ir->is_unary_op()) nuna+=1.0;
	}

	float total = (float)(v->get_last_idx() + 1);
	fprintf(h, "\nThe number of IR Total:%d, id:%d(%.1f)%%, "
					"ld:%d(%.1f)%%, st:%d(%.1f)%%, lda:%d(%.1f)%%,"
					"callee:%d(%.1f)%%, stpr:%d(%.1f)%%, ist:%d(%.1f)%%,"
					"bin:%d(%.1f)%%, una:%d(%.1f)%%",
			(int)total,
			(int)nid, nid / total * 100,
			(int)nld, nld / total * 100,
			(int)nst, nst / total * 100,
			(int)nlda, nlda / total * 100,
			(int)ncallee, ncallee / total * 100,
			(int)nstpr, nstpr / total * 100,
			(int)nist, nist / total * 100,
			(int)nbin, nbin / total * 100,
			(int)nuna, nuna / total * 100);
}


//Dump all irs and ordering by IR_id.
void REGION::dump_all_ir()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP IR VECTOR ----==");
	INT n = get_ir_vec()->get_last_idx();
	INT i = 1;
	g_indent = 2;
	UINT num_has_du = 0;

	//Dump which IR has allocate DU structure.
	while (i <= n) {
		IR * ir = get_ir_vec()->get(i);
		IS_TRUE0(ir);
		i++;
		DU * du = ir->get_du();
		if (du != NULL) {
			num_has_du++;
		}
	}
	if (i > 0) {
		note("\nTotal IR %d, total DU allocated %d, rate:(%.1f)%%",
			 i, num_has_du, (float)num_has_du / (float)i * 100);
	}
	//

	//Dump IR dispers in free tab.
	fprintf(g_tfile, "\n== Dump IR dispers in free tab ==");
	for (UINT w = 0; w < MAX_OFFSET_AT_FREE_TABLE + 1; w++) {
		IR * lst = RU_ana(this)->m_free_tab[w];
		fprintf(g_tfile, "\nbyte(%d)", (INT)(w + sizeof(IR)));
		if (lst == NULL) { continue; }

		UINT num = 0;
		IR * p = lst;
		while (p != NULL) { p = IR_next(p); num++; }
		fprintf(g_tfile, ", num%d : ", num);

		while (lst != NULL) {
			fprintf(g_tfile, "%s", IRNAME(lst));
			lst = IR_next(lst);
			if (lst != NULL) {
				fprintf(g_tfile, ", ");
			}
		}
	}
	fflush(g_tfile);

	CHAR buf[256]; //record data-type.
	DT_MGR * dm = get_dm();

	i = 1;
	while (i <= n) {
		IR * ir = get_ir_vec()->get(i);
		IS_TRUE0(ir);
		DTD const* d = NULL;
		if (IR_type(ir) != IR_UNDEF) {
			if (IR_dt(ir) != VOID_TY) {
				d = dm->get_dtd(IR_dt(ir));
			}
			if (d == NULL) {
				note("\nid(%d): %s 0x%.8x", IR_id(ir), IRNAME(ir), ir);
			} else {
				note("\nid(%d): %s r:%s 0x%.8x",
					 IR_id(ir), IRNAME(ir), dm->dump_dtd(d, buf), ir);
			}
		} else {
			note("\nid(%d): undef 0x%.8x", IR_id(ir), ir);
		}

		i++;

		DU * du = ir->get_du();
		if (du != NULL) {
			fprintf(g_tfile, " has du");
		}
	}
	fflush(g_tfile);
}


//Only dump STMT IR.
void REGION::dump_all_stmt()
{
	INT i = get_ir_vec()->get_last_idx(), j = 0;
	g_indent = 1;
	note("  == IR STATEMENT: ==");
	g_indent = 2;
	DT_MGR * dm = get_dm();
	while (j <= i) {
		IR * ir = get_ir_vec()->get(j);
		if (ir->is_stmt()) {
			note("\n%d(%d):  %s  (r:%s:%d>",
				 j, IR_id(ir), IRNAME(ir),
				 DTDES_name(&g_dtype_desc[IR_dt(ir)]),
				 ir->get_dt_size(dm));
		}
		j++;
	}
}


//Perform DEF/USE analysis.
IR_DU_MGR * REGION::init_du(OPT_CTX & oc)
{
	IS_TRUE0(RU_ana(this));
	if (get_du_mgr() == NULL) {
		IS_TRUE0(OPTC_is_aa_valid(oc));
		IS_TRUE0(OPTC_is_cfg_valid(oc));

		RU_ana(this)->m_ir_du_mgr = new IR_DU_MGR(this);

		UINT f = SOL_REACH_DEF|SOL_REF;
		//f |= SOL_AVAIL_REACH_DEF|SOL_AVAIL_EXPR|SOL_RU_REF;
		if (g_do_ivr) {
			f |= SOL_AVAIL_REACH_DEF|SOL_AVAIL_EXPR;
		}

		if (g_do_compute_available_exp) {
			f |= SOL_AVAIL_EXPR;
		}

		get_du_mgr()->perform(oc, f);

		get_du_mgr()->compute_du_chain(oc);
	}
	return get_du_mgr();
}


//Initialize alias analysis.
IR_AA * REGION::init_aa(OPT_CTX & oc)
{
	IS_TRUE0(RU_ana(this));
	if (get_aa() == NULL) {
		IS_TRUE0(OPTC_is_cfg_valid(oc));

		RU_ana(this)->m_ir_aa = new IR_AA(this);

		get_aa()->init_may_ptset();

		get_aa()->set_flow_sensitive(true);

		get_aa()->perform(oc);
	}
	return get_aa();
}


//Build CFG and do early control flow optimization.
IR_CFG * REGION::init_cfg(OPT_CTX & oc)
{
	IS_TRUE0(RU_ana(this));
	if (get_cfg() == NULL) {
		UINT n = MAX(8, get_nearest_power_of_2(get_bb_list()->get_elem_count()));
		RU_ana(this)->m_ir_cfg = new IR_CFG(C_SEME, get_bb_list(), this, n, n);
		IR_CFG * cfg = RU_ana(this)->m_ir_cfg;
		//cfg->remove_empty_bb();
		cfg->build(oc);
		cfg->build_eh();

		//Rebuild cfg may generate redundant empty bb, it
		//disturb the computation of entry and exit.
		cfg->remove_empty_bb(oc);
		cfg->compute_entry_and_exit(true, true);

		bool change = true;
		UINT count = 0;
		while (change && count < 20) {
			change = false;
			if (g_do_cfg_remove_empty_bb &&
				cfg->remove_empty_bb(oc)) {
				cfg->compute_entry_and_exit(true, true);
				change = true;
			}
			if (g_do_cfg_remove_unreach_bb &&
				cfg->remove_unreach_bb()) {
				cfg->compute_entry_and_exit(true, true);
				change = true;
			}
			if (g_do_cfg_remove_tramp_bb &&
				cfg->remove_tramp_edge()) {
				cfg->compute_entry_and_exit(true, true);
				change = true;
			}
			if (g_do_cfg_remove_unreach_bb &&
				cfg->remove_unreach_bb()) {
				cfg->compute_entry_and_exit(true, true);
				change = true;
			}
			if (g_do_cfg_remove_tramp_bb &&
				cfg->remove_tramp_bb()) {
				cfg->compute_entry_and_exit(true, true);
				change = true;
			}
			count++;
		}
		IS_TRUE0(!change);
	}
	IS_TRUE0(get_cfg()->verify());
	return get_cfg();
}


void REGION::process()
{
	if (get_ir_list() == NULL) { return ; }

	IS_TRUE0(verify_ir_inplace());

	IS_TRUE(RU_type(this) == RU_FUNC ||
			RU_type(this) == RU_EH ||
			RU_type(this) == RU_SUB,
			("Are you sure this kind of REGION executable?"));

	g_indent = 0;

	note("\nREGION_NAME:%s", get_ru_name());

	if (g_opt_level == NO_OPT) { return; }

	OPT_CTX oc;

	START_TIMER("PreScan");
	prescan(get_ir_list());
	END_TIMER();

	PASS_MGR * pm = new_pass_mgr();

	OPTC_pass_mgr(oc) = pm;

	high_process(oc);

	middle_process(oc);

	delete pm;

	OPTC_pass_mgr(oc) = pm;

	if (RU_type(this) != RU_FUNC) { return; }

	IR_BB_LIST * bbl = get_bb_list();

	if (bbl->get_elem_count() == 0) { return; }

	SIMP_CTX simp;
	simp.set_simp_cf();
	simp.set_simp_array();
	simp.set_simp_select();
	simp.set_simp_local_or_and();
	simp.set_simp_local_not();
	simp.set_simp_to_lowest_heigh();
	simplify_bb_list(bbl, &simp);

	if (g_cst_bb_list && SIMP_need_recon_bblist(&simp)) {
		if (reconstruct_ir_bb_list(oc) && g_do_cfg) {
			//Before CFG building.
			get_cfg()->remove_empty_bb(oc);

			get_cfg()->rebuild(oc);

			//After CFG building, it is perform different
			//operation to before building.
			get_cfg()->remove_empty_bb(oc);

			get_cfg()->compute_entry_and_exit(true, true);

			if (g_do_cdg) {
				CDG * cdg = (CDG*)OPTC_pass_mgr(oc)->register_opt(OPT_CDG);
				cdg->rebuild(oc, *get_cfg());
			}
		}
	}

	IS_TRUE0(verify_ir_and_bb(bbl, get_dm()));
	RF_CTX rf;
	refine_ir_bb_list(bbl, rf);
	IS_TRUE0(verify_ir_and_bb(bbl, get_dm()));
}


//Ensure that each IR in ir_list must be allocated in current region.
bool REGION::verify_ir_inplace()
{
	IR const* ir = get_ir_list();
	if (ir == NULL) { return true; }
	for (; ir != NULL; ir = IR_next(ir)) {
		IS_TRUE(get_ir(IR_id(ir)) == ir,
				("ir id:%d is not allocated in region %s", get_ru_name()));
	}
	return true;
}


//Verify cond/uncond target label.
bool REGION::verify_bbs(IR_BB_LIST & bbl)
{
	LAB2BB lab2bb;
	for (IR_BB * bb = bbl.get_head(); bb != NULL; bb = bbl.get_next()) {
		for (LABEL_INFO * li = IR_BB_lab_list(bb).get_head();
			 li != NULL; li = IR_BB_lab_list(bb).get_next()) {
			lab2bb.set(li, bb);
		}
	}

	for (IR_BB * bb = bbl.get_head(); bb != NULL; bb = bbl.get_next()) {
		IR * last = IR_BB_last_ir(bb);
		if (last == NULL) { continue; }

		IR_BB * target_bb;
		if (last->is_cond_br()) {
			target_bb = lab2bb.get(BR_lab(last));
			IS_TRUE(target_bb != NULL, ("target cannot be NULL"));
		} else if (last->is_multicond_br()) {
			IS_TRUE0(IR_type(last) == IR_SWITCH);

			for (IR * c = SWITCH_case_list(last); c != NULL; c = IR_next(c)) {
				target_bb = lab2bb.get(CASE_lab(last));
				IS_TRUE(target_bb != NULL, ("target cannot be NULL"));
			}

			if (SWITCH_deflab(last) != NULL) {
				target_bb = lab2bb.get(SWITCH_deflab(last));
				IS_TRUE(target_bb != NULL, ("target cannot be NULL"));
			}
		} else if (last->is_uncond_br()) {
			target_bb = lab2bb.get(GOTO_lab(last));
			IS_TRUE(target_bb != NULL, ("target cannot be NULL"));
		}
	}
	return true;
}


void REGION::dump_var_tab(INT indent)
{
	if (g_tfile == NULL) { return; }
	//Dump REGION name.
	fprintf(g_tfile, "\nREGION:%s:", get_ru_name());
	CHAR buf[8192]; //Is it too large?
	buf[0] = 0;
	m_var->dump_var_decl(buf, 100);

	//Dump formal parameter list.
	if (RU_type(this) == RU_FUNC) {
		g_indent = indent + 1;
		note("\nFORMAL PARAMETERS:\n");
		VTAB_ITER c;
		for (VAR * v = get_var_tab()->get_first(c);
			 v != NULL; v = get_var_tab()->get_next(c)) {
			if (VAR_is_formal_param(v)) {
				buf[0] = 0;
				v->dump(buf);
				g_indent = indent + 2;
				note("%s\n", buf);
				fflush(g_tfile);
			}
		}
	}

	//Dump local varibles.
	g_indent = indent;
	note(" ");
	fprintf(g_tfile, "\n");

	//Dump variables
	VAR_TAB * vt = get_var_tab();
	if (vt->get_elem_count() > 0) {
		g_indent = indent + 1;
		note("LOCAL VARIABLES:\n");
		VTAB_ITER c;
		for (VAR * v = vt->get_first(c); v != NULL; v = vt->get_next(c)) {
			//fprintf(g_tfile, "        ");
			buf[0] = 0;
			v->dump(buf);
			g_indent = indent + 2;
			note("%s\n", buf);
			//fprintf(g_tfile, buf);
			//fprintf(g_tfile, "\n");
			fflush(g_tfile);
		}
	} else {
		g_indent = indent + 1;
		note("NO VARIBLE:\n");
	}

	//Dump subregion.
	fprintf(g_tfile, "\n");
	if (RU_ana(this)->m_inner_region_lst != NULL &&
		RU_ana(this)->m_inner_region_lst->get_elem_count() > 0) {
		for (REGION * r = RU_ana(this)->m_inner_region_lst->get_head();
			 r != NULL; r = RU_ana(this)->m_inner_region_lst->get_next()) {
			r->dump_var_tab(indent + 1);
		}
	}
}


void REGION::check_valid_and_recompute(OPT_CTX * oc, ...)
{
	BITSET opts;
	UINT num = 0;
	va_list ptr;
	va_start(ptr, oc);
	OPT_TYPE opty = (OPT_TYPE)va_arg(ptr, UINT);
	while (opty != OPT_UNDEF && num < 1000) {
		IS_TRUE0(opty < OPT_NUM);
		opts.bunion(opty);
		num++;
		opty = (OPT_TYPE)va_arg(ptr, UINT);
	}
	va_end(ptr);
	IS_TRUE(num < 1000, ("miss ending placeholder"));

	if (num == 0) { return; }

	IR_CFG * cfg = get_cfg();
	IR_DU_MGR * du = get_du_mgr();
	IR_AA * aa = get_aa();

	if (opts.is_contain(OPT_CFG) && !OPTC_is_cfg_valid(*oc)) {
		IS_TRUE0(cfg); //CFG has not been created.
		//Caution: the validation of cfg should maintained by user.
		cfg->rebuild(*oc);
		cfg->remove_empty_bb(*oc);
		cfg->compute_entry_and_exit(true, true);
	}

	if (opts.is_contain(OPT_AA) && !OPTC_is_aa_valid(*oc)) {
		IS_TRUE0(cfg && aa); //cfg is not enable.
		IS_TRUE0(OPTC_is_cfg_valid(*oc));
		aa->perform(*oc);
	}

	if (opts.is_contain(OPT_DOM) && !OPTC_is_dom_valid(*oc)) {
		IS_TRUE0(cfg); //cfg is not enable.
		cfg->compute_dom_idom(*oc);
	}

	if (opts.is_contain(OPT_PDOM) && !OPTC_is_pdom_valid(*oc)) {
		IS_TRUE0(cfg); //cfg is not enable.
		cfg->compute_pdom_ipdom(*oc);
	}

	if (opts.is_contain(OPT_CDG) && !OPTC_is_cdg_valid(*oc)) {
		CDG * cdg = (CDG*)OPTC_pass_mgr(*oc)->register_opt(OPT_CDG);
		IS_TRUE0(cdg); //cdg is not enable.
		cdg->build(*oc, *cfg);
	}

	UINT f = 0;
	if (opts.is_contain(OPT_DU_REF) && !OPTC_is_ref_valid(*oc)) {
		f |= SOL_REF;
	}

	if (opts.is_contain(OPT_LIVE_EXPR) && !OPTC_is_live_expr_valid(*oc)) {
		f |= SOL_AVAIL_EXPR;
	}

	if (opts.is_contain(OPT_EXPR_TAB) && !OPTC_is_live_expr_valid(*oc)) {
		f |= SOL_AVAIL_EXPR;
	}

	if (opts.is_contain(OPT_AVAIL_REACH_DEF) &&
		!OPTC_is_avail_reach_def_valid(*oc)) {
		f |= SOL_AVAIL_REACH_DEF;
	}

	if (opts.is_contain(OPT_DU_CHAIN) &&
		!OPTC_is_du_chain_valid(*oc) &&
		!OPTC_is_reach_def_valid(*oc)) {
		f |= SOL_REACH_DEF;
	}

	if (HAVE_FLAG(f, SOL_REF) && !OPTC_is_aa_valid(*oc)) {
		IS_TRUE0(aa); //Default AA is not enable.
		aa->perform(*oc);
	}

	if (f != 0) {
		IS_TRUE0(du); //Default du manager is not enable.
		du->perform(*oc, f);
		if (HAVE_FLAG(f, SOL_REF)) {
			IS_TRUE0(du->verify_du_ref());
		}
		if (HAVE_FLAG(f, SOL_AVAIL_EXPR)) {
			IS_TRUE0(du->verify_livein_exp());
		}
	}

	if (opts.is_contain(OPT_DU_CHAIN) && !OPTC_is_du_chain_valid(*oc)) {
		IS_TRUE0(du); //Default du manager is not enable.
		du->compute_du_chain(*oc);
	}

	if (opts.is_contain(OPT_EXPR_TAB) && !OPTC_is_expr_tab_valid(*oc)) {
		IR_EXPR_TAB * exprtab =
			(IR_EXPR_TAB*)OPTC_pass_mgr(*oc)->register_opt(OPT_EXPR_TAB);
		IS_TRUE0(exprtab);
		exprtab->reperform(*oc);
	}

	if (opts.is_contain(OPT_LOOP_INFO) && !OPTC_is_loopinfo_valid(*oc)) {
		cfg->loop_analysis(*oc);
	}

	if (opts.is_contain(OPT_RPO)) {
		if (!OPTC_is_rpo_valid(*oc)) {
			cfg->compute_rpo(*oc);
		} else {
			IS_TRUE0(cfg->get_bblist_in_rpo()->get_elem_count() ==
					 get_bb_list()->get_elem_count());
		}
	}
}


bool REGION::partition_region()
{
	//----- DEMO CODE ----------
	IR * ir = get_ir_list();
	IR * start_pos = NULL;
	IR * end_pos = NULL;
	while (ir != NULL) {
		if (IR_type(ir) == IR_LABEL) {
			LABEL_INFO * li = LAB_lab(ir);
			if (LABEL_INFO_type(li) == L_CLABEL &&
				strcmp(SYM_name(LABEL_INFO_name(li)), "REGION_START") == 0) {
				start_pos = ir;
				break;
			}
		}
		ir = IR_next(ir);
	}
	if (ir == NULL) return false;
	ir = IR_next(ir);
	while (ir != NULL) {
		if (IR_type(ir) == IR_LABEL) {
			LABEL_INFO * li = LAB_lab(ir);
			if (LABEL_INFO_type(li) == L_CLABEL &&
				strcmp(SYM_name(LABEL_INFO_name(li)), "REGION_END") == 0) {
				end_pos = ir;
				break;
			}
		}
		ir = IR_next(ir);
	}
	if (start_pos == NULL || end_pos == NULL) return false;
	IS_TRUE0(start_pos != end_pos);
	//----------------

	//-----------
	//Generate IR region.
	CHAR b[64];
	sprintf(b, "inner_ru%d", get_inner_region_list()->get_elem_count());
	VAR * ruv = get_var_mgr()->register_var(b, D_MC, D_UNDEF, 0, 0, 1,
											VAR_LOCAL|VAR_FAKE);
	VAR_allocable(ruv) = false;
	add_to_var_tab(ruv);

	IR * ir_ru = build_region(RU_SUB, ruv);
	REGION * inner_ru = REGION_ru(ir_ru);
	copy_dbx(ir, ir_ru, inner_ru);
	//------------

	ir = IR_next(start_pos);
	while (ir != end_pos) {
		IR * t = ir;
		ir = IR_next(ir);
		remove(&RU_ana(this)->m_ir_list, t);
		IR * inner_ir = inner_ru->dup_irs(t);
		free_irs(t);
		inner_ru->add_to_ir_list(inner_ir);
	}
	dump_irs(inner_ru->get_ir_list(), get_dm());
	insertafter_one(&start_pos, ir_ru);
	dump_irs(get_ir_list(), get_dm());
	//-------------

	REGION_ru(ir_ru)->process();
	dump_irs(get_ir_list(), get_dm());

	/* Merger IR list in inner-region to outer region.
	remove(&RU_ana(this)->m_ir_list, ir_ru);
	IR * head = inner_ru->construct_ir_list();
	insertafter(&split_pos, dup_irs_list(head));
	dump_irs(get_ir_list());
	remove_inner_region(inner_ru); */
	return false;
}
//END REGION



//
//START REGION_MGR
//
REGION_MGR::~REGION_MGR()
{
	for (INT id = 0; id <= m_id2ru.get_last_idx(); id++) {
		REGION * ru = m_id2ru.get(id);
		if (ru != NULL) { delete ru; }
	}
	m_id2ru.clean();
	m_ru_count = 0;
	m_label_count = 1;

	if (m_md_sys != NULL) {
		delete m_md_sys;
		m_md_sys = NULL;
	}

	if (m_var_mgr != NULL) {
		delete m_var_mgr;
		m_var_mgr = NULL;
	}

	if (m_callg != NULL) {
		delete m_callg;
		m_callg = NULL;
	}
}


/* Map string variable to dedicated string md.
CASE: android/external/tagsoup/src/org/ccil/cowan/tagsoup/HTMLSchema.java
	This method allocates 3000+ string variable.
	Each string has been taken address.
	It will bloat may_point_to_set.
	So we need to regard all string variables as one if necessary. */
MD const* REGION_MGR::get_dedicated_str_md()
{
	if (m_is_regard_str_as_same_md) {
		//Regard all string variables as single one.
		if (m_str_md == NULL) {
			SYM * s  = add_to_symtab("Dedicated_VAR_be_regarded_as_string");
			VAR * sv = get_var_mgr()->
						register_str("Dedicated_String_Var", s, 1);
			VAR_allocable(sv) = false;
			MD md;
			MD_base(&md) = sv;
			MD_ty(&md) = MD_UNBOUND;
			IS_TRUE0(MD_is_str(&md));
			MD_is_addr_taken(&md) = true;
			MD * e = m_md_sys->register_md(md);
			IS_TRUE0(MD_id(e) > 0);
			m_str_md = e;
		}
		return m_str_md;
	}
	return NULL;
}


void REGION_MGR::register_global_mds()
{
	//Only top region can do initialize MD for global variable.
	ID2VAR * varvec = RU_MGR_var_mgr(this)->get_var_vec();
	for (INT i = 0; i <= varvec->get_last_idx(); i++) {
		VAR * v = varvec->get(i);
		if (v == NULL) { continue; }

		if (VAR_is_fake(v) ||
			VAR_is_local(v) ||
			VAR_is_func_unit(v) ||
			VAR_is_func_decl(v)) {
			continue;
		}
		IS_TRUE0(VAR_is_global(v) && VAR_allocable(v));
		if (VAR_is_str(v) && get_dedicated_str_md() != NULL) {
			continue;
		}
		MD md;
		MD_base(&md) = v;
		MD_ofst(&md) = 0;
		MD_size(&md) = VAR_data_size(v);
		MD_ty(&md) = MD_EXACT;
		MD * e = m_md_sys->register_md(md);
		m_global_var_mds.bunion(e);
	}
}


VAR_MGR * REGION_MGR::new_var_mgr()
{
	return new VAR_MGR(this);
}


REGION * REGION_MGR::new_region(REGION_TYPE rt)
{
	REGION * ru = new REGION(rt, this);
	RU_id(ru) = m_ru_count++;
	return ru;
}


//Record new region and delete it when REGION_MGR destroy.
void REGION_MGR::register_region(REGION * ru)
{
	IS_TRUE0(m_id2ru.get(RU_id(ru)) == NULL);
	m_id2ru.set(RU_id(ru), ru);
}


REGION * REGION_MGR::get_region(UINT id)
{
	return m_id2ru.get(id);
}


void REGION_MGR::delete_region(UINT id)
{
    REGION * ru = get_region(id);
	if (ru == NULL) return;
	m_id2ru.set(id, NULL);
	delete ru;
}


void REGION_MGR::dump()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\nDump REGION-UNIT\n");
	REGION * ru = get_top_region();
	if (ru == NULL) {
		return;
	}
	ru->dump_var_tab(0);
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


REGION * REGION_MGR::get_top_region()
{
	if (m_id2ru.get_last_idx() == -1) {
		return NULL;
	}
	REGION * ru = get_region(0);
	IS_TRUE(ru != NULL, ("region id must start at 0"));
	while (RU_parent(ru) != NULL) {
		ru = RU_parent(ru);
	}
	return ru;
}


//Process a RU_FUNC region.
void REGION_MGR::process_func(IN REGION * ru)
{
	IS_TRUE0(RU_type(ru) == RU_FUNC);
	if (ru->get_ir_list() == NULL) { return; }
	ru->process();
	tfree();
}


IPA * REGION_MGR::new_ipa()
{
	return new IPA(this);
}


//Scan call site and build call graph.
CALLG * REGION_MGR::init_cg(REGION * top)
{
	IS_TRUE0(m_callg == NULL);
	UINT vn = 0, en = 0;
	IR * irs = top->get_ir_list();
	while (irs != NULL) {
		if (IR_type(irs) == IR_REGION) {
			vn++;
			REGION * ru = REGION_ru(irs);
			LIST<IR const*> * call_list = ru->scan_call_list();
			IS_TRUE0(call_list);
			if (call_list->get_elem_count() == 0) {
				irs = IR_next(irs);
				continue;
			}
			for (IR const* ir = call_list->get_head();
				 ir != NULL; ir = call_list->get_next()) {
				en++;
			}
		}
		irs = IR_next(irs);
	}

	m_callg = new CALLG(MAX(vn, 1), MAX(en, 1), this);
	m_callg->build(top);
	return m_callg;
}


static void test_ru(REGION * ru, REGION * top, REGION_MGR * rm)
{
	INT i = 0;
	VAR * v = ru->get_ru_var();
	REGION * x = new REGION(RU_FUNC, rm);
	while (i < 10000) {
		x->init(RU_FUNC, rm);
		x->set_ru_var(v);
		IR * irs = ru->get_ir_list();
		x->set_ir_list(x->dup_irs_list(irs));
		x->process();
		//VAR_MGR * vm = x->get_var_mgr();
		//vm->dump();
		//MD_SYS * ms = x->get_md_sys();
		//ms->dump_all_mds();
		tfree();
		x->destroy();
		i++;
	}
	return;
}


//Only process top-level region unit.
//Top level region unit should be program unit.
void REGION_MGR::process()
{
	REGION * top = get_top_region();
	if (top == NULL) { return; }

	register_global_mds();
	IS_TRUE(RU_type(top) == RU_PROGRAM, ("TODO: support more operation."));
	OPT_CTX oc;
	if (g_do_inline) {
		CALLG * callg = init_cg(top);
		OPTC_is_callg_valid(oc) = true;
		//callg->dump_vcg(get_dt_mgr(), NULL);
		INLINER inl(this);
		inl.perform(oc);
	}

	//Test mem leak.
	//REGION * ru = REGION_ru(top->get_ir_list());
	//test_ru(ru, top, this);
	//return;

	IR * irs = top->get_ir_list();
	while (irs != NULL) {
		if (IR_type(irs) == IR_REGION) {
			REGION * ru = REGION_ru(irs);
			process_func(ru);
			if (!g_do_ipa) {
				ru->destroy();
				REGION_ru(irs) = NULL;
			}
		}
		irs = IR_next(irs);
	}

	if (g_do_ipa) {
		if (!OPTC_is_callg_valid(oc)) {
			init_cg(top);
			OPTC_is_callg_valid(oc) = true;
		}
		IPA ipa(this);
		ipa.perform(oc);
	}
}
//END REGION_MGR
