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

//Algebraic identities.
IR * REGION::refine_ild_1(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_ILD);
	/* Convert
		ILD,ofst1
		 LDA
		  LD,ofst2
	=>
		LD,ofst1+ofst2
	e.g: ((*(&q))->a).b => q.a.b
	or
	Convert
		ILD,ofst
		 LDA
		  ID
	=>
		LD,ofst
	e.g: (&q)->s => q.s */
	IR * lda = ILD_base(ir);
	IS_TRUE(IR_type(lda) == IR_LDA && LDA_ofst(lda) == 0,
			("not the case"));

	//ILD offset may not 0.
	INT ild_ofst = ILD_ofst(ir);

	IS_TRUE0(IR_type(LDA_base(lda)) == IR_ID);
	IR * ld = build_load(ID_info(LDA_base(lda)), IR_dt(ir));
	copy_dbx(ld, LDA_base(lda), this);

	LD_ofst(ld) += ild_ofst;
	if (get_du_mgr() != NULL) {
		//new_ir is IR_LD.
		//Consider the ir->get_ofst() and copying MD_SET info from 'ir'.
		IS_TRUE(ir->get_exact_ref(), ("In this case, ILD must use exact-md."));
		ld->copy_ref(ir, this);
		get_du_mgr()->change_use(ld, ir, get_du_mgr()->get_sbs_mgr());
	}
	free_irs(ir);
	change = true;

	//Do not need set parent.
	return ld;
}


IR * REGION::refine_ild_2(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_ILD);
	/* Convert
		ILD
		 LDA,ofst
		  ID
	=>
		LD,ofst */
	IR * rm = ir;
	IR * lda = ILD_base(ir);
	ir = LDA_base(lda);
	LDA_base(lda) = NULL;

	IR_DU_MGR * dumgr = get_du_mgr();

	if (IR_get_type(ir) == IR_ID) {
		IR * ld = build_load(ID_info(ir), IR_dt(rm));
		copy_dbx(ld, ir, this);
		if (dumgr != NULL) {
			ld->copy_ref(rm, this);
			dumgr->change_use(ld, rm, dumgr->get_sbs_mgr());
		}
		free_irs(ir);
		ir = ld;
	}

	IS_TRUE0(IR_type(ir) == IR_LD);
	LD_ofst(ir) += LDA_ofst(lda);
	if (dumgr != NULL) {
		MD const* md = ir->get_exact_ref();
		IS_TRUE0(md != NULL);

		MD tmd(*md);
		MD_ofst(&tmd) += LDA_ofst(lda);
		MD * entry = get_md_sys()->register_md(tmd);
		IS_TRUE0(MD_id(entry) > 0);

		ir->set_ref_md(entry, this);
		dumgr->compute_overlap_use_mds(ir, true);
	}
	free_irs(rm);
	change = true;
	//Do not need to set parent pointer.
	return ir;
}


IR * REGION::refine_ild(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_type(ir) == IR_ILD);
	IS_TRUE(IR_next(ir) == NULL && IR_prev(ir) == NULL, ("TODO"));
	IR * mem_addr = ILD_base(ir);
	if (IR_type(mem_addr) == IR_LDA && LDA_ofst(mem_addr) == 0) {
		/* Convert
				ILD,ofst1
				 LDA
				  LD,ofst2
			=>
				LD,ofst1+ofst2
			e.g: ((*(&q))->a).b => q.a.b
		or
			Convert
				ILD,ofst
				 LDA
				  ID
			=>
				LD,ofst
			e.g: (&q)->s => q.s */
		return refine_ild_1(ir, change, rc);
	} else if (IR_type(mem_addr) == IR_LDA && LDA_ofst(mem_addr) != 0) {
		/* Convert
			ILD
			 LDA,ofst
			  ID
		=>
			LD,ofst */
		return refine_ild_2(ir, change, rc);
	} else {
		ILD_base(ir) = refine_ir(mem_addr, change, rc);
		if (change) {
			IR_parent(ILD_base(ir)) = ir;
		}
	}
	return ir;
}


//NOTE: the function may change IR stmt.
IR * REGION::refine_lda(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_LDA);
	IS_TRUE(IR_next(ir) == NULL && IR_prev(ir) == NULL, ("TODO"));
	if (IR_type(LDA_base(ir)) == IR_ILD) {
		/* Convert
			LDA
			 ILD
			  LD,ofst
		=>
			LD,ofst
		e.g1: &(*(a.p)) => a.p


		Convert
			LDA
			 ILD,ofst2
			  LD,ofst1
		=>
			ADD((LD,ofst1), ofst2)
		e.g2:&(s.a->b) => LD(s.a) + ofst(b) */
		IR * newir = ILD_base(LDA_base(ir));
		ILD_base(LDA_base(ir)) = NULL;
		if (ILD_ofst(LDA_base(ir)) != 0) {
			UINT t = get_dm()->get_simplex_tyid_ex(D_U32);
			newir = build_binary_op_simp(IR_ADD, IR_dt(ir), newir,
								 build_imm_int(ILD_ofst(LDA_base(ir)), t));
			copy_dbx(newir, ir, this);
		}
		//Do not need to set parent pointer.
		free_irs(ir);
		change = true;
		return newir; //No need for updating DU chain and MDS.
	} else {
		LDA_base(ir) = refine_ir(LDA_base(ir), change, rc);
		IR_parent(LDA_base(ir)) = ir;
	}
	return ir;
}


IR * REGION::refine_istore(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_type(ir) == IR_IST);
	bool t = false;
	bool lchange = false;
	IST_base(ir) = refine_ir(IST_base(ir), t, rc);
	change |= t;
	lchange |= t;
	t = false;
	IST_rhs(ir) = refine_ir(IST_rhs(ir), t, rc);
	change |= lchange;
	lchange |= t;

	IR * lhs = IST_base(ir);
	IR * rhs = IST_rhs(ir);
	IR_DU_MGR * dumgr = get_du_mgr();
	if (IR_type(lhs) == IR_LDA && IR_type(LDA_base(lhs)) == IR_ID) {
		/* Convert :
		1. IST(LDA(ID))=X to ST(ID)=X
		2. IST(LDA(ID), ofst)=X to ST(ID, ofst)=X
		3. IST(LDA(ID,ofst))=X to ST(ID, ofst)=X
		4. IST(LDA(ID,ofst1), ofst2)=X to ST(ID, ofst1+ofst2)=X */
		IR * newir = build_store(ID_info(LDA_base(lhs)),
								 IR_dt(ir),
								 LDA_ofst(lhs) + IST_ofst(ir),
								 IST_rhs(ir));
		if (dumgr != NULL) {
			copy_dbx(newir, ir, this);
			newir->copy_ref(ir, this);
			dumgr->change_def(newir, ir, dumgr->get_sbs_mgr());
		}
		IST_rhs(ir) = NULL;
		free_ir(ir);
		ir = newir;
		change = true; //Keep the result type of ST unchanged.
		rhs = ST_rhs(ir); //No need for updating DU.
	}

	if (IR_type(rhs) == IR_LDA && IR_type(LDA_base(rhs)) == IR_ILD) {
		IS_TRUE(IR_next(rhs) == NULL && IR_prev(rhs) == NULL,
				("expression cannot be linked to chain"));
		//Convert IST(LHS, LDA(ILD(var))) => IST(LHS, var)
		IS_TRUE0(IR_type(ILD_base(LDA_base(rhs))) != IR_ID);
		IR * rm = rhs;
		if (ir->is_stid()) {
			ST_rhs(ir) = ILD_base(LDA_base(rhs));
		} else {
			IST_rhs(ir) = ILD_base(LDA_base(rhs));
		}
		ILD_base(LDA_base(rhs)) = NULL;
		free_irs(rm);
		change = true;
	}

	rhs = ir->get_rhs();
	if (IR_type(rhs) == IR_ILD && IR_type(ILD_base(rhs)) == IR_LDA) {
		//IST(X)=ILD(LDA(ID)) => IST(X)=LD
		IR * rm = rhs;
		IR * newrhs = build_load(ID_info(LDA_base(ILD_base(rhs))), IR_dt(rm));
		ir->set_rhs(newrhs);
		copy_dbx(newrhs, rhs, this);
		if (dumgr != NULL) {
			IS_TRUE0(LDA_base(ILD_base(rhs))->get_ref_mds() == NULL);
			newrhs->set_ref_md(gen_md_for_ld(newrhs), this);
			dumgr->copy_duset(newrhs, rhs);
			dumgr->change_use(newrhs, rhs, dumgr->get_sbs_mgr());
		}

		IS_TRUE(IR_next(rhs) == NULL && IR_prev(rhs) == NULL,
				("expression cannot be linked to chain"));
		free_irs(rm);
		change = true;
		rhs = newrhs;
	}

	if (RC_insert_cvt(rc)) {
		ir->set_rhs(insert_cvt(ir, rhs, change));
	}

	if (change) {
		ir->set_parent_pointer(false);
	}

	if (lchange && dumgr != NULL) {
		IS_TRUE0(!dumgr->remove_expired_du_chain(ir));
	}
	return ir;
}


//Return true if CVT is redundant.
static inline bool is_redundant_cvt(IR * ir)
{
	if (IR_type(ir) == IR_CVT) {
		if (IR_type(CVT_exp(ir)) == IR_CVT ||
			IR_dt(CVT_exp(ir)) == IR_dt(ir)) {
			return true;
		}
	}
	return false;
}


IR * REGION::refine_store(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(ir->is_stid() || ir->is_stpr());

	bool lchange = false;

	IR * rhs = ir->is_stid() ? ST_rhs(ir) : STPR_rhs(ir);

	if (RC_refine_stmt(rc) &&
		rhs->is_pr() &&
		ir->is_stpr() &&
		PR_no(rhs) == STPR_no(ir)) {

		//Remove pr1 = pr1.
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_ir_out_from_du_mgr(ir);
		}

		IR_BB * bb = ir->get_bb();
		if (bb != NULL) {
			IR_BB_ir_list(bb).remove(ir);
			RC_stmt_removed(rc) = true;
		}

		free_irs(ir);
		change = true;
		return NULL;
	}

	rhs = refine_ir(rhs, lchange, rc);
	ir->set_rhs(rhs);

	IS_TRUE0(!::is_redundant_cvt(rhs));
	if (RC_refine_stmt(rc)) {
		MD const* umd = rhs->get_exact_ref();
		if (umd != NULL && umd == ir->get_exact_ref()) {
			//Result and operand refered the same md.
			if (IR_type(rhs) == IR_CVT) {
				//CASE: pr(i64) = cvt(i64, pr(i32))
				//Do NOT remove 'cvt'.
				;
			} else {
				change = true;
				if (get_du_mgr() != NULL) {
					get_du_mgr()->remove_ir_out_from_du_mgr(ir);
				}
				IR_BB * bb = ir->get_bb();
				if (bb != NULL) {
					IR_BB_ir_list(bb).remove(ir);
					RC_stmt_removed(rc) = true;
				}
				free_irs(ir);
				return NULL;
			}
		}
	}

	if (RC_insert_cvt(rc)) {
		ir->set_rhs(insert_cvt(ir, ir->get_rhs(), lchange));
	}

	change |= lchange;

	if (lchange) {
		ir->set_parent_pointer(false);
		IR_DU_MGR * dumgr = get_du_mgr();
		if (dumgr != NULL) {
			IS_TRUE0(!dumgr->remove_expired_du_chain(ir));
		}
	}

	return ir;
}


IR * REGION::refine_call(IR * ir, bool & change, RF_CTX & rc)
{
	bool lchange = false;
	if (CALL_param_list(ir) != NULL) {
		IR * param = removehead(&CALL_param_list(ir));
		IR * newparamlst = NULL;
		IR * last = NULL;
		while (param != NULL) {
			IR * newp = refine_ir(param, lchange, rc);
			add_next(&newparamlst, &last, newp);
			last = newp;
			param = removehead(&CALL_param_list(ir));
		}
		CALL_param_list(ir) = newparamlst;
	}

	if (lchange) {
		change = true;
		ir->set_parent_pointer(false);
	}

	if (lchange) {
		IR_DU_MGR * dumgr = get_du_mgr();
		if (dumgr != NULL) {
			IS_TRUE0(!dumgr->remove_expired_du_chain(ir));
		}
	}
	return ir;
}


IR * REGION::refine_icall(IR * ir, bool & change, RF_CTX & rc)
{
	if (IR_get_type(ICALL_callee(ir)) == IR_LDA &&
		IR_get_type(LDA_base(ICALL_callee(ir))) == IR_ID) {

		IS_TRUE0(0); //Obsolete code and removed.

		/* Convert ICALL(LDA(ID), PARAM) to ICALL(LD, PARAM)
		IR * rm = ICALL_callee(ir);
		ICALL_callee(ir) = build_load(ID_info(LDA_base(ICALL_callee(ir))),
								   IR_dt(rm));
		IS_TRUE(IR_next(rm) == NULL &&
				IR_prev(rm) == NULL,
				("expression cannot be linked to chain"));
		free_ir(rm);
		ir->set_parent_pointer(false);
		change = true; */
	}

	refine_call(ir, change, rc);

	return ir;
}


IR * REGION::refine_switch(IR * ir, bool & change, RF_CTX & rc)
{
	bool l = false;
	SWITCH_vexp(ir) = refine_ir(SWITCH_vexp(ir), l, rc);
	if (l) {
		IR_DU_MGR * dumgr = get_du_mgr();
		if (dumgr != NULL) {
			IS_TRUE0(!dumgr->remove_expired_du_chain(ir));
		}
		IR_parent(SWITCH_vexp(ir)) = ir;
		change = true;
	}

	l = false;
	//SWITCH_body(ir) = refine_ir_list(SWITCH_body(ir), l, rc);
	change |= l;
	return ir;
}


IR * REGION::refine_br(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(ir->is_cond_br());
	bool l = false;
	BR_det(ir) = refine_det(BR_det(ir), l, rc);
	ir = refine_branch(ir, l, rc);
	if (l) {
		IR_DU_MGR * dumgr = get_du_mgr();
		if (dumgr != NULL) {
			IS_TRUE0(!dumgr->remove_expired_du_chain(ir));
		}
		change = true;
	}
	return ir;
}


IR * REGION::refine_return(IR * ir, bool & change, RF_CTX & rc)
{
	if (RET_exp(ir) == NULL) { return ir; }

	bool lchange = false;
	RET_exp(ir) = refine_ir(RET_exp(ir), lchange, rc);

	if (lchange) {
		change = true;
		ir->set_parent_pointer(false);
		if (get_du_mgr() != NULL) {
			IS_TRUE0(!get_du_mgr()->remove_expired_du_chain(ir));
		}
	}
	return ir;
}


IR * REGION::refine_select(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_SELECT);
	SELECT_det(ir) = refine_det(SELECT_det(ir), change, rc);
	SELECT_trueexp(ir) = refine_ir_list(SELECT_trueexp(ir), change, rc);
	SELECT_falseexp(ir) = refine_ir_list(SELECT_falseexp(ir), change, rc);
	SELECT_det(ir) = fold_const(SELECT_det(ir), change);
	IR * det = SELECT_det(ir);
	if (IR_is_const(det) && det->is_int(get_dm())) {
		HOST_INT v = CONST_int_val(det);
		if (v == 0) {
			IR * rm = SELECT_trueexp(ir);
			IR * rm2 = det;
			ir = SELECT_falseexp(ir);
			IS_TRUE0(ir->is_exp());
			free_irs(rm);
			free_irs(rm2);
			change = true;
		} else {
			IR * rm = SELECT_falseexp(ir);
			IR * rm2 = det;
			ir = SELECT_trueexp(ir);
			IS_TRUE0(ir->is_exp());
			free_irs(rm);
			free_irs(rm2);
			change = true;
		}
	} else if (IR_is_const(det) && det->is_fp(get_dm())) {
		double v = CONST_fp_val(det);
		if (v < EPSILON) { //means v == 0.0
			IR * rm = SELECT_trueexp(ir);
			IR * rm2 = det;
			ir = SELECT_falseexp(ir);
			IS_TRUE0(ir->is_exp());
			free_irs(rm);
			free_irs(rm2);
			change = true;
		} else {
			IR * rm = SELECT_falseexp(ir);
			IR * rm2 = det;
			ir = SELECT_trueexp(ir);
			IS_TRUE0(ir->is_exp());
			free_irs(rm);
			free_irs(rm2);
			change = true;
		}
	} else if (det->is_str(get_dm())) {
		IR * rm = SELECT_falseexp(ir);
		IR * rm2 = det;
		ir = SELECT_trueexp(ir);
		IS_TRUE0(ir->is_exp());
		free_irs(rm);
		free_irs(rm2);
		change = true;
	}
	if (change) {
		ir->set_parent_pointer(false);
	}
	return ir; //No need for updating DU.
}


IR * REGION::refine_neg(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_NEG);
	bool lchange = false;
	ir = fold_const(ir, lchange);
	change |= lchange;
	if (!lchange && IR_type(UNA_opnd0(ir)) == IR_NEG) {
		//-(-x) => x
		IR * tmp = UNA_opnd0(UNA_opnd0(ir));
		UNA_opnd0(UNA_opnd0(ir)) = NULL;
		free_irs(ir);
		change = true;
		return tmp;
	}
	return ir;
}


//Logic not: !(0001) = 0000
//Bitwise not: !(0001) = 1110
IR * REGION::refine_not(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_type(ir) == IR_LNOT || IR_get_type(ir) == IR_BNOT);
	UNA_opnd0(ir) = refine_ir(UNA_opnd0(ir), change, rc);
	if (change) {
		IR_parent(UNA_opnd0(ir)) = ir;
	}

	if (IR_type(ir) == IR_LNOT) {
		IR * op0 = UNA_opnd0(ir);
		bool lchange = false;
		switch (IR_type(op0)) {
		case IR_LT:
		case IR_LE:
		case IR_GT:
		case IR_GE:
		case IR_EQ:
		case IR_NE:
			op0->invert_ir_type(this);
			lchange = true;
			break;
		default: break;
		}
		if (lchange) {
			UNA_opnd0(ir) = NULL;
			free_irs(ir);
			change = true;
			ir = op0;
		}
	}

	ir = fold_const(ir, change);
	return ir;
}


//If the value of opnd0 is not a multiple of opnd1,
//((opnd0 div opnd1) mul opnd1) may not equal to opnd0.
IR * REGION::refine_div(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_DIV);
	IR * op1 = BIN_opnd1(ir);
	IR * op0 = BIN_opnd0(ir);
	DT_MGR * dm = get_dm();
	if (IR_is_const(op1) && op1->is_fp(dm) &&
		g_is_opt_float && !IR_is_const(op0)) {
		HOST_FP fp_imm = CONST_fp_val(op1);
		if (fp_imm == 1.0) {
			//X/1.0 => X
			IR * new_ir = op0;
			BIN_opnd0(ir) = NULL;
			free_irs(ir);
			ir = new_ir;
			change = true;
			return ir;
		}
		if (fp_imm == 0) {
			//X/0
			return ir;
		}
		if ((is_power_of_2(abs((INT)(fp_imm))) || is_power_of_5(fp_imm))) {
			//X/n => X*(1.0/n)
			IR_type(ir) = IR_MUL;
			CONST_fp_val(op1) = ((HOST_FP)1.0) / fp_imm;
			change = true;
			return ir;
		}
	} else if (IR_is_const(op1) &&
			   op1->is_int(dm) &&
			   is_power_of_2(CONST_int_val(op1)) &&
			   RC_refine_div_const(rc)) {
		//X/2 => X>>1, arith shift right.
		if (op0->is_sint(dm)) {
			IR_type(ir) = IR_ASR;
		} else if (op0->is_uint(dm)) {
			IR_type(ir) = IR_LSR;
		} else {
			IS_TRUE0(0);
		}
		CONST_int_val(op1) = get_power_of_2(CONST_int_val(op1));
		change = true;
		return ir; //No need for updating DU.
	} else if (op0->is_ir_equal(op1, true)) {
		//X/X => 1.
		IR * tmp = ir;
		UINT tyid;
		DT_MGR * dm = get_dm();
		if (op0->is_mc(dm) ||
			op0->is_str(dm) ||
			op0->is_ptr(dm)) {
			tyid = dm->get_simplex_tyid_ex(D_U32);
		} else {
			tyid = IR_dt(op0);
		}
		if (dm->is_fp(tyid)) {
			ir = build_imm_fp(1.0f, tyid);
		} else {
			ir = build_imm_int(1, tyid);
		}

		//Cut du chain for opnd0, opnd1 and their def-stmt.
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(tmp);
		}

		copy_dbx(ir, tmp, this);
		free_irs(tmp);
		change = true;
		return ir;
	}
	return ir;
}


IR * REGION::refine_mod(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_MOD);
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (IR_is_const(op1) &&
		op1->is_int(get_dm()) && CONST_int_val(op1) == 1) {
		//mod X,1 => 0
		IR * tmp = ir;
		ir = dup_irs(op1);
		CONST_int_val(ir) = 0;
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(tmp);
		}
		free_irs(tmp);
		change = true;
		return ir;
	}
	return ir;
}


IR * REGION::refine_rem(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_REM);
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (IR_is_const(op1) &&
		op1->is_int(get_dm()) && CONST_int_val(op1) == 1) {
		//rem X,1 => 0
		IR * tmp = ir;
		ir = dup_irs(op1);
		CONST_int_val(ir) = 0;
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(tmp);
		}
		free_irs(tmp);
		change = true;
		return ir;
	}
	return ir;
}


IR * REGION::refine_add(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_ADD);
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (IR_is_const(op1) &&
		op1->is_int(get_dm()) && CONST_int_val(op1) == 0) {
		//add X,0 => X
		IR * tmp = ir;
		BIN_opnd0(ir) = NULL;
		ir = op0;
		free_irs(tmp);
		change = true;
		return ir; //No need for updating DU.
	}
	return ir;
}


IR * REGION::refine_mul(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_MUL);
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (IR_is_const(op1) &&
		((op1->is_fp(get_dm()) && CONST_fp_val(op1) == (HOST_FP)2.0) ||
		 (op1->is_int(get_dm()) && CONST_int_val(op1) == 2))) {
		//mul X,2.0 => add.fp X,X
		//mul X,2 => add.int X,X
		IR_type(ir) = IR_ADD;
		free_irs(BIN_opnd1(ir));
		BIN_opnd1(ir) = dup_irs(BIN_opnd0(ir));
		if (get_du_mgr() != NULL && BIN_opnd1(ir)->is_memory_ref()) {
			BIN_opnd1(ir)->copy_ref(BIN_opnd0(ir), this);
		}
		ir->set_parent_pointer(false);
		change = true;
		return ir; //No need for updating DU.
	} else if (IR_is_const(op1) && op1->is_int(get_dm())) {
		if (CONST_int_val(op1) == 1) {
			//mul X,1 => X
			IR * newir = op0;
			BIN_opnd0(ir) = NULL;
			//Do MOT need revise IR_DU_MGR, just keep X original DU info.
			free_irs(ir);
			change = true;
			return newir;
		} else if (CONST_int_val(op1) == 0) {
			//mul X,0 => 0
			if (get_du_mgr() != NULL) {
				get_du_mgr()->remove_use_out_from_defset(ir);
			}
			IR * newir = op1;
			BIN_opnd1(ir) = NULL;
			free_irs(ir);
			change = true;
			return newir;
		} else if (is_power_of_2(CONST_int_val(op1)) &&
				   RC_refine_mul_const(rc)) {
			//mul X,4 => lsl X,2, logical shift left.
			CONST_int_val(op1) = get_power_of_2(CONST_int_val(op1));
			IR_type(ir) = IR_LSL;
			change = true;
			return ir; //No need for updating DU.
		}
	}
	return ir;
}


IR * REGION::refine_band(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_BAND);
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (IR_is_const(op1) &&
		op1->is_int(get_dm()) && CONST_int_val(op1) == -1) {
		//BAND X,-1 => X
		IR * tmp = ir;
		BIN_opnd0(ir) = NULL;
		ir = op0;
		free_irs(tmp);
		change = true;
		return ir; //No need for updating DU.
	}
	return ir;
}


IR * REGION::refine_bor(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_BOR);
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (IR_is_const(op1) &&	op1->is_int(get_dm()) && CONST_int_val(op1) == 0) {
		//BOR X,0 => X
		IR * tmp = ir;
		BIN_opnd0(ir) = NULL;
		ir = op0;
		free_irs(tmp);
		change = true;
		return ir; //No need for updating DU.
	}
	return ir;
}


IR * REGION::refine_land(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_LAND);
	IR * op0 = BIN_opnd0(ir);
	if (IR_is_const(op0) && op0->is_int(get_dm()) && CONST_int_val(op0) == 1) {
		//1 && x => x
		IR * tmp = BIN_opnd1(ir);
		BIN_opnd1(ir) = NULL;
		free_irs(ir);
		change = true;
		return tmp;
	}
	return ir;
}


IR * REGION::refine_lor(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_LOR);
	IR * op0 = BIN_opnd0(ir);
	if (IR_is_const(op0) && op0->is_int(get_dm()) && CONST_int_val(op0) == 1) {
		//1 || x => 1
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(ir);
		}
		IR * tmp = BIN_opnd0(ir);
		BIN_opnd0(ir) = NULL;
		free_irs(ir);
		change = true;
		return tmp;
	}
	return ir;
}


IR * REGION::refine_sub(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_SUB);

	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (op0->is_irs_equal(op1)) {
		//sub X,X => 0
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(ir);
		}
		IR * tmp = ir;
		UINT tyid;
		DT_MGR * dm = get_dm();
		if (op0->is_mc(dm) ||
			op0->is_str(dm) ||
			op0->is_ptr(dm)) {
			tyid = dm->get_simplex_tyid_ex(D_U32);
		} else {
			tyid = IR_dt(op0);
		}
		if (dm->is_fp(tyid)) {
			ir = build_imm_fp(0.0f, tyid);
		} else {
			ir = build_imm_int(0, tyid);
		}
		copy_dbx(ir, tmp, this);
		free_irs(tmp);
		change = true;
		return ir;
	}
	return ir;
}


IR * REGION::refine_xor(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_XOR);

	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (op0->is_irs_equal(op1)) {
		//xor X,X => 0
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(ir);
		}
		IR * tmp = ir;
		UINT tyid;
		DT_MGR * dm = get_dm();
		if (op0->is_mc(dm) ||
			op0->is_str(dm) ||
			op0->is_ptr(dm)) {
			tyid = dm->get_simplex_tyid_ex(D_U32);
		} else {
			tyid = IR_dt(op0);
		}
		IS_TRUE0(dm->is_sint(tyid) || dm->is_uint(tyid));
		ir = build_imm_int(0, tyid);
		copy_dbx(ir, tmp, this);
		free_irs(tmp);
		change = true;
		return ir; //No need for updating DU.
	}
	return ir;
}


IR * REGION::refine_eq(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_EQ);

	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (op0->is_irs_equal(op1) && RC_do_fold_const(rc)) {
		//eq X,X => 1
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(ir);
		}
		IR * tmp = ir;
		ir = build_imm_int(1, get_dm()->get_simplex_tyid_ex(D_B));
		copy_dbx(ir, tmp, this);
		free_irs(tmp);
		change = true;
		//TODO: Inform its parent stmt IR to remove use
		//of the stmt out of du-chain.
		return ir;
	}
	return ir;
}


IR * REGION::refine_ne(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_get_type(ir) == IR_NE);

	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IS_TRUE0(op0 != NULL && op1 != NULL);
	if (op0->is_irs_equal(op1) && RC_do_fold_const(rc)) {
		//ne X,X => 0
		if (get_du_mgr() != NULL) {
			get_du_mgr()->remove_use_out_from_defset(ir);
		}
		IR * tmp = ir;
		ir = build_imm_int(0, get_dm()->get_simplex_tyid_ex(D_B));
		copy_dbx(ir, tmp, this);
		free_irs(tmp);
		change = true;
		//TODO: Inform its parent stmt IR to remove use
		//of the stmt out of du-chain.
		return ir;
	}
	return ir;
}


IR * REGION::reassociation(IR * ir, bool & change)
{
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	IR_TYPE opt1 = (IR_TYPE)IR_type(ir);
	IR_TYPE opt2 = (IR_TYPE)IR_type(op0);

	//If n1,n2 is int const. OP1((OP2 X,n1), n2) => OP2(X, OP1(n1,n2))
	//where OP1, OP2 must be identical precedence.
	DT_MGR * dm = get_dm();
	if (IR_is_const(op1) &&
		op1->is_int(dm) &&
		op0->is_associative() &&
		ir->is_associative() &&
		IR_is_const(BIN_opnd1(op0)) &&
		BIN_opnd1(op0)->is_int(dm) &&
		::get_arth_precedence(opt1) == ::get_arth_precedence(opt2)) {

		HOST_INT v = calc_int_val(opt1,
									CONST_int_val(BIN_opnd1(op0)),
									CONST_int_val(op1));
		DATA_TYPE dt =
			ir->is_ptr(dm) ?
				dm->get_pointer_size_int_dtype():
				ir->is_mc(dm) ? dm->get_dtype(WORD_BITSIZE, true):
							    ir->get_rty(dm);
		IR * new_const = build_imm_int(v, get_dm()->get_simplex_tyid_ex(dt));
		copy_dbx(new_const, BIN_opnd0(ir), this);
		IR_parent(op0) = NULL;
		BIN_opnd0(ir) = NULL;
		free_irs(ir);
		free_irs(BIN_opnd1(op0));
		BIN_opnd1(op0) = new_const;
		change = true;
		op0->set_parent_pointer(false);
		return op0; //No need for updating DU.
	}
	return ir;
}


//Refine binary operations.
IR * REGION::refine_bin_op(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(BIN_opnd0(ir) != NULL && BIN_opnd1(ir) != NULL);
	BIN_opnd0(ir) = refine_ir(BIN_opnd0(ir), change, rc);
	BIN_opnd1(ir) = refine_ir(BIN_opnd1(ir), change, rc);
	if (change) { ir->set_parent_pointer(false); }
	bool lchange = false;
	if (RC_do_fold_const(rc)) {
		ir = fold_const(ir, lchange);
		change |= lchange;
		if (lchange) {
			return ir;
		}
	}
	switch (IR_get_type(ir)) {
	case IR_ADD:
	case IR_MUL:
	case IR_XOR:
	case IR_BAND:
	case IR_BOR:
	case IR_EQ:
	case IR_NE:
		{
			//Operation commutative: ADD(CONST, ...) => ADD(..., CONST)
			if (IR_is_const(BIN_opnd0(ir)) && !IR_is_const(BIN_opnd1(ir))) {
				IR * tmp = BIN_opnd0(ir);
				BIN_opnd0(ir) = BIN_opnd1(ir);
				BIN_opnd1(ir) = tmp;
			}
			if (IR_is_const(BIN_opnd1(ir))) {
				ir = reassociation(ir, lchange);
				change |= lchange;
				if (lchange) { break; }
			}

			if (IR_get_type(ir) == IR_ADD) { ir = refine_add(ir, change, rc); }
			else if (IR_get_type(ir) == IR_XOR) { ir = refine_xor(ir, change, rc); }
			else if (IR_get_type(ir) == IR_BAND) { ir = refine_band(ir, change, rc); }
			else if (IR_get_type(ir) == IR_BOR) { ir = refine_bor(ir, change, rc); }
			else if (IR_get_type(ir) == IR_MUL) { ir = refine_mul(ir, change, rc); }
			else if (IR_get_type(ir) == IR_EQ) { ir = refine_eq(ir, change, rc); }
			else if (IR_get_type(ir) == IR_NE) { ir = refine_ne(ir, change, rc); }
		}
		break;
	case IR_SUB:
		{
			if (IR_is_const(BIN_opnd1(ir))) {
				ir = reassociation(ir, lchange);
				change |= lchange;
			} else {
				ir = refine_sub(ir, change, rc);
			}
		}
		break;
	case IR_DIV:
		ir = refine_div(ir, change, rc);
		break;
	case IR_REM:
		ir = refine_rem(ir, change, rc);
		break;
	case IR_MOD:
		ir = refine_mod(ir, change, rc);
		break;
	case IR_LAND:
		ir = refine_land(ir, change, rc);
		break;
	case IR_LOR:
		ir = refine_lor(ir, change, rc);
		break;
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		break;
	case IR_LT:
		if (IR_is_const(BIN_opnd0(ir)) && !IR_is_const(BIN_opnd1(ir))) {
			IR * tmp = BIN_opnd0(ir);
			BIN_opnd0(ir) = BIN_opnd1(ir);
			BIN_opnd1(ir) = tmp;
			IR_type(ir) = IR_GT;
		}
		break;
	case IR_LE:
		if (IR_is_const(BIN_opnd0(ir)) && !IR_is_const(BIN_opnd1(ir))) {
			IR * tmp = BIN_opnd0(ir);
			BIN_opnd0(ir) = BIN_opnd1(ir);
			BIN_opnd1(ir) = tmp;
			IR_type(ir) = IR_GE;
		}
		break;
	case IR_GT:
		if (IR_is_const(BIN_opnd0(ir)) && !IR_is_const(BIN_opnd1(ir))) {
			IR * tmp = BIN_opnd0(ir);
			BIN_opnd0(ir) = BIN_opnd1(ir);
			BIN_opnd1(ir) = tmp;
			IR_type(ir) = IR_LT;
		}
		break;
	case IR_GE:
		if (IR_is_const(BIN_opnd0(ir)) && !IR_is_const(BIN_opnd1(ir))) {
			IR * tmp = BIN_opnd0(ir);
			BIN_opnd0(ir) = BIN_opnd1(ir);
			BIN_opnd1(ir) = tmp;
			IR_type(ir) = IR_LE;
		}
		break;
	default:
		IS_TRUE0(0);
	}

	//Insert convert if need.
	if (ir->is_bin_op() && RC_insert_cvt(rc)) {
		BIN_opnd0(ir) = insert_cvt(ir, BIN_opnd0(ir), change);
		BIN_opnd1(ir) = insert_cvt(ir, BIN_opnd1(ir), change);
		if (change) { ir->set_parent_pointer(false); }
		insert_cvt_for_binop(ir, change);
	} else if (change) {
		ir->set_parent_pointer(false);
	}
	return ir;
}


IR * REGION::refine_array(IR * ir, bool & change, RF_CTX & rc)
{
	IR * newbase = refine_ir(ARR_base(ir), change, rc);
	if (newbase != ARR_base(ir)) {
		ARR_base(ir) = newbase;
		IR_parent(newbase) = ir;
	}

	IR * newsublist = NULL;
	IR * last = NULL;
	IR * s = removehead(&ARR_sub_list(ir));
	bool lchange = false;
	for (; s != NULL;) {
		IR * newsub = refine_ir(s, change, rc);
		if (newsub != s) {
			IR_parent(newsub) = ir;
			lchange = true;
		}
		add_next(&newsublist, &last, newsub);
		s = removehead(&ARR_sub_list(ir));
	}
	ARR_sub_list(ir) = newsublist;
	return ir;
}


IR * REGION::refine_branch(IR * ir, bool & change, RF_CTX & rc)
{
	if (IR_get_type(ir) == IR_FALSEBR && IR_get_type(BR_det(ir)) == IR_NE) {
		IR_type(ir) = IR_TRUEBR;
		IR_type(BR_det(ir)) = IR_EQ;
	}
	return ir;
}


IR * REGION::refine_ld(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(LD_idinfo(ir));
	VAR * var = LD_idinfo(ir);
	if (VAR_is_array(var)) {
		//Convert LD(v) to LDA(ID(v)) if ID is array.
		/* I think the convert is incorrect. If var a is array,
		then LD(a,U32) means load 32bit element from a,
		e.g: load a[0]. So do not convert LD into LDA.

		//IR * rm = ir;
		//ir = build_lda(build_id(LD_info(ir)));
		//free_ir(rm);
		*/
	}
	return ir;
}


IR * REGION::refine_cvt(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(IR_type(ir) == IR_CVT);
	CVT_exp(ir) = refine_ir(CVT_exp(ir), change, rc);
	if (change) {
		IR_parent(CVT_exp(ir)) = ir;
	}
	if (IR_type(CVT_exp(ir)) == IR_CVT) {
		//cvt1(cvt2,xxx) => cvt1(xxx)
		IR * tmp = CVT_exp(ir);
		CVT_exp(ir) = CVT_exp(CVT_exp(ir));
		CVT_exp(tmp) = NULL;
		free_irs(tmp);
		IR_parent(CVT_exp(ir)) = ir;
		change = true;
	}
	if (IR_dt(ir) == IR_dt(CVT_exp(ir))) {
		//cvt(i64, ld(i64)) => ld(i64)
		IR * tmp = CVT_exp(ir);
		IR_parent(tmp) = IR_parent(ir);
		CVT_exp(ir) = NULL;
		free_irs(ir);
		ir = tmp;
		change = true;
	}
	if (IR_type(ir) == IR_CVT && IR_is_const(CVT_exp(ir)) &&
		((ir->is_int(get_dm()) && CVT_exp(ir)->is_int(get_dm())) ||
		 (ir->is_fp(get_dm()) && CVT_exp(ir)->is_fp(get_dm())))) {
		//cvt(i64, const) => const(i64)
		IR * tmp = CVT_exp(ir);
		IR_dt(tmp) = IR_dt(ir);
		IR_parent(tmp) = IR_parent(ir);
		CVT_exp(ir) = NULL;
		free_irs(ir);
		ir = tmp;
		change = true;
	}
	return ir;
}


/* Perform peephole optimizations.
This function also responsible for normalizing IR and reassociation.
NOTE: This function do NOT generate new STMT. */
IR * REGION::refine_ir(IR * ir, bool & change, RF_CTX & rc)
{
	if (!g_do_refine) return ir;
	if (ir == NULL) return NULL;
	bool tmpc = false;
	switch (IR_get_type(ir)) {
	case IR_CONST:
	case IR_ID:
		break;
	case IR_LD:
		ir = refine_ld(ir, tmpc, rc);
		break;
 	case IR_ST:
	case IR_STPR:
		ir = refine_store(ir, tmpc, rc);
 		break;
	case IR_ILD:
		ir = refine_ild(ir, tmpc, rc);
		break;
	case IR_IST:
		ir = refine_istore(ir, tmpc, rc);
		break;
	case IR_LDA:
		ir = refine_lda(ir, tmpc, rc);
		break;
	case IR_CALL:
		ir = refine_call(ir, tmpc, rc);
		break;
	case IR_ICALL:
		ir = refine_icall(ir, tmpc, rc);
		break;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		ir = refine_bin_op(ir, tmpc, rc);
		break;
	case IR_BNOT:
	case IR_LNOT:
		ir = refine_not(ir, tmpc, rc);
		break;
	case IR_NEG:
		ir = refine_neg(ir, tmpc, rc);
		break;
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
		/* Do NOT do fold_const for conditional expr.
		e.g: If NE(1, 0) => 1, one should generate NE(1, 0) again,
		because of TRUEBR/FALSEBR do not accept IR_CONST. */
		{
			RF_CTX t(rc);
			RC_do_fold_const(t) = false;
			ir = refine_bin_op(ir, tmpc, t);
		}
		break;
	case IR_GOTO:
		break;
	case IR_DO_WHILE:
	case IR_WHILE_DO:
		LOOP_det(ir) = refine_det(LOOP_det(ir), tmpc, rc);
		LOOP_body(ir) = refine_ir_list(LOOP_body(ir), tmpc, rc);
		break;
	case IR_DO_LOOP:
		LOOP_det(ir) = refine_det(LOOP_det(ir), tmpc, rc);
		LOOP_init(ir) = refine_ir_list(LOOP_init(ir), tmpc, rc);
		LOOP_step(ir) = refine_ir_list(LOOP_step(ir), tmpc, rc);
		LOOP_body(ir) = refine_ir_list(LOOP_body(ir), tmpc, rc);
		break;
	case IR_IF:
		IF_det(ir) = refine_det(IF_det(ir), tmpc, rc);
		IF_truebody(ir) = refine_ir_list(IF_truebody(ir), tmpc, rc);
		IF_falsebody(ir) = refine_ir_list(IF_falsebody(ir), tmpc, rc);
		break;
	case IR_LABEL:
		break;
	case IR_IGOTO:
		IGOTO_vexp(ir) = refine_ir(IGOTO_vexp(ir), tmpc, rc);
		break;
	case IR_SWITCH:
		ir = refine_switch(ir, tmpc, rc);
		break;
	case IR_CASE:
		break;
	case IR_ARRAY:
		ir = refine_array(ir, tmpc, rc);
		break;
	case IR_CVT:
		ir = refine_cvt(ir, tmpc, rc);
		break;
	case IR_PR:
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		ir = refine_br(ir, tmpc, rc);
		break;
	case IR_RETURN:
		ir = refine_return(ir, tmpc, rc);
		break;
	case IR_SELECT:
		ir = refine_select(ir, tmpc, rc);
		break;
	case IR_BREAK:
	case IR_CONTINUE:
		break;
	default: IS_TRUE0(0);
	}
	if (tmpc && ir != NULL && ir->is_stmt()) {
		ir->set_parent_pointer(true);
	}
	change |= tmpc;
	return ir;
}


//Reshaping determinate expression.
//Only the last non-stmt expression can be reserved to perform determinating.
IR * REGION::refine_det(IR * ir, bool & change, RF_CTX & rc)
{
	IS_TRUE0(ir);
	ir = refine_ir(ir, change, rc);
	if (!ir->is_judge()) {
		IR * old = ir;
		ir = build_judge(ir);
		copy_dbx(ir, old, this);
		change = true;
	}
	return ir;
}


/* Perform amendment for IRs that via primary
convertion in order to generate legal IR tree.
1. Mergering IR node like as :
	ST(LD(v)) => ST(ID(v))

	Generating icall instead of the deref of a function-pointer:
	DEREF(CALL) => ICALL

2. Delete non-statement node from statement list.
3. Checking OFST of IR.
4. Complementing det of control-flow node
		IF(pr100, TRUE_PART, FALSE_PART) =>
		IF(NE(pr100, 0), TRUE_PART, FALSE_PART)

'ir_list': list to refine.

NOTICE:
	While this function completed, IR's parent-pointer would be
	overrided, set_parent_pointer() should be invoked at all. */
IR * REGION::refine_ir_list(IR * ir_list, bool & change, RF_CTX & rc)
{
	bool lchange = true; //local flag
	while (lchange) {
		lchange = false;
		IR * new_list = NULL;
		IR * last = NULL;
		while (ir_list != NULL) {
			IR * ir = removehead(&ir_list);
			IR * new_ir = refine_ir(ir, lchange, rc);
			add_next(&new_list, &last, new_ir);
		}
		change |= lchange;
		ir_list = new_list;
	}
	return ir_list;
}


bool REGION::refine_ir_stmt_list(IN OUT BBIR_LIST & ir_list, RF_CTX & rc)
{
	if (!g_do_refine) return false;
	bool change = false;
	C<IR*> * ct, * next_ct;
	for (ir_list.get_head(&next_ct), ct = next_ct; ct != NULL; ct = next_ct) {
		IR * ir = C_val(ct);
		ir_list.get_next(&next_ct);
		bool tmpc = false;
		IR * newir = refine_ir(ir, tmpc, rc);
		if (newir != ir) {
			if (!RC_stmt_removed(rc)) {
				ir_list.remove(ct);
			}

			if (newir != NULL) {
				if (next_ct != NULL) {
					ir_list.insert_before(newir, next_ct);
				} else {
					ir_list.append_tail(newir);
				}
			}
			tmpc = true;
		}
		change |= tmpc;
	}
	return change;
}


bool REGION::refine_ir_bb_list(IN OUT IR_BB_LIST * ir_bb_list, RF_CTX & rc)
{
	if (!g_do_refine) { return false; }
	START_TIMER("Refine IR_BB list");
	bool change = false;
	for (IR_BB * bb = ir_bb_list->get_head(); bb != NULL;
		 bb = ir_bb_list->get_next()) {
		change |= refine_ir_stmt_list(IR_BB_ir_list(bb), rc);
	}
	END_TIMER();
	return change;
}


void REGION::insert_cvt_for_binop(IR * ir, bool & change)
{
	IS_TRUE0(ir->is_bin_op());
	IR * op0 = BIN_opnd0(ir);
	IR * op1 = BIN_opnd1(ir);
	DT_MGR * dm = get_dm();
	if (IR_dt(op0) == IR_dt(op1)) {
		if (op0->is_mc(dm)) {
			IS_TRUE(DTD_mc_sz(op0->get_dtd(dm)) ==
					DTD_mc_sz(op1->get_dtd(dm)),
					("invalid binop for two D_MC operands"));
		}
		return;
	}

	if (op0->is_ptr(get_dm())) {
		if (op1->get_dt_size(get_dm()) > op0->get_dt_size(get_dm())) {
			IS_TRUE(get_dm()->is_ptr_addend(IR_dt(op1)) &&
					!op1->is_ptr(get_dm()),
					("illegal pointer arith"));
			DATA_TYPE t = get_dm()->get_pointer_size_int_dtype();
			BIN_opnd1(ir) = build_cvt(op1, get_dm()->get_simplex_tyid_ex(t));
			copy_dbx(BIN_opnd1(ir), op1, this);
			ir->set_parent_pointer(false);
			change = true;
			return;
		}
		return;
	}

	IS_TRUE(!op1->is_ptr(get_dm()),
			("illegal binop for NoNpointer and Pointer"));

	//Both op0 and op1 are NOT pointer.
	if (op0->is_vec(dm) || op1->is_vec(dm)) {
		IS_TRUE0(op0->is_type_equal(op1));
		return;
	}

	//Both op0 and op1 are NOT vector type.
	UINT tyid = dm->hoist_dtype_for_binop(op0, op1);
	UINT dt_size = dm->get_dtd_bytesize(tyid);
	if (op0->get_dt_size(dm) != dt_size) {
		BIN_opnd0(ir) = build_cvt(op0, tyid);
		copy_dbx(BIN_opnd0(ir), op0, this);
		change = true;
		ir->set_parent_pointer(false);
	}
	if (op1->get_dt_size(dm) != dt_size) {
		BIN_opnd1(ir) = build_cvt(op1, tyid);
		copy_dbx(BIN_opnd1(ir), op1, this);
		change = true;
		ir->set_parent_pointer(false);
	}
}


//Insert CVT if need.
IR * REGION::insert_cvt(IR * parent, IR * kid, bool & change)
{
	switch (IR_get_type(parent)) {
	case IR_CONST:
	case IR_PR:
	case IR_ID:
	case IR_BREAK:
	case IR_CONTINUE:
		IS_TRUE0(0);
		return kid;
 	case IR_ST:
	case IR_STPR:
	case IR_LD:
	case IR_IST:
	case IR_ILD:
	case IR_LDA:
	case IR_CALL:
	case IR_ICALL:
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_GOTO:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_IGOTO:
	case IR_SWITCH:
	case IR_CASE:
	case IR_ARRAY:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_CVT:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_RETURN:
	case IR_SELECT:
		{
			DT_MGR * dm = get_dm();
			UINT tgt_ty = IR_dt(parent);
			if (parent->is_vec(dm) || kid->is_vec(dm)) {
				//Can not do CVT between vector type and general type.
				IS_TRUE0(parent->is_type_equal(kid));
				//Do nothing.
				return kid;
			}
			UINT tgt_size = parent->get_dt_size(dm);
			UINT src_size = kid->get_dt_size(dm);
			if (tgt_size <= src_size) {
				//Do nothing.
				return kid;
			}
			if (IR_is_const(kid) && kid->is_int(dm)) {
				IS_TRUE0(IR_dt(kid) != D_MC);
				if (dm->is_ptr(tgt_ty)) {
					IR_dt(kid) = dm->get_pointer_size_int_dtype();
				} else {
					IR_dt(kid) = tgt_ty;
				}
				change = true;
				return kid;
			}
			IR * new_kid = build_cvt(kid, IR_dt(parent));
			copy_dbx(new_kid, kid, this);
			change = true;
			return new_kid;
		}
		break;
	default: IS_TRUE0(0);
	}
	change = true;
	return kid;
}


HOST_INT REGION::calc_int_val(IR_TYPE ty, HOST_INT v0, HOST_INT v1)
{
	switch (ty) {
	case IR_ADD:
		v1 = v0 + v1;
		break;
	case IR_SUB:
		v1 = v0 - v1;
		break;
	case IR_MUL:
		v1 = v0 * v1;
		break;
	case IR_DIV:
		v1 = v0 / v1;
		break;
	case IR_REM:
		v1 = v0 % v1;
		break;
	case IR_MOD:
		v1 = v0 % v1;
		break;
	case IR_LAND:
		v1 = v0 && v1;
		break;
	case IR_LOR:
		v1 = v0 || v1;
		break;
	case IR_BAND:
		v1 = v0 & v1;
		break;
	case IR_BOR:
		v1 = v0 | v1;
		break;
	case IR_XOR:
		v1 = v0 ^ v1;
		break;
	case IR_BNOT:
		v1 = ~v0;
		break;
	case IR_LNOT:
		v1 = !v0;
		break;
	case IR_LT:
		v1 = v0 < v1;
		break;
	case IR_LE:
		v1 = v0 <= v1;
		break;
	case IR_GT:
		v1 = v0 > v1;
		break;
	case IR_GE:
		v1 = v0 >= v1;
		break;
	case IR_EQ:
		v1 = v0 == v1;
		break;
	case IR_NE:
		v1 = v0 != v1;
		break;
	case IR_ASR:
		v1 = v0 >> v1;
		break;
	case IR_LSR:
		v1 = ((HOST_UINT)v0) >> v1;
		break;
	case IR_LSL:
		v1 = v0 << v1;
		break;
	default:
		IS_TRUE0(0);
	} //end switch
	return v1;
}


IR * REGION::fold_const_int_una(IR * ir, bool & change)
{
	IS_TRUE0(ir->is_unary_op());
	DT_MGR * dm = get_dm();
	IS_TRUE0(IR_is_const(UNA_opnd0(ir)));
	HOST_INT v0 = CONST_int_val(UNA_opnd0(ir));
	if (IR_type(ir) == IR_NEG) {
		IS_TRUE(dm->get_dtd_bytesize(IR_dt(UNA_opnd0(ir))) <= 8, ("TODO"));
		IR * oldir = ir;
		ir = build_imm_int(-v0, IR_dt(ir));
		copy_dbx(ir, oldir, this);
		free_irs(oldir);
		change = true;
		return ir;
	} else if (IR_type(ir) == IR_LNOT) {
		IS_TRUE(dm->get_dtd_bytesize(IR_dt(UNA_opnd0(ir))) <= 8, ("TODO"));
		IR * oldir = ir;
		ir = build_imm_int(!v0, IR_dt(ir));
		copy_dbx(ir, oldir, this);
		free_irs(oldir);
		change = true;
		return ir;
	} else if (IR_type(ir) == IR_BNOT) {
		IS_TRUE(dm->get_dtd_bytesize(IR_dt(UNA_opnd0(ir))) <= 8, ("TODO"));
		IR * oldir = ir;
		ir = build_imm_int(~v0, IR_dt(ir));
		copy_dbx(ir, oldir, this);
		free_irs(oldir);
		change = true;
		return ir;
	}
	return ir;
}


//Fold const for binary operation.
IR * REGION::fold_const_int_bin(IR * ir, bool & change)
{
	IS_TRUE0(ir->is_bin_op());
	DT_MGR * dm = get_dm();
	IS_TRUE0(IR_is_const(BIN_opnd0(ir)));
	HOST_INT v0 = CONST_int_val(BIN_opnd0(ir));

	IS_TRUE0(IR_is_const(BIN_opnd1(ir)));
	HOST_INT v1 = CONST_int_val(BIN_opnd1(ir));
	INT tylen = MAX(dm->get_dtd_bytesize(IR_dt(BIN_opnd0(ir))),
					dm->get_dtd_bytesize(IR_dt(BIN_opnd1(ir))));
	IS_TRUE(tylen <= 8, ("TODO"));
	IR * oldir = ir;
	bool lchange = false;
	switch (IR_type(ir)) {
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		{
			IR * x = NULL;
			if (ir->is_bool(dm)) {
				UINT tyid = get_dm()->get_simplex_tyid_ex(D_U32);
				x = build_imm_int(calc_int_val(IR_get_type(ir), v0, v1), tyid);
			} else if (ir->is_fp(get_dm())) {
				//The result type of binary operation is
				//float point, inserting IR_CVT.
				UINT tyid =
					get_dm()->hoist_dtype_for_binop(BIN_opnd0(ir),
													BIN_opnd1(ir));

				x = build_cvt(
						build_imm_int(
							calc_int_val((IR_TYPE)IR_type(ir), v0, v1), tyid),
						IR_dt(ir));
			} else {
				IS_TRUE0(ir->is_int(get_dm()));
				x = build_imm_int(
						calc_int_val((IR_TYPE)IR_type(ir), v0, v1), IR_dt(ir));
			}
			copy_dbx(x, ir, this);
			ir = x;
			lchange = true;
		}
		break;
	default: IS_TRUE0(0);
	} //end switch
	if (lchange) {
		free_irs(oldir);
		change = true;
	}
	return ir; //No need for updating DU.
}


double REGION::calc_float_val(IR_TYPE ty, double v0, double v1)
{
	switch (ty) {
	case IR_ADD:
		v1 = v0 + v1;
		break;
	case IR_SUB:
		v1 = v0 - v1;
		break;
	case IR_MUL:
		v1 = v0 * v1;
		break;
	case IR_DIV:
		v1 = v0 / v1;
		break;
	case IR_LNOT:
		v1 = !v0;
		break;
	case IR_LT:
		v1 = v0 < v1;
		break;
	case IR_LE:
		v1 = v0 <= v1;
		break;
	case IR_GT:
		v1 = v0 > v1;
		break;
	case IR_GE:
		v1 = v0 >= v1;
		break;
	case IR_EQ:
		v1 = v0 == v1;
		break;
	case IR_NE:
		v1 = v0 != v1;
		break;
	default:
		;
	} //end switch
	return v1;
}


IR * REGION::fold_const_float_una(IR * ir, bool & change)
{
	IS_TRUE0(ir->is_unary_op());
	DT_MGR * dm = get_dm();
	if (IR_get_type(ir) == IR_NEG) {
		IS_TRUE(dm->get_dtd_bytesize(IR_dt(UNA_opnd0(ir))) <= 8, ("TODO"));
		IR * oldir = ir;
		ir = build_imm_fp(-CONST_fp_val(UNA_opnd0(ir)), IR_dt(ir));
		copy_dbx(ir, oldir, this);
		free_irs(oldir);
		change = true;
		return ir; //No need for updating DU.
	} else if (IR_get_type(ir) == IR_LNOT) {
		IS_TRUE(dm->get_dtd_bytesize(IR_dt(UNA_opnd0(ir))) <= 8, ("TODO"));
		IR * oldir = ir;
		HOST_FP t = CONST_fp_val(UNA_opnd0(ir));
		if (t == 0.0) {
			t = 1.0;
		} else {
			t = 0.0;
		}
		ir = build_imm_fp(t, IR_dt(ir));
		copy_dbx(ir, oldir, this);
		free_irs(oldir);
		change = true;
		return ir; //No need for updating DU.
	}
	return ir;
}


//Binary operation.
IR * REGION::fold_const_float_bin(IR * ir, bool & change)
{
	DT_MGR * dm = get_dm();
	IS_TRUE0(ir->is_bin_op());
	IS_TRUE0(IR_is_const(BIN_opnd0(ir)) && BIN_opnd0(ir)->is_fp(dm) &&
			IR_is_const(BIN_opnd1(ir)) && BIN_opnd1(ir)->is_fp(dm));
	double v0 = CONST_fp_val(BIN_opnd0(ir));
	double v1 = CONST_fp_val(BIN_opnd1(ir));
	INT tylen = MAX(dm->get_dtd_bytesize(IR_dt(BIN_opnd0(ir))),
					dm->get_dtd_bytesize(IR_dt(BIN_opnd1(ir))));
	IS_TRUE(tylen <= 8, ("TODO"));
	IR * oldir = ir;
	bool lchange = false;
	switch (IR_get_type(ir)) {
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
		ir = build_imm_fp(calc_float_val(IR_get_type(ir), v0, v1),
						  IR_dt(ir));
		copy_dbx(ir, oldir, this);
		lchange = true;
		break;
	default:
		;
	} //end switch

	if (lchange) {
		free_irs(oldir);
		change = true;
	}
	return ir; //No need for updating DU.
}


IR * REGION::fold_const(IR * ir, bool & change)
{
	bool doit = false;
	for(INT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid != NULL) {
			IR * new_kid = fold_const(kid, change);
			if (new_kid != kid) {
				doit = true;
				ir->set_kid(i, new_kid);
			}
		}
	}
	if (doit) {
		ir->set_parent_pointer();
	}

	DT_MGR * dm = get_dm();
	switch (IR_get_type(ir)) {
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		{

			IR * t0 = BIN_opnd0(ir);
			IR * t1 = BIN_opnd1(ir);
			IS_TRUE0(ir->is_bin_op());
			IS_TRUE(IR_MAX_KID_NUM(ir) == 2, ("binary op"));
			IS_TRUE(t0 != NULL && t1 != NULL, ("binary op"));
			if ((IR_is_const(t0) && t0->is_fp(dm) &&
				 IR_is_const(t1) && t1->is_fp(dm)) &&
				 g_is_opt_float) {
				return fold_const_float_bin(ir, change);
			} else if (IR_is_const(t0) && IR_is_const(t1) &&
					   t0->is_int(dm) && t1->is_int(dm)) {
				return fold_const_int_bin(ir, change);
			} //end if
		}
		break;
	case IR_BNOT:
	case IR_LNOT: //Boolean logical not e.g LNOT(0x0001) = 0x0000
	case IR_NEG:
		{
			//NEG(1.0) => INT(-1.0)
			IS_TRUE(IR_MAX_KID_NUM(ir) == 1, ("unary op"));
			IS_TRUE0(UNA_opnd0(ir) != NULL);
			if (IR_is_const(UNA_opnd0(ir)) &&
				UNA_opnd0(ir)->is_fp(dm) && g_is_opt_float) {
				ir = fold_const_float_una(ir, change);
				if (change) { return ir; }
			} else if (IR_is_const(UNA_opnd0(ir)) && UNA_opnd0(ir)->is_int(dm)) {
				ir = fold_const_int_una(ir, change);
				if (change) { return ir; }
			} //end if
		}
		break;
	default: return ir;
	}

	//Logical expression equvialence substitution.
	switch (IR_get_type(ir)) {
	case IR_LT:
		{
			//LT(UNSIGNED, 0) always be false.
			IR * opnd1 = BIN_opnd1(ir);
			if (!BIN_opnd0(ir)->is_signed(dm) && IR_is_const(opnd1) &&
				opnd1->is_int(dm) && CONST_int_val(opnd1) == 0) {
				if (get_du_mgr() != NULL) {
					get_du_mgr()->remove_use_out_from_defset(ir);
				}
				IR * x = ir;
				ir = build_imm_int(0, IR_dt(ir));
				copy_dbx(ir, x, this);
				free_irs(x);
			}
		}
		break;
	case IR_GE:
		{
			//GE(UNSIGNED, 0) always be true.
			IR * opnd1 = BIN_opnd1(ir);
			if (!BIN_opnd0(ir)->is_signed(dm) && IR_is_const(opnd1) &&
				opnd1->is_int(dm) && CONST_int_val(opnd1) == 0) {
				IR * x = build_imm_int(1, IR_dt(ir));
				copy_dbx(x, ir, this);
				if (get_du_mgr() != NULL) {
					get_du_mgr()->remove_use_out_from_defset(ir);
				}
				free_irs(ir);
				ir = x;
			}
		}
		break;
	case IR_NE:
		{
			//address of string always not be 0x0.
			//NE(LDA(SYM), 0) -> 1
			IR * opnd0 = BIN_opnd0(ir);
			IR * opnd1 = BIN_opnd1(ir);
			if ((IR_get_type(opnd0) == IR_LDA &&
				 LDA_base(opnd0)->is_str(dm) &&
				 IR_is_const(opnd1) && opnd1->is_int(dm) &&
				 CONST_int_val(opnd1) == 0)
				||
				(IR_get_type(opnd1) == IR_LDA &&
				 LDA_base(opnd1)->is_str(dm) &&
				 IR_is_const(opnd0) &&
				 opnd0->is_int(dm) &&
				 CONST_int_val(opnd0) == 0)) {
				IR * x = build_imm_int(1, IR_dt(ir));
				copy_dbx(x, ir, this);
				free_irs(ir);
				ir = x;
				change = true;
			}
		}
		break;
	case IR_EQ:
		{
			//address of string always not be 0x0.
			//EQ(LDA(SYM), 0) -> 0
			IR * opnd0 = BIN_opnd0(ir);
			IR * opnd1 = BIN_opnd1(ir);
			if ((IR_get_type(opnd0) == IR_LDA &&
				 LDA_base(opnd0)->is_str(dm) &&
				 IR_is_const(opnd1) &&
				 opnd1->is_int(dm) &&
				 CONST_int_val(opnd1) == 0)
				||
				(IR_get_type(opnd1) == IR_LDA &&
				 LDA_base(opnd1)->is_str(dm) &&
				 IR_is_const(opnd0) &&
				 opnd0->is_int(dm) &&
				 CONST_int_val(opnd0) == 0)) {
				IR * x = build_imm_int(0, IR_dt(ir));
				copy_dbx(x, ir, this);
				free_irs(ir);
				ir = x;
				change = true;
			}
		}
		break;
	case IR_ASR: //>>
	case IR_LSR: //>>
		{
			IR * opnd0 = BIN_opnd0(ir);
			IR * opnd1 = BIN_opnd1(ir);
			if (IR_is_const(opnd0) &&
				opnd0->is_int(dm) && CONST_int_val(opnd0) == 0) {
				IR * newir = build_imm_int(0, IR_dt(ir));
				copy_dbx(newir, ir, this);
				free_irs(ir);
				ir = newir;
				change = true;
			} else if (IR_is_const(opnd1) &&
						opnd1->is_int(dm) && CONST_int_val(opnd1) == 0) {
				IR * newir = opnd0;
				BIN_opnd0(ir) = NULL;
				free_irs(ir);
				ir = newir;
				change = true;
			}
		}
		break;
	case IR_LSL: //<<
		{
			IR * opnd0 = BIN_opnd0(ir);
			IR * opnd1 = BIN_opnd1(ir);
			if (IR_is_const(opnd0) &&
				opnd0->is_int(dm) && CONST_int_val(opnd0) == 0) {
				IR * newir = build_imm_int(0, IR_dt(ir));
				copy_dbx(newir, ir, this);
				free_irs(ir);
				ir = newir;
				change = true;
			} else if (IR_is_const(opnd1) &&
						opnd1->is_int(dm)) {
				if (CONST_int_val(opnd1) == 0) {
					//x<<0 => x
					IR * newir = opnd0;
					BIN_opnd0(ir) = NULL;
					free_irs(ir);
					ir = newir;
					change = true;
				} else if (opnd0->get_dt_size(dm) == 4 &&
						   CONST_int_val(opnd1) == 32) {
					//x<<32 => 0, x is 32bit
					IR * newir = build_imm_int(0, IR_dt(ir));
					copy_dbx(newir, ir, this);
					free_irs(ir);
					ir = newir;
					change = true;
				}
			}
		}
		break;
	default:;
	}
	return ir; //No need for updating DU.
}


IR * REGION::strength_reduce(IN OUT IR * ir, IN OUT bool & change)
{
	return fold_const(ir, change);
}


//User must invoke 'set_parent_pointer' to maintain the validation of IR tree.
void REGION::invert_cond(IR ** cond)
{
	switch (IR_get_type(*cond)) {
	case IR_LAND:
	case IR_LOR:
		{
			IR * parent = IR_parent(*cond);
			IR * newir = build_logical_op(IR_LNOT, *cond, NULL);
			copy_dbx(newir, *cond, this);
			IR_parent(newir) = parent;
			*cond = newir;
			newir->set_parent_pointer(true);
			//Or if you want, can generate ir as following rule:
			//	!(a||b) = !a && !b;
			//	!(a&&b) = !a || !b;
			break;
		}
	case IR_LT:
		IR_type(*cond) = IR_GE;
		break;
	case IR_LE:
		IR_type(*cond) = IR_GT;
		break;
	case IR_GT:
		IR_type(*cond) = IR_LE;
		break;
	case IR_GE:
		IR_type(*cond) = IR_LT;
		break;
	case IR_EQ:
		IR_type(*cond) = IR_NE;
		break;
	case IR_NE:
		IR_type(*cond) = IR_EQ;
		break;
	default:
		IS_TRUE(0, ("TODO"));
	}
}
