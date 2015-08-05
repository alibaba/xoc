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
#include "ir_rce.h"

//
//START IR_RCE
//
void IR_RCE::dump()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n\n==---- DUMP IR_RCE ----==\n");

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		//TODO:
	}
	fflush(g_tfile);
}


/*
If 'ir' is always true, set 'must_true', or if it is
always false, set 'must_false'.
*/
IR * IR_RCE::calc_cond_must_val(IN IR * ir, OUT bool & must_true,
								OUT bool & must_false)
{
	must_true = false;
	must_false = false;
	IS_TRUE0(ir->is_judge());
	switch (IR_type(ir)) {
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_NE:
	case IR_EQ:
	case IR_LAND:
	case IR_LOR:
	case IR_LNOT:
		{
			bool change = false;
			ir = m_ru->fold_const(ir, change);
			if (change) {
				IS_TRUE0(IR_is_const(ir) &&
						 (CONST_int_val(ir) == 0 || CONST_int_val(ir) == 1));
				if (CONST_int_val(ir) == 1) {
					must_true = true;
				} else if (CONST_int_val(ir) == 0) {
					must_false = true;
				}
				return ir;
			}
			if (m_gvn != NULL) {
				change = m_gvn->calc_cond_must_val(ir, must_true, must_false);
				if (change) {
					if (must_true) {
						IS_TRUE0(!must_false);
						UINT tyid = IR_dt(ir);
						m_ru->free_irs(ir);
						ir = m_ru->build_imm_int(1, tyid);
						return ir;
					} else {
						IS_TRUE0(must_false);
						UINT tyid = IR_dt(ir);
						m_ru->free_irs(ir);
						ir = m_ru->build_imm_int(0, tyid);
						return ir;
					}
				}
			}
		}
		break;
	default: IS_TRUE0(0);
	}
	return ir;
}


IR * IR_RCE::process_branch(IR * ir, IN OUT bool & cfg_mod)
{
	IS_TRUE0(IR_type(ir) == IR_FALSEBR || IR_type(ir) == IR_TRUEBR);
	bool must_true, must_false;
	BR_det(ir) = calc_cond_must_val(BR_det(ir), must_true, must_false);
	if (IR_type(ir) == IR_TRUEBR) {
		if (must_true) {
			//TRUEBR(0x1)
			IR_BB * from = ir->get_bb();
			IR_BB * to = m_cfg->get_fallthrough_bb(from);
			IS_TRUE0(from != NULL && to != NULL);

			IR * old = ir;
			ir = m_ru->build_goto(BR_lab(old));
			m_ru->free_irs(old);

			//Revise m_cfg. remove fallthrough edge.
			m_cfg->remove_edge(from, to);
			cfg_mod = true;
		} else if (must_false) {
			//TRUEBR(0x0)

			//Revise m_cfg. remove branch edge.
			IR_BB * from = ir->get_bb();
			IR_BB * to = m_cfg->get_target_bb(from);
			IS_TRUE0(from != NULL && to != NULL);
			m_cfg->remove_edge(from, to);

			m_ru->free_irs(ir);
			ir = NULL;
			cfg_mod = true;
		}
	} else {
		if (must_true) {
			//FALSEBR(0x1)
			//Revise m_cfg. remove branch edge.
			IR_BB * from = ir->get_bb();
			IR_BB * to = m_cfg->get_target_bb(from);
			IS_TRUE0(from != NULL && to != NULL);
			m_cfg->remove_edge(from, to);

			m_ru->free_irs(ir);
			ir = NULL;
			cfg_mod = true;
		} else if (must_false) {
			//FALSEBR(0x0)
			IR_BB * from = ir->get_bb();
			IR_BB * to = m_cfg->get_fallthrough_bb(from);
			IS_TRUE0(from != NULL && to != NULL);

			IR * old = ir;
			ir = m_ru->build_goto(BR_lab(old));
			m_ru->free_irs(old);

			//Revise m_cfg. remove fallthrough edge.
			m_cfg->remove_edge(from, to);
			cfg_mod = true;
		}
	}
	return ir;
}


//Perform dead store elmination: x = x;
IR * IR_RCE::process_st(IR * ir)
{
	IS_TRUE0(IR_type(ir) == IR_ST);
	if (ST_rhs(ir)->get_exact_ref() == ir->get_exact_ref()) {
		m_ru->free_irs(ir);
		return NULL;
	}
	return ir;
}


//Perform dead store elmination: x = x;
IR * IR_RCE::process_stpr(IR * ir)
{
	IS_TRUE0(IR_type(ir) == IR_STPR);
	if (STPR_rhs(ir)->get_exact_ref() == ir->get_exact_ref()) {
		m_ru->free_irs(ir);
		return NULL;
	}
	return ir;
}


/*
e.g:
	1. if (a == a) { ... } , remove redundant comparation.
	2. b = b; remove redundant store.
*/
bool IR_RCE::perform_simply_rce(IN OUT bool & cfg_mod)
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	bool change = false;
	C<IR_BB*> * ct_bb;
	for (IR_BB * bb = bbl->get_head(&ct_bb);
		 bb != NULL; bb = bbl->get_next(&ct_bb)) {
		BBIR_LIST * ir_list = &IR_BB_ir_list(bb);
		C<IR*> * ct, * next_ct;
		for (ir_list->get_head(&next_ct), ct = next_ct;
			 ct != NULL; ct = next_ct) {
			IR * ir = C_val(ct);
			ir_list->get_next(&next_ct);
			IR * new_ir = ir;
			switch (IR_type(ir)) {
			case IR_TRUEBR:
			case IR_FALSEBR:
				new_ir = process_branch(ir, cfg_mod);
				break;
			case IR_ST:
				new_ir = process_st(ir);
				break;
			default:
				;
			}
			if (new_ir != ir) {
				ir_list->remove(ct);
				if (next_ct != NULL) {
					ir_list->insert_before(new_ir, next_ct);
				} else {
					ir_list->append_tail(new_ir);
				}
				change = true;
			}
		}
	}
	return change;
}


bool IR_RCE::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	m_ru->check_valid_and_recompute(&oc, OPT_DU_REF, OPT_DU_CHAIN,
									OPT_UNDEF);
	if (!m_gvn->is_valid()) {
		m_gvn->reperform(oc);
	}

	bool cfg_mod = false;
	bool change = perform_simply_rce(cfg_mod);
	if (cfg_mod) {
		//m_gvn->set_valid(false); //rce do not violate gvn for now.
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
			}
		} while (lchange);

		//TODO: May be the change of CFG does not influence the
		//usage while we utilize du-chain and ir2mds.

		OPTC_is_expr_tab_valid(oc) = false;
		OPTC_is_du_chain_valid(oc) = false;
		OPTC_is_ref_valid(oc) = false;
		OPTC_is_aa_valid(oc) = false;
		OPTC_is_expr_tab_valid(oc) = false;
		OPTC_is_dom_valid(oc) = false;
		OPTC_is_pdom_valid(oc) = false;
		OPTC_is_reach_def_valid(oc) = false;
		OPTC_is_avail_reach_def_valid(oc) = false;
	}
	END_TIMER_AFTER(get_opt_name());
	return change;
}
//END IR_RCE
