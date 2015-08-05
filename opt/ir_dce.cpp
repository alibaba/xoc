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
#include "ir_dce.h"
#include "prdf.h"
#include "prssainfo.h"
#include "ir_ssa.h"

//
//START IR_DCE
//
void IR_DCE::dump(IN EFFECT_STMT const& is_stmt_effect,
				  IN BITSET const& is_bb_effect,
				  IN SVECTOR<SVECTOR<IR*>*> & all_ir)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP IR_DCE ----==\n");
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		fprintf(g_tfile, "\n------- BB%d", IR_BB_id(bb));
		if (!is_bb_effect.is_contain(IR_BB_id(bb))) {
			fprintf(g_tfile, "\t\tineffect BB!");
		}
		SVECTOR<IR*> * ir_vec = all_ir.get(IR_BB_id(bb));
		if (ir_vec == NULL) {
			continue;
		}
		for (INT j = 0; j <= ir_vec->get_last_idx(); j++) {
			IR * ir = ir_vec->get(j);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "\n");
			dump_ir(ir, m_dm);
			if (!is_stmt_effect.is_contain(IR_id(ir))) {
				fprintf(g_tfile, "\t\tremove!");
			}
		}
	}

	fprintf(g_tfile, "\n\n============= REMOVED IR ==================\n");
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		fprintf(g_tfile, "\n------- BB%d", IR_BB_id(bb));
		if (!is_bb_effect.is_contain(IR_BB_id(bb))) {
			fprintf(g_tfile, "\t\tineffect BB!");
		}
		SVECTOR<IR*> * ir_vec = all_ir.get(IR_BB_id(bb));
		if (ir_vec == NULL) {
			continue;
		}
		for (INT j = 0; j <= ir_vec->get_last_idx(); j++) {
			IR * ir = ir_vec->get(j);
			IS_TRUE0(ir != NULL);
			if (!is_stmt_effect.is_contain(IR_id(ir))) {
				fprintf(g_tfile, "\n");
				dump_ir(ir, m_dm);
				fprintf(g_tfile, "\t\tremove!");
			}
		}
	}
	fflush(g_tfile);
}


//Return true if ir is effect.
bool IR_DCE::check_stmt(IR const* ir)
{
	if (IR_may_throw(ir) || IR_has_sideeffect(ir)) { return true; }

	MD const* mustdef = ir->get_ref_md();
	if (mustdef != NULL) {
		if (is_effect_write(MD_base(mustdef))) {
			return true;
		}
	} else {
		MD_SET const* maydefs = ir->get_ref_mds();
		if (maydefs != NULL) {
			for (INT i = maydefs->get_first();
				 i >= 0; i = maydefs->get_next(i)) {
				MD * md = m_md_sys->get_md(i);
				IS_TRUE0(md);
				if (is_effect_write(MD_base(md))) {
					return true;
				}
			}
		}
	}

	m_citer.clean();
	for (IR const* x = ir_iter_rhs_init_c(ir, m_citer);
		 x != NULL; x = ir_iter_rhs_next_c(m_citer)) {
		if (!x->is_memory_ref()) { continue; }

		/* Check if using volatile variable.
		e.g: volatile int g = 0;
			while(g); //The stmt has effect.
		*/
		MD const* md = x->get_ref_md();
		if (md != NULL) {
			if (is_effect_read(MD_base(md))) {
				return true;
			}
		} else {
			MD_SET const* mds = x->get_ref_mds();
			if (mds != NULL) {
				for (INT i = mds->get_first();
					 i != -1; i = mds->get_next(i)) {
					MD * md = m_md_sys->get_md(i);
					IS_TRUE0(md != NULL);
					if (is_effect_read(MD_base(md))) {
						return true;
					}
				}
			}
		}
	}
	return false;
}


//Return true if ir is effect.
bool IR_DCE::check_call(IR const* ir)
{
	IS_TRUE0(ir->is_call());
	return !ir->is_readonly_call() || IR_has_sideeffect(ir);
}


void IR_DCE::mark_effect_ir(IN OUT EFFECT_STMT & is_stmt_effect,
							IN OUT BITSET & is_bb_effect,
							IN OUT LIST<IR const*> & work_list)
{
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	C<IR_BB*> * ct;
	for (IR_BB * bb = bbl->get_head(&ct);
		 bb != NULL; bb = bbl->get_next(&ct)) {
		for (IR const* ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			switch (IR_type(ir)) {
			case IR_RETURN:
				/* Do NOT set exit-bb to be effect.
				That will generate redundant control-flow dependence.

				CASE:
					IF (...)
						...
					ENDIF
					RETURN //EXIT BB

				IF clause stmt is redundant code. */
				is_bb_effect.bunion(IR_BB_id(bb));
				is_stmt_effect.bunion(IR_id(ir));
				work_list.append_tail(ir);
				break;
			case IR_CALL:
			case IR_ICALL:
				if (check_call(ir)) {
					is_stmt_effect.bunion(IR_id(ir));
					is_bb_effect.bunion(IR_BB_id(bb));
					work_list.append_tail(ir);
				}
				break;
			case IR_TRUEBR:
			case IR_FALSEBR:
			case IR_GOTO:
			case IR_IGOTO:
				if (!m_is_elim_cfs) {
					is_stmt_effect.bunion(IR_id(ir));
					is_bb_effect.bunion(IR_BB_id(bb));
					work_list.append_tail(ir);
				}
				break;
			default:
				{
					if (check_stmt(ir)) {
						is_stmt_effect.bunion(IR_id(ir));
						is_bb_effect.bunion(IR_BB_id(bb));
						work_list.append_tail(ir);
					}
				}
			} //end switch IR_type
		} //end for each IR
	} //end for each IR_BB
}


bool IR_DCE::find_effect_kid(IN IR_BB * bb, IN IR * ir,
							 IN EFFECT_STMT & is_stmt_effect)
{
	IS_TRUE0(m_cfg && m_cdg);
	IS_TRUE0(ir->get_bb() == bb);
	if (ir->is_cond_br() || ir->is_multicond_br()) {
		EDGE_C const* ec = VERTEX_out_list(m_cdg->get_vertex(IR_BB_id(bb)));
		while (ec != NULL) {
			IR_BB * succ = m_cfg->get_bb(VERTEX_id(EDGE_to(EC_edge(ec))));
			IS_TRUE0(succ != NULL);
			for (IR * r = IR_BB_ir_list(succ).get_head();
				 r != NULL; r = IR_BB_ir_list(succ).get_next()) {
				if (is_stmt_effect.is_contain(IR_id(r))) {
					return true;
				}
			}
			ec = EC_next(ec);
		}
	} else if (ir->is_uncond_br()) {
		EDGE_C const* ecp = VERTEX_in_list(m_cdg->get_vertex(IR_BB_id(bb)));
		while (ecp != NULL) {
			INT cd_pred = VERTEX_id(EDGE_from(EC_edge(ecp)));
			EDGE_C const* ecs = VERTEX_out_list(m_cdg->get_vertex(cd_pred));
			while (ecs != NULL) {
				INT cd_succ = VERTEX_id(EDGE_to(EC_edge(ecs)));
				IR_BB * succ = m_cfg->get_bb(cd_succ);
				IS_TRUE0(succ != NULL);
				for (IR * r = IR_BB_ir_list(succ).get_head();
					 r != NULL; r = IR_BB_ir_list(succ).get_next()) {
					if (is_stmt_effect.is_contain(IR_id(r))) {
						return true;
					}
				}
				ecs = EC_next(ecs);
			}
			ecp = EC_next(ecp);
		}
	} else {
		IS_TRUE0(0);
	}
	return false;
}


bool IR_DCE::preserve_cd(IN OUT BITSET & is_bb_effect,
						 IN OUT EFFECT_STMT & is_stmt_effect,
						 IN OUT LIST<IR const*> & act_ir_lst)
{
	IS_TRUE0(m_cfg && m_cdg);
	bool change = false;
	LIST<IR_BB*> lst_2;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		if (is_bb_effect.is_contain(IR_BB_id(bb))) {
			UINT bbid = IR_BB_id(bb);
			//Set control dep bb to be effective.
			IS_TRUE0(m_cdg->get_vertex(bbid));
			EDGE_C const* ec = VERTEX_in_list(m_cdg->get_vertex(bbid));
			while (ec != NULL) {
				INT cd_pred = VERTEX_id(EDGE_from(EC_edge(ec)));
				if (!is_bb_effect.is_contain(cd_pred)) {
					is_bb_effect.bunion(cd_pred);
					change = true;
				}
				ec = EC_next(ec);
			}

			IS_TRUE0(m_cfg->get_vertex(bbid));
			ec = VERTEX_in_list(m_cfg->get_vertex(bbid));
			if (cnt_list(ec) >= 2) {
				IS_TRUE0(IR_BB_rpo(bb) >= 0);
				UINT bbto = IR_BB_rpo(bb);
				while (ec != NULL) {
					IR_BB * pred =
						m_cfg->get_bb(VERTEX_id(EDGE_from(EC_edge(ec))));
					IS_TRUE0(pred);

					if (IR_BB_rpo(pred) > (INT)bbto &&
						!is_bb_effect.is_contain(IR_BB_id(pred))) {
						is_bb_effect.bunion(IR_BB_id(pred));
						change = true;
					}
					ec = EC_next(ec);
				}
			}

			if (IR_BB_ir_list(bb).get_elem_count() == 0) { continue; }

			IR * ir = IR_BB_last_ir(bb); //last IR of BB.
			IS_TRUE0(ir != NULL);
			if ((ir->is_cond_br() || ir->is_multicond_br()) &&
				!is_stmt_effect.is_contain(IR_id(ir))) {
				//switch might have multiple succ-BB.
				if (find_effect_kid(bb, ir, is_stmt_effect)) {
					is_stmt_effect.bunion(IR_id(ir));
					act_ir_lst.append_tail(ir);
					change = true;
				}
			}
		}

		/* CASE: test_pre1()
			GOTO 0xa4f634 id:23
			    CLABEL (name:L1) 0xa4f5e4 id:22 branch-target
			in BB3
			BB3 is ineffective, but GOTO can not be removed! */
		if (IR_BB_ir_list(bb).get_elem_count() == 0) { continue; }

		IR * ir = IR_BB_last_ir(bb); //last IR of BB.
		IS_TRUE0(ir);

		if (ir->is_uncond_br() && !is_stmt_effect.is_contain(IR_id(ir))) {
			if (find_effect_kid(bb, ir, is_stmt_effect)) {
				is_stmt_effect.bunion(IR_id(ir));
				is_bb_effect.bunion(IR_BB_id(bb));
				act_ir_lst.append_tail(ir);
				change = true;
			}
		}
	} //for each BB
	return change;
}


//Iterative record effect IRs, according to DU chain,
//and preserving the control flow dependence.
void IR_DCE::iter_collect(IN OUT EFFECT_STMT & is_stmt_effect,
						  IN OUT BITSET & is_bb_effect,
						  IN OUT LIST<IR const*> & work_list)
{
	LIST<IR const*> work_list2;
	LIST<IR const*> * pwlst1 = &work_list;
	LIST<IR const*> * pwlst2 = &work_list2;
	bool change = true;
	LIST<IR_BB*> succs;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	while (change) {
		change = false;
		for (IR const* ir = pwlst1->get_head();
			 ir != NULL; ir = pwlst1->get_next()) {
			m_citer.clean();
			for (IR const* x = ir_iter_rhs_init_c(ir, m_citer);
				 x != NULL; x = ir_iter_rhs_next_c(m_citer)) {
				if (!x->is_memory_opnd()) { continue; }

				if (x->is_pr() && is_ssa_available() && PR_ssainfo(x) != NULL) {
					IR const* d = SSA_def(PR_ssainfo(x));
					if (d != NULL) {
						IS_TRUE0(d->is_stmt());
						if (!is_stmt_effect.is_contain(IR_id(d))) {
							change = true;
							pwlst2->append_tail(d);
							is_stmt_effect.bunion(IR_id(d));
							IS_TRUE0(d->get_bb() != NULL);
							is_bb_effect.bunion(IR_BB_id(d->get_bb()));
						}
					}
				} else {
					DU_SET const* defset = m_du->get_du_c(x);
					if (defset == NULL) { continue; }

					DU_ITER di;
					for (INT i = defset->get_first(&di);
						 i >= 0; i = defset->get_next(i, &di)) {
						IR const* d = m_ru->get_ir(i);
						IS_TRUE0(d->is_stmt());
						if (!is_stmt_effect.is_contain(IR_id(d))) {
							change = true;
							pwlst2->append_tail(d);
							is_stmt_effect.bunion(IR_id(d));
							IS_TRUE0(d->get_bb() != NULL);
							is_bb_effect.bunion(IR_BB_id(d->get_bb()));
						}
					}
				}
			}
		}

		//dump_irs((IR_LIST&)*pwlst2);
		if (m_is_elim_cfs) {
			change |= preserve_cd(is_bb_effect, is_stmt_effect, *pwlst2);
		}

		//dump_irs((IR_LIST&)*pwlst2);
		pwlst1->clean();
		LIST<IR const*> * tmp = pwlst1;
		pwlst1 = pwlst2;
		pwlst2 = tmp;
	} //end while
}


//Fix control flow if BB is empty.
//It will be illegal if empty BB has non-taken branch.
void IR_DCE::fix_control_flow(LIST<IR_BB*> & bblst, LIST<C<IR_BB*>*> & ctlst)
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	C<IR_BB*> * ct = ctlst.get_head();
	for (IR_BB * bb = bblst.get_head(); bb != NULL;
		 bb = bblst.get_next(), ct = ctlst.get_next()) {
		IS_TRUE0(ct);
		if (IR_BB_ir_list(bb).get_elem_count() != 0) { continue; }

		EDGE_C * vout = VERTEX_out_list(m_cfg->get_vertex(IR_BB_id(bb)));
		if (vout == NULL || cnt_list(vout) <= 1) { continue; }

		C<IR_BB*> * next_ct = ct;
		bbl->get_next(&next_ct);
		IR_BB * next_bb = NULL;
		if (next_ct != NULL) {
			next_bb = C_val(next_ct);
		}
		while (vout != NULL) {
			EDGE * e = EC_edge(vout);
			if (EDGE_info(e) != NULL && EI_is_eh((EI*)EDGE_info(e))) {
				vout = EC_next(vout);
				continue;
			}
			VERTEX * s = EDGE_to(e);
			if (VERTEX_id(s) == IR_BB_id(bb) ||
				(next_bb != NULL && VERTEX_id(s) == IR_BB_id(next_bb))) {
				vout = EC_next(vout);
				continue;
			}
			if (!m_cdg->is_cd(IR_BB_id(bb), VERTEX_id(s))) {
				//See dce.c:lexrun(), bb5 control bb6, but not control bb8.
				//if bb5 is empty, insert goto to bb8.
				IR_BB * tgt = m_cfg->get_bb(VERTEX_id(s));
				IS_TRUE0(tgt);

				//Find a normal label as target.
				LABEL_INFO * li;
				for (li = IR_BB_lab_list(tgt).get_head();
					 li != NULL; li = IR_BB_lab_list(tgt).get_next()) {
					if (LABEL_INFO_is_catch_start(li) ||
						LABEL_INFO_is_try_start(li) ||
						LABEL_INFO_is_try_end(li) ||
						LABEL_INFO_is_pragma(li)) {
						continue;
					}
					break;
				}
				IS_TRUE0(li);

				IR * g = m_ru->build_goto(li);
				IR_BB_ir_list(bb).append_tail(g);
				bool change = true;
				VERTEX * bbv = m_cfg->get_vertex(IR_BB_id(bb));
				while (change) {
					EDGE_C * ec = VERTEX_out_list(bbv);
					change = false;
					while (ec != NULL) {
						if (EC_edge(ec) != e) {
							//May be remove multi edges.
							((GRAPH*)m_cfg)->remove_edges_between(
											EDGE_from(EC_edge(ec)),
											EDGE_to(EC_edge(ec)));
							change = true;
							break;
						}
						ec = EC_next(ec);
					}
				}
				break;
			} else {
				IS_TRUE0(IR_BB_ir_list(m_cfg->get_bb(VERTEX_id(s))).
						 get_elem_count() == 0);
			}
			vout = EC_next(vout);
		}
	}
}


void IR_DCE::record_all_ir(IN OUT SVECTOR<SVECTOR<IR*>*> & all_ir)
{
	#ifdef _DEBUG_
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		if (IR_BB_ir_list(bb).get_elem_count() == 0) { continue; }
		SVECTOR<IR*> * ir_vec = new SVECTOR<IR*>();
		all_ir.set(IR_BB_id(bb), ir_vec);
		UINT j = 0;
		for (IR * ir = IR_BB_first_ir(bb); ir != NULL; ir = IR_BB_next_ir(bb)) {
			ir_vec->set(j, ir);
			j++;
		}
	}
	#endif
}


//Fix control flow if BB is empty.
//It will be illegal if empty BB has non-taken branch.
void IR_DCE::revise_successor(IR_BB * bb, C<IR_BB*> * bbct, IR_BB_LIST * bbl)
{
	IS_TRUE0(bb && bbct);
	EDGE_C * ec = VERTEX_out_list(m_cfg->get_vertex(IR_BB_id(bb)));
	if (ec == NULL) { return; }

	C<IR_BB*> * next_ct = bbct;
	bbl->get_next(&next_ct);
	IR_BB * next_bb = NULL;
	if (next_ct != NULL) {
		next_bb = C_val(next_ct);
	}
	while (ec != NULL) {
		EDGE * e = EC_edge(ec);
		if (EDGE_info(e) != NULL && EI_is_eh((EI*)EDGE_info(e))) {
			ec = EC_next(ec);
			continue;
		}
		EDGE_C * next_ec = EC_next(ec);
		((GRAPH*)m_cfg)->remove_edge(e);
		ec = next_ec;
	}

	if (next_bb != NULL) {
		m_cfg->add_edge(IR_BB_id(bb), IR_BB_id(next_bb));
	}
}


//Remove DU chain in SSA form.
inline static void remove_ir_ssa_du(IR * stmt)
{
	IS_TRUE0(stmt->is_stmt());
	if (stmt->is_write_pr() || stmt->is_call()) {
		SSAINFO * ssainfo = stmt->get_ssainfo();
		if (ssainfo == NULL) { return; }
		ssainfo->clean_du();
	}
}


//An aggressive algo will be used if cdg is avaliable.
bool IR_DCE::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	if (m_is_elim_cfs) {
		m_ru->check_valid_and_recompute(&oc, OPT_DU_REF, OPT_CDG,OPT_PDOM,
										OPT_DU_CHAIN, OPT_UNDEF);
		m_cdg = (CDG*)OPTC_pass_mgr(oc)->register_opt(OPT_CDG);
	} else {
		m_ru->check_valid_and_recompute(&oc, OPT_DU_REF, OPT_PDOM,
										OPT_DU_CHAIN, OPT_UNDEF);
		m_cdg = NULL;
	}

	if (!OPTC_is_du_chain_valid(oc)) {
		return false;
	}

	//#define _DEBUG_DCE_
	#ifdef _DEBUG_DCE_
	SVECTOR<SVECTOR<IR*>*> all_ir;
	if (g_tfile != NULL) {
		record_all_ir(all_ir);
	}
	#endif

	//Mark effect IRs.
	EFFECT_STMT is_stmt_effect;
	LIST<IR const*> work_list;
	BITSET is_bb_effect;

	//Mark effect IRs.
	mark_effect_ir(is_stmt_effect, is_bb_effect, work_list);

	#ifdef _DEBUG_DCE_
	dump(is_stmt_effect, is_bb_effect, all_ir);
	#endif

	iter_collect(is_stmt_effect, is_bb_effect, work_list);

	bool change = false;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	C<IR_BB*> * ctbb;
	LIST<IR_BB*> bblst;
	LIST<C<IR_BB*>*> ctlst;
	for (IR_BB * bb = bbl->get_head(&ctbb);
		 bb != NULL; bb = bbl->get_next(&ctbb)) {
		C<IR*> * ctir, * next;
		bool tobecheck = false;
		for (IR_BB_ir_list(bb).get_head(&ctir), next = ctir;
			 ctir != NULL; ctir = next) {
			IR * ir = C_val(ctir);
			IR_BB_ir_list(bb).get_next(&next);
			if (!is_stmt_effect.is_contain(IR_id(ir))) {
				if (is_ssa_available()) {
					//Revise SSA info if PR is in SSA form.
					remove_ir_ssa_du(ir);
				}

				//Revise DU chains.
				//TODO: If ssa form is available, it doesn't need to maintain
				//DU chain of PR in DU manager counterpart.
				m_du->remove_ir_out_from_du_mgr(ir);

				if (ir->is_cond_br() || ir->is_uncond_br() ||
					ir->is_multicond_br()) {
					revise_successor(bb, ctbb, bbl);
				}

				IR_BB_ir_list(bb).remove(ctir);

				m_ru->free_irs(ir);

				change = true;

				tobecheck = true;
			}
		}
		if (tobecheck) {
			bblst.append_tail(bb);
			ctlst.append_tail(ctbb);
		}
	}
	//fix_control_flow(bblst, ctlst);

	#ifdef _DEBUG_DCE_
	if (g_tfile != NULL) {
		dump(is_stmt_effect, is_bb_effect, all_ir);
		for (INT i = 0; i <= all_ir.get_last_idx(); i++) {
			SVECTOR<IR*> * ir_vec = all_ir.get(i);
			if (ir_vec != NULL) {
				delete ir_vec;
			}
		}
	}
	#endif

	if (change) {
		bool ck_cfg = false;
		bool lchange;
		do {
			lchange = false;
			lchange |= m_cfg->remove_unreach_bb();
			lchange |= m_cfg->remove_empty_bb(oc);
			lchange |= m_cfg->remove_redundant_branch();
			lchange |= m_cfg->remove_tramp_edge();
			if (lchange) {
				OPTC_is_cdg_valid(oc) = false;
				OPTC_is_dom_valid(oc) = false;
				OPTC_is_pdom_valid(oc) = false;
				OPTC_is_loopinfo_valid(oc) = false;
				OPTC_is_rpo_valid(oc) = false;
				ck_cfg = true;
			}
		} while (lchange);

		#ifdef _DEBUG_
		if (ck_cfg) {
			//Check cfg validation, which
			//need cdg to be available.
			//This check is only in debug mode.
			OPTC_is_rpo_valid(oc) = false;
			m_cfg->compute_pdom_ipdom(oc, NULL);
			CDG * cdg = (CDG*)OPTC_pass_mgr(oc)->register_opt(OPT_CDG);
			cdg->rebuild(oc, *m_ru->get_cfg());
			IS_TRUE0(m_cfg->verify_rmbb(cdg, oc));
		}
		#endif

		//AA, DU chain and du reference are maintained.
		IS_TRUE0(m_du->verify_du_ref() && m_du->verify_du_chain());
		OPTC_is_expr_tab_valid(oc) = false;
		OPTC_is_live_expr_valid(oc) = false;
		OPTC_is_reach_def_valid(oc) = false;
		OPTC_is_avail_reach_def_valid(oc) = false;
	}

	END_TIMER_AFTER(get_opt_name());
	return change;
}
//END IR_DCE
