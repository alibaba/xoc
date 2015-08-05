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
#include "ir_loop_cvt.h"

bool IR_LOOP_CVT::is_while_do(LI<IR_BB> const* li, OUT IR_BB ** gobackbb,
							  UINT * succ1, UINT * succ2)
{
	IS_TRUE0(gobackbb);
	IR_BB * head = LI_loop_head(li);
	IS_TRUE0(head);

	*gobackbb = ::find_single_backedge_start_bb(li, m_cfg);
	if (*gobackbb == NULL) {
		//loop may be too messy.
		return false;
	}

	if (IR_BB_rpo(head) > IR_BB_rpo(*gobackbb)) {
		//loop may already be do-while.
		return false;
	}

	IR * lastir = IR_BB_last_ir(head);
	if (!lastir->is_cond_br()) {
		return false;
	}

	bool f = find_loop_header_two_succ_bb(li, m_cfg, succ1, succ2);
	if (!f) { return false; }

	if (li->inside_loop(*succ1) && li->inside_loop(*succ2)) {
		return false;
	}
	return true;
}


bool IR_LOOP_CVT::try_convert(LI<IR_BB> * li, IR_BB * gobackbb,
							  UINT succ1, UINT succ2)
{
	IS_TRUE0(gobackbb);

	IR_BB * loopbody_start_bb;
	IR_BB * epilog;
	if (li->inside_loop(succ1)) {
		IS_TRUE0(!li->inside_loop(succ2));
		loopbody_start_bb = m_cfg->get_bb(succ1);
		epilog = m_cfg->get_bb(succ2);
	} else {
		IS_TRUE0(li->inside_loop(succ2));
		IS_TRUE0(!li->inside_loop(succ1));
		loopbody_start_bb = m_cfg->get_bb(succ2);
		epilog = m_cfg->get_bb(succ1);
	}
	IS_TRUE0(loopbody_start_bb && epilog);
	IR_BB * next = m_cfg->get_fallthrough_bb(gobackbb);
	if (next == NULL || next != epilog) {
		//No benefit to be get to convert this kind of loop.
		return false;
	}

	C<IR*> * irct;
	IR * lastir = IR_BB_ir_list(gobackbb).get_tail(&irct);
	IS_TRUE0(IR_type(lastir) == IR_GOTO);

	IR_BB * head = LI_loop_head(li);
	IS_TRUE0(head);

	//Copy ir in header to gobackbb.
	IR * last_cond_br = NULL;
	DU_ITER di;
	SVECTOR<IR*> rmvec;
	for (IR * ir = IR_BB_first_ir(head);
		 ir != NULL; ir = IR_BB_next_ir(head)) {
		IR * newir = m_ru->dup_irs(ir);

		m_du->copy_ir_tree_du_info(newir, ir, true);

		m_ii.clean();
		for (IR * x = ir_iter_rhs_init(ir, m_ii);
			 x != NULL; x = ir_iter_rhs_next(m_ii)) {
			if (!x->is_memory_ref()) { continue; }

			DU_SET const* defset = x->get_duset_c();
			if (defset == NULL) { continue; }

			UINT cnt = 0;
			for (INT d = defset->get_first(&di);
				 d >= 0; d = defset->get_next(d, &di)) {
				IR * def = m_ru->get_ir(d);

				IS_TRUE0(def->get_bb());
				if (li->inside_loop(IR_BB_id(def->get_bb()))) {
					rmvec.set(cnt++, def);
				}
			}

			if (cnt != 0) {
				for (UINT i = 0; i < cnt; i++) {
					IR * d = rmvec.get(i);
					m_du->remove_du_chain(d, x);
				}
			}
		}

		IR_BB_ir_list(gobackbb).insert_before(newir, irct);
		if (newir->is_cond_br()) {
			IS_TRUE0(ir == IR_BB_last_ir(head));
			last_cond_br = newir;
			newir->invert_ir_type(m_ru);
		}
	}
	IS_TRUE0(last_cond_br);
	IR_BB_ir_list(gobackbb).remove(irct);
	m_ru->free_ir(lastir);
	m_cfg->remove_edge(gobackbb, head); //revise cfg.

	LABEL_INFO * loopbody_start_lab =
		IR_BB_lab_list(loopbody_start_bb).get_head();
	if (loopbody_start_lab == NULL) {
		loopbody_start_lab = ::new_ilabel(m_ru->get_pool());
		m_cfg->add_lab(loopbody_start_bb, loopbody_start_lab);
	}
	last_cond_br->set_label(loopbody_start_lab);

	//Add back edge.
	m_cfg->add_edge(IR_BB_id(gobackbb), IR_BB_id(loopbody_start_bb));

	//Add fallthrough edge.
	m_cfg->add_edge(IR_BB_id(gobackbb), IR_BB_id(next));
	IR_BB_is_fallthrough(next) = true;
	return true;
}


bool IR_LOOP_CVT::find_and_convert(LIST<LI<IR_BB>*> & worklst)
{
	bool change = false;
	while (worklst.get_elem_count() > 0) {
		LI<IR_BB> * x = worklst.remove_head();
		IR_BB * gobackbb;
		UINT succ1;
		UINT succ2;
		if (is_while_do(x, &gobackbb, &succ1, &succ2)) {
			change |= try_convert(x, gobackbb, succ1, succ2);
		}

		x = LI_inner_list(x);
		while (x != NULL) {
			worklst.append_tail(x);
			x = LI_next(x);
		}
	}
	return change;
}


bool IR_LOOP_CVT::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	m_ru->check_valid_and_recompute(&oc, OPT_LOOP_INFO, OPT_RPO, OPT_UNDEF);

	LI<IR_BB> * li = m_cfg->get_loop_info();
	if (li == NULL) { return false; }

	LIST<LI<IR_BB>*> worklst;
	while (li != NULL) {
		worklst.append_tail(li);
		li = LI_next(li);
	}

	bool change = find_and_convert(worklst);
	if (change) {
		//DU reference and du chain has maintained.
		IS_TRUE0(m_du->verify_du_ref());
		IS_TRUE0(m_du->verify_du_chain());

		//All these changed.
		OPTC_is_reach_def_valid(oc) = false;
		OPTC_is_avail_reach_def_valid(oc) = false;
		OPTC_is_live_expr_valid(oc) = false;

		oc.set_flag_if_cfg_changed();
		OPTC_is_cfg_valid(oc) = true; //Only cfg is avaiable.

		//TODO: make rpo, dom valid.
	}

	END_TIMER_AFTER(get_opt_name());
	return change;
}
