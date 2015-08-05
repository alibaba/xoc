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
#include "ir_gvn.h"
#include "ir_gcse.h"

//#define DEBUG_GCSE
#ifdef DEBUG_GCSE
static INT g_num_of_elim = 0;
LIST<IR_TYPE> g_elim_irt;
#endif

//
//START IR_GCSE
//

/* Replace use cse with PR related to gen.
e.g: ...=a+b <--generate CSE
	 ...
	 ...=a+b <--use CSE
This function do replacement via gvn info.

'use': the referrence of cse.
'use_stmt': the stmt contains use.
'gen': the referrence of cse.

NOTE: 'use' should be freed.
	'use' must be rhs of 'use_stmt'. */
void IR_GCSE::elim_cse_at_store(IR * use, IR * use_stmt, IR * gen)
{
	IS_TRUE0(use_stmt->is_stid() || use_stmt->is_stpr() ||
			 IR_type(use_stmt) == IR_IST);
	#ifdef DEBUG_GCSE
	IS_TRUE0(++g_num_of_elim);
	IS_TRUE0(g_elim_irt.append_tail(IR_type(use)));
	#endif
	IS_TRUE0(use->is_exp() && gen->is_exp());
	IS_TRUE0(use_stmt->get_rhs() == use);

	//Cut off du chain for use and its definitions.
	m_du->remove_use_out_from_defset(use);

	//gen_pr hold the CSE value come from gen-stmt.
	//We eliminate the redundant computation via replace use by gen_pr.
	IR * gen_pr = m_exp2pr.get(gen);
	IS_TRUE0(gen_pr);

	IR * newrhs = m_ru->dup_irs(gen_pr);
	use_stmt->set_rhs(newrhs);
	IR_parent(newrhs) = use_stmt;

	//Use stmt is just a move.
	IR_may_throw(use_stmt) = false;

	//Add du chain.
	IR * gen_stmt = gen->get_stmt();
	IS_TRUE0(gen_stmt->is_pr_equal(gen_pr));
	m_du->build_du_chain(gen_stmt, newrhs);

	//Assign the identical vn to newrhs.
	IS_TRUE0(m_gvn);
	VN * vn = m_gvn->map_ir2vn(gen);
	IS_TRUE0(vn);
	m_gvn->set_map_ir2vn(newrhs, vn);
	m_gvn->set_map_ir2vn(use_stmt, vn);

	//Assign MD to newrhs.
	MD const* r_md = m_ru->gen_md_for_pr(newrhs);
	IS_TRUE0(r_md);
	newrhs->set_ref_md(r_md, m_ru);

	m_ru->free_irs(use);
}


void IR_GCSE::elim_cse_at_br(IR * use, IR * use_stmt, IN IR * gen)
{
	#ifdef DEBUG_GCSE
	IS_TRUE0(++g_num_of_elim);
	IS_TRUE0(g_elim_irt.append_tail(IR_type(use)));
	#endif
	IS_TRUE0(use->is_exp() && gen->is_exp());

	//Cut off du chain for use and its definitions.
	m_du->remove_use_out_from_defset(use);

	IR * gen_pr = m_exp2pr.get(gen);
	IS_TRUE0(gen_pr);
	IS_TRUE0(BR_det(use_stmt) == use);
	IR * r = m_ru->dup_irs(gen_pr);

	//Add du chain.
	IR * gen_stmt = gen->get_stmt();
	IS_TRUE0(gen_stmt->is_pr_equal(gen_pr));
	m_du->build_du_chain(gen_stmt, r);

	//Assign the idential vn to r.
	IS_TRUE0(m_gvn);
	VN * vn = m_gvn->map_ir2vn(gen);
	IS_TRUE0(vn);
	m_gvn->set_map_ir2vn(r, vn);

	//Assign MD to PR.
	MD const* r_md = m_ru->gen_md_for_pr(r);
	IS_TRUE0(r_md);
	r->set_ref_md(r_md, m_ru);

	IR * newdet = m_ru->build_cmp(IR_NE, r, m_ru->build_imm_int(0, IR_dt(r)));
	IR_parent(newdet) = use_stmt;
	BR_det(use_stmt) = newdet;
	IR_may_throw(use_stmt) = false;

	m_ru->free_irs(use);
}


/* Replace use_cse with PR related to gen_cse.
This function do replacement via gvn info.

e.g: ...=a+b <--generate CSE
	 ...
	 ...call, a+b, a+b <--two use CSE.

'use': the referrence expression of cse.
'use_stmt': the stmt which 'use' is belong to.
'gen': the first occurrence of CSE.

NOTE: 'use' should be freed. */
void IR_GCSE::elim_cse_at_call(IR * use, IR * use_stmt, IR * gen)
{
	#ifdef DEBUG_GCSE
	IS_TRUE0(++g_num_of_elim);
	IS_TRUE0(g_elim_irt.append_tail(IR_type(use)));
	#endif
	IS_TRUE0(use->is_exp() && gen->is_exp() && use_stmt->is_stmt());

	//Cut off du chain for use and its definitions.
	m_du->remove_use_out_from_defset(use);

	IR * gen_pr = m_exp2pr.get(gen);
	IS_TRUE0(gen_pr && gen_pr->is_pr());
	IR * use_pr = m_ru->dup_irs(gen_pr);

	//Set identical vn to use_pr with CSE.
	IR * gen_stmt = gen->get_stmt();
	IS_TRUE0(m_gvn);
	VN * vn = m_gvn->map_ir2vn(gen);
	IS_TRUE0(vn);
	m_gvn->set_map_ir2vn(use_pr, vn);

	//Allocate MD to use_pr to make up DU manager request.
	MD const* r_md = m_ru->gen_md_for_pr(use_pr);
	IS_TRUE0(r_md);
	use_pr->set_ref_md(r_md, m_ru);

	//Add du chain from gen_pr's stmt to the use of pr.
	bool f = use_stmt->replace_kid(use, use_pr, false);
	IS_TRUE0(f);
	m_ru->free_irs(use);
	m_du->build_du_chain(gen_stmt, use_pr);
}


/* Replace use_cse with PR related to gen_cse.
e.g: ...=a+b <--generate CSE
	 ...
	 ...return, a+b, a+b <--two use CSE.

'use': the referrence expression of cse.
'use_stmt': the stmt which 'use' is belong to.
'gen': the first occurrence of CSE.

NOTE: 'use' should be freed. */
void IR_GCSE::elim_cse_at_return(IR * use, IR * use_stmt, IR * gen)
{
	return elim_cse_at_call(use, use_stmt, gen);
}


/* Process the expression in CSE generation.
This function do replacement via gvn info.

e.g: ...=a+b <--generate CSE
	 ...
	 ...=a+b <--use CSE
'gen': generated cse. */
void IR_GCSE::process_cse_gen(IN IR * gen, IR * gen_stmt, bool & change)
{
	IS_TRUE0(gen->is_exp() && gen_stmt->is_stmt());
	//Move STORE_VAL to temp PR.
	//e.g: a = 10, expression of store_val is NULL.
	IR_BB * bb = gen_stmt->get_bb();
	IS_TRUE0(bb);
	IR * tmp_pr = m_exp2pr.get(gen);
	if (tmp_pr != NULL) { return; }

	//First process cse generation point.
	tmp_pr = m_ru->build_pr(IR_dt(gen));
	m_exp2pr.set(gen, tmp_pr);

	//Assign MD to PR.
	MD const* tmp_pr_md = m_ru->gen_md_for_pr(tmp_pr);
	IS_TRUE0(tmp_pr_md);
	tmp_pr->set_ref_md(tmp_pr_md, m_ru);

	//Assign MD to ST.
	IR * new_st = m_ru->build_store_pr(PR_no(tmp_pr), IR_dt(tmp_pr), gen);
	new_st->set_ref_md(tmp_pr_md, m_ru);

	IS_TRUE0(m_gvn && m_gvn->map_ir2vn(gen));
	m_gvn->set_map_ir2vn(new_st, m_gvn->map_ir2vn(gen));

	copy_dbx(new_st, gen_stmt, m_ru);

	//The 'find()' is fast because it is implemented with hash.
	C<IR*> * holder = NULL;
	bool f = IR_BB_ir_list(bb).find(gen_stmt, &holder);
	IS_TRUE0(f && holder);
	IR_BB_ir_list(bb).insert_before(new_st, holder);

	if (gen_stmt->is_cond_br() && gen == BR_det(gen_stmt)) {
		IR * old = tmp_pr;
		tmp_pr = m_ru->build_judge(tmp_pr);
		copy_dbx(tmp_pr, old, m_ru);
	}

	bool v = gen_stmt->replace_kid(gen, tmp_pr, false);
	IS_TRUE0(v);

	//Keep original du unchange, add new du chain for new stmt.
	m_du->build_du_chain(new_st, tmp_pr);

	IR_may_throw(gen_stmt) = false;
	change = true;
}


bool IR_GCSE::is_cse_cand(IR * ir)
{
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
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_ILD:
		return true;
	default: break;
	}
	return false;
}


bool IR_GCSE::elim(IR * use, IR * use_stmt, IR * gen, IR * gen_stmt)
{
	/* exp is CSE.
	e.g: ...=a+b <--generate CSE
		 ...
		 ...=a+b <--use CSE */
	bool change = false;
	process_cse_gen(gen, gen_stmt, change);
	switch (IR_type(use_stmt)) {
	case IR_ST:
	case IR_STPR:
	case IR_IST:
		elim_cse_at_store(use, use_stmt, gen);
		change = true;
		break;
	case IR_CALL:
	case IR_ICALL:
		elim_cse_at_call(use, use_stmt, gen);
		change = true;
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		elim_cse_at_br(use, use_stmt, gen);
		change = true;
		break;
	case IR_RETURN:
		elim_cse_at_return(use, use_stmt, gen);
		change = true;
		break;
	default: break;
	}
	return change;
}


// If find 'exp' is cse, replace it with related pr.
//NOTE: exp should be freed.
bool IR_GCSE::find_and_elim(IR * exp, VN const* vn, IR * gen)
{
	IS_TRUE0(exp && vn && gen);
	IS_TRUE0(exp != gen);
	IR * exp_stmt = exp->get_stmt();
	IR * gen_stmt = gen->get_stmt();
	IS_TRUE0(exp_stmt->get_bb() && gen_stmt->get_bb());

	IR_BB * gen_bb;
	IR_BB * exp_bb;
	if (m_cfg->has_eh_edge()) {
		IS_TRUE0(m_tg);
		if ((gen_bb = gen_stmt->get_bb()) == (exp_bb = exp_stmt->get_bb())) {
			if (!gen_bb->is_dom(gen_stmt, exp_stmt, true)) {
				return false;
			}
		} else if (!m_tg->is_dom(IR_BB_id(gen_bb), IR_BB_id(exp_bb))) {
			return false;
		}
	} else {
		if ((gen_bb = gen_stmt->get_bb()) == (exp_bb = exp_stmt->get_bb())) {
			if (!gen_bb->is_dom(gen_stmt, exp_stmt, true)) {
				return false;
			}
		} else if (!m_cfg->is_dom(IR_BB_id(gen_bb), IR_BB_id(exp_bb))) {
			return false;
		}
	}
	return elim(exp, exp_stmt, gen, gen_stmt);
}


//If find 'exp' is cse, replace it with related pr.
//NOTE: exp should be freed.
bool IR_GCSE::process_cse(IN IR * exp, IN LIST<IR*> & livexp)
{
	IR * expstmt = exp->get_stmt();
	IR_EXPR * irie = m_expr_tab->map_ir2ir_expr(exp);
	IS_TRUE0(irie && expstmt->get_bb());
	C<IR*> * ct;
	bool change = false;
	for (IR * gen = livexp.get_head(&ct);
		 gen != NULL; gen = livexp.get_next(&ct)) {
		IR_EXPR * xie = m_expr_tab->map_ir2ir_expr(gen);
		IS_TRUE0(xie);
		if (irie != xie) { continue; }
		IR * gen_stmt = gen->get_stmt();
		IS_TRUE0(gen_stmt->get_bb());
		UINT iid = IR_BB_id(expstmt->get_bb());
		UINT xid = IR_BB_id(gen_stmt->get_bb());
		if (!m_cfg->get_dom_set(iid)->is_contain(xid)) {
			continue;
		}
		return elim(exp, expstmt, gen, gen_stmt);
	}
	return change;
}


void IR_GCSE::handle_cand(IR * exp, IR_BB * bb, UINT entry_id, bool & change)
{
	VN const* vn = NULL;
	IR * gen = NULL;
	if ((vn = m_gvn->map_ir2vn(exp)) != NULL &&
		(gen = m_vn2exp.get(vn)) != NULL &&
		find_and_elim(exp, vn, gen)) {
		//Found cse and replaced it with pr.
		change = true;
	} else if (vn != NULL && gen == NULL) {
		if (m_cfg->has_eh_edge()) {
			IS_TRUE0(m_tg);
			if (m_tg->is_pdom(IR_BB_id(bb), entry_id)) {
				m_vn2exp.set(vn, exp);
			}
		} else if (m_cfg->is_pdom(IR_BB_id(bb), entry_id)) {
			m_vn2exp.set(vn, exp);
		}
	}
}


//Determine if det-exp of truebr/falsebr ought to be cse.
bool IR_GCSE::should_be_cse(IR * det)
{
	IS_TRUE0(det->is_judge());
	//If the det if simply enough, cse is dispensable.
	if (IR_type(IR_parent(det)) != IR_TRUEBR &&
		IR_type(IR_parent(det)) != IR_FALSEBR) {
		return true;
	}
	if (!det->is_relation()) {
		//det is complex operation.
		return true;
	}

	IR const* op0 = BIN_opnd0(det);
	IR const* op1 = BIN_opnd1(det);
	if (IR_type(op0) != IR_PR && IR_type(op0) != IR_CONST) {
		return true;
	}
	if (IR_type(op1) != IR_PR && IR_type(op1) != IR_CONST) {
		return true;
	}
	return false;
}


//Do propagation according to value numbering.
bool IR_GCSE::do_prop_vn(IR_BB * bb, UINT entry_id, MD_SET & tmp)
{
	bool change = false;
	C<IR*> * ct;
	for (IR * ir = IR_BB_ir_list(bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&ct)) {
		switch (IR_type(ir)) {
		case IR_ST:
		case IR_STPR:
		case IR_IST:
			{
				IR * rhs = ir->get_rhs();
				//Find cse and replace it with properly pr.
				if (is_cse_cand(rhs)) {
					handle_cand(rhs, bb, entry_id, change);
				}
			}
			break;
		case IR_CALL:
		case IR_ICALL:
			{
				IR * p = CALL_param_list(ir);
				IR * next = NULL;
				bool lchange = false;
				m_newst_lst.clean();
				while (p != NULL) {
					next = IR_next(p);
					if (is_cse_cand(p)) {
						handle_cand(p, bb, entry_id, lchange);
					}
					p = next;
				}
				change |= lchange;
			}
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			//Find cse and replace it with properly pr.
			if (is_cse_cand(BR_det(ir)) && should_be_cse(BR_det(ir))) {
				handle_cand(BR_det(ir), bb, entry_id, change);
			}
			break;
		case IR_RETURN:
			if (is_cse_cand(RET_exp(ir)) && should_be_cse(RET_exp(ir))) {
				handle_cand(RET_exp(ir), bb, entry_id, change);
			}
			break;
		default: break;
		}
	}
	return change;
}


//Do propagation according to lexciographic equivalence.
bool IR_GCSE::do_prop(IR_BB * bb, LIST<IR*> & livexp)
{
	livexp.clean();
	DBITSETC * x = m_du->get_availin_expr(IR_BB_id(bb));
	SC<SEG*> * st;
	for (INT i = x->get_first(&st); i != -1; i = x->get_next(i, &st)) {
		IR * y = m_ru->get_ir(i);
		IS_TRUE0(y && y->is_exp());
		livexp.append_tail(y);
	}
	BITSET * domset = m_cfg->get_dom_set(IR_BB_id(bb));
	bool change = false;
	C<IR*> * ct;
	MD_SET tmp;
	for (IR * ir = IR_BB_ir_list(bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&ct)) {
		switch (IR_type(ir)) {
		case IR_ST:
			//Find cse and replace it with properly pr.
			if (is_cse_cand(ST_rhs(ir))) {
				if (process_cse(ST_rhs(ir), livexp)) {
					//Has found cse and replaced cse with pr.
					change = true;
				} else {
					//Generate new cse.
					livexp.append_tail(ST_rhs(ir));
				}
			}
			break;
		case IR_STPR:
			//Find cse and replace it with properly pr.
			if (is_cse_cand(STPR_rhs(ir))) {
				if (process_cse(STPR_rhs(ir), livexp)) {
					//Has found cse and replaced cse with pr.
					change = true;
				} else {
					//Generate new cse.
					livexp.append_tail(STPR_rhs(ir));
				}
			}
			break;
		case IR_IST:
			//Find cse and replace it with properly pr.
			if (is_cse_cand(IST_rhs(ir))) {
				if (process_cse(IST_rhs(ir), livexp)) {
					//Has found cse and replaced cse with pr.
					change = true;
				} else {
					//Generate new cse.
					livexp.append_tail(IST_rhs(ir));
				}
			}
			break;
		case IR_CALL:
		case IR_ICALL:
			{
				IR * param = CALL_param_list(ir);
				IR * next = NULL;
				while (param != NULL) {
					next = IR_next(param);
					if (is_cse_cand(param)) {
						if (process_cse(param, livexp)) {
							//Has found cse and replaced cse with pr.
							change = true;
						} else {
							//Generate new cse.
							livexp.append_tail(param);
						}
					}
					param = next;
				}
			}
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			if (is_cse_cand(BR_det(ir)) && should_be_cse(BR_det(ir))) {
				if (process_cse(BR_det(ir), livexp)) {
					//Has found cse and replaced cse with pr.
					change = true;
				} else {
					//Generate new cse.
					livexp.append_tail(BR_det(ir));
				}
			}
			break;
		case IR_RETURN:
			if (is_cse_cand(RET_exp(ir)) && should_be_cse(RET_exp(ir))) {
				if (process_cse(RET_exp(ir), livexp)) {
					//Has found cse and replaced cse with pr.
					change = true;
				} else {
					//Generate new cse.
					livexp.append_tail(RET_exp(ir));
				}
			}
			break;
		default: break;
		}

		//Remove may-killed live-expr.
		switch (IR_type(ir)) {
		case IR_ST:
		case IR_STPR:
		case IR_IST:
		case IR_CALL:
		case IR_ICALL:
			{
				MD_SET const* maydef = m_du->get_may_def(ir);
				if (maydef != NULL && !maydef->is_empty()) {
					C<IR*> * ct2, * next;
					for (livexp.get_head(&ct2), next = ct2;
						 ct2 != NULL; ct2 = next) {
						livexp.get_next(&next);
						IR * x = C_val(ct2);
						tmp.clean();
						m_du->collect_may_use_recur(x, tmp, true);
						if (maydef->is_intersect(tmp)) {
							livexp.remove(ct2);
						}
					}
				}

				MD const* mustdef = m_du->get_must_def(ir);
				if (mustdef != NULL) {
					C<IR*> * ct2, * next;
					for (livexp.get_head(&ct2), next = ct2;
						 ct2 != NULL; ct2 = next) {
						livexp.get_next(&next);
						IR * x = C_val(ct2);
						tmp.clean();
						m_du->collect_may_use_recur(x, tmp, true);
						if (tmp.is_overlap(mustdef)) {
							livexp.remove(ct2);
						}
					}
				}
			}
			break;
		default: break;
		}
	}
	return change;
}


bool IR_GCSE::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	if (m_gvn != NULL) {
		m_ru->check_valid_and_recompute(&oc, OPT_DOM, OPT_PDOM,
										OPT_DU_REF, OPT_DU_CHAIN, OPT_UNDEF);
		if (!m_gvn->is_valid()) {
			m_gvn->reperform(oc);
			m_gvn->dump();
		}
		m_expr_tab = NULL;
	} else {
		m_ru->check_valid_and_recompute(&oc, OPT_DOM, OPT_PDOM, OPT_EXPR_TAB,
										OPT_DU_REF, OPT_DU_CHAIN, OPT_UNDEF);
		m_expr_tab = (IR_EXPR_TAB*)OPTC_pass_mgr(oc)->register_opt(OPT_EXPR_TAB);
	}

	bool change = false;
	IR_BB * entry = m_cfg->get_entry_list()->get_head();
	IS_TRUE(IR_BB_is_entry(entry), ("first bb should be func entry"));

	GRAPH domtree;
	m_cfg->get_dom_tree(domtree);
	LIST<VERTEX*> lst;
	VERTEX * root = domtree.get_vertex(IR_BB_id(entry));
	m_cfg->sort_dom_tree_in_preorder(domtree, root, lst);

	LIST<IR*> livexp;
	if (m_cfg->has_eh_edge()) {
		m_tg = new TG(m_ru);
		m_tg->clone(*m_cfg);
		m_tg->pick_eh();
		m_tg->remove_unreach_node(IR_BB_id(entry));
		m_tg->compute_dom_idom();
		m_tg->compute_pdom_ipdom(root);
	}

	#ifdef DEBUG_GCSE
	g_num_of_elim = 0;
	g_elim_irt.clean();
	#endif

	if (m_gvn != NULL) {
		m_vn2exp.clean();
		m_exp2pr.clean();
		MD_SET tmp;
		for (VERTEX * v = lst.get_head(); v != NULL; v = lst.get_next()) {
			IR_BB * bb = m_ru->get_bb(VERTEX_id(v));
			IS_TRUE0(bb);
			change |= do_prop_vn(bb, IR_BB_id(entry), tmp);
		}
	} else {
		m_vn2exp.clean();
		m_exp2pr.clean();
		for (VERTEX * v = lst.get_head(); v != NULL; v = lst.get_next()) {
			IR_BB * bb = m_ru->get_bb(VERTEX_id(v));
			IS_TRUE0(bb);
			change |= do_prop(bb, livexp);
		}
	}

	if (change) {
		#ifdef DEBUG_GCSE
		FILE * h = fopen("gcse.effect.log", "a+");
		fprintf(h, "\n\"%s\",", m_ru->get_ru_name());
		/*
		fprintf(h, " elim_num:%d, ", g_num_of_elim);
		for (UINT i = (UINT)g_elim_irt.get_head();
			 i != 0; i = (UINT)g_elim_irt.get_next()) {
			fprintf(h, "%s,", IRTNAME(i));
		}
		*/
		fclose(h);
		#endif
		//no new expr generated, only new pr.

		OPTC_is_expr_tab_valid(oc) = false;
		OPTC_is_aa_valid(oc) = false;

		//DU reference and du chain has maintained.
		IS_TRUE0(m_du->verify_du_ref());
		IS_TRUE0(m_du->verify_du_chain());

		OPTC_is_live_expr_valid(oc) = false;
		OPTC_is_reach_def_valid(oc) = false;

		//gvn has updated correctly.
	}

	if (m_cfg->has_eh_edge()) {
		delete m_tg;
		m_tg = NULL;
	}
	IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));
	END_TIMER_AFTER(get_opt_name());
	return change;
}
//END IR_GCSE
