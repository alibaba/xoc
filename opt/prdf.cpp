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
#include "prdf.h"

//#define STATISTIC_PRDF

//
//START PRDF
//
void PRDF::dump()
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n==---- DUMP PRDF : liveness of PR ----==\n");
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	g_indent = 2;
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		fprintf(g_tfile, "\n\n\n-- BB%d --", IR_BB_id(bb));
		SBITSETC * live_in = get_livein(IR_BB_id(bb));
		SBITSETC * live_out = get_liveout(IR_BB_id(bb));
		SBITSETC * def = get_def(IR_BB_id(bb));
		SBITSETC * use = get_use(IR_BB_id(bb));

		fprintf(g_tfile, "\nLIVE-IN: ");
		live_in->dump(g_tfile);

		fprintf(g_tfile, "\nLIVE-OUT: ");
		live_out->dump(g_tfile);

		fprintf(g_tfile, "\nDEF: ");
		def->dump(g_tfile);

		fprintf(g_tfile, "\nUSE: ");
		use->dump(g_tfile);
	}
	fflush(g_tfile);
}


void PRDF::process_may(IR const* pr, SBITSETC * gen, SBITSETC * use, bool is_lhs)
{
	if (!m_handle_may) { return; }

	MD_SET const* mds = pr->get_ref_mds();
	if (mds != NULL) {
		MD const* prmd = pr->get_exact_ref();
		IS_TRUE0(prmd);
		for (INT i = mds->get_first(); i >= 0; i = mds->get_next(i)) {
			MD const* md = m_md_sys->get_md(i);
			IS_TRUE0(md);
			if (MD_base(md) != MD_base(prmd)) {
				bool find;
				IS_TRUE0(m_var2pr); //One should initialize m_var2pr.
				UINT prno = m_var2pr->get(VAR_id(MD_base(md)), &find);
				IS_TRUE0(find);
				if (is_lhs) {
					IS_TRUE0(gen && use);
					gen->bunion(prno, m_sbs_mgr);
					use->diff(prno, m_sbs_mgr);
				} else {
					IS_TRUE0(use);
					use->bunion(prno, m_sbs_mgr);
				}
			}
		}
	}
}


void PRDF::process_opnd(IR const* ir, LIST<IR const*> & lst,
						SBITSETC * use, SBITSETC * gen)
{
	for (IR const* k = ir_iter_init_c(ir, lst);
		 k != NULL; k = ir_iter_next_c(lst)) {
		if (k->is_pr()) {
			use->bunion(k->get_prno(), m_sbs_mgr);
			process_may(k, gen, use, false);
		}
	}
}


void PRDF::compute_local(IR_BB * bb, LIST<IR const*> & lst)
{
	SBITSETC * gen = get_def(IR_BB_id(bb));
	SBITSETC * use = get_use(IR_BB_id(bb));
	gen->clean(m_sbs_mgr);
	use->clean(m_sbs_mgr);
	for (IR * x = IR_BB_last_ir(bb); x != NULL; x = IR_BB_prev_ir(bb)) {
		IS_TRUE0(x->is_stmt());
		switch (IR_type(x)) {
		case IR_ST:
			lst.clean();
			process_opnd(ST_rhs(x), lst, use, gen);
			break;
		case IR_STPR:
			gen->bunion(STPR_no(x), m_sbs_mgr);
			use->diff(STPR_no(x), m_sbs_mgr);
			process_may(x, gen, use, true);

			lst.clean();
			process_opnd(STPR_rhs(x), lst, use, gen);
			break;
		case IR_SETEPR:
			gen->bunion(SETEPR_no(x), m_sbs_mgr);
			use->diff(SETEPR_no(x), m_sbs_mgr);
			process_may(x, gen, use, true);

			lst.clean();
			process_opnd(SETEPR_rhs(x), lst, use, gen);

			lst.clean();
			process_opnd(SETEPR_ofst(x), lst, use, gen);
			break;
		case IR_GETEPR:
			gen->bunion(GETEPR_no(x), m_sbs_mgr);
			use->diff(GETEPR_no(x), m_sbs_mgr);
			process_may(x, gen, use, true);

			lst.clean();
			process_opnd(GETEPR_base(x), lst, use, gen);

			lst.clean();
			process_opnd(GETEPR_ofst(x), lst, use, gen);
			break;
		case IR_IST:
			lst.clean();
			process_opnd(x, lst, use, gen);
			break;
		case IR_SWITCH:
			lst.clean();
			process_opnd(SWITCH_vexp(x), lst, use, gen);
			break;
		case IR_IGOTO:
			lst.clean();
			process_opnd(IGOTO_vexp(x), lst, use, gen);
			break;
		case IR_GOTO: break;
		case IR_CALL:
		case IR_ICALL:
			if (x->has_return_val()) {
				gen->bunion(CALL_prno(x), m_sbs_mgr);
				use->diff(CALL_prno(x), m_sbs_mgr);
				process_may(x, gen, use, true);
			}

			lst.clean();
			process_opnd(CALL_param_list(x), lst, use, gen);

			if (IR_type(x) == IR_ICALL && ICALL_callee(x)->is_pr()) {
				use->bunion(PR_no(ICALL_callee(x)), m_sbs_mgr);
				process_may(ICALL_callee(x), gen, use, false);
			}
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			lst.clean();
			process_opnd(BR_det(x), lst, use, gen);
			break;
		case IR_RETURN:
			lst.clean();
			process_opnd(RET_exp(x), lst, use, gen);
			break;
		case IR_PHI:
			gen->bunion(PHI_prno(x), m_sbs_mgr);
			use->diff(PHI_prno(x), m_sbs_mgr);
			process_may(x, gen, use, true);

			lst.clean();
			process_opnd(PHI_opnd_list(x), lst, use, gen);
			break;
		case IR_REGION: break;
		default: IS_TRUE0(0);
		}
	}
}


/* Note this function does not consider PHI effect.
e.g:
BB1:             BB2:
st $pr4 = 0      st $pr3 = 1
     \               /
      \             /
      BB3:
      phi $pr5 = $pr4, $pr4
      ...

In actually , $pr4 only lived out from BB1, and $pr3 only lived out
from BB2. For present, $pr4 both live out from BB1 and BB2, and $pr3
is similar. */
#ifdef STATISTIC_PRDF
static UINT g_max_times = 0;
#endif
void PRDF::compute_global()
{
	IS_TRUE0(IR_BB_is_entry(m_cfg->get_entry_list()->get_head()) &&
			m_cfg->get_entry_list()->get_elem_count() == 1);

	//Rpo should be available.
	LIST<IR_BB*> * vlst = m_cfg->get_bblist_in_rpo();
	IS_TRUE0(vlst->get_elem_count() == m_ru->get_bb_list()->get_elem_count());
	for (IR_BB * bb = vlst->get_head(); bb != NULL; bb = vlst->get_next()) {
		UINT bbid = IR_BB_id(bb);
		get_livein(bbid)->clean(m_sbs_mgr);
		get_liveout(bbid)->clean(m_sbs_mgr);
	}

	bool change;
	UINT count = 0;
	UINT thres = 1000;
	SBITSETC news;
	do {
		change = false;
		for (IR_BB * bb = vlst->get_tail(); bb != NULL; bb = vlst->get_prev()) {
			UINT bbid = IR_BB_id(bb);

			SBITSETC * out = m_liveout.get(bbid);
			news.copy(*out, m_sbs_mgr);

			IS_TRUE0(m_def.get(bbid));
			news.diff(*m_def.get(bbid), m_sbs_mgr);
			news.bunion(*m_use.get(bbid), m_sbs_mgr);
			m_livein.get(bbid)->copy(news, m_sbs_mgr);

			EDGE_C const* ec = VERTEX_out_list(m_cfg->get_vertex(IR_BB_id(bb)));
			if (ec != NULL) {
				bool first = true;
				INT succ = VERTEX_id(EDGE_to(EC_edge(ec)));
				news.copy(*m_livein.get(succ), m_sbs_mgr);
				ec = EC_next(ec);

				for (; ec != NULL; ec = EC_next(ec)) {
					news.bunion(*m_livein.get(succ), m_sbs_mgr);
				}

				if (!out->is_equal(news)) {
					out->copy(news, m_sbs_mgr);
					change = true;
				}
			}
		}
		count++;
	} while (change && count < thres);
	IS_TRUE(!change, ("result of equation is convergent slowly"));

	news.clean(m_sbs_mgr);

	#ifdef STATISTIC_PRDF
	g_max_times = MAX(g_max_times, count);
	FILE * h = fopen("prdf.sat.dump", "a+");
	fprintf(h, "\n%s run %u times, maxtimes %u",
			m_ru->get_ru_name(), count, g_max_times);
	fclose(h);
	#endif
}


bool PRDF::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	m_ru->check_valid_and_recompute(&oc, OPT_RPO, OPT_UNDEF);
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	if (bbl->get_elem_count() == 0) { return false; }

	LIST<IR const*> lst;
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		compute_local(bb, lst);
	}

	compute_global();

	//dump();
	END_TIMER_AFTER(get_opt_name());
	return false;
}
//END PRDF
