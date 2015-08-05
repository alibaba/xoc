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
#include "cfs_mgr.h"

bool REGION::is_lowest_heigh_exp(IR const* ir) const
{
	IS_TRUE0(ir->is_exp());
	if (ir->is_leaf()) return true;
	switch (IR_type(ir)) {
	case IR_LAND: //logical and &&
	case IR_LOR: //logical or ||
	case IR_LNOT: //logical not
	case IR_ARRAY:
		return false;
	case IR_ILD:
	case IR_BAND: //inclusive and &
	case IR_BOR: //inclusive or  |
	case IR_XOR: //exclusive or ^
	case IR_BNOT: //bitwise not
	case IR_NEG: //negative
	case IR_EQ: //==
	case IR_NE: //!=
	case IR_LT:
	case IR_GT:
	case IR_GE:
	case IR_LE:
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LABEL:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_CVT: //data type convertion
		{
			for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
				IR * x = ir->get_kid(i);
				if (x != NULL && !x->is_leaf()) {
					return false;
				}
			}
			return true;
		}
		break;
	case IR_SELECT:
		{
			IR * x = SELECT_det(ir);
			if (!is_lowest_heigh_exp(x)) { return false; }
			x = SELECT_trueexp(ir);
			if (x != NULL && !x->is_leaf()) { return false; }
			x = SELECT_falseexp(ir);
			if (x != NULL && !x->is_leaf()) { return false; }
			return true;
		}
	default:
		{
			CHAR const* n = IRNAME(ir);
			IS_TRUE0(0);
		}
	} //end switch
	return true;
}


bool REGION::is_lowest_heigh(IR const* ir) const
{
	IS_TRUE0(ir);
	IS_TRUE0(ir->is_stmt());
	switch (IR_type(ir)) {
	case IR_CALL:
	case IR_ICALL:
		for (IR * p = CALL_param_list(ir); p != NULL; p = IR_next(p)) {
			if (!is_lowest_heigh_exp(p)) {
				return false;
			}
		}
		return true;
	case IR_ST:
		return is_lowest_heigh_exp(ST_rhs(ir));
	case IR_STPR:
		return is_lowest_heigh_exp(STPR_rhs(ir));
	case IR_IST: //indirective store
		if (!is_lowest_heigh_exp(IST_base(ir))) return false;
		return is_lowest_heigh_exp(IST_rhs(ir));
	case IR_GOTO:
	case IR_LABEL:
	case IR_CASE:
		return true;
	case IR_RETURN:
		if (RET_exp(ir) != NULL) {
			return is_lowest_heigh_exp(RET_exp(ir));
		}
		return true;
	case IR_TRUEBR:
	case IR_FALSEBR:
		{
			IR * e = BR_det(ir);
			IS_TRUE0(e);
			return is_lowest_heigh_exp(e);
		}
	case IR_BREAK:
	case IR_CONTINUE:
	case IR_IF:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_SWITCH:
	case IR_IGOTO:
		return false;
	default: IS_TRUE0(0);
	}
	return true;
}


/* Transform if to:
	falsebr(label(ELSE_START))
	...
	TRUE-BODY-STMT-LIST;
	...
	goto IF_END;
	ELSE_START:
	...
	FALSE-BODY-STMT-LIST;
	...
	IF_END: */
IR * REGION::simplify_if(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	IS_TRUE(IR_type(ir) == IR_IF, ("expect IR_IF node"));
	SIMP_CTX tcont(*ctx);
	SIMP_ret_array_val(&tcont) = 1;

	//Det exp.
	//When we first lowering CFS, det-expression should not be TRUEBR/FASLEBR.
	IS_TRUE0(IF_det(ir)->is_judge());
	IR * det = simplify_det(IF_det(ir), &tcont);
	IR * last = removetail(&det);
	IS_TRUE(!last->is_stmt(), ("invalide det exp"));
	IS_TRUE(last->is_judge(),
		("While lowering CFS, det-expression should be judgement."));
	IR * falsebr = build_branch(false, last, gen_ilab());
	copy_dbx(falsebr, IF_det(ir), this);
	add_next(&det, falsebr);

	IR * truebody = simplify_stmt_list(IF_truebody(ir), ctx);
	IR * elsebody = NULL;
	if (IF_falsebody(ir) != NULL) { //Simplify ELSE body
		//append GOTO following end of true body
		IR * go = build_goto(gen_ilab());
		copy_dbx(go, IF_det(ir), this);
		add_next(&truebody, go);

		//truebody end label
		add_next(&truebody, build_label(BR_lab(falsebr)));

		//simplify false body
		elsebody = simplify_stmt_list(IF_falsebody(ir), ctx);

		//falsebody end label
		add_next(&elsebody, build_label(GOTO_lab(go)));
	} else {
		//end label	of truebody.
		add_next(&truebody, build_label(BR_lab(falsebr)));
	}

	if (SIMP_is_record_cfs(ctx)) {
		//Record high level control flow structure.
		CFS_INFO * ci = NULL;

		IS_TRUE0(SIMP_cfs_mgr(ctx));
		ci = SIMP_cfs_mgr(ctx)->new_cfs_info(IR_IF);
		SIMP_cfs_mgr(ctx)->set_map_ir2cfsinfo(falsebr, ci);
		CFS_INFO_ir(ci) = falsebr;

		SIMP_cfs_mgr(ctx)->record_ir_stmt(truebody, *CFS_INFO_true_body(ci));
		SIMP_cfs_mgr(ctx)->record_ir_stmt(elsebody, *CFS_INFO_false_body(ci));
	}
	IR * ret_list = NULL;
	add_next(&ret_list, det);
	add_next(&ret_list, truebody);
	add_next(&ret_list, elsebody);
	for (IR * p = ret_list; p != NULL; p = IR_next(p)) {
		IR_parent(p) = NULL;
	}
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	return ret_list;
}


/* Transform while-do to:
	LABEL: start
	WHILE-DO-DET
	FALSEBR L1
	BODY-STMT
	GOTO start
	LABEL: L1 */
IR * REGION::simplify_while_do(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	SIMP_CTX local;
	local.copy_topdown_flag(*ctx);
	IR * ret_list = NULL;
	IS_TRUE(IR_type(ir) == IR_WHILE_DO, ("expect IR_WHILE_DO node"));
	LABEL_INFO * startl = gen_ilab();

	//det exp
	//When we first lowering CFS,
	//det-expression should not be TRUEBR/FASLEBR.
	IS_TRUE0(LOOP_det(ir)->is_judge());
	SIMP_CTX tcont(*ctx);
	SIMP_ret_array_val(&tcont) = 1;
	IR * det = simplify_det(LOOP_det(ir), &tcont);
	IR * last  = removetail(&det);
	IS_TRUE(!last->is_stmt() && last->is_judge(), ("invalide det exp"));
	IR * falsebr = build_branch(false, last, gen_ilab());
	copy_dbx(falsebr, ir, this);
	add_next(&det, falsebr);

	SIMP_break_label(&local) = BR_lab(falsebr);
	SIMP_continue_label(&local) = startl;

	//loop body
	IR * body = simplify_stmt_list(LOOP_body(ir), &local);
	add_next(&body, build_goto(startl));

	if (SIMP_is_record_cfs(ctx)) {
		//Record high level control flow structure.
		CFS_INFO * ci = NULL;
		IS_TRUE0(SIMP_cfs_mgr(ctx));
		ci = SIMP_cfs_mgr(ctx)->new_cfs_info(IR_WHILE_DO);
		SIMP_cfs_mgr(ctx)->set_map_ir2cfsinfo(falsebr, ci);
		CFS_INFO_ir(ci) = falsebr;
		SIMP_cfs_mgr(ctx)->record_ir_stmt(body, *CFS_INFO_loop_body(ci));

		//Falsebr is executed each iter.
		CFS_INFO_loop_body(ci)->bunion(IR_id(falsebr));
	}

	add_next(&ret_list, build_label(startl));
	add_next(&ret_list, det);
	add_next(&ret_list, body);
	add_next(&ret_list, build_label(BR_lab(falsebr)));
	for (IR * p = ret_list; p != NULL; p = IR_next(p)) {
		IR_parent(p) = NULL;
	}
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	return ret_list;
}


/* Transform do-while to:
	LABEL: start
	BODY-STMT
	LABEL: det_start
	DO-WHILE-DET
	TRUEBR start */
IR * REGION::simplify_do_while (IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	SIMP_CTX local;
	local.copy_topdown_flag(*ctx);
	IR * ret_list = NULL;
	IS_TRUE(IR_type(ir) == IR_DO_WHILE, ("expect IR_DO_WHILE node"));

	LABEL_INFO * startl = gen_ilab();
	LABEL_INFO * endl = gen_ilab();
	LABEL_INFO * det_startl = gen_ilab();

	//det exp
	//When we first lowering CFS, det-expression should not be TRUEBR/FASLEBR.
	IS_TRUE0(LOOP_det(ir)->is_judge());
	SIMP_CTX tcont(*ctx);
	SIMP_ret_array_val(&tcont) = 1;
	IR * det = simplify_det(LOOP_det(ir), &tcont);
	IR * last  = removetail(&det);
	IS_TRUE(!last->is_stmt() && last->is_judge(), ("invalide det exp"));
	IR * truebr = build_branch(true, last, startl);
	copy_dbx(truebr, ir, this);
	add_next(&det, truebr);

	SIMP_break_label(&local) = endl;
	SIMP_continue_label(&local) = det_startl;

	IR * body = simplify_stmt_list(LOOP_body(ir), &local);
	insertbefore_one(&body, body, build_label(startl));

	if (SIMP_is_record_cfs(ctx)) {
		//Record high level control flow structure.
		CFS_INFO * ci = NULL;
		IS_TRUE0(SIMP_cfs_mgr(ctx));
		ci = SIMP_cfs_mgr(ctx)->new_cfs_info(IR_DO_WHILE);
		SIMP_cfs_mgr(ctx)->set_map_ir2cfsinfo(truebr, ci);
		CFS_INFO_ir(ci) = truebr;
		SIMP_cfs_mgr(ctx)->record_ir_stmt(body, *CFS_INFO_loop_body(ci));

		//'truebr' is executed during each iteration.
		CFS_INFO_loop_body(ci)->bunion(IR_id(truebr));
	}

	last = get_last(ret_list);
	add_next(&ret_list, &last, body);
	add_next(&ret_list, &last, build_label(det_startl));
	add_next(&ret_list, &last, det);
	add_next(&ret_list, &last, build_label(endl));
	for (IR * p = ret_list; p != NULL; p = IR_next(p)) {
		IR_parent(p) = NULL;
	}
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	return ret_list;
}


/* Transform do-loop to:
	INIT-STMT
	LABEL: start
	  WHILE-DO-DET
	FALSEBR L1
	BODY-STMT
	LABEL: step
	STEP
	GOTO start
	LABEL: L1 */
IR * REGION::simplify_do_loop(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	SIMP_CTX local;
	local.copy_topdown_flag(*ctx);
	IR * ret_list = NULL;
	IS_TRUE(IR_type(ir) == IR_DO_LOOP, ("expect IR_DO_LOOP node"));

	LABEL_INFO * startl = gen_ilab();

	//det exp
	//When we first lowering CFS, det-expression should not be TRUEBR/FASLEBR.
	IS_TRUE0(LOOP_det(ir)->is_judge());
	SIMP_CTX tcont(*ctx);
	SIMP_ret_array_val(&tcont) = 1;
	IR * det = simplify_det(LOOP_det(ir), &tcont);
	IR * last  = removetail(&det);
	IS_TRUE(!last->is_stmt() && last->is_judge(), ("invalide det exp"));
	IR * falsebr = build_branch(false, last, gen_ilab());
	copy_dbx(falsebr, LOOP_det(ir), this);
	add_next(&det, falsebr);

	LABEL_INFO * stepl = gen_ilab();

	SIMP_break_label(&local) = BR_lab(falsebr);
	SIMP_continue_label(&local) = stepl;

	IR * init = simplify_stmt_list(LOOP_init(ir), &local);
	IR * step = simplify_stmt_list(LOOP_step(ir), &local);
	IR * body = simplify_stmt_list(LOOP_body(ir), &local);

	//step label , for simp 'continue' used
	add_next(&body, build_label(stepl));
	add_next(&body, step);
	add_next(&body, build_goto(startl));

	if (SIMP_is_record_cfs(ctx)) {
		//Record high level control flow structure.
		CFS_INFO * ci = NULL;
		IS_TRUE0(SIMP_cfs_mgr(ctx) != NULL);
		ci = SIMP_cfs_mgr(ctx)->new_cfs_info(IR_DO_LOOP);
		SIMP_cfs_mgr(ctx)->set_map_ir2cfsinfo(falsebr, ci);
		CFS_INFO_ir(ci) = falsebr;
		SIMP_cfs_mgr(ctx)->record_ir_stmt(body, *CFS_INFO_loop_body(ci));

		//'falsebr' is executed during each iteration.
		CFS_INFO_loop_body(ci)->bunion(IR_id(falsebr));
	}

	add_next(&ret_list, init);
	add_next(&ret_list, build_label(startl));
	add_next(&ret_list, det);
	add_next(&ret_list, body);
	add_next(&ret_list, build_label(BR_lab(falsebr)));
	for (IR * p = ret_list; p != NULL; p = IR_next(p)) {
		IR_parent(p) = NULL;
	}
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	return ret_list;
}


//Simplify determination of Control Flow Structure.
IR * REGION::simplify_det(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	IR * ret_exp_list = NULL;
	IR * next;
	while (ir != NULL) {
		next = IR_next(ir);
		IR_next(ir) = IR_prev(ir) = NULL;
		if (ir->is_stmt()) {
			SIMP_CTX tcont(*ctx);
			IR * new_stmt_list = simplify_stmt(ir, &tcont);
			ctx->union_bottomup_flag(tcont);

			#ifdef _DEBUG_
			IR * x = new_stmt_list;
			while (x != NULL) {
				IS_TRUE0(x->is_stmt());
				x = IR_next(x);
			}
			#endif
			ctx->append_irs(new_stmt_list);
		} else if (ir->is_exp()) {
			SIMP_CTX tcont(*ctx);
			IR * new_exp = simplify_judge_exp(ir, &tcont);
			IS_TRUE0(new_exp->is_exp());
			add_next(&ret_exp_list, new_exp);
			ctx->append_irs(tcont);
			ctx->union_bottomup_flag(tcont);
		} else {
			IS_TRUE(0, ("unknonw ir type"));
		}
		ir = next;
	}
	for (IR * p = ret_exp_list; p != NULL; p = IR_next(p)) {
		IR_parent(p) = NULL;
	}
	return ret_exp_list;
}


/* Return expression's value.s
e.g: a = !(exp), where rhs would be translated to:
	----------
	truebr(exp != 0), L1
	pr = 1
	goto L2
	L1:
	pr = 0
	L2:
	----------
	a = pr
In the case, return 'pr'. */
IR * REGION::simplify_logical_not(IN IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(IR_type(ir) == IR_LNOT);
	LABEL_INFO * label1 = gen_ilab();
	IR * pr = build_pr(IR_dt(ir));
	alloc_ref_for_pr(pr);
	IR * ret_list = NULL;

	//truebr(exp != 0), L1
	IR * opnd0 = UNA_opnd0(ir);
	UNA_opnd0(ir) = NULL;
	if (!opnd0->is_judge()) {
		opnd0 = build_cmp(IR_NE, opnd0, build_imm_int(0, IR_dt(opnd0)));
	}
	IR * true_br = build_branch(true, opnd0, label1);
	copy_dbx(true_br, ir, this);
	add_next(&ret_list, true_br);

	DT_MGR * dm = get_dm();
	//pr = 1
	UINT t = dm->get_simplex_tyid_ex(
				dm->get_dtype(WORD_LENGTH_OF_HOST_MACHINE, true));
	IR * imm0 = build_imm_int(1, t);
	IR * x = build_store_pr(PR_no(pr), IR_dt(pr), imm0);
	copy_dbx(x, imm0, this);
	add_next(&ret_list, x);

	//goto L2
	LABEL_INFO * label2 = gen_ilab();
	add_next(&ret_list, build_goto(label2));

	//L1:
	add_next(&ret_list, build_label(label1));

	//pr = 0
	UINT t2 = dm->get_simplex_tyid_ex(
					dm->get_dtype(WORD_LENGTH_OF_HOST_MACHINE, true));
	IR * imm1 = build_imm_int(0, t2);

	x = build_store_pr(PR_no(pr), IR_dt(pr), imm1);
	copy_dbx(x, imm1, this);
	add_next(&ret_list, x);

	//L2:
	add_next(&ret_list, build_label(label2));
	ctx->append_irs(ret_list);
	free_irs(ir);
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	return pr;
}


/* Return expression's value.
e.g: a = b&&c, where rhs would be translated to:
	----------
	falsebr(b != 0), L0
	truebr(c != 0), L1
	L0:
	pr = 0
	goto L2
	L1:
	pr = 1
	L2:
	----------
	a = pr
In the case, return 'pr'. */
IR * REGION::simplify_logical_and(IN IR * ir, IN LABEL_INFO * label,
								  SIMP_CTX * ctx)
{
	IS_TRUE0(IR_type(ir) == IR_LAND);
	LABEL_INFO * label1 = gen_ilab();
	IR * pr = build_pr(IR_dt(ir));
	alloc_ref_for_pr(pr);
	IR * ret_list = simplify_logical_and_truebr(ir, label1);
	DT_MGR * dm = get_dm();
	UINT t = dm->get_simplex_tyid_ex(
				dm->get_dtype(WORD_LENGTH_OF_HOST_MACHINE, true));
	IR * imm0 = build_imm_int(0, t);
	IR * x = build_store_pr(PR_no(pr), IR_dt(pr), imm0);
	copy_dbx(x, imm0, this);
	add_next(&ret_list, x);

	LABEL_INFO * label2 = gen_ilab();
	add_next(&ret_list, build_goto(label2));
	add_next(&ret_list, build_label(label1));
	UINT t2 = dm->get_simplex_tyid_ex(
				dm->get_dtype(WORD_LENGTH_OF_HOST_MACHINE, true));
	IR * imm1 = build_imm_int(1, t2);
	x = build_store_pr(PR_no(pr), IR_dt(pr), imm1);
	copy_dbx(x, imm1, this);
	add_next(&ret_list, x);
	add_next(&ret_list, build_label(label2));
	ctx->append_irs(ret_list);
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	return pr;
}


/* L1 is tgt_label.
e.g: b&&c,L1
would be translated to:
	----------
	falsebr(b != 0), L2
	truebrbr(c != 0), L1
	L2:
	----------
NOTE: ir's parent can NOT be FALSEBR. */
IR * REGION::simplify_logical_and_truebr(IN IR * ir, IN LABEL_INFO * tgt_label)
{
	IS_TRUE0(IR_type(ir) == IR_LAND && tgt_label != NULL);
	IR * ret_list = NULL;

	//Process opnd0.
	IR * opnd0 = BIN_opnd0(ir);
	BIN_opnd0(ir) = NULL;
	if (!opnd0->is_judge()) {
		opnd0 = build_cmp(IR_NE, opnd0, build_imm_int(0, IR_dt(opnd0)));
	}
	//Generate falsebranch label.
	LABEL_INFO * lab = gen_ilab();

	IR * br = build_branch(false, opnd0, lab);
	copy_dbx(br, ir, this);
	add_next(&ret_list, br);

	//Process opnd1.
	IR * opnd1 = BIN_opnd1(ir);
	BIN_opnd1(ir) = NULL;
	if (!opnd1->is_judge()) {
		opnd1 = build_cmp(IR_NE, opnd1, build_imm_int(0, IR_dt(opnd1)));
	}
	br = build_branch(true, opnd1, tgt_label);
	copy_dbx(br, ir, this);
	add_next(&ret_list, br);

	//Add false-branch label.
	add_next(&ret_list, build_label(lab));
	free_irs(ir);
	return ret_list;
}


/* L1 is tgt_label.
e.g: falsebr(b&&c),L1
would be translated to:
	----------
	falsebr(b != 0), L2
	truebrbr(c != 0), L1
	L2:
	----------
NOTE: ir's parent must be FALSEBR. */
IR * REGION::simplify_logical_and_falsebr(IN IR * ir, IN LABEL_INFO * tgt_label)
{
	IS_TRUE0(IR_type(ir) == IR_LAND && tgt_label != NULL);
	IR * ret_list = NULL;

	//Process opnd0.
	IR * opnd0 = BIN_opnd0(ir);
	BIN_opnd0(ir) = NULL;
	if (!opnd0->is_judge()) {
		opnd0 = build_cmp(IR_NE, opnd0, build_imm_int(0, IR_dt(opnd0)));
	}
	IR * false_br = build_branch(false, opnd0, tgt_label);
	copy_dbx(false_br, ir, this);
	add_next(&ret_list, false_br);

	//Process opnd1
	IR * opnd1 = BIN_opnd1(ir);
	BIN_opnd1(ir) = NULL;
	if (!opnd1->is_judge()) {
		opnd1 = build_cmp(IR_NE, opnd1,	build_imm_int(0, IR_dt(opnd1)));
	}
	false_br = build_branch(false, opnd1, tgt_label);
	copy_dbx(false_br, ir, this);
	add_next(&ret_list, false_br);
	free_irs(ir);
	return ret_list;
}


/* e.g: b||c,L1
would be translated to:
	----------
	truebr(b != 0), L1
	truebr(c != 0), L1
	----------
or
	----------
	truebr(b != 0), L1
	pr = (c != 0)
	----------
NOTE: ir's parent can NOT be FALSEBR. */
IR * REGION::simplify_logical_or_truebr(IN IR * ir, IN LABEL_INFO * tgt_label)
{
	IS_TRUE0(IR_type(ir) == IR_LOR && tgt_label != NULL);
	IR * ret_list = NULL;

	//Process opnd0.
	IR * opnd0 = BIN_opnd0(ir);
	BIN_opnd0(ir) = NULL;
	if (!opnd0->is_judge()) {
		opnd0 = build_cmp(IR_NE, opnd0, build_imm_int(0, IR_dt(opnd0)));
	}
	IR * true_br = build_branch(true, opnd0, tgt_label);
	copy_dbx(true_br, ir, this);
	add_next(&ret_list, true_br);

	//Process opnd1.
	IR * opnd1 = BIN_opnd1(ir);
	BIN_opnd1(ir) = NULL;
	if (!opnd1->is_judge()) {
		opnd1 = build_cmp(IR_NE, opnd1, build_imm_int(0, IR_dt(opnd1)));
	}
	IR * op = NULL;
	//if (SIMP_logical_not(ctx)) {
		op = build_branch(true, opnd1, tgt_label);
	//} else {
	//	In the case ir's parent is if(a||b),L1, generate STORE is invalid.
	//	IS_TRUE0(res_pr != NULL);
	//	op = build_store(res_pr, opnd1);
	//}
	copy_dbx(op, ir, this);
	add_next(&ret_list, op);
	free_irs(ir);
	return ret_list;
}


/* e.g: falsebr(b||c),L1
would be translated to:
	----------
	truebr(b != 0), L2
	falsebr(c != 0), L1
	L2:
	----------
NOTE: ir's parent must be be FALSEBR. */
IR * REGION::simplify_logical_or_falsebr(IN IR * ir, IN LABEL_INFO * tgt_label)
{
	IS_TRUE0(IR_type(ir) == IR_LOR && tgt_label != NULL);
	IR * ret_list = NULL;

	//ir is FALSEBR
	IR * opnd0 = BIN_opnd0(ir);
	BIN_opnd0(ir) = NULL;
	if (!opnd0->is_judge()) {
		opnd0 = build_cmp(IR_NE, opnd0, build_imm_int(0, IR_dt(opnd0)));
	}
	LABEL_INFO * true_lab = gen_ilab();
	IR * true_br = build_branch(true, opnd0, true_lab);
	copy_dbx(true_br, ir, this);
	add_next(&ret_list, true_br);

	IR * opnd1 = BIN_opnd1(ir);
	BIN_opnd1(ir) = NULL;
	if (!opnd1->is_judge()) {
		opnd1 = build_cmp(IR_NE, opnd1, build_imm_int(0, IR_dt(opnd1)));
	}
	IR * false_br = build_branch(false, opnd1, tgt_label);
	copy_dbx(false_br, ir, this);
	add_next(&ret_list, false_br);
	add_next(&ret_list, build_label(true_lab));
	free_irs(ir);
	return ret_list;
}


/* Return expression's value.
e.g: a = b||c, where rhs would be translated to:
	----------
	truebr(b != 0), L1
	truebr(c != 0), L1
	pr = 0
	goto L2
	L1:
	pr = 1
	L2:
	----------
	a = pr
or
	----------
	truebr(b != 0), L1
	pr = (c != 0)
	goto L2
	L1:
	pr = 1
	L2:
	----------
	a = pr

In the case, return 'pr'. */
IR * REGION::simplify_logical_or(IN IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(IR_type(ir) == IR_LOR);
	LABEL_INFO * label1 = gen_ilab();
	IR * pr = build_pr(IR_dt(ir));
	alloc_ref_for_pr(pr);
	IR * ret_list = simplify_logical_or_truebr(ir, label1);
	DT_MGR * dm = get_dm();
	UINT tyid = dm->get_simplex_tyid_ex(
					dm->get_dtype(WORD_LENGTH_OF_HOST_MACHINE, true));
	IR * imm0 = build_imm_int(0, tyid);
	IR * x = build_store_pr(PR_no(pr), IR_dt(pr), imm0);
	copy_dbx(x, imm0, this);
	add_next(&ret_list, x);

	LABEL_INFO * label2 = gen_ilab();
	add_next(&ret_list, build_goto(label2));
	add_next(&ret_list, build_label(label1));

	tyid = dm->get_simplex_tyid_ex(
				dm->get_dtype(WORD_LENGTH_OF_HOST_MACHINE, true));
	IR * imm1 = build_imm_int(1, tyid);
	x = build_store_pr(PR_no(pr), IR_dt(pr), imm1);
	copy_dbx(x, imm1, this);
	add_next(&ret_list, x);
	add_next(&ret_list, build_label(label2));
	ctx->append_irs(ret_list);
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	return pr;
}


//Simplify logical OR, logical AND operations into comparision operations.
//Return generate IR stmts.
IR * REGION::simplify_logical_det(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	IS_TRUE0(IR_type(ir) == IR_TRUEBR || IR_type(ir) == IR_FALSEBR);
	IS_TRUE0(BR_det(ir)->is_logical());
	IR * ret_list = NULL;
	if (IR_type(BR_det(ir)) == IR_LOR) {
		if (IR_type(ir) == IR_TRUEBR) {
			ret_list = simplify_logical_or_truebr(BR_det(ir), BR_lab(ir));
			BR_det(ir) = NULL;
			free_irs(ir);
			SIMP_changed(ctx) = true;
			SIMP_need_recon_bblist(ctx) = true;
			return ret_list;
		}

		//ir is FALSEBR
		ret_list = simplify_logical_or_falsebr(BR_det(ir), BR_lab(ir));
		BR_det(ir) = NULL;
		free_irs(ir);
		SIMP_changed(ctx) = true;
		SIMP_need_recon_bblist(ctx) = true;
		return ret_list;
	} else if (IR_type(BR_det(ir)) == IR_LAND) {
		if (IR_type(ir) == IR_TRUEBR) {
			ret_list = simplify_logical_and_truebr(BR_det(ir), BR_lab(ir));
			BR_det(ir) = NULL;
			free_irs(ir);
			SIMP_changed(ctx) = true;
			SIMP_need_recon_bblist(ctx) = true;
			return ret_list;
		}

		//ir is FALSEBR
		ret_list = simplify_logical_and_falsebr(BR_det(ir), BR_lab(ir));
		BR_det(ir) = NULL;
		free_irs(ir);
		SIMP_changed(ctx) = true;
		SIMP_need_recon_bblist(ctx) = true;
		return ret_list;
	} else if (IR_type(BR_det(ir)) == IR_LNOT) {
		if (IR_type(ir) == IR_TRUEBR) {
			IR_type(ir) = IR_FALSEBR;
		} else {
			IR_type(ir) = IR_TRUEBR;
		}
		BR_det(ir) = UNA_opnd0(BR_det(ir));
		if (!BR_det(ir)->is_judge()) {
			IR * old = BR_det(ir);
			BR_det(ir) = build_judge(old);
			copy_dbx(BR_det(ir), old, this);
		}
		IR_parent(BR_det(ir)) = ir;
		return simplify_stmt(ir, ctx);
	}
	IS_TRUE(0, ("TODO"));
	return ret_list;
}


//Simplify kids of ir.
IR * REGION::simplify_kids(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) { return NULL; }
	IS_TRUE0(ir->is_exp());
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid == NULL) { continue; }
		IR * new_kid = simplify_exp(kid, ctx);

		if (SIMP_to_lowest_heigh(ctx) &&
			!IR_parent(ir)->is_stmt() &&
			!new_kid->is_leaf() &&
			IR_type(ir) != IR_SELECT) { //select's det must be judge.

			IR * pr = build_pr(IR_dt(new_kid));
			alloc_ref_for_pr(pr);
			IR * st = build_store_pr(PR_no(pr), IR_dt(pr), new_kid);
			copy_dbx(st, new_kid, this);
			ctx->append_irs(st);
			SIMP_changed(ctx) = true;
			ir->set_kid(i, pr);
		} else {
			ir->set_kid(i, new_kid);
		}
	}
	return ir;
}


//Generate comparision and branch.
IR * REGION::simplify_select(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) { return NULL; }
	IS_TRUE(IR_type(ir) == IR_SELECT, ("expect select node"));
	if (!SIMP_select(ctx)) {
		return simplify_kids(ir, ctx);
	}

	/* Transform select to:
		falsebr det, (label(ELSE_START))
		res = true_exp
		goto END
		ELSE_START:
		res = false_exp
		END: */
	SIMP_CTX tcont(*ctx);
	SIMP_ret_array_val(&tcont) = 1;

	//det exp
	//When we first lowering CFS, det-expression should not be TRUEBR/FASLEBR.
	IS_TRUE0(SELECT_det(ir)->is_judge());
	IR * newdet = simplify_det(SELECT_det(ir), &tcont);
	ctx->append_irs(tcont);

	//Build branch.
	IR * last = removetail(&newdet);
	IS_TRUE(!last->is_stmt() && last->is_judge(), ("invalide det exp"));
	IR * falsebr = build_branch(false, last, gen_ilab());
	copy_dbx(falsebr, SELECT_det(ir), this);
	newdet = falsebr;

	IS_TRUE0(SELECT_trueexp(ir)->is_type_equal(SELECT_falseexp(ir)));
	IR * res = build_pr(IR_dt(SELECT_trueexp(ir)));
	alloc_ref_for_pr(res);
	//Simp true exp.
	SIMP_CTX tcont2(*ctx);
	SIMP_ret_array_val(&tcont2) = true;
	IR * true_exp = simplify_exp(SELECT_trueexp(ir), &tcont2);
	ctx->append_irs(tcont2);
	IR * mv = build_store_pr(PR_no(res), IR_dt(res), true_exp);
	copy_dbx(mv, true_exp, this);
	add_next(&newdet, mv);

	//---
	//Simp false expression
	IS_TRUE0(SELECT_falseexp(ir) != NULL);
	//append GOTO following end of true body
	IR * gotoend = build_goto(gen_ilab());
	copy_dbx(gotoend, SELECT_det(ir), this);
	add_next(&newdet, gotoend);

	//true body end label
	add_next(&newdet, build_label(BR_lab(falsebr)));

	//simplify false expression
	SIMP_CTX tcont3(*ctx);
	SIMP_ret_array_val(&tcont3) = true;
	IR * else_exp = simplify_exp(SELECT_falseexp(ir), &tcont3);
	ctx->append_irs(tcont3);
	IR * mv2 = build_store_pr(PR_no(res), IR_dt(res), else_exp);
	copy_dbx(mv2, else_exp, this);
	add_next(&newdet, mv2);
	//---

	//generate END label.
	add_next(&newdet, build_label(GOTO_lab(gotoend)));
	//

	for (IR * p = newdet; p != NULL; p = IR_next(p)) {
		IR_parent(p) = NULL;
	}
	ctx->append_irs(newdet);
	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;
	if (SIMP_is_record_cfs(ctx)) {
		//Record high level control flow structure.
		CFS_INFO * ci = NULL;
		IS_TRUE0(SIMP_cfs_mgr(ctx) != NULL);
		ci = SIMP_cfs_mgr(ctx)->new_cfs_info(IR_IF);
		SIMP_cfs_mgr(ctx)->set_map_ir2cfsinfo(falsebr, ci);
		CFS_INFO_ir(ci) = falsebr;
		SIMP_cfs_mgr(ctx)->record_ir_stmt(true_exp, *CFS_INFO_true_body(ci));
		SIMP_cfs_mgr(ctx)->record_ir_stmt(else_exp, *CFS_INFO_false_body(ci));
	}
	return res;
}


IR * REGION::simplify_igoto(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) { return NULL; }
	IR * ret = NULL;
	IS_TRUE(IR_type(ir) == IR_IGOTO, ("expect igoto"));

	IGOTO_vexp(ir) = simplify_exp(IGOTO_vexp(ir), ctx);
	return ir;
}


IR * REGION::simplify_switch(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) { return NULL; }
	IR * ret = NULL;
	IS_TRUE(IR_type(ir) == IR_SWITCH, ("expect switch node"));

	IR * vexp_stmt = NULL;
	IR * last = NULL;
	IR * swt_val = SWITCH_vexp(ir);
	if (IR_type(swt_val) != IR_PR) {
		IR * pr = build_pr(IR_dt(swt_val));
		alloc_ref_for_pr(pr);
		vexp_stmt = build_store_pr(PR_no(pr), IR_dt(pr), swt_val);
		copy_dbx(vexp_stmt, swt_val, this);
		swt_val = pr;
	}
	SWITCH_vexp(ir) = NULL;

	IR * case_lst = get_last(SWITCH_case_list(ir));
	IR * prev_ir_tree = NULL;
	if (case_lst == NULL) {
		return NULL;
	}

	//Simplify CASE table into IF as default to enable
	//more advantage high level optimizations.
	if (SWITCH_deflab(ir) != NULL) {
		prev_ir_tree = build_goto(SWITCH_deflab(ir));
		copy_dbx(prev_ir_tree, ir, this);
		SWITCH_deflab(ir) = NULL;
	}

	while (case_lst != NULL) {
		IR * ifstmt = build_if(build_cmp(IR_EQ, dup_irs(swt_val),
										 CASE_vexp(case_lst)),
								build_goto(CASE_lab(case_lst)),
								prev_ir_tree);
		copy_dbx(ifstmt, case_lst, this);
		CASE_vexp(case_lst) = NULL;
		prev_ir_tree = ifstmt;
		case_lst = IR_prev(case_lst);
	}

	add_next(&prev_ir_tree, SWITCH_body(ir));
	SWITCH_body(ir) = NULL;

	if (SIMP_if(ctx)) {
		//Simpilify IF to TRUEBR/FALSEBR.
		//Generate the end label of SWITCH to serve as the target
		//branch label of TRUEBR/FALSEBR.
		LABEL_INFO * switch_endlab = gen_ilab();
		add_next(&prev_ir_tree, build_label(switch_endlab));

		SIMP_CTX tctx(*ctx);
		SIMP_break_label(&tctx) = switch_endlab;
		prev_ir_tree = simplify_stmt_list(prev_ir_tree, &tctx);
	}

	for (IR * p = prev_ir_tree; p != NULL; p = IR_next(p)) {
		IR_parent(p) = NULL;
	}

	SIMP_changed(ctx) = true;
	SIMP_need_recon_bblist(ctx) = true;

	IR * ret_list = prev_ir_tree;
	if (vexp_stmt != NULL) {
		add_next(&vexp_stmt, prev_ir_tree);
		ret_list = vexp_stmt;
	}
	free_irs(ir);
	return ret_list;
}


/* In C language, the ARRAY operator is also avaiable for
dereference of pointer,

  e.g: char * p; p[x] = 10; the operator is actually invoke an ILD operator,
	namely, its behavor is *(p + x) = 10, and the will generate:
		t1 = [p]
		t2 = t1 + x
		[t2] = 10
	In the contrast, char q[]; q[x] = 10, will generate:
		t1 = &q
		t2 = t1 + x
		[t2] = 10

So, p[] must be converted already to *p via replace IR_ARRAY with IR_ILD
before go into this function. */
IR * REGION::simplify_array_addr_exp(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(SIMP_array(ctx) && IR_type(ir) == IR_ARRAY);
	if (ir == NULL) { return NULL; }
	IS_TRUE0(IR_type(ir) == IR_ARRAY && ARR_sub_list(ir));

	DT_MGR * dm = get_dm(); //new pointer type may be registered.
	IS_TRUE0(ir->get_dt_size(dm) > 0);

	//For n dimension array, enumb record the number
	//of elements at 0~n-1 dimension.
	UINT enumb = 0;

	UINT indextyid = get_target_machine_array_index_tyid();
	UINT dim = 0;
	IR * ofst_exp = NULL;
	TMWORD const* elemnumbuf = ARR_elem_num_buf(ir);
	IR * s = removehead(&ARR_sub_list(ir));
	for (; s != NULL; dim++) {
		SIMP_CTX tcont(*ctx);
		SIMP_ret_array_val(&tcont) = true;
		IR * news = simplify_exp(s, &tcont);
		ctx->append_irs(tcont);
		ctx->union_bottomup_flag(tcont);

		/* 'ir' is exact array.
		CASE1: elem-type v[n]
			can simply to: &v + n*sizeof(BASETYPE)
		CASE2: a = (*p)[n]
		can simply to: (*p) + n*sizeof(BASETYPE)

		CASE3: struct S {int a, b;} s[10];
			the elem_ty is struct S.
			s[1].b has ARR_ofst(ir)==4
			can simply to: &s + 1*sizeof(struct S) + offset(4) */
		IR * news2 = NULL;
		if (IR_is_const(news) && news->is_int(dm)) {
			//Subexp is const.
			if (enumb != 0) {
				news2 = build_imm_int(
					((HOST_INT)enumb) * CONST_int_val(news), indextyid);
			} else {
				news2 = dup_irs(news);
			}
		} else {
			if (enumb != 0) {
				news2 = build_binary_op(IR_MUL, indextyid, dup_irs(news),
									   build_imm_int(enumb, indextyid));
			} else {
				news2 = dup_irs(news);
			}
		}

		if (ofst_exp != NULL) {
			ofst_exp = build_binary_op_simp(IR_ADD, indextyid, ofst_exp, news2);
		} else {
			ofst_exp = news2;
		}

		IS_TRUE(elemnumbuf[dim] != 0,
			("Incomplete array dimension info, we need to "
			 "know how many elements in each dimension."));
		if (dim == 0) {
			enumb = elemnumbuf[dim];
		} else {
			enumb *= elemnumbuf[dim];
		}
		s = removehead(&ARR_sub_list(ir));
	}

	IS_TRUE0(ofst_exp);

	UINT elemsize = dm->get_dtd_bytesize(ARR_elem_ty(ir));
	if (elemsize != 1) {
		//e.g: short g[i], subexp is i*sizeof(short)
		ofst_exp = build_binary_op(IR_MUL, indextyid, ofst_exp,
								   build_imm_int(elemsize, indextyid));
	}

	if (ARR_ofst(ir) != 0) {
		/* CASE: struct S {int a, b;} s[10];
		the elem_ty is struct S.
		s[1].b has ARR_ofst(ir)==4
		can simply to: 1*sizeof(struct S) + offset(4) + lda(s). */
		IR * imm = build_imm_int((HOST_INT)(ARR_ofst(ir)), indextyid);
		ofst_exp = build_binary_op_simp(IR_ADD, indextyid, ofst_exp, imm);
	}

	IS_TRUE0(ARR_base(ir) && ARR_base(ir)->is_ptr(dm));
	SIMP_CTX tcont(*ctx);
	SIMP_ret_array_val(&tcont) = false;
	IR * newbase = simplify_exp(ARR_base(ir), &tcont);
	ctx->append_irs(tcont);
	ctx->union_bottomup_flag(tcont);

	IS_TRUE0(newbase && newbase->is_ptr(dm));
	ARR_base(ir) = NULL;

	/* 'array_addr' is address of an ARRAY, and it is pointer type.
	Given 'array_addr + n', the result-type of '+' must
	be pointer type as well.
	Note do not call build_binary_op(IR_ADD...) to generate ir.
	Because that when 'sub' is pointer, the extra IR_MUL
	operation will be generated. */
	IR * array_addr = build_binary_op_simp(IR_ADD, 0, newbase, ofst_exp);
	array_addr->set_pointer_type(ir->get_dt_size(dm), dm);
	if (SIMP_to_pr_mode(ctx) && IR_type(array_addr) != IR_PR) {
		SIMP_CTX ttcont(*ctx);
		SIMP_ret_array_val(&ttcont) = true;
		array_addr->set_parent_pointer(true);
		array_addr = simplify_exp(array_addr, &ttcont);
		ctx->append_irs(ttcont);
		ctx->union_bottomup_flag(ttcont);
	}

	if (SIMP_to_lowest_heigh(ctx)) {
			/*CASE: If IR_parent is NULL, the situation is
			caller attempt to enforce simplification to array expression
			whenever possible. */
			SIMP_CTX ttcont(*ctx);
			SIMP_ret_array_val(&ttcont) = true;
			array_addr = simplify_exp(array_addr, &ttcont);
			ctx->append_irs(ttcont);
			ctx->union_bottomup_flag(ttcont);

			//IR * t = build_pr(IR_dt(array_addr));
			//IR * mv = build_store_pr(PR_no(t), IR_dt(t), array_addr);
			//ctx->append_irs(mv);
			//array_addr = t;
	}

	free_irs(ir);
	SIMP_changed(ctx) = true;
	return array_addr;
}


IR * REGION::simplify_lda(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(IR_type(ir) == IR_LDA);
	IR * lda_base = LDA_base(ir);
	IS_TRUE0(lda_base != NULL);

	if (IR_type(lda_base) == IR_ARRAY) {
		SIMP_CTX tc(*ctx);
		SIMP_array(&tc) = true;
		return simplify_array_addr_exp(lda_base, &tc);
	}
	if (IR_type(lda_base) == IR_ID || lda_base->is_str(get_dm())) {
		if (SIMP_to_pr_mode(ctx)) {
			IR * pr = build_pr(IR_dt(ir));
			alloc_ref_for_pr(pr);
			IR * st = build_store_pr(PR_no(pr), IR_dt(pr), ir);
			copy_dbx(st, ir, this); //keep dbg info for new STMT.
			ctx->append_irs(st);
			ir = pr;
			SIMP_changed(ctx) = true;
		}
		return ir;
	}
	IS_TRUE(0, ("morbid array form"));
	return NULL;
}


//Return new generated expression's value.
//'ir': ir may be in parameter list if its prev or next is not empty.
IR * REGION::simplify_exp(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	IS_TRUE(ir->is_exp(), ("expect non-statement node"));

	//ir can not in list, or it may incur illegal result.
	IS_TRUE0(IR_next(ir) == NULL && IR_prev(ir) == NULL);
	switch (IR_type(ir)) {
	case IR_CONST:
		if (ir->is_str(get_dm())) {
			SIMP_changed(ctx) = true;
			return build_lda(ir);
		}
		return ir;
	case IR_ID:
		return ir;
	case IR_LD:
		if (SIMP_to_pr_mode(ctx)) {
			IR * pr = build_pr(IR_dt(ir));
			alloc_ref_for_pr(pr);
			IR * st = build_store_pr(PR_no(pr), IR_dt(pr), ir);
			copy_dbx(st, ir, this); //keep dbg info for new STMT.
			ctx->append_irs(st);
			ir = pr;
			SIMP_changed(ctx) = true;
		}
		return ir;
	case IR_LAND: //logical and &&
	case IR_LOR: //logical or ||
	case IR_LNOT: //logical not
	case IR_EQ: //==
	case IR_NE: //!=
	case IR_LT:
	case IR_GT:
	case IR_GE:
	case IR_LE:
		return simplify_judge_exp(ir, ctx);
	case IR_ILD:
	case IR_BAND: //inclusive and &
	case IR_BOR: //inclusive or  |
	case IR_XOR: //exclusive or ^
	case IR_BNOT: //bitwise not
	case IR_NEG: //negative
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LABEL:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_CVT: //data type convertion
		{
			for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
				IR * kid = ir->get_kid(i);
				if (kid != NULL) {
					IR * x = simplify_exp(kid, ctx);
					ir->set_kid(i, x);
				}
			}

			bool doit = false; //indicate whether does the lowering.
			IR * p = IR_parent(ir);
			if (SIMP_to_lowest_heigh(ctx) && p != NULL) {
				if (!p->is_stmt() || //tree height is more than 2.

					p->is_call() || //parent is call, ir is the parameter.
					//We always reduce the height for parameter even if its height is 2.

					//ir is lhs expression, lower it.
					(IR_type(p) == IR_IST && ir == IST_base(p))) {
					doit = true;
				}
			}

			if (doit) {
				for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
					IR * k = ir->get_kid(i);
					if (k != NULL) {
						/* Lower ARRAY arith :
							a[i] + b[i]
							=>
							t1 = a[i]
							t2 = b[i]
							t1 + t2 */
						if (SIMP_array_to_pr_mode(ctx) &&
							IR_type(k) == IR_ARRAY) {
							IR * pr = build_pr(IR_dt(k));
							alloc_ref_for_pr(pr);
							IR * st = build_store_pr(PR_no(pr), IR_dt(pr), k);
							copy_dbx(st, k, this); //keep dbg info for new STMT.
							ctx->append_irs(st);
							ir->set_kid(i, pr);
							SIMP_changed(ctx) = true;
						}
					}
				}
				IR * pr = build_pr(IR_dt(ir));
				alloc_ref_for_pr(pr);
				IR * st = build_store_pr(PR_no(pr), IR_dt(pr), ir);
				copy_dbx(st, ir, this); //keep dbg info for new STMT.
				ctx->append_irs(st);
				ir = pr;
				SIMP_changed(ctx) = true;
			}
			return ir;
		}
		break;
	case IR_PR:
		return ir;
	case IR_SELECT:
		return simplify_select(ir, ctx);
	case IR_LDA: //&sym, get address of 'sym'
		return simplify_lda(ir, ctx);
	case IR_ARRAY:
		return simplify_array(ir, ctx);
	default:
		IS_TRUE(0, ("cannot simplify '%s'", IRNAME(ir)));
	} //end switch
	return NULL;
}


//Return new generated expression's value.
IR * REGION::simplify_judge_exp(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) return NULL;
	IS_TRUE0(ir->is_judge());
	IS_TRUE0(IR_prev(ir) == NULL && IR_next(ir) == NULL);
	switch (IR_type(ir)) {
	case IR_LAND: //logical and &&
	case IR_LOR: //logical or ||
		{
			if (SIMP_logical_or_and(ctx)) {
				SIMP_CTX tcont(*ctx);
				SIMP_ret_array_val(&tcont) = true;
				IR * newir = NULL;
				if (IR_type(ir) == IR_LOR) {
					newir = simplify_logical_or(ir, &tcont);
				} else {
					newir = simplify_logical_and(ir, NULL, &tcont);
				}
				ctx->union_bottomup_flag(tcont);

				IS_TRUE0(newir->is_exp());
				IR * lst = SIMP_ir_stmt_list(&tcont);
				IS_TRUE0(newir != ir);
				SIMP_CTX t_tcont(tcont);
				lst = simplify_stmt_list(lst, &t_tcont);
				ctx->append_irs(lst);
				ir = newir;
			} else {
				for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
					IR * kid = ir->get_kid(i);
					if (kid != NULL) {
						ir->set_kid(i, simplify_exp(kid, ctx));
					}
				}
			}

			if (SIMP_to_lowest_heigh(ctx) &&
				IR_parent(ir) != NULL && !IR_parent(ir)->is_stmt()) {
				IR * pr = build_pr(IR_dt(ir));
				alloc_ref_for_pr(pr);
				IR * st = build_store_pr(PR_no(pr), IR_dt(pr), ir);
				copy_dbx(st, ir, this); //keep dbg info for new STMT.
				ctx->append_irs(st);
				ir = pr;
				SIMP_changed(ctx) = true;
			}
			return ir;
		}
		IS_TRUE0(0);
	case IR_LNOT: //logical not
		{
			if (SIMP_logical_not(ctx)) {
				SIMP_CTX tcont(*ctx);
				SIMP_ret_array_val(&tcont) = true;
				IR * newir = simplify_logical_not(ir, &tcont);
				IS_TRUE0(newir->is_exp());
				IR * lst = SIMP_ir_stmt_list(&tcont);
				IS_TRUE(IR_type(newir) == IR_PR,
						("For now, newir will "
						"fairly be IR_PR. But it is not "
						"certain in the future."));
				SIMP_CTX t_tcont(tcont);
				lst = simplify_stmt_list(lst, &t_tcont);
				ctx->append_irs(lst);
				ir = newir;
				ctx->union_bottomup_flag(tcont);
			} else {
				for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
					IR * kid = ir->get_kid(i);
					if (kid != NULL) {
						ir->set_kid(i, simplify_exp(kid, ctx));
					}
				}
			}
			if (SIMP_to_lowest_heigh(ctx) &&
				IR_parent(ir) != NULL && !IR_parent(ir)->is_stmt()) {
				IR * pr = build_pr(IR_dt(ir));
				alloc_ref_for_pr(pr);
				IR * st = build_store_pr(PR_no(pr), IR_dt(pr), ir);
				copy_dbx(st, ir, this); //keep dbg info for new STMT.
				ctx->append_irs(st);
				ir = pr;
				SIMP_changed(ctx) = true;
			}
			return ir;
		}
		IS_TRUE0(0);
	case IR_EQ:
	case IR_NE:
	case IR_LT:
	case IR_GT:
	case IR_GE:
	case IR_LE:
		{
			IS_TRUE0(IR_parent(ir));
			for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
				IR * kid = ir->get_kid(i);
				if (kid != NULL) {
					ir->set_kid(i, simplify_exp(kid, ctx));
				}
			}
			if (SIMP_to_lowest_heigh(ctx) && !IR_parent(ir)->is_stmt()) {
				bool is_det = false;
				if (IR_type(IR_parent(ir)) == IR_SELECT) {
					/* Check if det is lowest binary operation to COND_EXE,
					although its parent is not a stmt.
					e.g: Regard NE expression as lowest heigh.
						COND_EXE
							NE (r:BOOL:1)
								LD (r:I32:4, ofst:0 'i4' r:I32:4 decl:int i4)
								INTCONST (0 (0x0), r:I32:4)
							TRUE_STMT
								INTCONST (0 (0x0), r:I32:4)
							FALSE_BODY
								INTCONST (1 (0x1), r:I32:4)
						END_COND_EXE */
					if (BIN_opnd0(ir)->is_leaf() && BIN_opnd1(ir)->is_leaf()) {
						return ir;
					}
					is_det = true;
				}
				IR * pr = build_pr(IR_dt(ir));
				alloc_ref_for_pr(pr);
				IR * st = build_store_pr(PR_no(pr), IR_dt(pr), ir);
				copy_dbx(st, ir, this); //keep dbg info for new STMT.
				ctx->append_irs(st);

				if (is_det) {
					//Amend det to be valid judgement.
					ir = build_judge(pr);
					copy_dbx(ir, pr, this);
				} else {
					ir = pr;
				}

				SIMP_changed(ctx) = true;
			}
			return ir;
		}
		break;
	default: IS_TRUE0(0);
	} //end switch
	return NULL;
}


//Simplify array operator.
IR * REGION::simplify_array(IR * ir, SIMP_CTX * ctx)
{
	if (!SIMP_array(ctx)) { return ir; }

	//ir will be freed in simplify_array(), record its parent here.
	IR * parent = IR_parent(ir);
	UINT res_rtype = IR_dt(ir);
	IR * array_addr = simplify_array_addr_exp(ir, ctx);
	SIMP_changed(ctx) = true;
	if (!SIMP_ret_array_val(ctx)) {
		return array_addr;
	}

	if (IR_type(array_addr) == IR_ID) {
		IR * ld = build_load(ID_info(array_addr), IR_dt(array_addr));
		free_irs(array_addr);
		return ld;
	}

	if (SIMP_to_pr_mode(ctx)) {
		if (IR_type(array_addr) != IR_PR) {
			IR * pr = build_pr(IR_dt(array_addr));
			alloc_ref_for_pr(pr);
			IR * st = build_store_pr(PR_no(pr), IR_dt(pr), array_addr);

			//keep dbg info for new STMT.
			copy_dbx(st, array_addr, this);
			ctx->append_irs(st);
			array_addr = pr;
		}

		array_addr = build_iload(array_addr, res_rtype);
		IR * pr = build_pr(IR_dt(array_addr));
		alloc_ref_for_pr(pr);
		IR * st = build_store_pr(PR_no(pr), IR_dt(pr), array_addr);

		//keep dbg info for new STMT.
		copy_dbx(st, array_addr, this);
		ctx->append_irs(st);
		return pr;
	}

	if (SIMP_to_lowest_heigh(ctx)) {
		if (!array_addr->is_leaf()) {
			IR * pr = build_pr(IR_dt(array_addr));
			alloc_ref_for_pr(pr);
			IR * st = build_store_pr(PR_no(pr), IR_dt(pr), array_addr);

			//keep dbg info for new STMT.
			copy_dbx(st, array_addr, this);
			ctx->append_irs(st);
			array_addr = pr;
		}

		array_addr = build_iload(array_addr, res_rtype);
		IR * pr = build_pr(IR_dt(array_addr));
		alloc_ref_for_pr(pr);
		IR * st = build_store_pr(PR_no(pr), IR_dt(pr), array_addr);

		//keep dbg info for new STMT.
		copy_dbx(st, array_addr, this);
		ctx->append_irs(st);
		return pr;
	}

	return build_iload(array_addr, res_rtype);
}


IR * REGION::simplify_call(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(IR_type(ir) == IR_CALL || IR_type(ir) == IR_ICALL);
	SIMP_CTX tcont(*ctx);
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
	SIMP_ret_array_val(&tcont) = true;

	bool origin_to_lowest_height = SIMP_to_lowest_heigh(&tcont);

	IR * newp = NULL;
	IR * last = NULL;
	bool lchange = false;
	while (CALL_param_list(ir) != NULL) {
		IR * p = removehead(&CALL_param_list(ir));

		if (!p->is_leaf()) {
			/* We always simplify parameters to lowest height to
			facilitate the query of point-to set.
			e.g: IR_DU_MGR is going to compute may point-to while
			ADD is pointer type. But only MD has point-to set.
			The query of point-to to ADD(id:6) is failed.
			So we need to store the add's value to a PR,
			and it will reserved the point-to set information.

				call 'getNeighbour'
			       add (ptr<24>) param4 id:6
			           lda (ptr<96>) id:31
			               id (mc<96>, 'pix_a')
			           mul (u32) id:13
			               ld (i32 'i')
			               intconst 24|0x18 (u32) id:14
			*/
			SIMP_to_lowest_heigh(&tcont) = true;
		}

		p = simplify_exp(p, &tcont);

		SIMP_to_lowest_heigh(&tcont) = origin_to_lowest_height;

		add_next(&newp, &last, p);

		lchange |= SIMP_changed(&tcont);

		if (SIMP_changed(&tcont)) {
			IR_parent(p) = ir;
		}

		SIMP_changed(&tcont) = false;
	}

	CALL_param_list(ir) = newp;
	if (newp != NULL) {
		ir->set_parent(newp);
	}

	IR * ret_list = SIMP_ir_stmt_list(&tcont);
	add_next(&ret_list, ir);
	SIMP_changed(ctx) |= lchange;
	ctx->union_bottomup_flag(tcont);
	return ret_list;
}


IR * REGION::simplify_store_pr(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(ir->is_stpr());

	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);

	SIMP_CTX tcont(*ctx);
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
	SIMP_ret_array_val(&tcont) = true;
	STPR_rhs(ir) = simplify_exp(STPR_rhs(ir), &tcont);
	IR_parent(STPR_rhs(ir)) = ir;
	ctx->union_bottomup_flag(tcont);

	IR * ret_list = NULL;
	IR * last = NULL;
	if (SIMP_ir_stmt_list(&tcont) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(&tcont));
	}
	add_next(&ret_list, &last, ir);
	return ret_list;
}


IR * REGION::simplify_setepr(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(ir->is_setepr());

	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);

	SIMP_CTX tcont(*ctx);
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
	SIMP_ret_array_val(&tcont) = true;
	SETEPR_rhs(ir) = simplify_exp(SETEPR_rhs(ir), &tcont);
	IR_parent(SETEPR_rhs(ir)) = ir;
	ctx->union_bottomup_flag(tcont);

	IR * ret_list = NULL;
	IR * last = NULL;
	if (SIMP_ir_stmt_list(&tcont) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(&tcont));
	}
	add_next(&ret_list, &last, ir);

	//Process offset.
	tcont.copy(*ctx);
	SIMP_ret_array_val(&tcont) = true;
	SETEPR_ofst(ir) = simplify_exp(SETEPR_ofst(ir), &tcont);
	IR_parent(SETEPR_ofst(ir)) = ir;
	ctx->union_bottomup_flag(tcont);
	last = NULL;
	if (SIMP_ir_stmt_list(&tcont) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(&tcont));
	}
	add_next(&ret_list, &last, ir);

	return ret_list;
}


IR * REGION::simplify_getepr(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(ir->is_getepr());

	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);

	//Process base.
	SIMP_CTX tcont(*ctx);
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
	SIMP_ret_array_val(&tcont) = true;
	GETEPR_base(ir) = simplify_exp(GETEPR_base(ir), &tcont);
	IR_parent(GETEPR_base(ir)) = ir;
	ctx->union_bottomup_flag(tcont);

	IR * ret_list = NULL;

	IR * last = NULL;
	if (SIMP_ir_stmt_list(&tcont) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(&tcont));
	}
	add_next(&ret_list, &last, ir);

	//Process offset.
	tcont.copy(*ctx);
	SIMP_ret_array_val(&tcont) = true;
	GETEPR_ofst(ir) = simplify_exp(GETEPR_ofst(ir), &tcont);
	IR_parent(GETEPR_ofst(ir)) = ir;
	ctx->union_bottomup_flag(tcont);
	last = NULL;
	if (SIMP_ir_stmt_list(&tcont) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(&tcont));
	}
	add_next(&ret_list, &last, ir);

	return ret_list;
}


IR * REGION::simplify_istore(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(IR_type(ir) == IR_IST);
	IR * ret_list = NULL;
	IR * last = NULL;
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);

	IST_base(ir) = simplify_exp(IST_base(ir), ctx);
	IR_parent(IST_base(ir)) = ir;
	if (SIMP_ir_stmt_list(ctx) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(ctx));
		SIMP_ir_stmt_list(ctx) = NULL;
	}

	if (SIMP_to_lowest_heigh(ctx) && IR_type(IST_base(ir)) == IR_ILD) {
		/* Simplify
			IST(ILD(PR1), ...)
		=>
			PR2 = ILD(PR1)
			IST(PR2, ...) */
		IR * memd = IST_base(ir);
		IR * pr = build_pr(IR_dt(memd));
		alloc_ref_for_pr(pr);
		IR * st = build_store_pr(PR_no(pr), IR_dt(pr), memd);
		copy_dbx(st, memd, this);
		add_next(&ret_list, &last, st);
		IST_base(ir) = pr;
		IR_parent(pr) = ir;
		SIMP_changed(ctx) = true;
	}

	SIMP_CTX tcont(*ctx);
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
	SIMP_ret_array_val(&tcont) = true;
	IST_rhs(ir) = simplify_exp(IST_rhs(ir), &tcont);
	IR_parent(IST_rhs(ir)) = ir;

	ctx->union_bottomup_flag(tcont);
	if (SIMP_ir_stmt_list(&tcont) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(&tcont));
	}
	add_next(&ret_list, &last, ir);
	return ret_list;
}


IR * REGION::simplify_store(IR * ir, SIMP_CTX * ctx)
{
	IS_TRUE0(ir->is_stid());

	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);

	SIMP_CTX tcont(*ctx);
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
	SIMP_ret_array_val(&tcont) = true;
	ST_rhs(ir) = simplify_exp(ST_rhs(ir), &tcont);
	IR_parent(ST_rhs(ir)) = ir;

	ctx->union_bottomup_flag(tcont);
	IR * ret_list = NULL;
	IR * last = NULL;
	if (SIMP_ir_stmt_list(&tcont) != NULL) {
		add_next(&ret_list, &last, SIMP_ir_stmt_list(&tcont));
	}
	add_next(&ret_list, &last, ir);
	return ret_list;
}


//Simply IR and its kids.
//Remember copy DBX info for new STMT to conveninent to dump use.
//Return new ir stmt-list.
IR * REGION::simplify_stmt(IR * ir, SIMP_CTX * ctx)
{
	if (ir == NULL) { return NULL; }
	IS_TRUE(IR_prev(ir) == NULL && IR_next(ir) == NULL,
			("ir should be remove out of list"));
	IR * ret_list = NULL;
	IS_TRUE(ir->is_stmt(), ("expect statement node"));
	switch (IR_type(ir)) {
	case IR_CALL:
	case IR_ICALL: //indirective call
		ret_list = simplify_call(ir, ctx);
		break;
	case IR_ST:
		ret_list = simplify_store(ir, ctx);
		break;
	case IR_STPR:
		ret_list = simplify_store_pr(ir, ctx);
		break;
	case IR_SETEPR:
		ret_list = simplify_setepr(ir, ctx);
		break;
	case IR_GETEPR:
		ret_list = simplify_getepr(ir, ctx);
		break;
	case IR_IST:
		ret_list = simplify_istore(ir, ctx);
		break;
	case IR_IGOTO:
		IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
		ret_list = simplify_igoto(ir, ctx);
		break;
	case IR_GOTO:
	case IR_LABEL:
	case IR_CASE:
	case IR_RETURN:
		{
			SIMP_CTX tcont(*ctx);
			IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
			SIMP_ret_array_val(&tcont) = true;
			for (INT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
				IR * kid = ir->get_kid(i);
				if (kid != NULL) {
					ir->set_kid(i, simplify_exp(kid, &tcont));
					ctx->union_bottomup_flag(tcont);
					if (SIMP_ir_stmt_list(&tcont) != NULL) {
						add_next(&ret_list, SIMP_ir_stmt_list(&tcont));
						SIMP_ir_stmt_list(&tcont) = NULL;
					}
				}
			}
			add_next(&ret_list, ir);
		}
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		{
			SIMP_CTX tcont(*ctx);
			IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
			SIMP_ret_array_val(&tcont) = true;
			if ((SIMP_logical_or_and(ctx) &&
				 (IR_type(BR_det(ir)) == IR_LOR ||
				  IR_type(BR_det(ir)) == IR_LAND)) ||
				(SIMP_logical_not(ctx) &&
				 IR_type(BR_det(ir)) == IR_LNOT)) {

				ret_list = simplify_logical_det(ir, &tcont);
				ret_list = simplify_stmt_list(ret_list, &tcont);
			} else {
				for (INT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
					IR * kid = ir->get_kid(i);
					if (kid != NULL) {
						ir->set_kid(i, simplify_exp(kid, &tcont));
						if (SIMP_ir_stmt_list(&tcont) != NULL) {
							add_next(&ret_list, SIMP_ir_stmt_list(&tcont));
							SIMP_ir_stmt_list(&tcont) = NULL;
						}
					}
				}
				add_next(&ret_list, ir);
			}
			ctx->union_bottomup_flag(tcont);
		}
		break;
	case IR_BREAK:
		{
			IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
			if (SIMP_break(ctx)) {
				IS_TRUE0(SIMP_break_label(ctx));
				IR * go = build_goto(SIMP_break_label(ctx));
				copy_dbx(go, ir, this);
				add_next(&ret_list, go);
			} else {
				add_next(&ret_list, ir);
			}
			break;
		}
	case IR_CONTINUE:
		{
			IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
			if (SIMP_continue(ctx)) {
				IR * go = build_goto(SIMP_continue_label(ctx));
				copy_dbx(go, ir, this);
				add_next(&ret_list, go);
			} else {
				add_next(&ret_list, ir);
			}
			break;
		}
	case IR_IF:
		IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
		ret_list = ir;
		if (SIMP_if(ctx)) {
			ret_list = simplify_if(ir, ctx);
		}
		break;
	case IR_DO_WHILE:
		IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
		ret_list = ir;
		if (SIMP_do_while(ctx)) {
			ret_list = simplify_do_while(ir, ctx);
		}
		break;
	case IR_WHILE_DO:
		IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
		ret_list = ir;
		if (SIMP_while_do(ctx)) {
			ret_list = simplify_while_do(ir, ctx);
		}
		break;
	case IR_DO_LOOP:
		IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
		ret_list = ir;
		if (SIMP_do_loop(ctx)) {
			ret_list = simplify_do_loop(ir, ctx);
		}
		break;
	case IR_SWITCH:
		IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
		ret_list = ir;
		if (SIMP_switch(ctx)) {
			ret_list = simplify_switch(ir, ctx);
		}
		break;
	case IR_REGION:
	case IR_PHI:
		return ir;
	default:
		IS_TRUE(0, ("'%s' is not a statement",IRNAME(ir)));
	}
	irs_set_parent_pointer(ret_list);
	return ret_list;
}


//Return new generated IR stmt.
IR * REGION::simplify_stmt_list(IR * ir_list, SIMP_CTX * ctx)
{
	if (ir_list == NULL) return NULL;
	IR * ret_list_header = NULL;
	IR * last = NULL;
	IR * next;
	IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
	bool doit = true;
	while (doit) {
		doit = false;
		while (ir_list != NULL) {
			next = IR_next(ir_list);
			IR_next(ir_list) = IR_prev(ir_list) = NULL;
			IR * new_ir_lst = simplify_stmt(ir_list, ctx);
			IS_TRUE0(SIMP_ir_stmt_list(ctx) == NULL);
			if (SIMP_to_lowest_heigh(ctx)) {
				IR * x = new_ir_lst;
				while (x != NULL) {
					if (!is_lowest_heigh(x)) {
						doit = true;
						break;
					}
					x = IR_next(x);
				}
			}
			if (ret_list_header == NULL) {
				//Record the header.
				ret_list_header = new_ir_lst;
			}
			insertafter(&last, new_ir_lst);
			last = get_last(new_ir_lst);
			ir_list = next;
		}

		if (doit) {
			ir_list = ret_list_header;
			ret_list_header = NULL;
		}
	}
	return ret_list_header;
}


//NOTE: simplification should not generate indirect memory operation.
void REGION::simplify_bb(IR_BB * bb, SIMP_CTX * ctx)
{
	LIST<IR*> new_ir_list;
	C<IR*> * ct;
	for (IR * stmt = IR_BB_ir_list(bb).get_head(&ct); stmt != NULL;
		 stmt = IR_BB_ir_list(bb).get_next(&ct)) {
		IS_TRUE0(IR_next(stmt) == NULL && IR_prev(stmt) == NULL);
		IR * newstmt_lst = simplify_stmt(stmt, ctx);
		while (newstmt_lst != NULL) {
			IR * newir = removehead(&newstmt_lst);
			newir->set_bb(bb);
			IS_TRUE0(newir->verify(get_dm()));
			new_ir_list.append_tail(newir);
		}
	}
	IR_BB_ir_list(bb).copy(new_ir_list);
}


//NOTE: simplification should not generate indirect memory operation.
void REGION::simplify_bb_list(IR_BB_LIST * bbl, SIMP_CTX * ctx)
{
	START_TIMER("Simplify IR_BB list");
	C<IR_BB*> * ct;
	for (IR_BB * bb = bbl->get_head(&ct);
		 bb != NULL; bb = bbl->get_next(&ct)) {
		simplify_bb(bb, ctx);
	}
	END_TIMER();
}
