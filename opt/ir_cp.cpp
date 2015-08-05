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
#include "ir_cp.h"

//
//START IR_CP
//
//Return true if ir's type is consistent with 'cand_expr'.
bool IR_CP::check_type_consistency(IR const* ir, IR const* cand_expr) const
{
	UINT t1 = IR_dt(ir);
	UINT t2 = IR_dt(cand_expr);
	IS_TRUE0(t1 != VOID_TY && t2 != VOID_TY);
	if (t1 == t2) { return true; }

	if (m_dm->is_scalar(t1) && m_dm->is_scalar(t2)) {
		if (m_dm->is_signed(t1) ^ m_dm->is_signed(t2)) {
			//Sign must be consistent.
			return false;
		}
		if (m_dm->get_dtd_bytesize(t1) < m_dm->get_dtd_bytesize(t2)) {
			//ir size must be equal or great than cand.
			return false;
		}
		return true;
	}

	return false;
}


/* Check and replace 'ir' with 'cand_expr' if they are
equal, and update DU info. If 'cand_expr' is NOT leaf,
that will create redundant computation, and
depends on later Redundancy Elimination to reverse back.

'cand_expr': substitute cand_expr for ir.
	e.g: cand_expr is *p, cand_expr_md is MD3
		*p(MD3) = 10 //p point to MD3
		...
		g = *q(MD3) //q point to MD3

NOTE: Do NOT handle stmt. */
void IR_CP::replace_expr(IR * exp, IR const* cand_expr, IN OUT CP_CTX & ctx)
{
	IS_TRUE0(exp && exp->is_exp() && cand_expr);
	IS_TRUE0(exp->get_exact_ref());

	if (!check_type_consistency(exp, cand_expr)) {
		return;
	}

	IR * parent = IR_parent(exp);
	if (IR_type(parent) == IR_ILD) {
		CPC_need_recomp_aa(ctx) = true;
	} else if (IR_type(parent) == IR_IST && exp == IST_base(parent)) {
		if (IR_type(cand_expr) != IR_LD &&
			!cand_expr->is_pr() &&
			IR_type(cand_expr) != IR_LDA) {
			return;
		}
		CPC_need_recomp_aa(ctx) = true;
	}

	IR * newir = m_ru->dup_irs(cand_expr);
	m_du->copy_ir_tree_du_info(newir, cand_expr, true);

	IS_TRUE0(cand_expr->get_stmt());
	m_du->remove_use_out_from_defset(exp);
	CPC_change(ctx) = true;

	IS_TRUE0(exp->get_stmt());
	bool doit = parent->replace_kid(exp, newir, false);
	IS_TRUE0(doit);
	m_ru->free_irs(exp);
}


bool IR_CP::is_copy(IR * ir) const
{
	switch (IR_type(ir)) {
	case IR_ST:
	case IR_STPR:
	case IR_IST:
		return can_be_candidate(ir->get_rhs());
	default: break;
	}
	return false;
}


/* Return true if 'occ' does not be modified till meeting 'use_ir'.
e.g:
	xx = occ  //def_ir
	..
	..
	yy = xx  //use_ir

'def_ir': ir stmt.
'occ': opnd of 'def_ir'
'use_ir': stmt in use-list of 'def_ir'. */
bool IR_CP::is_available(IR * def_ir, IR * occ, IR * use_ir)
{
	if (def_ir == use_ir) {	return false; }
	if (IR_is_const(occ)) {
		return true;
	}

	/* Need check overlapped MD_SET.
	e.g: Suppose occ is '*p + *q', p->a, q->b.
	occ can NOT reach 'def_ir' if one of p, q, a, b
	modified during the path. */

	IR_BB * defbb = def_ir->get_bb();
	IR_BB * usebb = use_ir->get_bb();
	if (defbb == usebb) {
		//Both def_ir and use_ir are in same BB.
		C<IR*> * ir_holder = NULL;
		bool f = IR_BB_ir_list(defbb).find(def_ir, &ir_holder);
		IS_TRUE0(f);
		IR * ir;
		for (ir = IR_BB_ir_list(defbb).get_next(&ir_holder);
			 ir != NULL && ir != use_ir;
			 ir = IR_BB_ir_list(defbb).get_next(&ir_holder)) {
			if (m_du->is_may_def(ir, occ, true)) {
				return false;
			}
		}
		if (ir == NULL) {
			;//use_ir appears prior to def_ir. Do more check via live_in_expr.
		} else {
			IS_TRUE(ir == use_ir, ("def_ir should be in same bb to use_ir"));
			return true;
		}
	}

	IS_TRUE0(use_ir->is_stmt());
	DBITSETC const* availin_expr = m_du->get_availin_expr(IR_BB_id(usebb));
	IS_TRUE0(availin_expr);

	if (availin_expr->is_contain(IR_id(occ))) {
		IR * u;
		for (u = IR_BB_first_ir(usebb); u != use_ir && u != NULL;
			 u = IR_BB_next_ir(usebb)) {
			//Check if 'u' override occ's value.
			if (m_du->is_may_def(u, occ, true)) {
				return false;
			}
		}
		IS_TRUE(u != NULL && u == use_ir,
				("Not find use_ir in bb, may be it has "
				 "been removed by other optimization"));
		return true;
	}
	return false;
}


//CVT with simply cvt-exp is copy-propagate candidate.
bool IR_CP::is_simp_cvt(IR const* ir) const
{
	if (IR_type(ir) != IR_CVT) return false;
	while (true) {
		if (IR_type(ir) == IR_CVT) {
			ir = CVT_exp(ir);
		} else if (IR_type(ir) == IR_LD || IR_is_const(ir) || ir->is_pr()) {
			return true;
		} else {
			return false;
		}
	}
	return false;
}


//'usevec': for local used.
bool IR_CP::do_prop(IN IR_BB * bb, SVECTOR<IR*> & usevec)
{
	bool change = false;
	C<IR*> * cur_iter, * next_iter;

	for (IR_BB_ir_list(bb).get_head(&cur_iter),
		 next_iter = cur_iter; cur_iter != NULL; cur_iter = next_iter) {

		IR * def_stmt = C_val(cur_iter);

		IR_BB_ir_list(bb).get_next(&next_iter);

		DU_SET const* useset = m_du->get_du_c(def_stmt);
		if (useset == NULL || !is_copy(def_stmt)) { continue; }

		if (def_stmt->get_exact_ref() == NULL) { continue; }

		if (useset->get_elem_count() == 0) { continue; }

		IR * rhs = def_stmt->get_rhs();

		//Record use_stmt if it is not in use-list
		//any more after copy-propagation.
		DU_ITER di;
		UINT num = 0;
		for	(INT u = useset->get_first(&di);
			 u >= 0; u = useset->get_next(u, &di)) {
			IR * use = m_ru->get_ir(u);
			usevec.set(num, use);
			num++;
		}

		for (UINT i = 0; i < num; i++) {
			IR * use = usevec.get(i);
			IS_TRUE0(use->is_exp());
			IR * use_stmt = use->get_stmt();
			IS_TRUE0(use_stmt->is_stmt());

			IS_TRUE0(use_stmt->get_bb() != NULL);
			IR_BB * use_bb = use_stmt->get_bb();
			if (!(bb == use_bb && bb->is_dom(def_stmt, use_stmt, true)) &&
				!m_cfg->is_dom(IR_BB_id(bb), IR_BB_id(use_bb))) {
				/* 'def_stmt' must dominate 'use_stmt'.
				e.g:
					if (...) {
						g = 10; //S1
					}
					... = g; //S2
				g can not be propagted since S1 is not dominate S2. */
				continue;
			}

			if (!is_available(def_stmt, rhs, use_stmt)) {
				/* The value that will be propagated can
				not be killed during 'ir' and 'use_stmt'.
				e.g:
					g = a; //S1
					if (...) {
						a = ...; //S3
					}
					... = g; //S2
				g can not be propagted since a is killed by S3. */
				continue;
			}

			if (!m_du->is_exact_unique_def(def_stmt, use)) {
				/* Only single definition is allowed.
				e.g:
					g = 20; //S3
					if (...) {
						g = 10; //S1
					}
					... = g; //S2
				g can not be propagted since there are
				more than one definitions are able to get to S2. */
				continue;
			}

			if (!can_be_candidate(rhs)) {
				continue;
			}

			CP_CTX lchange;
			IR * old_use_stmt = use_stmt;
			replace_expr(use, rhs, lchange);

			IS_TRUE(use_stmt && use_stmt->is_stmt(),
					("ensure use_stmt still legal"));
			change |= CPC_change(lchange);

			//Indicate whether use_stmt is the next stmt of def_stmt.
			bool is_next = false;
			if (next_iter != NULL && use_stmt == C_val(next_iter)) {
				is_next = true;
			}

			RF_CTX rf;
			use_stmt = m_ru->refine_ir(use_stmt, change, rf);
			if (use_stmt == NULL && is_next) {
				//use_stmt has been optimized and removed by refine_ir().
				next_iter = cur_iter;
				IR_BB_ir_list(bb).get_next(&next_iter);
			}

			if (use_stmt != NULL && use_stmt != old_use_stmt) {
				//use_stmt has been removed and new stmt generated.
				IS_TRUE(IR_type(old_use_stmt) == IR_UNDEF,
						("the old one should be freed"));

				C<IR*> * irct = NULL;
				IR_BB_ir_list(use_bb).find(old_use_stmt, &irct);
				IS_TRUE0(irct);
				IR_BB_ir_list(use_bb).insert_before(use_stmt, irct);
				IR_BB_ir_list(use_bb).remove(irct);
			}
		} //end for each USE
	} //end for IR
	return change;
}


bool IR_CP::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	IS_TRUE0(OPTC_is_cfg_valid(oc));
	m_ru->check_valid_and_recompute(&oc, OPT_DOM, OPT_DU_REF, OPT_LIVE_EXPR,
									OPT_DU_CHAIN, OPT_UNDEF);

	bool change = false;
	IR_BB * entry = m_ru->get_cfg()->get_entry_list()->get_head();
	IS_TRUE(entry != NULL, ("Not unique entry, invalid REGION"));
	GRAPH domtree;
	m_cfg->get_dom_tree(domtree);
	VERTEX * v = domtree.get_vertex(IR_BB_id(entry));
	LIST<VERTEX*> lst;
	VERTEX * root = domtree.get_vertex(IR_BB_id(entry));
	m_cfg->sort_dom_tree_in_preorder(domtree, root, lst);
	SVECTOR<IR*> usevec;

	for (VERTEX * v = lst.get_head(); v != NULL; v = lst.get_next()) {
		IR_BB * bb = m_ru->get_bb(VERTEX_id(v));
		IS_TRUE0(bb);
		change |= do_prop(bb, usevec);
	}
	if (change) {
		OPTC_is_expr_tab_valid(oc) = false;
		OPTC_is_aa_valid(oc) = false;
		OPTC_is_du_chain_valid(oc) = true; //already update.
		OPTC_is_ref_valid(oc) = true; //already update.
		IS_TRUE0(m_du->verify_du_ref() && m_du->verify_du_chain());
	}
	END_TIMER_AFTER(get_opt_name());
	return change;
}
//END IR_CP
