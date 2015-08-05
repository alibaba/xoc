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
#include "ir_lcse.h"

//
//START IR_LCSE
//
IR_LCSE::IR_LCSE(REGION * ru)
{
	IS_TRUE0(ru != NULL);
	m_ru = ru;
	m_dm = ru->get_dm();
	m_du = m_ru->get_du_mgr();
	m_expr_tab = NULL;
	m_expr_vec = NULL;
	m_enable_filter = true;
}


//Hoist CSE's computation, and replace its occurrence with the result pr.
IR * IR_LCSE::hoist_cse(IN IR_BB * bb, IN IR * ir_pos, IN IR_EXPR * ie)
{
	C<IR*> * pos_holder = NULL;
	bool f = IR_BB_ir_list(bb).find(ir_pos, &pos_holder);
	IS_TRUE0(f);
	switch (IR_type(ir_pos)) {
	case IR_ST:
	case IR_IST:
		{
			//return the pr that hold the cse value.
			IR * ret = NULL;
			//Move STORE_VAL to temp PR.
			IR_EXPR * tie = m_expr_tab->map_ir2ir_expr(ST_rhs(ir_pos));
			if (tie != NULL && tie == ie) {
				//e.g: a = 10, expression of store_val is NULL.
				IR * x = ST_rhs(ir_pos);
				ret = m_ru->build_pr(IR_dt(x));
				IR * new_st = m_ru->build_store_pr(PR_no(ret), IR_dt(ret), x);

				//Insert into IR list of BB.
				IR_BB_ir_list(bb).insert_before(new_st, pos_holder);
				ST_rhs(ir_pos) = ret;
				ir_pos->set_parent_pointer(false);
			} //end if

			if (IR_type(ir_pos) == IR_IST) {
				//Move MEM ADDR to Temp PR.
				tie = m_expr_tab->map_ir2ir_expr(IST_base(ir_pos));
				if (tie != NULL && tie == ie) {
					if (ret == NULL) {
						IR * x = IST_base(ir_pos);
						ret = m_ru->build_pr(IR_dt(x));
						IR * new_st = m_ru->build_store_pr(PR_no(ret),
											IR_dt(ret), m_ru->dup_irs(x));

						//Insert into IR list of BB.
						IR_BB_ir_list(bb).insert_before(new_st, pos_holder);
					} else {
						ret = m_ru->dup_irs(ret);
					}

					//Replace orignial referenced IR with the new PR.
					IST_base(ir_pos) = ret;
					ir_pos->set_parent_pointer(false);
				}
			}
			return ret;
		}
		break;
	case IR_CALL:
	case IR_ICALL:
		{
			IR * parm = CALL_param_list(ir_pos);
			IR * ret = NULL; //return the pr that hold the cse value.
			IR * next_parm = NULL;
			while (parm != NULL) {
				next_parm = IR_next(parm);
				IR_EXPR * tie = m_expr_tab->map_ir2ir_expr(parm);
				if (tie != NULL && tie == ie) {
					bool insert_st;
					if (ret == NULL) {
						ret = m_ru->build_pr(IR_dt(parm));
						replace(&CALL_param_list(ir_pos), parm, ret);
						insert_st = true;
					} else {
						replace(&CALL_param_list(ir_pos), parm,
								m_ru->dup_irs(ret));
						insert_st = false;
					}
					IS_TRUE0(IR_prev(parm) == NULL && IR_next(parm) == NULL);

					if (insert_st) {
						IR * new_st = m_ru->build_store_pr(PR_no(ret),
														   IR_dt(ret), parm);
						IR_BB_ir_list(bb).insert_before(new_st, pos_holder);
					}
				}
				parm = next_parm;
			} //end while
			ir_pos->set_parent_pointer(false);
			return ret;
		}
		break;
	case IR_GOTO:
		break;
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_CASE:
		IS_TRUE(0, ("TODO"));
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		{
			IR_EXPR * tie = m_expr_tab->map_ir2ir_expr(BR_det(ir_pos));
			if (tie != NULL && tie == ie) {
				IR * x = BR_det(ir_pos);
				IR * ret = m_ru->build_pr(IR_dt(x));
				IR * new_st = m_ru->build_store_pr(PR_no(ret), IR_dt(ret), x);
				IR_BB_ir_list(bb).insert_before(new_st, pos_holder);
				BR_det(ir_pos) = ret;
				ir_pos->set_parent_pointer(false);
				return ret;
			}
		}
		break;
	case IR_IGOTO:
		{
			IR_EXPR * tie = m_expr_tab->map_ir2ir_expr(IGOTO_vexp(ir_pos));
			if (tie != NULL && tie == ie) {
				IR * x = IGOTO_vexp(ir_pos);
				IR * ret = m_ru->build_pr(IR_dt(x));
				IR * new_st = m_ru->build_store_pr(PR_no(ret), IR_dt(ret), x);
				IR_BB_ir_list(bb).insert_before(new_st, pos_holder);
				IGOTO_vexp(ir_pos) = ret;
				ir_pos->set_parent_pointer(false);
				return ret;
			}
		}
		break;
	case IR_SWITCH:
		{
			IR_EXPR * tie = m_expr_tab->map_ir2ir_expr(SWITCH_vexp(ir_pos));
			if (tie != NULL && tie == ie) {
				IR * x = SWITCH_vexp(ir_pos);
				IR * ret = m_ru->build_pr(IR_dt(x));
				IR * new_st = m_ru->build_store_pr(PR_no(ret), IR_dt(ret), x);
				IR_BB_ir_list(bb).insert_before(new_st, pos_holder);
				SWITCH_vexp(ir_pos) = ret;
				ir_pos->set_parent_pointer(false);
				return ret;
			}
		}
		break;
	case IR_RETURN:
		{
			IR_EXPR * tie = m_expr_tab->map_ir2ir_expr(RET_exp(ir_pos));
			if (tie != NULL && tie == ie) {
				IR * x = RET_exp(ir_pos);
				IR * ret = m_ru->build_pr(IR_dt(x));
				IR * new_st = m_ru->build_store_pr(PR_no(ret), IR_dt(ret), x);
				IR_BB_ir_list(bb).insert_before(new_st, pos_holder);
				RET_exp(ir_pos) = ret;
				ir_pos->set_parent_pointer(false);
				return ret;
			}
			ir_pos->set_parent_pointer(false);
		}
		break;
	default:
		IS_TRUE0(0);
	} //end switch
	return NULL;
}


bool IR_LCSE::process_br(IN IR_BB * bb, IN IR * ir,
						 IN OUT BITSET & avail_ir_expr,
						 IN OUT SVECTOR<IR*> & map_expr2avail_pos,
						 IN OUT SVECTOR<IR*> & map_expr2avail_pr)
{
	IS_TRUE0(IR_type(ir) == IR_TRUEBR || IR_type(ir) == IR_FALSEBR);
	bool change = false;
	if (!can_be_candidate(BR_det(ir))) { return false; }
	IR_EXPR * ie = m_expr_tab->map_ir2ir_expr(BR_det(ir));
	if (ie != NULL) {
		avail_ir_expr.bunion(IR_EXPR_id(ie));
		IR * ir_pos = map_expr2avail_pos.get(IR_EXPR_id(ie));
		if (ir_pos != NULL) {
			IR * pr = map_expr2avail_pr.get(IR_EXPR_id(ie));
			if (pr == NULL) {
				/* e.g: before:
					=a||b
					...
					falsebr(a||b)
				after:
					t=a||b
					=t
					...
					=a||b
				*/
				pr = hoist_cse(bb, ir_pos, ie);
				IS_TRUE0(pr != NULL);
				map_expr2avail_pr.set(IR_EXPR_id(ie), pr);
				change = true;
			}
			IR * imm0 = m_ru->build_imm_int(0, IR_dt(pr));
			BR_det(ir) = m_ru->build_cmp(IR_NE, m_ru->dup_irs(pr), imm0);
			ir->set_parent_pointer(false);
			change = true;
		} else {
			map_expr2avail_pos.set(IR_EXPR_id(ie), ir);
		}
	}
	return change;
}


/* Return new IR_PR if 'ie' has been regarded as cse candidate expression.
e.g:
		call(a+b, a+b);
	=>
		p1 = a+b;
		call(p1, p1);
	return p1 as new expression.

'ie': cse candidate expression indicator.
'stmt': the stmt contains 'exp'. */
IR * IR_LCSE::process_exp(IN IR_BB * bb, IN IR_EXPR * ie,
						 IN IR * stmt, IN OUT BITSET & avail_ir_expr,
						 IN OUT SVECTOR<IR*> & map_expr2avail_pos,
						 IN OUT SVECTOR<IR*> & map_expr2avail_pr)
{
	IS_TRUE0(ie);
	avail_ir_expr.bunion(IR_EXPR_id(ie));

	//Find which stmt that the expression start to live at.
	IR * ir_pos = map_expr2avail_pos.get(IR_EXPR_id(ie));
	if (ir_pos != NULL) {
		IR * cse_val_pr = map_expr2avail_pr.get(IR_EXPR_id(ie));
		if (cse_val_pr == NULL) {
			/*
			e.g:
			before:
				=a||b
				...
				falsebr(a||b)
			after:
				t=a||b
				=t
				...
				=a||b
			*/
			cse_val_pr = hoist_cse(bb, ir_pos, ie);
			IS_TRUE0(cse_val_pr != NULL);
			map_expr2avail_pr.set(IR_EXPR_id(ie), cse_val_pr);
		}
		return cse_val_pr;
	}

	//First meet the expression, and record its stmt.
	map_expr2avail_pos.set(IR_EXPR_id(ie), stmt);
	return NULL;
}


bool IR_LCSE::can_be_candidate(IR * ir)
{
	if (!m_enable_filter) {
		return ir->is_bin_op() ||
			IR_type(ir) == IR_BNOT ||
			IR_type(ir) == IR_LNOT ||
			IR_type(ir) == IR_NEG;
	}
	IS_TRUE0(ir->is_exp());
	if (IR_type(ir) == IR_ILD) {
		/*
		Avoid perform the opposite behavior to Copy-Propagation.
		e.g:
			ST(P1, ILD(P2))
			ST(P3, ILD(P2))
			after LCSE, we get:
			ST(P1, ILD(P2))
			ST(P3, P1)
			But CP will progagate P1 because ILD is the copy-prop candidate.
		*/
		return false;
	}
	return ir->is_bin_op() || IR_type(ir) == IR_BNOT ||
		   IR_type(ir) == IR_LNOT || IR_type(ir) == IR_NEG;
}


bool IR_LCSE::process_st_rhs(IN IR_BB * bb, IN IR * ir,
							 IN OUT BITSET & avail_ir_expr,
							 IN OUT SVECTOR<IR*> & map_expr2avail_pos,
							 IN OUT SVECTOR<IR*> & map_expr2avail_pr)
{
	IS_TRUE0(IR_type(ir) == IR_ST || IR_type(ir) == IR_IST);
	bool change = false;
	if (!can_be_candidate(ST_rhs(ir))) {
		return false;
	}
	IR_EXPR * ie = m_expr_tab->map_ir2ir_expr(ST_rhs(ir));
	if (ie != NULL) {
		avail_ir_expr.bunion(IR_EXPR_id(ie));
		//e.g: a = 10, expression of store_val is NULL.
		IR * ir_pos = map_expr2avail_pos.get(IR_EXPR_id(ie));
		if (ir_pos != NULL) {
			IR * pr = map_expr2avail_pr.get(IR_EXPR_id(ie));
			if (pr == NULL) {
				/*
				e.g: before:
					=a+b
					...
						=a+b
				after:
					t=a+b
					=t
					...
					=a+b
				*/
				pr = hoist_cse(bb, ir_pos, ie);
				IS_TRUE0(pr != NULL);
				map_expr2avail_pr.set(IR_EXPR_id(ie), pr);
				change = true;
			}
			ST_rhs(ir) = m_ru->dup_irs(pr);
			ir->set_parent_pointer(false);
			change = true;
		} else {
			//Record position of IR stmt.
			map_expr2avail_pos.set(IR_EXPR_id(ie), ir);
		}
	}

	if (IR_type(ir) == IR_IST) {
		ie = m_expr_tab->map_ir2ir_expr(IST_base(ir));
		if (ie != NULL) {
			avail_ir_expr.bunion(IR_EXPR_id(ie));
			IR * ir_pos = map_expr2avail_pos.get(IR_EXPR_id(ie));
			if (ir_pos != NULL) {
				IR * pr = map_expr2avail_pr.get(IR_EXPR_id(ie));
				if (pr == NULL) {
					/* e.g: before:
						=a+b
						...
							=a+b
					after:
						t=a+b
						=t
						...
						=a+b */
					pr = hoist_cse(bb, ir_pos, ie);
					IS_TRUE0(pr != NULL);
					map_expr2avail_pr.set(IR_EXPR_id(ie), pr);
					change = true;
				}
				IST_base(ir) = m_ru->dup_irs(pr);
				ir->set_parent_pointer(false);
				change = true;
			} else {
				map_expr2avail_pos.set(IR_EXPR_id(ie), ir);
			}
		}
	}
	return change;
}


bool IR_LCSE::process_param_list(IN IR_BB * bb, IN IR * ir,
								 IN OUT BITSET & avail_ir_expr,
								 IN OUT SVECTOR<IR*> & map_expr2avail_pos,
								 IN OUT SVECTOR<IR*> & map_expr2avail_pr)
{
	bool change = false;
	bool lchange = true;
	while (lchange) {
		/*
		Iterative analyse cse, e.g:
			CALL(ADD(x,y), SUB(a,b), ADD(x,y), SUB(a,b))
		*/
		IR * parm = CALL_param_list(ir);
		lchange = false;
		while (parm != NULL) {
			if (can_be_candidate(parm)) {
				IR_EXPR * ie = m_expr_tab->map_ir2ir_expr(parm);
				IR * newparam = process_exp(bb,	ie, ir,
											avail_ir_expr,
											map_expr2avail_pos,
											map_expr2avail_pr);
				if (newparam != NULL) {
					change = true;
					lchange = true;
					break;
				}
			}
			parm = IR_next(parm);
		}
	}
	return change;
}


bool IR_LCSE::process_use(IN IR_BB * bb, IN IR * ir,
						  IN OUT BITSET & avail_ir_expr,
						  IN OUT SVECTOR<IR*> & map_expr2avail_pos,
						  IN OUT SVECTOR<IR*> & map_expr2avail_pr)
{
	bool change = false;
	switch (IR_type(ir)) {
	case IR_ST:
	case IR_IST:
		change |= process_st_rhs(bb, ir, avail_ir_expr, map_expr2avail_pos,
								 map_expr2avail_pr);
		break;
	case IR_CALL:
	case IR_ICALL:
		change |= process_param_list(bb, ir, avail_ir_expr, map_expr2avail_pos,
									 map_expr2avail_pr);
		break;
	case IR_GOTO:
		break;
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_CASE:
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		{
			IS_TRUE0(BR_det(ir));
			if (!can_be_candidate(BR_det(ir))) { break; }
			IR_EXPR * ie = m_expr_tab->map_ir2ir_expr(BR_det(ir));
			IS_TRUE0(ie);
			IR * cse_val = process_exp(bb, ie, ir, avail_ir_expr,
									   map_expr2avail_pos,
									   map_expr2avail_pr);
			if (cse_val != NULL) {
				if (!cse_val->is_judge()) {
					IR * old = cse_val;
					cse_val = m_ru->build_cmp(IR_NE,
								m_ru->dup_irs(cse_val),
								m_ru->build_imm_int(0, IR_dt(cse_val)));
					BR_det(ir) = cse_val;
				} else {
					BR_det(ir) = m_ru->dup_irs(cse_val);
				}
				ir->set_parent_pointer();
				change = true;
			}
		}
		break;
	case IR_IGOTO:
		{
			IS_TRUE0(IGOTO_vexp(ir));
			if (!can_be_candidate(IGOTO_vexp(ir))) { break; }
			IR_EXPR * ie = m_expr_tab->map_ir2ir_expr(IGOTO_vexp(ir));
			IS_TRUE0(ie);
			IR * cse_val = process_exp(bb, ie, ir, avail_ir_expr,
									   map_expr2avail_pos,
									   map_expr2avail_pr);
			if (IGOTO_vexp(ir) != cse_val) {
				if (!cse_val->is_judge()) {
					IR * old = cse_val;
					cse_val = m_ru->build_cmp(IR_NE, m_ru->dup_irs(cse_val),
									  m_ru->build_imm_int(0, IR_dt(cse_val)));
					IGOTO_vexp(ir) = cse_val;
				} else {
					IGOTO_vexp(ir) = m_ru->dup_irs(cse_val);
				}
				ir->set_parent_pointer();
				change = true;
			}
		}
		break;
	case IR_SWITCH:
		{
			IS_TRUE0(SWITCH_vexp(ir));
			if (!can_be_candidate(SWITCH_vexp(ir))) { break; }
			IR_EXPR * ie = m_expr_tab->map_ir2ir_expr(SWITCH_vexp(ir));
			IS_TRUE0(ie);
			IR * cse_val = process_exp(bb, ie, ir, avail_ir_expr,
									   map_expr2avail_pos,
									   map_expr2avail_pr);
			if (SWITCH_vexp(ir) != cse_val) {
				if (!cse_val->is_judge()) {
					IR * old = cse_val;
					cse_val = m_ru->build_cmp(IR_NE, m_ru->dup_irs(cse_val),
									  m_ru->build_imm_int(0, IR_dt(cse_val)));
					SWITCH_vexp(ir) = cse_val;
				} else {
					SWITCH_vexp(ir) = m_ru->dup_irs(cse_val);
				}
				ir->set_parent_pointer();
				change = true;
			}
		}
		break;
	case IR_RETURN:
		{
			IS_TRUE0(SWITCH_vexp(ir));
			if (!can_be_candidate(RET_exp(ir))) { break; }
			IR_EXPR * ie = m_expr_tab->map_ir2ir_expr(RET_exp(ir));
			IS_TRUE0(ie);
			IR * cse_val = process_exp(bb, ie, ir, avail_ir_expr,
									   map_expr2avail_pos,
									   map_expr2avail_pr);
			if (RET_exp(ir) != cse_val) {
				SWITCH_vexp(ir) = m_ru->dup_irs(cse_val);
				ir->set_parent_pointer();
				change = true;
			}
		}
		break;
	default:
		IS_TRUE0(0);
	} //end switch
	return change;
}


//Return true if common expression has been substituted.
bool IR_LCSE::process_def(IN IR_BB * bb, IN IR * ir,
						  IN OUT BITSET & avail_ir_expr,
						  IN OUT SVECTOR<IR*> & map_expr2avail_pos,
						  IN OUT SVECTOR<IR*> & map_expr2avail_pr,
						  IN MD_SET & tmp)
{
	bool change = false;
	switch (IR_type(ir)) {
	case IR_ST:
	case IR_IST:
	case IR_CALL:
	case IR_ICALL:
	case IR_RETURN:
		{
			//Compute killed ir-expr.
			MD_SET const* maydef = m_du->get_may_def(ir);
			MD const* mustdef = m_du->get_must_def(ir);
			if ((maydef != NULL && !maydef->is_empty()) ||
				mustdef != NULL) {
				for (INT j = avail_ir_expr.get_first(); j != -1;
					 j = avail_ir_expr.get_next(j)) {
					IR_EXPR * ie = m_expr_vec->get(j);
					IS_TRUE0(ie != NULL);
					for (IR * occ = IR_EXPR_occ_list(ie).get_head();
						 occ != NULL; occ = IR_EXPR_occ_list(ie).get_next()) {
						IR * occ_stmt = occ->get_stmt();
						IS_TRUE0(occ_stmt != NULL && occ_stmt->get_bb());
						IS_TRUE0(ir->get_bb() == bb);
						if (occ_stmt->get_bb() != bb) {
							continue;
						}
						tmp.clean();
						m_du->collect_may_use_recur(occ, tmp, true);
						if ((maydef != NULL && maydef->is_intersect(tmp)) ||
							(mustdef != NULL && tmp.is_contain(mustdef))) {
							avail_ir_expr.diff(IR_EXPR_id(ie));
							map_expr2avail_pos.set(IR_EXPR_id(ie), NULL);
							map_expr2avail_pr.set(IR_EXPR_id(ie), NULL);
						}
					}
				}
			}
		}
		break;
	case IR_GOTO:
	case IR_IGOTO:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_SWITCH:
	case IR_LABEL:
	case IR_CASE:
	case IR_TRUEBR:
	case IR_FALSEBR:
		break;
	default:
		IS_TRUE0(0);
	} //end switch
	return change;
}


bool IR_LCSE::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	m_ru->check_valid_and_recompute(&oc, OPT_DU_REF, OPT_DU_CHAIN,
									OPT_EXPR_TAB, OPT_UNDEF);

	m_expr_tab = (IR_EXPR_TAB*)OPTC_pass_mgr(oc)->register_opt(OPT_EXPR_TAB);
	IS_TRUE0(m_expr_tab);

	m_expr_vec = m_expr_tab->get_expr_vec();
	IS_TRUE0(m_expr_vec);

	IR_BB_LIST * bbl = m_ru->get_bb_list();

	bool change = false;

	//Record lived expression during analysis.
	BITSET avail_ir_expr;

	//Record ir stmt's address as a position.
	SVECTOR<IR*> map_expr2avail_pos;

	//Record pr that hold the value of expression.
	SVECTOR<IR*> map_expr2avail_pr;
	C<IR_BB*> * ctbb;
	MD_SET tmp;
	for (IR_BB * bb = bbl->get_head(&ctbb);
		 bb != NULL; bb = bbl->get_next(&ctbb)) {
		map_expr2avail_pos.clean();
		map_expr2avail_pr.clean();
		avail_ir_expr.clean();
		C<IR*> * ct = NULL;
		for (IR * ir = IR_BB_ir_list(bb).get_head(&ct); ir != NULL;
			 ir = IR_BB_ir_list(bb).get_next(&ct)) {
			change |= process_use(bb, ir, avail_ir_expr,
								  map_expr2avail_pos,
								  map_expr2avail_pr);
			if (ir->has_result()) {
				//There may have expressions be killed.
				//Remove them out the avail_ir_expr.
				change |= process_def(bb, ir, avail_ir_expr,
									  map_expr2avail_pos,
									  map_expr2avail_pr,
									  tmp);
			}
		} //end for each IR
	}

	IS_TRUE0(verify_ir_and_bb(bbl, m_dm));
	if (change) {
		//Found CSE and processed them.
		OPTC_is_expr_tab_valid(oc) = false;
		OPTC_is_aa_valid(oc) = false;
		OPTC_is_du_chain_valid(oc) = false;
		OPTC_is_ref_valid(oc) = false;
	}
	END_TIMER_AFTER(get_opt_name());
	return change;
}
//END IR_LCSE
