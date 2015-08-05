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
#include "ir_rp.h"

class REF_HASH_FUNC {
	IR_GVN * m_gvn;
	CIR_ITER m_iter;
public:
	void init_mem(IR_GVN * gvn)
	{
		IS_TRUE0(gvn);
		m_gvn = gvn;
	}

	UINT get_hash_value(IR * t, UINT bucket_size)
	{
		IS_TRUE0(bucket_size != 0 && is_power_of_2(bucket_size));
		UINT hval = 0;
		switch (IR_type(t)) {
		case IR_LD:
			hval = IR_type(t) + (t->get_ofst() + 1) + IR_dt(t);
			break;
		case IR_ILD:
			m_iter.clean();
			for (IR const* x = ir_iter_init_c(t, m_iter);
				 x != NULL; x = ir_iter_next_c(m_iter)) {
				UINT v = IR_type(x) + (x->get_ofst() + 1) + IR_dt(x);
				if (IR_type(x) == IR_ID) {
					v += VAR_id(ID_info(x)) * 5;
				}
				hval += v;
			}
			break;
		case IR_ST:
			hval = ((UINT)IR_LD) + (t->get_ofst() + 1) + IR_dt(t);
			break;
		case IR_IST:
			m_iter.clean();
			for (IR const* x = ir_iter_init_c(IST_base(t), m_iter);
				 x != NULL; x = ir_iter_next_c(m_iter)) {
				UINT v = IR_type(x) + (x->get_ofst() + 1) + IR_dt(x);
				if (IR_type(x) == IR_ID) {
					v += VAR_id(ID_info(x)) * 5;
				}
				hval += v;
			}
			hval += ((UINT)IR_ILD) + (t->get_ofst() + 1) + IR_dt(t);
			break;
		case IR_ARRAY:
			m_iter.clean();
			for (IR const* x = ir_iter_init_c(t, m_iter);
				 x != NULL; x = ir_iter_next_c(m_iter)) {
				UINT v = IR_type(x) + (x->get_ofst() + 1) + IR_dt(x);
				if (IR_type(x) == IR_ID) {
					v += VAR_id(ID_info(x)) * 5;
				}
				hval += v;
			}
			break;
		default: IS_TRUE0(0); //unsupport.
		}
		return hash32bit(hval) & (bucket_size - 1);
	}

	bool compare_array(IR * t1, IR * t2) const
	{
		IS_TRUE0(m_gvn);
		if (t1 == t2) { return true; }

		IS_TRUE0(m_gvn->map_ir2vn(ARR_base(t1)) &&
				 m_gvn->map_ir2vn(ARR_base(t2)));

		if (m_gvn->map_ir2vn(ARR_base(t1)) != m_gvn->map_ir2vn(ARR_base(t2))) {
			return false;
		}

		if (((CARRAY*)t1)->get_dimn() != ((CARRAY*)t2)->get_dimn()) {
			return false;
		}

		IR * s1 = ARR_sub_list(t1);
		IR * s2 = ARR_sub_list(t2);
		for (; s1 != NULL && s2 != NULL; s1 = IR_next(s1), s2 = IR_next(s2)) {
			IS_TRUE0(m_gvn->map_ir2vn(s1) && m_gvn->map_ir2vn(s2));
			if (m_gvn->map_ir2vn(s1) != m_gvn->map_ir2vn(s2)) {
				return false;
			}
		}

		if (s1 != NULL || s2 != NULL) { return false; }

		if (ARR_ofst(t1) != ARR_ofst(t2)) { return false; }

		if (IR_dt(t1) != IR_dt(t2)) { return false; }

		return true;
	}

	bool compare_indirect_access(IR * t1, IR * t2) const
	{
		IS_TRUE0(m_gvn);
		if (t1 == t2) { return true; }

		IR const* base1 = NULL;
		if (IR_type(t1) == IR_ILD) { base1 = ILD_base(t1); }
		else if (IR_type(t1) == IR_IST) { base1 = IST_base(t1); }
		IS_TRUE0(base1);

		IR const* base2 = NULL;
		if (IR_type(t2) == IR_ILD) { base2 = ILD_base(t2); }
		else if (IR_type(t2) == IR_IST) { base2 = IST_base(t2); }
		IS_TRUE0(base2);

		IS_TRUE0(m_gvn->map_ir2vn(base1) && m_gvn->map_ir2vn(base2));
		if (m_gvn->map_ir2vn(base1) != m_gvn->map_ir2vn(base2)) {
			return false;
		}

		if (t1->get_ofst() != t2->get_ofst()) { return false; }

		if (IR_dt(t1) != IR_dt(t2)) { return false; }

		return true;
	}

	bool compare_direct_access(IR * t1, IR * t2) const
	{
		IS_TRUE0(m_gvn);
		if (t1 == t2) { return true; }

		IS_TRUE0(m_gvn->map_ir2vn(t1) && m_gvn->map_ir2vn(t2));
		if (m_gvn->map_ir2vn(t1) != m_gvn->map_ir2vn(t2)) {
			return false;
		}

		if (t1->get_ofst() != t2->get_ofst()) { return false; }

		if (IR_dt(t1) != IR_dt(t2)) { return false; }

		return true;
	}

	bool compare(IR * t1, IR * t2) const
	{
		if (IR_type(t1) == IR_ARRAY && IR_type(t2) == IR_ARRAY) {
			return compare_array(t1, t2);
		} else if ((IR_type(t1) == IR_ILD || IR_type(t1) == IR_IST) &&
				   (IR_type(t2) == IR_ILD || IR_type(t2) == IR_IST)) {
			return compare_indirect_access(t1, t2);
		} else if ((IR_type(t1) == IR_ILD || IR_type(t1) == IR_IST) &&
				   (IR_type(t2) == IR_ILD || IR_type(t2) == IR_IST)) {
			return compare_direct_access(t1, t2);
		}
		return false;
	}
};


class REF_TAB : public SHASH<IR*, REF_HASH_FUNC> {
public:
	REF_TAB(UINT bucksize) : SHASH<IR*, REF_HASH_FUNC>(bucksize) {}

	void init_mem(IR_GVN * gvn)
	{ m_hf.init_mem(gvn); }
};


class PROMOTED_TAB : public SBITSET {
public:
	PROMOTED_TAB(SEG_MGR * sm) : SBITSET(sm) {}

	//Add whole ir tree into table.
	void add(IR * ir, IR_ITER & ii)
	{
		ii.clean();
		for (IR * x = ir_iter_init(ir, ii);
			 x != NULL; x = ir_iter_next(ii)) {
			bunion(IR_id(x));
		}
	}
};


static void dump_delegate_tab(REF_TAB & dele_tab, DT_MGR * dm)
{
	if (g_tfile == NULL) { return; }
	IS_TRUE0(dm);

	fprintf(g_tfile, "\n==---- DUMP Delegate Table ----==");
	INT cur = 0;
	for (IR * dele = dele_tab.get_first(cur);
		 cur >= 0; dele = dele_tab.get_next(cur)) {
		dump_ir(dele, dm);
	}
	fflush(g_tfile);
}


//
//START IR_RP
//
void IR_RP::dump()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP IR_RP ----==\n");

	g_indent = 2;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		fprintf(g_tfile, "\n---- BB%d -----", IR_BB_id(bb));
		BITSET * live_in = get_livein_mds(bb);
		BITSET * live_out = get_liveout_mds(bb);
		BITSET * def = get_def_mds(bb);
		BITSET * use = get_use_mds(bb);
		INT i;

		fprintf(g_tfile, "\nLIVE-IN: ");
		for (i = live_in->get_first(); i != -1; i = live_in->get_next(i)) {
			fprintf(g_tfile, "MD%d, ", i);
		}

		fprintf(g_tfile, "\nLIVE-OUT: ");
		for (i = live_out->get_first(); i != -1; i = live_out->get_next(i)) {
			fprintf(g_tfile, "MD%d, ", i);
		}

		fprintf(g_tfile, "\nDEF: ");
		for (i = def->get_first(); i != -1; i = def->get_next(i)) {
			fprintf(g_tfile, "MD%d, ", i);
		}

		fprintf(g_tfile, "\nUSE: ");
		for (i = use->get_first(); i != -1; i = use->get_next(i)) {
			fprintf(g_tfile, "MD%d, ", i);
		}
	}
	fflush(g_tfile);
}


void IR_RP::dump_occ_list(LIST<IR*> & occs, DT_MGR * dm)
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n---- DUMP exact occ list ----");
	BITSET visit;
	for (IR * x = occs.get_head(); x != NULL; x = occs.get_next()) {
		dump_ir(x, dm, NULL, true, false, false);
		fprintf(g_tfile, "\n");
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


void IR_RP::dump_access2(TTAB<IR*> & access, DT_MGR * dm)
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n---- DUMP inexact access ----");
	TAB_ITER<IR*> iter;
	for (IR * ir = access.get_first(iter);
		ir != NULL; ir = access.get_next(iter)) {
		dump_ir(ir, dm);
		fprintf(g_tfile, "\n");
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


void IR_RP::dump_access(TMAP<MD const*, IR*> & access, DT_MGR * dm)
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n---- DUMP exact access ----");
	TMAP_ITER<MD const*, IR*> iter;
	IR * ref;
	for (MD const* md = access.get_first(iter, &ref);
	     ref != NULL; md = access.get_next(iter, &ref)) {
	    md->dump();
	    dump_ir(ref, dm);
		fprintf(g_tfile, "\n");
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


BITSET * IR_RP::get_livein_mds(IR_BB * bb)
{
	BITSET * set = m_livein_mds_vec.get(IR_BB_id(bb));
	if (set == NULL) {
		set = m_bs_mgr.create();
		m_livein_mds_vec.set(IR_BB_id(bb), set);
	}
	return set;
}


BITSET * IR_RP::get_liveout_mds(IR_BB * bb)
{
	BITSET * set = m_liveout_mds_vec.get(IR_BB_id(bb));
	if (set == NULL) {
		set = m_bs_mgr.create();
		m_liveout_mds_vec.set(IR_BB_id(bb), set);
	}
	return set;
}


BITSET * IR_RP::get_def_mds(IR_BB * bb)
{
	BITSET * set = m_def_mds_vec.get(IR_BB_id(bb));
	if (set == NULL) {
		set = m_bs_mgr.create();
		m_def_mds_vec.set(IR_BB_id(bb), set);
	}
	return set;
}


BITSET * IR_RP::get_use_mds(IR_BB * bb)
{
	BITSET * set = m_use_mds_vec.get(IR_BB_id(bb));
	if (set == NULL) {
		set = m_bs_mgr.create();
		m_use_mds_vec.set(IR_BB_id(bb), set);
	}
	return set;
}


void IR_RP::compute_local_liveness(IR_BB * bb, IR_DU_MGR & du_mgr)
{
	BITSET * gen = get_def_mds(bb);
	BITSET * use = get_use_mds(bb);
	gen->clean();
	use->clean();
	MD_SET mustuse;
	for (IR * ir = IR_BB_last_ir(bb); ir != NULL; ir = IR_BB_prev_ir(bb)) {
		MD const* def = ir->get_exact_ref();
		if (def != NULL) {
			gen->bunion(MD_id(def));
			use->diff(*gen);
		}
		mustuse.clean();
		du_mgr.collect_must_use(ir, mustuse);
		use->bunion(mustuse);
	}
}


MD_LT * IR_RP::gen_mdlt(MD * md)
{
	MD_LT * lt;
	if ((lt = m_md2lt_map->get(md)) != NULL) {
		return lt;
	}
	lt = (MD_LT*)xmalloc(sizeof(MD_LT));
	MDLT_id(lt) = ++m_mdlt_count;
	MDLT_md(lt) = md;
	MDLT_livebbs(lt) = m_bs_mgr.create();
	m_md2lt_map->set(md, lt);
	return lt;
}


void IR_RP::clean_livebbs()
{
	//Clean.
	SVECTOR<MD_LT*> * bs_vec = m_md2lt_map->get_tgt_elem_vec();
	for (INT i = 0; i <= bs_vec->get_last_idx(); i++) {
		MD_LT * lt = bs_vec->get(i);
		if (lt != NULL) {
			IS_TRUE0(MDLT_livebbs(lt) != NULL);
			MDLT_livebbs(lt)->clean();
		}
	}
}


void IR_RP::dump_mdlt()
{
	if (g_tfile == NULL) return;
	BITSET mdbs;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		BITSET * livein = get_livein_mds(bb);
		BITSET * liveout = get_liveout_mds(bb);
		if (livein->is_empty() && liveout->is_empty()) { continue; }
		mdbs.bunion(*livein);
		mdbs.bunion(*liveout);
	}
	mdbs.dump(g_tfile);

	CHAR buf[255];
	fprintf(g_tfile, "\n==---- DUMP MD LIFE TIME ----==");
	for (INT i = mdbs.get_first(); i >= 0; i = mdbs.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md != NULL);
		MD_LT * lt = m_md2lt_map->get(md);
		IS_TRUE0(lt != NULL);
		BITSET * livebbs = MDLT_livebbs(lt);
		IS_TRUE0(livebbs != NULL);

		//Print MD name.
		fprintf(g_tfile, "\nMD%d", MD_id(md));
		fprintf(g_tfile, ":");

		//Print live BB.
		if (livebbs == NULL || livebbs->is_empty()) { continue; }
		INT start = 0;
		for (INT u = livebbs->get_first(); u >= 0; u = livebbs->get_next(u)) {
			for (INT j = start; j < u; j++) {
				sprintf(buf, "%d,", j);
				for (UINT k = 0; k < strlen(buf); k++) {
					fprintf(g_tfile, " ");
				}
			}
			fprintf(g_tfile, "%d,", u);
			start = u + 1;
		}
	}
	fflush(g_tfile);
}


void IR_RP::build_lt()
{
	clean_livebbs();

	//Rebuild life time.
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		BITSET * livein = get_livein_mds(bb);
		BITSET * liveout = get_liveout_mds(bb);
		if (livein->is_empty() && liveout->is_empty()) { continue; }

		for (INT i = livein->get_first(); i >= 0; i = livein->get_next(i)) {
			MDLT_livebbs(gen_mdlt(m_md_sys->get_md(i)))->bunion(IR_BB_id(bb));
		}
		for (INT i = liveout->get_first(); i >= 0; i = liveout->get_next(i)) {
			MDLT_livebbs(gen_mdlt(m_md_sys->get_md(i)))->bunion(IR_BB_id(bb));
		}
	}

	//dump_mdlt();
}


void IR_RP::compute_global_liveness()
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		get_livein_mds(bb)->clean();
		get_liveout_mds(bb)->clean();
	}
	bool change;
	INT count = 0;
	IR_CFG * cfg = m_ru->get_cfg();
	BITSET news;
	LIST<IR_BB*> succs;
	do {
		change = false;
		for (IR_BB * bb = bbl->get_tail(); bb != NULL; bb = bbl->get_prev()) {
			BITSET * out = get_liveout_mds(bb);
			news.copy(*out);
			news.diff(*get_def_mds(bb));
			news.bunion(*get_use_mds(bb));
			BITSET * in = get_livein_mds(bb);
			if (!in->is_equal(news)) {
				in->copy(news);
				change = true;
			}

			cfg->get_succs(succs, bb);
			news.clean();
			for (IR_BB * p = succs.get_head(); p != NULL; p = succs.get_next()) {
				news.bunion(*get_livein_mds(p));
			}
			if (!out->is_equal(news)) {
				out->copy(news);
				change = true;
			}
		}
		count++;
	} while (change && count < 220);
	IS_TRUE(!change, ("result of equation is convergent slowly"));
}


void IR_RP::compute_liveness()
{
	IR_DU_MGR * du_mgr = m_ru->get_du_mgr();
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		compute_local_liveness(bb, *du_mgr);
	}
	compute_global_liveness();
}


void IR_RP::add_exact_access(OUT TMAP<MD const*, IR*> & exact_access,
							 OUT LIST<IR*> & exact_occ_list,
							 MD const* exact_md,
							 IR * ir)
{
	IS_TRUE0(exact_md && exact_md->is_exact());
	if (!exact_access.find(exact_md)) {
		exact_access.set(exact_md, ir);
	}
	exact_occ_list.append_tail(ir);
}


void IR_RP::add_inexact_access(TTAB<IR*> & inexact_access, IR * ir)
{
	inexact_access.append_and_retrieve(ir);
}


bool IR_RP::check_exp_is_loop_invariant(IN IR * ir, LI<IR_BB> const* li)
{
	if (ir->is_memory_opnd()) {
		DU_SET const* duset = ir->get_duset_c();
		if (duset == NULL) { return true; }

		DU_ITER dui;
		for (INT i = duset->get_first(&dui);
			 i >= 0; i = duset->get_next(i, &dui)) {
			IR const* defstmt = m_ru->get_ir(i);
			IS_TRUE0(defstmt->is_stmt());
			IR_BB * bb = defstmt->get_bb();

			if (li->inside_loop(IR_BB_id(bb))) { return false; }
		}
		return true;
	}

	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid == NULL) { continue; }
		if (!check_exp_is_loop_invariant(kid, li)) {
			return false;
		}
	}

	return true;
}


bool IR_RP::check_array_is_loop_invariant(IN IR * ir, LI<IR_BB> const* li)
{
	IS_TRUE0(IR_type(ir) == IR_ARRAY && li);
	for (IR * s = ARR_sub_list(ir); s != NULL; s = IR_next(s)) {
		if (!check_exp_is_loop_invariant(s, li)) {
			return false;
		}
	}
	if (!check_exp_is_loop_invariant(ARR_base(ir), li)) {
		return false;
	}
	return true;
}


//Return true if the caller can keep doing the analysis.
bool IR_RP::handle_array_ref(IN IR * ir,
							LI<IR_BB> const* li,
							OUT TMAP<MD const*, IR*> & exact_access,
							OUT LIST<IR*> & exact_occ_list,
							OUT TTAB<IR*> & inexact_access)
{
	IS_TRUE0(IR_type(ir) == IR_ARRAY);

	if (ARR_ofst(ir) != 0) {
		//The array reference can not be promoted.
		//Check the promotable candidates in list if ir may overlap with them.
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}

	MD const* mustuse = ir->get_ref_md();
	if (mustuse == NULL || !mustuse->is_effect()) { return false; }

	if (mustuse->is_volatile()) {
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}

	if (mustuse->is_exact()) {
		if (!m_dont_promot.is_overlap(mustuse)) {
			//Exact memory access.
			add_exact_access(exact_access, exact_occ_list, mustuse, ir);
		}
		return true;
	}

	if (m_dont_promot.is_overlap(mustuse)) {
		return true;
	}

	//MD is inexact. Check if it is loop invariant.
	if (!check_array_is_loop_invariant(ir, li)) {
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}

	bool add = true;
	TAB_ITER<IR*> ti;
	for (IR * ref = inexact_access.get_first(ti);
		 ref != NULL; ref = inexact_access.get_next(ti)) {
		UINT st = analyze_array_status(ir, ref);
		if (st == RP_SAME_ARRAY) { continue; }
		if (st == RP_DIFFERENT_ARRAY) { continue; }

		//The result can not be promoted.
		//Check the promotable candidates if current stmt modify the related MD.
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}
	add_inexact_access(inexact_access, ir);
	return true;
}


bool IR_RP::handle_general_ref(IR * ir,
							LI<IR_BB> const* li,
							OUT TMAP<MD const*, IR*> & exact_access,
							OUT LIST<IR*> & exact_occ_list,
							OUT TTAB<IR*> & inexact_access)
{
	IS_TRUE0(ir->is_memory_ref());
	IS_TRUE0(IR_type(ir) != IR_ARRAY);

	if (ir->get_ofst() != 0) {
		//TODO:not yet support, x is MC type.
		//clobber_access_in_list(ir, exact_access, exact_occ_list,
		//					   inexact_access);
		//return true;
	}

	MD const* mustuse = ir->get_ref_md();
	if (mustuse == NULL || !mustuse->is_effect()) {
		//It is dispensable clobbering access if all loop is not analysable.
		//clobber_access_in_list(ir, exact_access, exact_occ_list,
		//					   inexact_access);
		return false;
	}

	if (mustuse->is_volatile()) {
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}

	if (mustuse->is_exact()) {
		if (!m_dont_promot.is_overlap(mustuse)) {
			//Exact memory access.
			add_exact_access(exact_access, exact_occ_list, mustuse, ir);
		}

		//mustuse already be in the dont_promot table.
		return true;
	}

	if (m_dont_promot.is_overlap(mustuse)) {
		return true;
	}

	//MD is inexact. Check if it is loop invariant.
	if (!check_indirect_access_is_loop_invariant(ir, li)) {
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}

	TAB_ITER<IR*> ti;
	for (IR * ref = inexact_access.get_first(ti);
		 ref != NULL; ref = inexact_access.get_next(ti)) {
		UINT st = analyze_indirect_access_status(ir, ref);
		if (st == RP_SAME_OBJ) { continue; }
		if (st == RP_DIFFERENT_OBJ) { continue; }

		//The ir can not be promoted.
		//Check the promotable candidates if current stmt modify the related MD.
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}
	add_inexact_access(inexact_access, ir);
	return true;
}


//Return true if find promotable memory reference, and
//the caller can keep doing the analysis.
bool IR_RP::handle_st_array(IN IR * ir,
							LI<IR_BB> const* li,
							OUT TMAP<MD const*, IR*> & exact_access,
							OUT LIST<IR*> & exact_occ_list,
							OUT TTAB<IR*> & inexact_access)
{
	IS_TRUE0(ir->is_st_array());
	if (IST_ofst(ir) != 0) {
		//The result can not be promoted.
		//Check the promotable candidates if current stmt
		//modify the related MD.
		clobber_access_in_list(ir, exact_access, exact_occ_list,
							   inexact_access);
		return true;
	}

	//MustDef of st-array stmt must be same with its lhs.
	IS_TRUE0(IST_base(ir)->get_ref_md() == ir->get_ref_md());
	return handle_array_ref(IST_base(ir), li, exact_access, exact_occ_list,
							inexact_access);
}


//The result of 'ir' can not be promoted.
//Check the promotable candidates if current stmt modify the related MD.
void IR_RP::clobber_access_in_list(IR * ir,
								OUT TMAP<MD const*, IR*> & exact_access,
								OUT LIST<IR*> & exact_occ_list,
								OUT TTAB<IR*> & inexact_access)
{
	MD const* mustdef = ir->get_ref_md();
	MD_SET const* maydef = ir->get_ref_mds();

	if (mustdef != NULL) { m_dont_promot.bunion(mustdef); }
	else if (maydef != NULL) { m_dont_promot.bunion(*maydef); }

	TMAP_ITER<MD const*, IR*> iter;
	SVECTOR<MD const*> need_to_be_removed;
	INT cnt = 0;
	if (mustdef != NULL) {
		for (MD const* md = exact_access.get_first(iter, NULL);
			 md != NULL; md = exact_access.get_next(iter, NULL)) {
			if (mustdef == md || mustdef->is_overlap(md)) {
				//Current ir may modify the candidate's md.
				//We think the candidate is not suite to promot any more.
				need_to_be_removed.set(cnt, md);
				cnt++;
			}
		}
	} else if (maydef != NULL && !maydef->is_empty()) {
		for (MD const* md = exact_access.get_first(iter, NULL);
			 md != NULL; md = exact_access.get_next(iter, NULL)) {
			if (maydef->is_contain(md)) {
				//Current ir may modify the candidate's md.
				//We think the candidate is not suite to promot any more.
				need_to_be_removed.set(cnt, md);
				cnt++;
			}
		}
	}

	for (cnt = cnt - 1; cnt >= 0; cnt--) {
		MD const* md = need_to_be_removed.get(cnt);
		exact_access.remove(md);
		//Do not remove occ in occ_list here.
		//We will remove the related occ in occ_list after
		//scan_bb() all at once.
	}

	TAB_ITER<IR*> iter2;
	SVECTOR<IR*> need_to_be_removed2;
	cnt = 0;
	if (mustdef != NULL) {
		for (IR * acc = inexact_access.get_first(iter2);
			 acc != NULL; acc = inexact_access.get_next(iter2)) {
			MD const* acc_md = acc->get_ref_md();
			MD_SET const* acc_mds = acc->get_ref_mds();
			if (acc_md != NULL) {
				if (mustdef == acc_md || mustdef->is_overlap(acc_md)) {
					//ir is not suite to promot any more, all mds which
					//overlapped with it are also not promotable.
					need_to_be_removed2.set(cnt, acc);
					cnt++;
				}
			} else if (acc_mds != NULL && acc_mds->is_overlap(mustdef)) {
				//ir is not suite to promot any more, all mds which
				//overlapped with it are also not promotable.
				need_to_be_removed2.set(cnt, acc);
				cnt++;
			}
		}
	} else if (maydef != NULL && !maydef->is_empty()) {
		for (IR * acc = inexact_access.get_first(iter2);
			 acc != NULL; acc = inexact_access.get_next(iter2)) {
			MD const* acc_md = acc->get_ref_md();
			MD_SET const* acc_mds = acc->get_ref_mds();
			if ((acc_md != NULL && maydef->is_overlap(acc_md)) ||
				(acc_mds != NULL &&
				 (acc_mds == maydef || maydef->is_intersect(*acc_mds)))) {
				//ir is not suite to promot any more, all mds which
				//overlapped with it are also not promotable.
				need_to_be_removed2.set(cnt, acc);
				cnt++;
			}
		}
	}

	for (cnt = cnt - 1; cnt >= 0; cnt--) {
		IR * e = need_to_be_removed2.get(cnt);
		inexact_access.remove(e);
	}
}


bool IR_RP::check_indirect_access_is_loop_invariant(IN IR * ir,
													LI<IR_BB> const* li)
{
	IS_TRUE0((IR_type(ir) == IR_ILD || IR_type(ir) == IR_IST) && li);
	if (IR_type(ir) == IR_ILD) {
		if (!check_exp_is_loop_invariant(ILD_base(ir), li)) {
			return false;
		}
	} else if (IR_type(ir) == IR_IST) {
		if (!check_exp_is_loop_invariant(IST_base(ir), li)) {
			return false;
		}
	}
	return true;
}


//Determine whether the memory reference is same object or different.
UINT IR_RP::analyze_indirect_access_status(IR const* ref1, IR const* ref2)
{
	UINT status = RP_UNKNOWN;
	IR const* base1 = NULL;
	if (IR_type(ref1) == IR_ILD) {
		base1 = ILD_base(ref1);
	} else if (IR_type(ref1) == IR_IST) {
		base1 = IST_base(ref1);
	} else {
		return RP_UNKNOWN;
	}

	IR const* base2 = NULL;
	if (IR_type(ref2) == IR_ILD) {
		base2 = ILD_base(ref2);
	} else if (IR_type(ref2) == IR_IST) {
		base2 = IST_base(ref2);
	} else {
		return RP_UNKNOWN;
	}

	IS_TRUE0(base1->is_ptr(m_dm) && base2->is_ptr(m_dm));

	IS_TRUE0(m_gvn);

	VN const* vn1 = m_gvn->map_ir2vn(base1);
	VN const* vn2 = m_gvn->map_ir2vn(base2);
	if (vn1 == NULL || vn2 == NULL) { return RP_UNKNOWN; }

	UINT tysz1 = ref1->get_dt_size(m_dm);
	UINT tysz2 = ref2->get_dt_size(m_dm);
	UINT ofst1 = ref1->get_ofst();
	UINT ofst2 = ref2->get_ofst();
	if ((((ofst1 + tysz1) <= ofst2) ||
		((ofst2 + tysz2) <= ofst1))) {
		return RP_DIFFERENT_OBJ;
	}
	if (ofst1 == ofst2 && tysz1 == tysz2) {
		return RP_SAME_OBJ;
	}
	return RP_UNKNOWN;
}


/* Find promotable candidate memory references.
Return true if current memory referense does not clobber other
candidate in list. Or else return false means there are ambiguous
memory reference. */
bool IR_RP::scan_opnd(IR * ir,
					LI<IR_BB> const* li,
					OUT TMAP<MD const*, IR*> & exact_access,
					OUT LIST<IR*> & exact_occ_list,
					OUT TTAB<IR*> & inexact_access,
					IR_ITER & ii)
{
	ii.clean();
	for (IR * x = ir_iter_rhs_init(ir, ii);
		 x != NULL; x = ir_iter_rhs_next(ii)) {
		if (!x->is_memory_opnd() || x->is_pr()) { continue; }
		if (IR_type(x) == IR_ARRAY) {
			if (!handle_array_ref(x, li, exact_access, exact_occ_list,
								  inexact_access)) {
				return false;
			}
			continue;
		}

		if (!handle_general_ref(x, li, exact_access, exact_occ_list,
								inexact_access)) {
			return false;
		}
	}
	return true;
}


//Return true if find promotable memory reference.
bool IR_RP::scan_res(IN IR * ir,
					 LI<IR_BB> const* li,
					 OUT TMAP<MD const*, IR*> & exact_access,
					 OUT LIST<IR*> & exact_occ_list,
					 OUT TTAB<IR*> & inexact_access)
{
	if (IR_type(ir) == IR_IST) {
		if (ir->is_st_array()) {
			return handle_st_array(ir, li, exact_access, exact_occ_list,
							 	inexact_access);
		}
		return handle_general_ref(ir, li, exact_access, exact_occ_list,
								inexact_access);
	} else if (ir->is_stid()) {
		return handle_general_ref(ir, li, exact_access, exact_occ_list,
							   inexact_access);
	}
	return true;
}


/* Scan BB and find promotable memory reference.
If this function find unpromotable access that with ambiguous
memory reference, all relative promotable accesses which in list
will not be scalarized.
e.g: a[0] = ...
	a[i] = ...
	a[0] is promotable, but a[i] is not, then a[0] can not be promoted.

If there exist memory accessing that we do not know where it access,
whole loop is unpromotable. */
bool IR_RP::scan_bb(IN IR_BB * bb,
					LI<IR_BB> const* li,
					OUT TMAP<MD const*, IR*> & exact_access,
					OUT TTAB<IR*> & inexact_access,
					OUT LIST<IR*> & exact_occ_list,
					IR_ITER & ii)
{
	for (IR * ir = IR_BB_last_ir(bb);
		 ir != NULL; ir = IR_BB_prev_ir(bb)) {
		if (ir->is_call() && !ir->is_readonly_call()) {
			return false;
		}
	}
	for (IR * ir = IR_BB_first_ir(bb);
		 ir != NULL; ir = IR_BB_next_ir(bb)) {
		if (!ir->is_contain_mem_ref()) { continue; }
		if (IR_type(ir) == IR_REGION) { return false; }
		if (!scan_res(ir, li, exact_access, exact_occ_list, inexact_access)) {
			return false;
		}
		if (!scan_opnd(ir, li, exact_access, exact_occ_list,
					  inexact_access, ii)) {
			return false;
		}
	}
	return true;
}


IR_BB * IR_RP::find_single_exit_bb(LI<IR_BB> const* li)
{
	IR_BB * head = LI_loop_head(li);
	IS_TRUE0(head);

	IS_TRUE0(IR_BB_last_ir(head));
	IR * x = IR_BB_last_ir(head);
	if (x->is_multicond_br()) {
		//If the end ir of loop header is multi-conditional-branch, we
		//are not going to handle it.
		return NULL;
	}

	IR_BB * check_bb = head;
	LIST<IR_BB*> succs;
	for (;;) {
		IR * z = IR_BB_last_ir(check_bb);
		m_cfg->get_succs(succs, check_bb);
		if (z->is_cond_br()) {
			IS_TRUE0(succs.get_elem_count() == 2);
			IR_BB * succ = succs.get_head();
			if (!li->inside_loop(IR_BB_id(succ))) {
				return succ;
			}

			succ = succs.get_next();
			if (!li->inside_loop(IR_BB_id(succ))) {
				return succ;
			}

			return NULL;
		}

		if (z->is_call() || z->is_uncond_br() || z->is_stid() ||
			IR_type(z) == IR_IST || z->is_write_pr()) {
			if (succs.get_elem_count() > 1) {
				//Stmt may throw exception.
				return NULL;
			}
			check_bb = succs.get_head();
			continue;
		}
		return NULL;
	}
	return NULL;
}


//Replace the use for oldir to newir.
//If oldir is not leaf, cut off the du chain to all of its kids.
void IR_RP::replace_use_for_tree(IR * oldir, IR * newir)
{
	IS_TRUE0(oldir->is_exp() && newir->is_exp());
	if (IR_type(oldir) == IR_LD) {
		m_du->change_use(newir, oldir, m_du->get_sbs_mgr());
	} else if (IR_type(oldir) == IR_ILD) {
		m_du->remove_use_out_from_defset(ILD_base(oldir));
		m_du->change_use(newir, oldir, m_du->get_sbs_mgr());
	} else if (IR_type(oldir) == IR_ARRAY) {
		m_du->remove_use_out_from_defset(ARR_base(oldir));
		m_du->remove_use_out_from_defset(ARR_sub_list(oldir));
		m_du->change_use(newir, oldir, m_du->get_sbs_mgr());
	} else {
		IS_TRUE0(0); //TODO
	}
}


void IR_RP::handle_restore2mem(TTAB<IR*> & restore2mem,
							TMAP<IR*, IR*> & delegate2stpr,
							TMAP<IR*, IR*> & delegate2pr,
							TMAP<IR*, DU_SET*> & delegate2use,
							TMAP<IR*, SLIST<IR*>*> &
									delegate2has_outside_uses_ir_list,
							TAB_ITER<IR*> & ti,
							IR_BB * exit_bb)
{
	//Restore value from delegate PR to delegate memory object.
	ti.clean();
	for (IR * delegate = restore2mem.get_first(ti);
		 delegate != NULL; delegate = restore2mem.get_next(ti)) {
		IR * pr = m_ru->dup_irs(delegate2pr.get_c(delegate));
		m_ru->alloc_ref_for_pr(pr);

		IR * stpr = delegate2stpr.get(delegate);
		m_du->build_du_chain(stpr, pr);

		IR * stmt = NULL;
		if (IR_type(delegate) == IR_ARRAY) {
			IS_TRUE(delegate->get_ofst() == 0, ("TODO: not yet support."));
			IR * dups = m_ru->dup_irs(delegate);
			dups->copy_ref_for_tree(delegate, m_ru);

			m_du->copy_ir_tree_du_info(ARR_base(dups),
									ARR_base(delegate), true);
			m_du->copy_ir_tree_du_info(ARR_sub_list(dups),
									ARR_sub_list(delegate), true);

			stmt = m_ru->build_istore(dups, pr, IR_dt(delegate));
			IS_TRUE0(stmt->is_st_array());
		} else if (IR_type(delegate) == IR_IST) {
			IR * lhs = m_ru->dup_irs(IST_base(delegate));
			m_du->copy_ir_tree_du_info(lhs, IST_base(delegate), true);

			stmt = m_ru->build_istore(lhs, pr,
									IST_ofst(delegate), IR_dt(delegate));
		} else if (IR_type(delegate) == IR_ST) {
			stmt = m_ru->build_store(ST_idinfo(delegate),
									IR_dt(delegate),
									ST_ofst(delegate), pr);
		} else if (IR_type(delegate) == IR_ILD) {
			IR * base = m_ru->dup_irs(ILD_base(delegate));
			stmt = m_ru->build_istore(base, pr,
									ILD_ofst(delegate),
									IR_dt(delegate));
			m_du->copy_ir_tree_du_info(base, ILD_base(delegate), true);
		} else if (IR_type(delegate) == IR_LD) {
			stmt = m_ru->build_store(LD_idinfo(delegate),
									IR_dt(delegate),
									LD_ofst(delegate), pr);
		} else {
			IS_TRUE0(0); //Unsupport.
		}

		IS_TRUE0(stmt);

		//Compute the DU chain of stmt.
		DU_SET const* set = delegate2use.get(delegate);
		if (set != NULL) {
			m_du->copy_duset(stmt, set);
			m_du->union_def(set, stmt);
		}

		stmt->copy_ref(delegate, m_ru);

		SLIST<IR*> * irlst =
			delegate2has_outside_uses_ir_list.get(delegate);
		IS_TRUE0(irlst);
		SC<IR*> * sct;
		for (IR * def = irlst->get_head(&sct);
			 def != NULL; def = irlst->get_next(&sct)) {
			IS_TRUE0(STPR_no(def) == PR_no(pr));
			m_du->build_du_chain(def, pr);
		}

		IR_BB_ir_list(exit_bb).append_head(stmt);
	}
}


//Return true if there is IR be promoted, otherwise return false.
bool IR_RP::promote_exact_access(LI<IR_BB> const* li,
								 IR_ITER & ii,
								 TAB_ITER<IR*> & ti,
								 IR_BB * preheader,
								 IR_BB * exit_bb,
								 TMAP<MD const*, IR*> & exact_access,
								 LIST<IR*> & exact_occ_list)
{
	IS_TRUE0(preheader && exit_bb && li);

	//Map IR expression to STPR which generate the scalar value.
	//Map IR expression to STPR in preheader BB.
	TMAP<IR*, IR*> delegate2stpr;

	//Map IR expression to promoted PR.
	TMAP<IR*, IR*> delegate2pr;

	//Map delegate to a list of references which use memory outside the loop.
	TMAP<IR*, SLIST<IR*>*> delegate2has_outside_uses_ir_list;

	//Map delegate to USE set.
	TMAP<IR*, DU_SET*> delegate2use;

	//Map delegate to DEF set.
	TMAP<IR*, DU_SET*> delegate2def;

	LIST<IR*> fixup_list; //record the IR that need to fix up duset.

	BITSET * bbset = LI_bb_set(li);
	IS_TRUE0(bbset && !bbset->is_empty()); //loop is empty.

	SDBITSET_MGR * sbs_mgr = m_du->get_sbs_mgr();

	TMAP_ITER<MD const*, IR*> mi;
	IR * delegate = NULL;
	MD const* next_md = NULL;
	for (MD const* md = exact_access.get_first(mi, &delegate); md != NULL;) {
		IS_TRUE0(delegate);
		if (!is_promotable(delegate)) {
			//Do not promote the reference.
			MD const* next_md = exact_access.get_next(mi, &delegate);
			exact_access.remove(md);
			md = next_md;
			continue;
		}

		create_delegate_info(delegate, delegate2pr,
							delegate2has_outside_uses_ir_list);
		md = exact_access.get_next(mi, &delegate);
		IS_TRUE0(!((md == NULL) ^ (delegate == NULL)));
	}

	if (exact_access.get_elem_count() == 0) { return false; }

	for (IR * ref = exact_occ_list.get_head();
		 ref != NULL; ref = exact_occ_list.get_next()) {
		 MD const* md = ref->get_ref_md();
		IS_TRUE0(md && md->is_exact());

		//Get the unique delegate.
		IR * delegate = exact_access.get(md);
		if (delegate == NULL) {
			continue;
		}

		compute_outer_def_use(ref, delegate, delegate2def,
							   delegate2use, sbs_mgr, li);
	}

	mi.clean();
	for (exact_access.get_first(mi, &delegate);
		 delegate != NULL; exact_access.get_next(mi, &delegate)) {
		IR * pr = delegate2pr.get(delegate);
		IS_TRUE0(pr);
		handle_prelog(delegate, pr, delegate2stpr, delegate2def, preheader);
	}

	//Map delegate MD to it define stmt in the loop.
	//These MD need to restore to memory in exit BB.
	TTAB<IR*> restore2mem;
	PROMOTED_TAB promoted(m_du->get_sbs_mgr()->get_seg_mgr());
	for (IR * ref = exact_occ_list.get_head();
		 ref != NULL; ref = exact_occ_list.get_next()) {
		if (promoted.is_contain(IR_id(ref))) { continue; }

		MD const* md = ref->get_ref_md();
		IS_TRUE0(md && md->is_exact());

		//Get the unique delegate.
		IR * delegate = exact_access.get(md);
		if (delegate == NULL) { continue; }

		IR * delegate_pr = delegate2pr.get(delegate);
		IS_TRUE0(delegate_pr);
		handle_access_in_body(ref, delegate, delegate_pr,
							delegate2has_outside_uses_ir_list,
							restore2mem, fixup_list, delegate2stpr, li, ii);

		//Each memory reference in the tree has been promoted.
		promoted.add(ref, ii);
	}

	IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));

	handle_restore2mem(restore2mem, delegate2stpr, delegate2pr, delegate2use,
						delegate2has_outside_uses_ir_list, ti, exit_bb);

	remove_redundant_du_chain(fixup_list);

	free_local_struct(delegate2use, delegate2def, delegate2pr, sbs_mgr);

	for (IR * ref = exact_occ_list.get_head();
		 ref != NULL; ref = exact_occ_list.get_next()) {
		if (IR_type(ref) == IR_UNDEF) {
			//ref has been freed, since it is the kid of other ir.
			continue;
		}

		MD const* md = ref->get_ref_md();
		IS_TRUE0(md);

		IR * delegate = exact_access.get(md);
		if (delegate == NULL) {
			//If delegate does not exist, the reference still in its place.
			continue;
		}

		m_ru->free_irs(ref);
	}
	return true;
}


//Return true if some node at IR tree may throw exception.
bool IR_RP::is_may_throw(IR * ir, IR_ITER & iter)
{
	iter.clean();
	IR const* k = ir_iter_init(ir, iter);
	for (; k != NULL; k = ir_iter_next(iter)) {
		if (k->is_memory_ref() && !k->is_write_pr() && !k->is_read_pr()) {
			return true;
		} else if (k->is_call()) {
			return true;
		}

		if (IR_type(k) == IR_DIV || IR_type(k) == IR_REM ||
			IR_type(k) == IR_MOD) {
			return true;
		}
	}
	return false;
}


bool IR_RP::has_loop_outside_use(IR const* stmt, LI<IR_BB> const* li)
{
	IS_TRUE0(stmt->is_stmt());
	DU_SET const* useset = stmt->get_duset_c();
	if (useset == NULL) { return false; }

	DU_ITER di;
	for (INT i = useset->get_first(&di);
		 i >= 0; i = useset->get_next(i, &di)) {
		IR const* u = m_ru->get_ir(i);
		IS_TRUE0(u->is_exp());
		IS_TRUE0(u->get_stmt());
		IR * s = u->get_stmt();
		IS_TRUE0(s->get_bb());
		if (!li->inside_loop(IR_BB_id(s->get_bb()))) {
			return true;
		}
	}
	return false;
}


//Fix up DU chain if there exist untrue dependence.
//'fixup_list': record the IR stmt/exp that need to fix up.
void IR_RP::remove_redundant_du_chain(LIST<IR*> & fixup_list)
{
	SVECTOR<IR const*> * rmvec = new SVECTOR<IR const*>(10);
	for (IR * ref = fixup_list.get_head();
		 ref != NULL; ref = fixup_list.get_next()) {
		IS_TRUE0(ref->is_stpr() || ref->is_pr());

		DU_SET const* duset = ref->get_duset_c();
		if (duset == NULL) { continue; }

		DU_ITER di;
		UINT cnt = 0;
		if (ref->is_stpr()) {
			UINT prno = STPR_no(ref);
			for (INT i = duset->get_first(&di);
				 i >= 0; i = duset->get_next(i, &di)) {
				IR const* u = m_ru->get_ir(i);
				IS_TRUE0(u->is_exp());
				if (u->is_pr()) {
					if (PR_no(u) == prno) { continue; }

					/* DU manager may be confused and build the redundant
					chain because of inexact indirect memory access.
					Here, we remove the dependence that confirmed to be
					redundant. */
					rmvec->set(cnt++, u);
					continue;
				}

				IS_TRUE0(IR_type(u) == IR_ILD || IR_type(u) == IR_ARRAY ||
						 IR_type(u) == IR_LD);

				rmvec->set(cnt++, u);
			}

			//Remove redundant du chain.
			for (UINT i = 0; i < cnt; i++) {
				m_du->remove_du_chain(ref, rmvec->get(i));
			}
		} else {
			IS_TRUE0(ref->is_pr());
			UINT prno = PR_no(ref);
			for (INT i = duset->get_first(&di);
				 i >= 0; i = duset->get_next(i, &di)) {
				IR const* d = m_ru->get_ir(i);
				IS_TRUE0(d->is_stmt());

				if ((d->is_write_pr() || d->is_call_has_retval()) &&
					d->get_prno() == prno) {
					continue;
				}

				/* DU manager may be confused and build the redundant
				chain because of inexact indirect memory access.
				Here, we remove the dependence that confirmed to be
				redundant.

				If the result PR of call does not coincide with ref.
				Remove the redundant chain. */
				rmvec->set(cnt++, d);
			}

			//Remove redundant du chain.
			for (UINT i = 0; i < cnt; i++) {
				m_du->remove_du_chain(rmvec->get(i), ref);
			}
		}
	}
	delete rmvec;
}


//fixup_list: record the IR that need to fix up duset.
void IR_RP::handle_access_in_body(IR * ref, IR * delegate,
						IR const* delegate_pr,
						TMAP<IR*, SLIST<IR*>*> const&
								delegate2has_outside_uses_ir_list,
						OUT TTAB<IR*> & restore2mem,
						OUT LIST<IR*> & fixup_list,
						TMAP<IR*, IR*> const& delegate2stpr,
						LI<IR_BB> const* li,
						IR_ITER & ii)
{
	IS_TRUE0(ref && delegate && delegate_pr && li);
	IR * stmt;
	if (ref->is_stmt()) {
		stmt = ref;
	} else {
		stmt = ref->get_stmt();
	}
	IS_TRUE0(stmt);

	if (stmt->is_st_array() && IST_base(stmt) == ref) {
		bool has_use = false;

		/* Note, may be some USE of 'ref' has already been promoted to
		PR, but it doesn't matter, you don't need to check the truely
		dependence here, since we just want to see whether
		there exist outer of loop references to this stmt. And all
		the same group memory reference will be promoted to PR after
		the function return. */
		if (has_loop_outside_use(stmt, li) || may_be_global_ref(ref)) {
			has_use = true;
		}

		//Substitute STPR(array) for IST.
		IR * stpr = m_ru->build_store_pr(PR_no(delegate_pr),
										 IR_dt(delegate_pr),
										 IST_rhs(stmt));

		m_ru->alloc_ref_for_pr(stpr);

		//New IR has same VN with original one.
		m_gvn->set_map_ir2vn(stpr, m_gvn->map_ir2vn(stmt));

		if (has_use) {
			restore2mem.append_and_retrieve(delegate);
			SLIST<IR*> * irlst =
				delegate2has_outside_uses_ir_list.get_c(delegate);
			IS_TRUE0(irlst);
			irlst->append_tail(stpr);
		}

		m_du->remove_use_out_from_defset(ref);

		//Change DU chain.
		m_du->change_def(stpr, stmt, m_du->get_sbs_mgr());
		fixup_list.append_tail(stpr);

		IST_rhs(stmt) = NULL;
		IST_base(stmt) = NULL;

		IR_BB * stmt_bb = stmt->get_bb();
		IS_TRUE0(stmt_bb);
		C<IR*> * ct = NULL;
		IR_BB_ir_list(stmt_bb).find(stmt, &ct);
		IS_TRUE0(ct != NULL);

		IR_BB_ir_list(stmt_bb).insert_after(stpr, ct);
		IR_BB_ir_list(stmt_bb).remove(ct);
		m_ru->free_irs(stmt);
	} else if (IR_type(ref) == IR_IST || ref->is_stid()) {
		IS_TRUE0(ref == stmt);
		bool has_use = false;

		/* Note, may be some USE of 'ref' has already been promoted to
		PR, but it doesn't matter, you don't need to check the truely
		dependence here, since we just want to see whether
		there exist outer of loop references to this stmt. And all
		the same group memory reference will be promoted to PR after
		the function return. */
		if (has_loop_outside_use(ref, li) || may_be_global_ref(ref)) {
			has_use = true;
		}

		//Substitute STPR(exp) for IST(exp) or
		//substitute STPR(exp) for ST(exp).

		IR * stpr = m_ru->build_store_pr(PR_no(delegate_pr),
										 IR_dt(delegate_pr),
										 ref->get_rhs());
		m_ru->alloc_ref_for_pr(stpr);

		//New IR has same VN with original one.
		m_gvn->set_map_ir2vn(stpr, m_gvn->map_ir2vn(stmt));

		if (has_use) {
			restore2mem.append_and_retrieve(delegate);
			SLIST<IR*> * irlst =
				delegate2has_outside_uses_ir_list.get_c(delegate);
			IS_TRUE0(irlst);
			irlst->append_tail(stpr);
		}

		if (IR_type(ref) == IR_IST) {
			m_du->remove_use_out_from_defset(IST_base(ref));
		}

		//Change DU chain.
		m_du->change_def(stpr, ref, m_du->get_sbs_mgr());
		fixup_list.append_tail(stpr);

		ref->set_rhs(NULL);

		IR_BB * stmt_bb = ref->get_bb();
		IS_TRUE0(stmt_bb);
		C<IR*> * ct = NULL;
		IR_BB_ir_list(stmt_bb).find(ref, &ct);
		IS_TRUE0(ct != NULL);

		IR_BB_ir_list(stmt_bb).insert_after(stpr, ct);
		IR_BB_ir_list(stmt_bb).remove(ct);

		//Do not free stmt here since it will be freed later.
	} else {
		IS_TRUE0(IR_type(ref) == IR_ILD || IR_type(ref) == IR_LD ||
				 IR_type(ref) == IR_ARRAY);
		IR * pr = m_ru->dup_ir(delegate_pr);
		m_ru->alloc_ref_for_pr(pr);

		//New IR has same VN with original one.
		m_gvn->set_map_ir2vn(pr, m_gvn->map_ir2vn(ref));

		//Find the stpr that correspond to delegate MD,
		//and build DU chain bewteen stpr and new ref PR.
		replace_use_for_tree(ref, pr);
		fixup_list.append_tail(pr);

		//Add du chain between new PR and the generated STPR.
		IR * stpr = delegate2stpr.get_c(delegate);
		IS_TRUE0(stpr);
		m_du->build_du_chain(stpr, pr);

		IS_TRUE0(IR_parent(ref));
		bool r = IR_parent(ref)->replace_kid(ref, pr, false);
		IS_TRUE0(r);

		if (IR_may_throw(stmt) && !is_may_throw(stmt, ii)) {
			IR_may_throw(stmt) = false;
		}
	}
}


void IR_RP::handle_prelog(IR * delegate, IR * pr,
						TMAP<IR*, IR*> & delegate2stpr,
						TMAP<IR*, DU_SET*> & delegate2def,
						IR_BB * preheader)
{
	IR * rhs = NULL;
	IR * stpr = NULL;
	if (IR_type(delegate) == IR_ARRAY) {
		//Load array value into PR.
		rhs = m_ru->dup_irs(delegate);
		m_du->copy_ir_tree_du_info(ARR_base(rhs), ARR_base(delegate), true);
		m_du->copy_ir_tree_du_info(ARR_sub_list(rhs),
								ARR_sub_list(delegate), true);
		stpr = m_ru->build_store_pr(PR_no(pr), IR_dt(pr), rhs);
	} else if (IR_type(delegate) == IR_IST) {
		//Load indirect value into PR.
		rhs = m_ru->build_iload(m_ru->dup_irs(IST_base(delegate)),
								IST_ofst(delegate), IR_dt(delegate));
		m_du->copy_ir_tree_du_info(ILD_base(rhs), IST_base(delegate), true);

		stpr = m_ru->build_store_pr(PR_no(pr), IR_dt(pr), rhs);
	} else if (IR_type(delegate) == IR_ILD) {
		//Load indirect value into PR.
		rhs = m_ru->dup_irs(delegate);
		m_du->copy_ir_tree_du_info(ILD_base(rhs), ILD_base(delegate), true);
		stpr = m_ru->build_store_pr(PR_no(pr), IR_dt(pr), rhs);
	} else if (IR_type(delegate) == IR_LD) {
		//Load scalar into PR.
		rhs = m_ru->dup_irs(delegate);
		m_du->copy_ir_tree_du_info(rhs, delegate, false);
		stpr = m_ru->build_store_pr(PR_no(pr), IR_dt(pr), rhs);
	} else if (IR_type(delegate) == IR_ST) {
		//Load scalar into PR.
		rhs = m_ru->build_load(ST_idinfo(delegate));
		LD_ofst(rhs) = ST_ofst(delegate);
		stpr = m_ru->build_store_pr(PR_no(pr), IR_dt(pr), rhs);
	} else {
		IS_TRUE0(0); //unsupport.
	}

	IS_TRUE0(rhs && stpr);

	rhs->copy_ref(delegate, m_ru);

	//Build DU chain if there exist outer loop reach-def.
	DU_SET const* set = delegate2def.get(delegate);
	if (set != NULL) {
		m_du->copy_duset(rhs, set);
		m_du->union_use(set, rhs);
	}

	m_ru->alloc_ref_for_pr(stpr);

	IS_TRUE0(delegate2stpr.get(pr) == NULL);
	delegate2stpr.set(delegate, stpr);
	IR_BB_ir_list(preheader).append_tail_ex(stpr);
}


void IR_RP::compute_outer_def_use(IR * ref, IR * delegate,
								TMAP<IR*, DU_SET*> & delegate2def,
								TMAP<IR*, DU_SET*> & delegate2use,
								SDBITSET_MGR * sbs_mgr,
								LI<IR_BB> const* li)
{
	if (IR_type(ref) == IR_ILD || IR_type(ref) == IR_LD ||
		(IR_type(ref) == IR_ARRAY &&
		 (IR_type(ref->get_stmt()) != IR_IST ||
		  IST_base(ref->get_stmt()) != ref))) {
		//ref is USE.
		DU_SET * defset = delegate2def.get(delegate);
		DU_SET const* refduset = ref->get_duset_c();
		if (defset == NULL && refduset != NULL) {
			defset = (DU_SET*)sbs_mgr->create_sbitsetc();
			delegate2def.set(delegate, defset);
		}

		if (refduset != NULL) {
			DU_ITER di;
			for (INT i = refduset->get_first(&di);
				 i >= 0; i = refduset->get_next(i, &di)) {
				IR const* d = m_ru->get_ir(i);
				IS_TRUE0(d->is_stmt());
				if (!li->inside_loop(IR_BB_id(d->get_bb()))) {
					defset->bunion(i, *sbs_mgr);
				}
			}
		}
	} else {
		//ref is DEF.
		IS_TRUE0(IR_type(ref) == IR_ST || IR_type(ref) == IR_IST ||
				 (IR_type(ref) == IR_ARRAY &&
				  IR_type(ref->get_stmt()) == IR_IST &&
				  IST_base(ref->get_stmt()) == ref));
		DU_SET * set = delegate2use.get(delegate);
		DU_SET const* refduset = ref->get_duset_c();
		if (set == NULL && refduset != NULL) {
			set = (DU_SET*)sbs_mgr->create_sbitsetc();
			delegate2use.set(delegate, set);
		}

		if (refduset != NULL) {
			DU_ITER di;
			for (INT i = refduset->get_first(&di);
				 i >= 0; i = refduset->get_next(i, &di)) {
				IR const* u = m_ru->get_ir(i);
				IS_TRUE0(u->is_exp());
				if (!li->inside_loop(IR_BB_id(u->get_stmt()->get_bb()))) {
					set->bunion(i, *sbs_mgr);
				}
			}
		}
	}
}


void IR_RP::create_delegate_info(IR * delegate,
								TMAP<IR*, IR*> & delegate2pr,
								TMAP<IR*, SLIST<IR*>*> &
									delegate2has_outside_uses_ir_list)
{
	SLIST<IR*> * irlst = (SLIST<IR*>*)xmalloc(sizeof(SLIST<IR*>));
	irlst->set_pool(m_ir_ptr_pool);
	delegate2has_outside_uses_ir_list.set(delegate, irlst);

	//Ref is the delegate of all the semantic equivalent expressions.
	IR * pr = delegate2pr.get(delegate);
	if (pr == NULL) {
		pr = m_ru->build_pr(IR_dt(delegate));
		delegate2pr.set(delegate, pr);
	}
}


//Return true if there is IR be promoted, otherwise return false.
bool IR_RP::promote_inexact_access(LI<IR_BB> const* li,
								   IR_BB * preheader,
								   IR_BB * exit_bb,
								   TTAB<IR*> & inexact_access,
								   IR_ITER & ii,
								   TAB_ITER<IR*> & ti)
{
	IS_TRUE0(li && exit_bb && preheader);

	//Record a delegate to IR expressions which have same value in
	//array base and subexpression.
	REF_TAB delegate_tab(get_nearest_power_of_2(
					inexact_access.get_elem_count()));
	delegate_tab.init_mem(m_gvn);

	//Map IR expression to STPR which generate the scalar value.
	//Map IR expression to STPR in preheader BB.
	TMAP<IR*, IR*> delegate2stpr;

	//Map IR expression to promoted PR.
	TMAP<IR*, IR*> delegate2pr;

	//Map delegate to a list of references which use memory outside the loop.
	TMAP<IR*, SLIST<IR*>*> delegate2has_outside_uses_ir_list;

	//Map delegate to USE set.
	TMAP<IR*, DU_SET*> delegate2use;

	//Map delegate to DEF set.
	TMAP<IR*, DU_SET*> delegate2def;

	LIST<IR*> fixup_list; //record the IR that need to fix up duset.

	BITSET * bbset = LI_bb_set(li);
	IS_TRUE0(bbset && !bbset->is_empty()); //loop is empty.

	SDBITSET_MGR * sbs_mgr = m_du->get_sbs_mgr();

	//Prepare delegate table and related information.
	for (IR * ref = inexact_access.get_first(ti);
		 ref != NULL; ref = inexact_access.get_next(ti)) {
		bool find = false;
		if (!is_promotable(ref)) {
			//Do not promote the reference.
			continue;
		}

		IR * delegate = delegate_tab.append(ref, NULL, &find);
		IS_TRUE0(delegate);

		if (!find) {
			create_delegate_info(delegate,
								delegate2pr,
								delegate2has_outside_uses_ir_list);
		} else {
			IS_TRUE0(delegate2has_outside_uses_ir_list.get(delegate));
		}
		compute_outer_def_use(ref, delegate, delegate2def,
							  delegate2use, sbs_mgr, li);
 	}

	//dump_delegate_tab(delegate_tab, m_dm);

	if (delegate_tab.get_elem_count() == 0) { return false; }

	INT cur;
	for (IR * delegate = delegate_tab.get_first(cur);
		 cur >= 0; delegate = delegate_tab.get_next(cur)) {
		IR * pr = delegate2pr.get(delegate);
		IS_TRUE0(pr);
		handle_prelog(delegate, pr, delegate2stpr, delegate2def, preheader);
	}

	//Map IR expression which need to restore
	//into memory at epilog of loop.
	TTAB<IR*> restore2mem;
	ti.clean();
	for (IR * ref = inexact_access.get_first(ti);
		 ref != NULL; ref = inexact_access.get_next(ti)) {
		//Get the unique delegate.
		IR * delegate = NULL;
		delegate_tab.find(ref, &delegate);

		if (delegate == NULL) {
			//If delegate does not exist, the reference can not
			//be promoted.
			continue;
		}

		IR * delegate_pr = delegate2pr.get(delegate);
		IS_TRUE0(delegate_pr);
		handle_access_in_body(ref, delegate, delegate_pr,
							delegate2has_outside_uses_ir_list,
							restore2mem, fixup_list, delegate2stpr, li, ii);
	}

	IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));

	handle_restore2mem(restore2mem, delegate2stpr, delegate2pr, delegate2use,
						delegate2has_outside_uses_ir_list, ti, exit_bb);

	remove_redundant_du_chain(fixup_list);

	ti.clean();
	for (IR * ref = inexact_access.get_first(ti);
		 ref != NULL; ref = inexact_access.get_next(ti)) {
		IR * delegate = NULL;
		delegate_tab.find(ref, &delegate);
		if (delegate == NULL) {
			//If delegate does not exist, the reference still in its place.
			continue;
		}
		m_ru->free_irs(ref);
	}

	free_local_struct(delegate2use, delegate2def, delegate2pr, sbs_mgr);
	return true;
}


void IR_RP::free_local_struct(TMAP<IR*, DU_SET*> & delegate2use,
							TMAP<IR*, DU_SET*> & delegate2def,
							TMAP<IR*, IR*> & delegate2pr,
							SDBITSET_MGR * sbs_mgr)
{
	TMAP_ITER<IR*, DU_SET*> map_iter2;
	DU_SET * duset;
	for (IR * x = delegate2use.get_first(map_iter2, &duset);
		 x != NULL; x = delegate2use.get_next(map_iter2, &duset)) {
		if (duset != NULL) {
			sbs_mgr->free_sbitsetc(duset);
		}
	}

	map_iter2.clean();
	for (IR * x = delegate2def.get_first(map_iter2, &duset);
		 x != NULL; x = delegate2def.get_next(map_iter2, &duset)) {
		if (duset != NULL) {
			sbs_mgr->free_sbitsetc(duset);
		}
	}

	TMAP_ITER<IR*, IR*> map_iter;
	IR * pr;
	for (IR * x = delegate2pr.get_first(map_iter, &pr);
		 x != NULL; x = delegate2pr.get_next(map_iter, &pr)) {
		m_ru->free_irs(pr);
	}
}


//Determine whether the memory reference is same array or
//definitly different array.
UINT IR_RP::analyze_array_status(IR const* ref1, IR const* ref2)
{
	if (IR_type(ref1) != IR_ARRAY || IR_type(ref2) != IR_ARRAY) {
		return RP_UNKNOWN;
	}

	IR const* base1 = ARR_base(ref1);
	IR const* base2 = ARR_base(ref2);
	if (IR_type(base1) == IR_LDA && IR_type(base2) == IR_LDA) {
		IR const* b1 = LDA_base(base1);
		IR const* b2 = LDA_base(base2);
		if (IR_type(b1) == IR_ID && IR_type(b2) == IR_ID) {
			if (ID_info(b1) == ID_info(b2)) { return RP_SAME_ARRAY; }
			return RP_DIFFERENT_ARRAY;
		}
		return RP_UNKNOWN;
	}

	IS_TRUE0(base1->is_ptr(m_dm) && base2->is_ptr(m_dm));

	IS_TRUE0(m_gvn);

	VN const* vn1 = m_gvn->map_ir2vn(base1);
	VN const* vn2 = m_gvn->map_ir2vn(base2);
	if (vn1 == NULL || vn2 == NULL) { return RP_UNKNOWN; }
	if (vn1 == vn2) { return RP_SAME_ARRAY; }
	return RP_DIFFERENT_ARRAY;
}


//This function perform the rest work of scan_bb().
void IR_RP::check_and_remove_invalid_exact_occ(LIST<IR*> & exact_occ_list)
{
	C<IR*> * ct, * nct;
	for (exact_occ_list.get_head(&ct), nct = ct; ct != NULL; ct = nct) {
		IR * occ = C_val(ct);
		exact_occ_list.get_next(&nct);

		MD const* md = occ->get_ref_md();
		IS_TRUE0(md && md->is_exact());

		//We record all MD that are not suit for promotion, and perform
		//the rest job here that remove all related OCC in exact_list.
		//The MD of promotable candidate must not overlapped each other.
		if (m_dont_promot.is_overlap(md)) {
			exact_occ_list.remove(ct);
		}
	}
}


void IR_RP::build_dep_graph(TMAP<MD const*, IR*> & exact_access,
							TTAB<IR*> & inexact_access,
							LIST<IR*> & exact_occ_list)
{

}


bool IR_RP::try_promote(LI<IR_BB> const* li,
						IR_BB * exit_bb,
						IR_ITER & ii,
						TAB_ITER<IR*> & ti,
						TMAP<MD const*, IR*> & exact_access,
						TTAB<IR*> & inexact_access,
						LIST<IR*> & exact_occ_list)
{
	IS_TRUE0(li && exit_bb);
	exact_access.clean();
	inexact_access.clean();
	exact_occ_list.clean();
	m_dont_promot.clean();
	bool change = false;

	for (INT i = LI_bb_set(li)->get_first();
		 i != -1; i = LI_bb_set(li)->get_next(i)) {
		IR_BB * bb = m_ru->get_bb(i);
		IS_TRUE0(bb && m_cfg->get_vertex(IR_BB_id(bb)));
		if (bb->is_bb_has_return()) {
			return false;
		}
		if (!scan_bb(bb, li, exact_access, inexact_access,
					 exact_occ_list, ii)) {
			return false;
		}
	}

	//dump_access(exact_access, m_dm);
	//dump_occ_list(exact_occ_list, m_dm);
	//dump_access2(inexact_access, m_dm);

	IR_BB * preheader = NULL;
	if (exact_access.get_elem_count() != 0 ||
		inexact_access.get_elem_count() != 0) {
		preheader = ::find_and_insert_prehead(li, m_ru, m_is_insert_bb, false);
		IS_TRUE0(preheader);
		IR const* last = IR_BB_last_ir(preheader);
		if (last != NULL && last->is_call()) {
			preheader = ::find_and_insert_prehead(li, m_ru,
												m_is_insert_bb, true);
			IS_TRUE0(preheader);
			IS_TRUE0(IR_BB_last_ir(preheader) == NULL);
		}
	}

	if (exact_access.get_elem_count() != 0) {
		#ifdef _DEBUG_
		BITSET visit;
		for (IR * x = exact_occ_list.get_head();
			 x != NULL; x = exact_occ_list.get_next()) {
			IS_TRUE0(!visit.is_contain(IR_id(x)));
			visit.bunion(IR_id(x));
		}
		#endif

		IS_TRUE0(exact_occ_list.get_elem_count() != 0);
		check_and_remove_invalid_exact_occ(exact_occ_list);
		change |= promote_exact_access(li, ii, ti, preheader, exit_bb,
							 exact_access, exact_occ_list);
	}

	if (inexact_access.get_elem_count() != 0) {
		build_dep_graph(exact_access, inexact_access, exact_occ_list);
		change |= promote_inexact_access(li, preheader, exit_bb, inexact_access, ii, ti);
	}
	return change;
}


bool IR_RP::evaluable_scalar_replace(SLIST<LI<IR_BB> const*> & worklst)
{
	//Record the map between MD and ARRAY access expression.
	TMAP<MD const*, IR*> access;

	TMAP<MD const*, IR*> exact_access;
	TTAB<IR*> inexact_access;
	LIST<IR*> exact_occ_list;
	IR_ITER ii;
	TAB_ITER<IR*> ti;
	bool change = false;
	while (worklst.get_elem_count() > 0) {
		LI<IR_BB> const* x = worklst.remove_head();
		IR_BB * exit_bb = find_single_exit_bb(x);
		if (exit_bb != NULL) {
			//If we did not find a single exit bb, this loop is nontrivial.
			change |= try_promote(x, exit_bb, ii, ti, exact_access,
								  inexact_access, exact_occ_list);
		}

		x = LI_inner_list(x);
		while (x != NULL) {
			worklst.append_tail(x);
			x = LI_next(x);
		}
	}
	return change;
}


//Perform scalar replacement of aggregates and array.
bool IR_RP::perform(OPT_CTX & oc)
{
	START_TIMER_AFTER();
	m_ru->check_valid_and_recompute(&oc, OPT_DU_CHAIN, OPT_LOOP_INFO,
									OPT_DU_REF, OPT_UNDEF);
	//compute_liveness();

	LI<IR_BB> const* li = m_cfg->get_loop_info();
	if (li == NULL) { return false; }

	SMEM_POOL * cspool = smpool_create_handle(sizeof(SC<LI<IR_BB> const*>),
											  MEM_CONST_SIZE);
	SLIST<LI<IR_BB> const*> worklst(cspool);
	while (li != NULL) {
		worklst.append_tail(li);
		li = LI_next(li);
	}

	if (m_gvn == NULL) {
		//Need gvn.
		return false;
	}

	if (!m_gvn->is_valid()) {
		m_gvn->reperform(oc);
	}

	bool change = evaluable_scalar_replace(worklst);
	if (change) {
		//DU reference and du chain has maintained.
		IS_TRUE0(m_du->verify_du_ref());
		IS_TRUE0(m_du->verify_du_chain());

		OPTC_is_reach_def_valid(oc) = false;
		OPTC_is_avail_reach_def_valid(oc) = false;
		OPTC_is_live_expr_valid(oc) = false;

		//Enforce followed pass to recompute gvn.
		m_gvn->set_valid(false);
	}

	if (m_is_insert_bb) {
		OPTC_is_cdg_valid(oc) = false;
		OPTC_is_dom_valid(oc) = false;
		OPTC_is_pdom_valid(oc) = false;
		OPTC_is_rpo_valid(oc) = false;
		//Loop info is unchanged.
	}

	//build_lt();
	//dump_mdlt();
	smpool_free_handle(cspool);
	END_TIMER_AFTER(get_opt_name());
	return change;
}
//END IR_RP
