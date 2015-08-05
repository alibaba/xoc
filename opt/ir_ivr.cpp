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
#include "ir_ivr.h"

//
//START IR_IVR
//
bool IR_IVR::comp_init_val(IR const* ir, IV * iv)
{
	if (IR_type(ir) != IR_ST && IR_type(ir) != IR_IST) {
		return false;
	}

	IR const* v = ST_rhs(ir);
	if (IR_type(v) == IR_CVT) {
		//You should have performed refine_ir to optimize cvt.
		v = CVT_exp(v);
	}

	if (IR_type(v) == IR_CONST && v->is_int(m_dm)) {
		if (IV_initv_i(iv) == NULL) {
			IV_initv_i(iv) = (LONGLONG*)xmalloc(sizeof(LONGLONG));
		}
		*IV_initv_i(iv) = CONST_int_val(v);
		IV_initv_type(iv) = IV_INIT_VAL_IS_INT;
		return true;
	}

	MD const* md = v->get_exact_ref();
	if (md != NULL) {
		IV_initv_md(iv) = md;
		IV_initv_type(iv) = IV_INIT_VAL_IS_VAR;
		return true;
	}
	IV_initv_i(iv) = NULL;
	return false;
}


bool IR_IVR::find_init_val(IV * iv)
{
	IR const* def = IV_iv_def(iv);
	IS_TRUE0(def->is_stmt());
	DU_SET const* defs = m_du->get_du_c(def);
	IS_TRUE0(defs);
	IR const* domdef = m_du->find_dom_def(IV_iv_occ(iv), def, defs, true);
	if (domdef == NULL) { return false; }

	MD const* md = domdef->get_exact_ref();
	if (md == NULL || md != IV_iv(iv)) {
		return false;
	}
	LI<IR_BB> const* li = IV_li(iv);
	IS_TRUE0(li);
	IR_BB * dbb = domdef->get_bb();
	if (dbb == LI_loop_head(li) || !li->inside_loop(IR_BB_id(dbb))) {
		return comp_init_val(domdef, iv);
	}
	return false;
}


/* Find all iv.
'map_md2defcount': record the number of definitions to MD.
'map_md2defir': map MD to define stmt. */
void IR_IVR::find_biv(LI<IR_BB> const* li, BITSET & tmp,
					  SVECTOR<UINT> & map_md2defcount,
					  UINT2IR & map_md2defir)
{
	IR_BB * head = LI_loop_head(li);
	UINT headi = IR_BB_id(head);
	tmp.clean(); //tmp is used to record exact MD which be modified.
	map_md2defir.clean();
	map_md2defcount.clean();
	for (INT i = LI_bb_set(li)->get_first();
		 i != -1; i = LI_bb_set(li)->get_next(i)) {
		//if ((UINT)i == headi) { continue; }
		IR_BB * bb = m_ru->get_bb(i);
		IS_TRUE0(bb && m_cfg->get_vertex(IR_BB_id(bb)));
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			if (IR_type(ir) != IR_ST && IR_type(ir) != IR_IST &&
				IR_type(ir) != IR_CALL && IR_type(ir) != IR_ICALL) {
				continue;
			}
			MD const* exact_md = m_du->get_must_def(ir);
			if (exact_md != NULL) {
				if (exact_md->is_exact()) {
					tmp.bunion(MD_id(exact_md));
					UINT c = map_md2defcount.get(MD_id(exact_md)) + 1;
					map_md2defcount.set(MD_id(exact_md), c);
					if (c == 1) {
						map_md2defir.set(MD_id(exact_md), ir);
					} else {
						map_md2defir.aset(MD_id(exact_md), NULL);
						tmp.diff(MD_id(exact_md));
					}
				}
			}

			//May kill other definitions.
			MD_SET const* maydef = m_du->get_may_def(ir);
			if (maydef == NULL) { continue; }

			for (INT i = tmp.get_first(); i != -1; i = tmp.get_next(i)) {
				MD const* md = m_md_sys->get_md(i);
				IS_TRUE0(md->is_exact());
				if (maydef->is_contain(md)) {
					map_md2defcount.set(i, 0);
					tmp.diff(i);
				}
			}
		} //end for
	}

	//Find biv.
	bool find = false;
	LIST<MD*> sdlst; //single def md lst.
	for (INT i = tmp.get_first(); i != -1; i = tmp.get_next(i)) {
		if (map_md2defcount.get(i) != 1) { continue; }
		IR * def = map_md2defir.get(i);
		IS_TRUE0(def);
		if (m_du->get_avail_in_reach_def(headi)->is_contain(IR_id(def))) {
			//MD i is biv.
			sdlst.append_head(m_md_sys->get_md(i));
			find = true;
		}
	}
	if (!find) { return; }

	for (MD * biv = sdlst.get_head(); biv != NULL; biv = sdlst.get_next()) {
		IR * def = map_md2defir.get(MD_id(biv));
		IS_TRUE0(def);
		if (IR_type(def) != IR_ST &&
			IR_type(def) != IR_STPR &&
			IR_type(def) != IR_IST) {
			continue;
		}

		//Find the self modify stmt: i = i + ...
		DU_SET const* useset = m_du->get_du_c(def);
		if (useset == NULL) { continue; }
		bool selfmod = false;
		DU_ITER di;
		for (INT i = useset->get_first(&di);
			 i >= 0; i = useset->get_next(i, &di)) {
			IR const* use = m_ru->get_ir(i);
			IS_TRUE0(use->is_exp());

			IR const* use_stmt = use->get_stmt();
			IS_TRUE0(use_stmt && use_stmt->is_stmt());

			if (use_stmt == def) {
				selfmod = true;
				break;
			}
		}
		if (!selfmod) { continue; }

		IR * stv = ST_rhs(def);
		if (IR_type(stv) != IR_ADD && IR_type(stv) != IR_SUB) { continue; }

		//Make sure self modify stmt is monotonic.
		IR * op0 = BIN_opnd0(stv);
		IR * op1 = BIN_opnd1(stv);
		if (!op1->is_int(m_dm)) { continue; }

		MD const* op0md = op0->get_exact_ref();
		if (op0md == NULL || op0md != biv) { continue; }

		//Work out IV.
		IV * x = new_iv();
		IV_iv(x) = biv;
		IV_li(x) = li;
		IV_iv_occ(x) = op0;
		IV_iv_def(x) = def;
		IV_step(x) = CONST_int_val(op1);
		if (IR_type(stv) == IR_ADD) {
			IV_is_inc(x) = true;
		} else {
			IV_is_inc(x) = false;
		}
		find_init_val(x);
		SLIST<IV*> * ivlst = m_li2bivlst.get(LI_id(li));
		if (ivlst == NULL) {
			ivlst = (SLIST<IV*>*)xmalloc(sizeof(SLIST<IV*>));
			ivlst->set_pool(m_sc_pool);
			m_li2bivlst.set(LI_id(li), ivlst);
		}
		ivlst->append_tail(x);
	}
}


//Return true if 'ir' is loop invariant expression.
//'defs': def set of 'ir'.
bool IR_IVR::is_loop_invariant(LI<IR_BB> const* li, IR const* ir)
{
	IS_TRUE0(ir->is_exp());
	DU_SET const* defs = m_du->get_du_c(ir);
	if (defs == NULL) { return true; }
	DU_ITER di;
	for (INT i = defs->get_first(&di); i >= 0; i = defs->get_next(i, &di)) {
		IR const* d = m_ru->get_ir(i);
		IS_TRUE0(d->is_stmt() && d->get_bb());
		if (li->inside_loop(IR_BB_id(d->get_bb()))) {
			return false;
		}
	}
	return true;
}


//Return true if ir can be regard as induction expression.
//'defs': def list of 'ir'.
bool IR_IVR::scan_exp(IR const* ir, LI<IR_BB> const* li, BITSET const& ivmds)
{
	IS_TRUE0(ir->is_exp());
	switch (IR_type(ir)) {
	case IR_CONST:
	case IR_LDA:
		return true;
	case IR_LD:
	case IR_ILD:
	case IR_ARRAY:
	case IR_PR:
		{
			MD const* irmd = ir->get_exact_ref();
			if (irmd == NULL) { return false; }
			if (ivmds.is_contain(MD_id(irmd))) {
				return true;
			}
			if (is_loop_invariant(li, ir)) {
				return true;
			}
			return false;
		}
		IS_TRUE0(0);
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		if (!scan_exp(BIN_opnd0(ir), li, ivmds)) { return false; }
		if (!scan_exp(BIN_opnd1(ir), li, ivmds)) { return false; }
		return true;
	case IR_CVT:
		return scan_exp(CVT_exp(ir), li, ivmds);
	default:;
	}
	return false;
}


//Try to add iv expresion into div list if 'ive' do not exist in the list.
void IR_IVR::add_divlst(LI<IR_BB> const* li, IR const* e)
{
	SLIST<IR const*> * divlst = m_li2divlst.get(LI_id(li));
	if (divlst == NULL) {
		divlst = (SLIST<IR const*>*)xmalloc(sizeof(SLIST<IR const*>));
		divlst->set_pool(m_sc_pool);
		m_li2divlst.set(LI_id(li), divlst);
	}
	SC<IR const*> * sc;
	bool find = false;
	for (IR const* ive = divlst->get_head(&sc);
		 ive != NULL; ive = divlst->get_next(&sc)) {
		if (ive->is_ir_equal(e, true)) {
			find = true;
			break;
		}
	}
	if (find) { return; }
	divlst->append_tail(e);
}


void IR_IVR::find_div(IN LI<IR_BB> const* li, IN SLIST<IV*> const& bivlst,
					  BITSET & tmp)
{
	if (bivlst.get_elem_count() == 0) { return; }
	SC<IV*> * sc;
	tmp.clean();
	for (IV * iv = bivlst.get_head(&sc);
		 iv != NULL; iv = bivlst.get_next(&sc)) {
		tmp.bunion(MD_id(IV_iv(iv)));
	}

	IR_BB * head = LI_loop_head(li);
	UINT headi = IR_BB_id(head);
	for (INT i = LI_bb_set(li)->get_first();
		 i != -1; i = LI_bb_set(li)->get_next(i)) {
		IR_BB * bb = m_ru->get_bb(i);
		IS_TRUE0(bb && m_cfg->get_vertex(IR_BB_id(bb)));
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			switch (IR_type(ir)) {
			case IR_ST:
			case IR_STPR:
			case IR_IST:
				{
					IR * rhs = ir->get_rhs();
					if (scan_exp(rhs, li, tmp)) {
						add_divlst(li, rhs);
					}
				}
				break;
			case IR_CALL:
			case IR_ICALL:
				for (IR * p = CALL_param_list(ir);
					 p != NULL; p = IR_next(p)) {
					if (scan_exp(p, li, tmp)) {
						add_divlst(li, p);
					}
				}
				break;
			case IR_TRUEBR:
			case IR_FALSEBR:
				if (scan_exp(BR_det(ir), li, tmp)) {
					add_divlst(li, BR_det(ir));
				}
				break;
			default:;
			}
		}
	}
}


void IR_IVR::_dump(LI<IR_BB> * li, UINT indent)
{
	while (li != NULL) {
		fprintf(g_tfile, "\n");
		for (UINT i = 0; i < indent; i++) { fprintf(g_tfile, " "); }
		fprintf(g_tfile, "LI%d:BB%d", LI_id(li), IR_BB_id(LI_loop_head(li)));
		fprintf(g_tfile, ",BODY:");
		for (INT i = LI_bb_set(li)->get_first();
			 i != -1; i = LI_bb_set(li)->get_next(i)) {
			fprintf(g_tfile, "%d,", i);
		}

		SLIST<IV*> * bivlst = m_li2bivlst.get(LI_id(li));
		if (bivlst != NULL) {
			SC<IV*> * sc;
			for (IV * iv = bivlst->get_head(&sc);
				 iv != NULL; iv = bivlst->get_next(&sc)) {
				fprintf(g_tfile, "\n");
				for (UINT i = 0; i < indent; i++) { fprintf(g_tfile, " "); }
				fprintf(g_tfile, "BIV(md%d", MD_id(IV_iv(iv)));
				if (IV_is_inc(iv)) {
					fprintf(g_tfile, ",step=%lld", IV_step(iv));
				} else {
					fprintf(g_tfile, ",step=-%lld", IV_step(iv));
				}
				if (IV_initv_i(iv) != NULL) {
					if (IV_initv_type(iv) == IV_INIT_VAL_IS_INT) {
						fprintf(g_tfile, ",init=%lld", *IV_initv_i(iv));
					} else {
						IS_TRUE0(IV_initv_type(iv) == IV_INIT_VAL_IS_VAR);
						fprintf(g_tfile, ",init=md%d", MD_id(IV_initv_md(iv)));
					}
				}
				fprintf(g_tfile, ")");
			}
		}

		SLIST<IR const*> * divlst = m_li2divlst.get(LI_id(li));
		if (divlst != NULL) {
			SC<IR const*> * sc;
			if (divlst->get_elem_count() > 0) {
				fprintf(g_tfile, "\n");
				for (UINT i = 0; i < indent; i++) { fprintf(g_tfile, " "); }
			}
			for (IR const* iv = divlst->get_head(&sc);
				 iv != NULL; iv = divlst->get_next(&sc)) {
				fprintf(g_tfile, "\n");
				g_indent = indent;
				dump_ir(iv, m_dm);
			}
		}

		_dump(LI_inner_list(li), indent + 2);
		li = LI_next(li);
		fflush(g_tfile);
	}
}


//Dump IVR info for loop.
void IR_IVR::dump()
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n==---- DUMP IVR -- ru:'%s' ----==", m_ru->get_ru_name());
	_dump(m_cfg->get_loop_info(), 0);
	fflush(g_tfile);
}


void IR_IVR::clean()
{
	for (INT i = 0; i <= m_li2bivlst.get_last_idx(); i++) {
		SLIST<IV*> * ivlst = m_li2bivlst.get(i);
		if (ivlst == NULL) { continue; }
		ivlst->clean();
	}
	for (INT i = 0; i <= m_li2divlst.get_last_idx(); i++) {
		SLIST<IR const*> * ivlst = m_li2divlst.get(i);
		if (ivlst == NULL) { continue; }
		ivlst->clean();
	}

}


bool IR_IVR::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	UINT n = m_ru->get_bb_list()->get_elem_count();
	if (n == 0) { return false; }

	m_ru->check_valid_and_recompute(&oc, OPT_AVAIL_REACH_DEF, OPT_DU_REF,
									OPT_DOM, OPT_LOOP_INFO, OPT_DU_CHAIN,
									OPT_RPO, OPT_UNDEF);

	LI<IR_BB> const* li = m_cfg->get_loop_info();
	clean();
	if (li == NULL) { return false; }

	BITSET tmp;
	SVECTOR<UINT> map_md2defcount;
	UINT2IR map_md2defir;

	SLIST<LI<IR_BB> const*> worklst(m_pool);
	while (li != NULL) {
		worklst.append_tail(li);
		li = LI_next(li);
	}

	while (worklst.get_elem_count() > 0) {
		LI<IR_BB> const* x = worklst.remove_head();
		find_biv(x, tmp, map_md2defcount, map_md2defir);
		SLIST<IV*> const* bivlst = m_li2bivlst.get(LI_id(x));
		if (bivlst != NULL && bivlst->get_elem_count() > 0) {
			find_div(x, *bivlst, tmp);
		}
		x = LI_inner_list(x);
		while (x != NULL) {
			worklst.append_tail(x);
			x = LI_next(x);
		}
	}

	//dump();
	END_TIMER_AFTER(get_opt_name());
	return false;
}
//END IR_IVR
