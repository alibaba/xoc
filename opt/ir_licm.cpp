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
#include "ir_licm.h"

//
//START IR_LICM
//

/* Scan operand to find invariant candidate.
'invariant_stmt': Record if the result of stmt is invariant.
'is_legal': set to true if loop is legal to perform invariant motion.
	otherwise set to false to prohibit code motion.
Return true if find loop invariant expression. */
bool IR_LICM::scan_opnd(IN LI<IR_BB> * li,
						OUT TTAB<IR*> & invariant_exp,
						TTAB<IR*> & invariant_stmt,
						bool * is_legal, bool first_scan)
{
	bool change = false;
	IR_BB * head = LI_loop_head(li);
	UINT headid = IR_BB_id(head);
	for (INT i = LI_bb_set(li)->get_first();
		 i != -1; i = LI_bb_set(li)->get_next(i)) {
		if ((UINT)i == headid) { continue; }

		if (!m_cfg->is_dom(headid, i)) {
			//The candidate BB must dominate all other loop body BBs.
			continue;
		}

		IR_BB * bb = m_ru->get_bb(i);
		IS_TRUE0(bb && m_cfg->get_vertex(i));
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			if (!ir->is_contain_mem_ref()) { continue; }
			if ((ir->is_call() && !ir->is_readonly_call()) ||
				IR_type(ir) == IR_REGION ||
				IR_type(ir) == IR_PHI) {
				//TODO: support call.
				*is_legal = false;
				return false;
			}
			if (first_scan) { update_md2num(ir); }

			//Check if rhs is loop invariant.
			bool is_cand = true;
			m_iriter.clean();
			for (IR const* x = ir_iter_rhs_init_c(ir, m_iriter);
				 x != NULL; x = ir_iter_rhs_next_c(m_iriter)) {
				if (!x->is_memory_opnd() ||
					x->is_readonly_exp() ||
					invariant_exp.find(const_cast<IR*>(x))) {
					continue;
				}

				DU_SET const* defset = m_du->get_du_c(x);
				if (defset == NULL) { continue; }

				DU_ITER di;
				for (INT i = defset->get_first(&di);
					 i >= 0; i = defset->get_next(i, &di)) {
					IR const* d = m_ru->get_ir(i);
					IS_TRUE0(d->get_bb() && d->is_stmt());
					if (!invariant_stmt.find(const_cast<IR*>(d)) &&
						li->inside_loop(IR_BB_id(d->get_bb()))) {
						is_cand = false;
						break;
					}
				}
				if (!is_cand) { break; }
			}

			if (!is_cand) { continue; }

			change |= mark_exp_and_stmt(ir, invariant_exp);
		}
	}
	return change;
}


//Record rhs of ir to be invariant, and record the stmt in
//work list to next round analysis.
//Return true if we find new invariant expression.
bool IR_LICM::mark_exp_and_stmt(IR * ir, TTAB<IR*> & invariant_exp)
{
	bool change = false;
	IR * e;
	switch (IR_type(ir)) {
	case IR_ST:
		e = ST_rhs(ir);
		if (!e->is_const_exp() && !e->is_pr()) {
			//e.g: ST(P1, P2), do not move P2 out of loop.
			if (!invariant_exp.find(e)) {
				invariant_exp.append(e);
				change = true;
			}
		}

		if (!VAR_is_volatile(ST_idinfo(ir))) {
			IS_TRUE0(!m_analysable_stmt_list.find(ir));
			m_analysable_stmt_list.append_tail(ir);
		}
		break;
	case IR_STPR:
		e = STPR_rhs(ir);
		if (!e->is_const_exp() && !e->is_pr()) {
			//e.g: ST(P1, P2), do not move P2 out of loop.
			if (!invariant_exp.find(e)) {
				invariant_exp.append(e);
				change = true;
			}
		}
		IS_TRUE0(!m_analysable_stmt_list.find(ir));
		m_analysable_stmt_list.append_tail(ir);
		break;
	case IR_IST:
		e = IST_rhs(ir);
		if (!e->is_const_exp() && !e->is_pr()) {
			if (!invariant_exp.find(e)) {
				invariant_exp.append(e);
				change = true;
			}
		}

		e = IST_base(ir);
		if (!e->is_pr() && IR_type(e) != IR_ARRAY) {
			//e.g: IST(a+b, P2), regard a+b as cand.
			if (!invariant_exp.find(e)) {
				invariant_exp.append(e);
				change = true;
			}
		}

		IS_TRUE0(!m_analysable_stmt_list.find(ir));
		m_analysable_stmt_list.append_tail(ir);
		break;
	case IR_CALL:
	case IR_ICALL:
		{
			//Hoisting CALL out of loop may be unsafe if the loop
			//will never execute.
			IR * param = CALL_param_list(ir);
			while (param != NULL) {
				if (!param->is_const_exp() && !param->is_pr()) {
					if (!invariant_exp.find(param)) {
						invariant_exp.append(param);
						change = true;
					}
				}
				param = IR_next(param);
			}

			//The result of call may not be loop invariant.
			if (ir->is_readonly_call()) {
				IS_TRUE0(!m_analysable_stmt_list.find(ir));
				m_analysable_stmt_list.append_tail(ir);
			}
		}
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		e = BR_det(ir);
		if (!BIN_opnd0(e)->is_leaf() ||
			!BIN_opnd1(e)->is_leaf()) {
			if (!invariant_exp.find(e)) {
				invariant_exp.append(e);
				change = true;
			}
		}
		break;
	case IR_SWITCH:
		e = SWITCH_vexp(ir);
		if (!e->is_const_exp() && !e->is_pr()) {
			//e.g: SWITCH(P2), do not move P2 out of loop.
			if (!invariant_exp.find(e)) {
				invariant_exp.append(e);
				change = true;
			}
		}
		break;
	default:;
	}
	return change;
}


//Return true if md is modified in loop only once.
bool IR_LICM::unique_def(MD const* md)
{
	UINT * n = m_md2num.get(md);
	IS_TRUE0(n);

	if (*n > 1) { return false; }

	MD_TAB * mdt = m_md_sys->get_md_tab(MD_base(md));
	if (mdt == NULL) { return true; }

	MD * x = mdt->get_effect_md();
	if (x != NULL && x != md && x->is_overlap(md)) {
		UINT * n = m_md2num.get(x);
		if (*n > 1) { return false; }
	}

	OFST_TAB * ofstab = mdt->get_ofst_tab();
	if (ofstab == NULL) { return true; }

	if (ofstab->get_elem_count() == 0) { return true; }

	m_mditer.clean();
	for (MD * x = ofstab->get_first(m_mditer, NULL);
		 x != NULL; x = ofstab->get_next(m_mditer, NULL)) {
		if (x != md && x->is_overlap(md)) {
			UINT * n = m_md2num.get(x);
			if (n != NULL && *n > 1) { return false; }
		}
	}
	return true;
}


//Propagate invariant property to result.
//This operation will generate more invariant.
//'invariant_stmt': Record if the result of stmt is invariant.
bool IR_LICM::scan_result(OUT TTAB<IR*> & invariant_stmt)
{
	bool change = false;
	for (IR * stmt = m_analysable_stmt_list.remove_head(); stmt != NULL;
		 stmt = m_analysable_stmt_list.remove_head()) {
		switch (IR_type(stmt)) {
		case IR_ST:
		case IR_STPR:
			{
				MD const* must = stmt->get_ref_md();
				IS_TRUE0(must);
				if (unique_def(must) &&
					!invariant_stmt.find(stmt)) {
					invariant_stmt.append(stmt);
					change = true;
				}
			}
			break;
		case IR_IST:
			{
				MD const* must = stmt->get_ref_md();
				if (must != NULL && must->is_effect() &&
					unique_def(must) &&
					!invariant_stmt.find(stmt)) {
					invariant_stmt.append(stmt);
					change = true;
				}
			}
			break;
		case IR_CALL:
		case IR_ICALL:
			//TODO: hoist readonly call.
			break;
		default: IS_TRUE0(0); //TODO: support more operations.
		}
	}
	return change;
}


void IR_LICM::update_md2num(IR * ir)
{
	switch (IR_type(ir)) {
	case IR_ST:
	case IR_STPR:
		{
			MD const* md = ir->get_ref_md();
			IS_TRUE0(md);
			UINT * n = m_md2num.get(const_cast<MD*>(md));
			if (n == NULL) {
				n = (UINT*)xmalloc(sizeof(UINT));
				m_md2num.set(md, n);
			}
			(*n)++;
		}
		break;
	case IR_IST:
		{
			MD_SET const* mds = NULL;
			MD const* md = ir->get_ref_md();
			if (md != NULL) {
				UINT * n = m_md2num.get(const_cast<MD*>(md));
				if (n == NULL) {
					n = (UINT*)xmalloc(sizeof(UINT));
					m_md2num.set(md, n);
				}
				(*n)++;
			} else if ((mds = ir->get_ref_mds()) != NULL) {
				for (INT i = mds->get_first(); i >= 0; i = mds->get_next(i)) {
					MD * md = m_md_sys->get_md(i);
					UINT * n = m_md2num.get(md);
					if (n == NULL) {
						n = (UINT*)xmalloc(sizeof(UINT));
						m_md2num.set(md, n);
					}
					(*n)++;
				}
			}
		}
		break;
	case IR_CALL:
	case IR_ICALL:
		IS_TRUE0(ir->is_readonly_call());
		//TODO: support not readonly call.
		break;
	case IR_GOTO:
	case IR_IGOTO:
	case IR_SWITCH:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_RETURN:
		break;
	default: IS_TRUE0(0); //Unsupport.
	}
}


//Given loop info li, dump the invariant stmt and invariant expression.
void IR_LICM::dump(LI<IR_BB> const* li,
					TTAB<IR*> const& invariant_stmt,
					TTAB<IR*> const& invariant_exp)
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n==---- DUMP LICM Analysis Result ----==\n");
	if (invariant_exp.get_elem_count() > 0) {
		TAB_ITER<IR*> ti;
		fprintf(g_tfile, "-- Invariant Expression (num=%d) -- :",
				invariant_exp.get_elem_count());
		g_indent = 3;
		for (IR * c = invariant_exp.get_first(ti);
			 c != NULL; c = invariant_exp.get_next(ti)) {
			 dump_ir(c, m_dm);
		}

	}
	fprintf(g_tfile, "\n");
	if (invariant_stmt.get_elem_count() > 0) {
		TAB_ITER<IR*> ti;
		fprintf(g_tfile, "-- Invariant Statement (num=%d) -- :",
				invariant_stmt.get_elem_count());
		g_indent = 3;
		for (IR * c = invariant_stmt.get_first(ti);
			 c != NULL; c = invariant_stmt.get_next(ti)) {
			 dump_ir(c, m_dm);
		}
	}
}


//Analysis loop invariant expression and stmt.
//Return true if find them, otherwise return false.
bool IR_LICM::analysis(IN LI<IR_BB> * li,
					OUT TTAB<IR*> & invariant_stmt,
					OUT TTAB<IR*> & invariant_exp)
{
	invariant_stmt.clean();
	invariant_exp.clean();
	m_analysable_stmt_list.clean();
	m_md2num.clean();

	//Record if the result of stmt is invariant.
	bool change = true;
	bool find = false;
	bool first_scan = true;
	while (change) {
		bool is_legal = true;
		change = scan_opnd(li, invariant_exp, invariant_stmt,
						   &is_legal, first_scan);
		if (!is_legal) {
			invariant_exp.clean();
			return false;
		}

		if (change) {
			find = true;
			if (m_analysable_stmt_list.get_elem_count() > 0) {
				change |= scan_result(invariant_stmt);
			}
			IS_TRUE0(m_analysable_stmt_list.get_elem_count() == 0);
		} else {
			//Before next round analysis, we must make sure all
			//stmts in this list is invariant or not.
			m_analysable_stmt_list.clean();
		}
		first_scan = false;
	}
	return find;
}


//Return true if ir dominate all USE which in loop.
bool IR_LICM::is_dom_all_use_in_loop(IR const* ir, LI<IR_BB> * li)
{
	IS_TRUE0(ir->is_stmt());
	DU_SET const* useset = ir->get_duset_c();
	if (useset == NULL) { return true; }

	IR_BB * irbb = ir->get_bb();
	UINT irbbid = IR_BB_id(irbb);
	DU_ITER di;
	for (INT i = useset->get_first(&di);
		 i >= 0; i = useset->get_next(i, &di)) {
		IR const* u = m_ru->get_ir(i);
		IS_TRUE0(u->is_exp() && u->get_stmt());

		IR * ustmt = u->get_stmt();
		UINT ubbid = IR_BB_id(ustmt->get_bb());
		if (!li->inside_loop(ubbid)) { continue; }

		if ((irbbid != ubbid && m_cfg->is_dom(irbbid, ubbid)) ||
			(irbbid == ubbid && irbb->is_dom(ir, ustmt, true))) {
			continue;
		}
		return false;
	}
	return true;
}


bool IR_LICM::stmt_can_be_hoisted(IR * stmt, TTAB<IR*> & invariant_stmt,
								  LI<IR_BB> * li, IR_BB * backedge_bb)
{
	//Loop has multiple exits.
	if (backedge_bb == NULL) { return false; }

	//Stmt is at the dominate path in loop.
	if (stmt->get_bb() != backedge_bb &&
		!m_cfg->is_dom(IR_BB_id(stmt->get_bb()), IR_BB_id(backedge_bb))) {
		return false;
	}
	return true;
}


//Hoist candidate IRs to preheader BB.
bool IR_LICM::hoist_cand(TTAB<IR*> & invariant_exp,
						 TTAB<IR*> & invariant_stmt,
						 IN IR_BB * prehead, IN LI<IR_BB> * li)
{
	bool du_set_info_changed = false;
	SVECTOR<IR*> removed;
	TAB_ITER<IR*> ti;
	IR_BB * backedge_bb = ::find_single_backedge_start_bb(li, m_cfg);
	while (invariant_exp.get_elem_count() > 0) {
		UINT removednum = 0;
		for (IR * c = invariant_exp.get_first(ti);
			 c != NULL; c = invariant_exp.get_next(ti)) {
			IS_TRUE0(c->is_exp());
			if (!is_worth_hoist(c)) { continue; }
			IR * stmt = c->get_stmt();

			//Check that each definitions of candidate have been
			//already hoisted out of the loop.
			bool do_hoist_now = true;
			m_iriter.clean();
			for (IR const* x = ir_iter_init_c(c, m_iriter);
				 x != NULL; x = ir_iter_next_c(m_iriter)) {
			 	DU_SET const* defset = m_du->get_du_c(x);
				if (defset == NULL) { continue; }

				DU_ITER di;
				for (INT i = defset->get_first(&di);
					 i >= 0; i = defset->get_next(i, &di)) {
					IR const* d = m_ru->get_ir(i);
					IS_TRUE0(d->is_stmt());

					IR_BB * dbb = d->get_bb();
					IS_TRUE0(dbb);
					if (li->inside_loop(IR_BB_id(dbb))) {
						do_hoist_now = false;
						break;
					}
				}
			}

			if (!do_hoist_now) { continue; }

			removed.set(removednum, c);
			removednum++;

			if ((stmt->is_stid() || stmt->is_stpr()) &&
				c == stmt->get_rhs() &&
				invariant_stmt.find(stmt) &&
				backedge_bb != NULL && //loop only have one exit.
				is_dom_all_use_in_loop(stmt, li)) {
				//stmt_can_be_hoisted(stmt, invariant_stmt, li, backedge_bb)

				//cand is store value and the result memory object is ID|PR.

				/* Fix bug: If we hoist whole stmt out of loop,
				we should make sure the stmt will be execute at least once
				or never. TRUEBR should be generate and encapsulate
				the hoisted stmt to ensure that.
					while (a > 0) {
						a = 10;
						foo();
					}
					=>
					if (a > 0)  {
						a = 10;
					}
					while (a > 0) {
						foo();
					}
				*/

				IS_TRUE0(stmt->get_bb());
				IR_BB_ir_list(stmt->get_bb()).remove(stmt);
				IR_BB_ir_list(prehead).append_tail_ex(stmt);

				/* The code motion do not modify DU chain info of 'exp' and
				'stmt'. So it is no need to revise DU chain.
				But the live-expr, reach-def, avail-reach-def set
				info of each BB changed. */
			} else {
				/* e.g: Given S1: n = cand_exp;
				Generate new stmt S2, and modify S1 to S3:
					p1 = cand_exp; //S2
					n = p1; //S3
				move S2 into prehead BB. */
				IR * t = m_ru->build_pr(IR_dt(c));
				bool f = stmt->replace_kid(c, t, false);
				IS_TRUE0(f);

				IR * st = m_ru->build_store_pr(PR_no(t), IR_dt(t), c);

				//Generate DU chain bewteen 'st' and 't'.
				m_du->build_du_chain(st, t);
				IR_BB_ir_list(prehead).append_tail(st);

				//Revise MD info.
				MD const* tmd = m_ru->gen_md_for_pr(t);
				t->set_ref_md(tmd, m_ru);
				st->set_ref_md(tmd, m_ru);
			}
			du_set_info_changed = true;
		}

		IS_TRUE(removednum > 0, ("not find any hoistable exp?"));

		for (UINT i = 0; i < removednum; i++) {
			IR * c = removed.get(i);
			IS_TRUE0(c);
			invariant_exp.remove(c);
		}
	} //end while
	return du_set_info_changed;
}


//Return true if do code motion successfully.
//This funtion maintain LOOP INFO.
bool IR_LICM::do_loop_tree(LI<IR_BB> * li,
						OUT bool & du_set_info_changed,
						OUT bool & insert_bb,
						TTAB<IR*> & invariant_stmt,
						TTAB<IR*> & invariant_exp)
{
	if (li == NULL) { return false; }
	bool doit = false;
	LI<IR_BB> * tli = li;
	while (tli != NULL) {
		do_loop_tree(LI_inner_list(tli), du_set_info_changed,
					 insert_bb, invariant_stmt, invariant_exp);
		analysis(tli, invariant_stmt, invariant_exp);
		if (invariant_exp.get_elem_count() == 0) {
			tli = LI_next(tli);
			continue;
		}

		doit = true;
		bool flag;
		IR_BB * prehead = ::find_and_insert_prehead(tli, m_ru, flag, false);
		IS_TRUE0(prehead);
		insert_bb |= flag;
		if (flag && LI_outer(tli) != NULL) {
			LI_bb_set(LI_outer(tli))->bunion(IR_BB_id(prehead));
		}

		du_set_info_changed |=
			hoist_cand(invariant_exp, invariant_stmt, prehead, li);
		tli = LI_next(tli);
	}
	return doit;
}


bool IR_LICM::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	m_ru->check_valid_and_recompute(&oc, OPT_DOM, OPT_DU_REF,
									OPT_DU_CHAIN, OPT_UNDEF);

	bool du_set_info_changed = false;
	bool insert_bb = false;
	TTAB<IR*> invariant_stmt;
	TTAB<IR*> invariant_exp;

	bool doit = do_loop_tree(m_cfg->get_loop_info(),
							 du_set_info_changed,
							 insert_bb,
							 invariant_stmt,
							 invariant_exp);
	if (doit) {
		OPTC_is_expr_tab_valid(oc) = false;

		//DU chain and du ref is maintained.
		IS_TRUE0(m_du->verify_du_ref());
		IS_TRUE0(m_du->verify_du_chain());

		if (du_set_info_changed) {
			OPTC_is_live_expr_valid(oc) = false;
			OPTC_is_avail_reach_def_valid(oc) = false;
			OPTC_is_reach_def_valid(oc) = false;
		}

		if (insert_bb) {
			//Loop info is maintained, but dom pdom and cdg is changed.

			OPTC_is_dom_valid(oc) = false;
			OPTC_is_pdom_valid(oc) = false;
			OPTC_is_cdg_valid(oc) = false;
		}
	}
	END_TIMER_AFTER(get_opt_name());
	return doit;
}
//END IR_LICM
