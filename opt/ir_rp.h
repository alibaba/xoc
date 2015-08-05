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
#ifndef _IR_REG_PROMOTION_H_
#define _IR_REG_PROMOTION_H_

//
//START MD_LT
//
#define MDLT_id(g)			((g)->id)
#define MDLT_md(g)			((g)->md)
#define MDLT_livebbs(g)		((g)->livebbs)

class MD_LT {
public:
	UINT id;
	MD * md;
	BITSET * livebbs;
};
//END MD_LT

class IR_GVN;

typedef HMAP<MD*, MD_LT*> MD2MDLT_MAP;


class DONT_PROMOT_TAB : public MD_SET {
	MD_SYS * m_md_sys;
public:
	DONT_PROMOT_TAB(REGION * ru)
	{
		IS_TRUE0(ru);
		m_md_sys = ru->get_md_sys();
	}

	inline bool is_overlap(MD const* md)
	{
		if (MD_SET::is_overlap(md)) { return true; }
		for (INT i = get_first(); i >= 0; i = get_next(i)) {
			MD const* t = m_md_sys->get_md(i);
			IS_TRUE0(t);
			if (t->is_overlap(md)) { return true; }
		}
		return false;
	}

	void dump()
	{
		if (g_tfile == NULL) { return; }
		fprintf(g_tfile, "\n==---- DUMP Dont Promot Tabel ----==\n");
		for (INT i = get_first(); i >= 0; i = get_next(i)) {
			MD const* t = m_md_sys->get_md(i);
			IS_TRUE0(t);
			t->dump();
		}

	}
};

#define RP_UNKNOWN				0
#define RP_SAME_ARRAY			1
#define RP_DIFFERENT_ARRAY		2
#define RP_SAME_OBJ				3
#define RP_DIFFERENT_OBJ		4

/* Perform Register Promotion.
Register Promotion combines multiple memory load of the
same memory location into one PR. */
class IR_RP : public IR_OPT {
protected:
	SVECTOR<BITSET*> m_livein_mds_vec;
	SVECTOR<BITSET*> m_liveout_mds_vec;
	SVECTOR<BITSET*> m_def_mds_vec;
	SVECTOR<BITSET*> m_use_mds_vec;
	BITSET_MGR m_bs_mgr;
	MD2MDLT_MAP * m_md2lt_map;
	REGION * m_ru;
	UINT m_mdlt_count;
	SMEM_POOL * m_pool;
	SMEM_POOL * m_ir_ptr_pool;
	MD_SYS * m_md_sys;
	MD_SET_MGR * m_mds_mgr;
	IR_CFG * m_cfg;
	DT_MGR * m_dm;
	IR_DU_MGR * m_du;
	IR_GVN * m_gvn;
	DONT_PROMOT_TAB m_dont_promot;
	bool m_is_insert_bb; //indicate if new bb inserted and cfg changed.

	UINT analyze_indirect_access_status(IR const* ref1, IR const* ref2);
	UINT analyze_array_status(IR const* ref1, IR const* ref2);
	void add_exact_access(OUT TMAP<MD const*, IR*> & exact_access,
						OUT LIST<IR*> & exact_occ_list,
						MD const* exact_md,
						IR * ir);
	void add_inexact_access(TTAB<IR*> & inexact_access, IR * ir);

	void build_dep_graph(TMAP<MD const*, IR*> & exact_access,
						 TTAB<IR*> & inexact_access,
						 LIST<IR*> & exact_occ_list);

	void check_and_remove_invalid_exact_occ(LIST<IR*> & exact_occ_list);
	void clobber_access_in_list(IR * ir,
								OUT TMAP<MD const*, IR*> & exact_access,
								OUT LIST<IR*> & exact_occ_list,
								OUT TTAB<IR*> & inexact_access);
	bool check_array_is_loop_invariant(IN IR * ir, LI<IR_BB> const* li);
	bool check_exp_is_loop_invariant(IN IR * ir, LI<IR_BB> const* li);
	bool check_indirect_access_is_loop_invariant(IN IR * ir,
												LI<IR_BB> const* li);
	void create_delegate_info(IR * delegate,
							TMAP<IR*, IR*> & delegate2pr,
							TMAP<IR*, SLIST<IR*>*> &
								delegate2has_outside_uses_ir_list);
	void compute_outer_def_use(IR * ref, IR * delegate,
								TMAP<IR*, DU_SET*> & delegate2def,
								TMAP<IR*, DU_SET*> & delegate2use,
								SDBITSET_MGR * sbs_mgr,
								LI<IR_BB> const* li);

	IR_BB * find_single_exit_bb(LI<IR_BB> const* li);
	void free_local_struct(TMAP<IR*, DU_SET*> & delegate2use,
						TMAP<IR*, DU_SET*> & delegate2def,
						TMAP<IR*, IR*> & delegate2pr,
						SDBITSET_MGR * sbs_mgr);

	void handle_access_in_body(IR * ref, IR * delegate,
							IR const* delegate_pr,
							TMAP<IR*, SLIST<IR*>*> const&
									delegate2has_outside_uses_ir_list,
							OUT TTAB<IR*> & restore2mem,
							OUT LIST<IR*> & fixup_list,
							TMAP<IR*, IR*> const& delegate2stpr,
							LI<IR_BB> const* li,
							IR_ITER & ii);
	void handle_restore2mem(TTAB<IR*> & restore2mem,
							TMAP<IR*, IR*> & delegate2stpr,
							TMAP<IR*, IR*> & delegate2pr,
							TMAP<IR*, DU_SET*> & delegate2use,
							TMAP<IR*, SLIST<IR*>*> &
									delegate2has_outside_uses_ir_list,
							TAB_ITER<IR*> & ti,
							IR_BB * exit_bb);
	void handle_prelog(IR * delegate, IR * pr,
						TMAP<IR*, IR*> & delegate2stpr,
						TMAP<IR*, DU_SET*> & delegate2def,
						IR_BB * preheader);
	bool has_loop_outside_use(IR const* stmt, LI<IR_BB> const* li);
	bool handle_array_ref(IN IR * ir,
						LI<IR_BB> const* li,
						OUT TMAP<MD const*, IR*> & exact_access,
						OUT LIST<IR*> & exact_occ_list,
						OUT TTAB<IR*> & inexact_access);
	bool handle_st_array(IN IR * ir,
						LI<IR_BB> const* li,
						OUT TMAP<MD const*, IR*> & exact_access,
						OUT LIST<IR*> & exact_occ_list,
						OUT TTAB<IR*> & inexact_access);
	bool handle_general_ref(IR * ir,
							LI<IR_BB> const* li,
							OUT TMAP<MD const*, IR*> & exact_access,
							OUT LIST<IR*> & exact_occ_list,
							OUT TTAB<IR*> & inexact_access);

	bool is_may_throw(IR * ir, IR_ITER & iter);
	bool may_be_global_ref(IR * ref)
	{
		MD const* md = ref->get_ref_md();
		if (md != NULL && md->is_global()) { return true; }

		MD_SET const* mds = ref->get_ref_mds();
		if (mds == NULL) { return false; }

		for (INT i = mds->get_first(); i >= 0; i = mds->get_next(i)) {
			MD const* md = m_md_sys->get_md(i);
			IS_TRUE0(md);
			if (md->is_global()) { return true; }
		}
		return false;
	}

	bool scan_opnd(IR * ir,
				LI<IR_BB> const* li,
				OUT TMAP<MD const*, IR*> & exact_access,
				OUT LIST<IR*> & exact_occ_list,
				OUT TTAB<IR*> & inexact_access,
				IR_ITER & ii);
	bool scan_res(IN IR * ir,
				LI<IR_BB> const* li,
				OUT TMAP<MD const*, IR*> & exact_access,
				OUT LIST<IR*> & exact_occ_list,
				OUT TTAB<IR*> & inexact_access);
	bool scan_bb(IN IR_BB * bb,
				LI<IR_BB> const* li,
				OUT TMAP<MD const*, IR*> & exact_access,
				OUT TTAB<IR*> & inexact_access,
				OUT LIST<IR*> & exact_occ_list,
				IR_ITER & ii);
	bool should_be_promoted(IR const* occ, LIST<IR*> & exact_occ_list);

	bool promote_inexact_access(LI<IR_BB> const* li, IR_BB * preheader,
								IR_BB * exit_bb, TTAB<IR*> & inexact_access,
								IR_ITER & ii, TAB_ITER<IR*> & ti);
	bool promote_exact_access(LI<IR_BB> const* li, IR_ITER & ii,
							TAB_ITER<IR*> & ti,
							IR_BB * preheader, IR_BB * exit_bb,
							TMAP<MD const*, IR*> & cand_list,
							LIST<IR*> & occ_list);

	void remove_redundant_du_chain(LIST<IR*> & fixup_list);
	void replace_use_for_tree(IR * oldir, IR * newir);

	bool evaluable_scalar_replace(SLIST<LI<IR_BB> const*> & worklst);

	bool try_promote(LI<IR_BB> const* li,
					IR_BB * exit_bb,
					IR_ITER & ii,
					TAB_ITER<IR*> & ti,
					TMAP<MD const*, IR*> & exact_access,
					TTAB<IR*> & inexact_access,
					LIST<IR*> & exact_occ_list);

	void * xmalloc(UINT size)
	{
		IS_TRUE0(m_pool != NULL);
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p != NULL);
		memset(p, 0, size);
		return p;
	}
public:
	IR_RP(REGION * ru, IR_GVN * gvn) : m_dont_promot(ru)
	{
		IS_TRUE0(ru != NULL);
		m_ru = ru;
		m_md_sys = ru->get_md_sys();
		m_cfg = ru->get_cfg();
		m_dm = ru->get_dm();
		m_du = ru->get_du_mgr();
		m_mds_mgr = ru->get_mds_mgr();
		m_gvn = gvn;
		m_is_insert_bb = false;

		UINT c = MAX(11, m_ru->get_md_sys()->get_num_of_md());
		m_md2lt_map = new MD2MDLT_MAP(c);
		m_mdlt_count = 0;
		m_pool = smpool_create_handle(2 * sizeof(MD_LT), MEM_COMM);
		m_ir_ptr_pool = smpool_create_handle(4 * sizeof(SC<IR*>),
											 MEM_CONST_SIZE);
	}

	virtual ~IR_RP()
	{
		delete m_md2lt_map;
		m_md2lt_map = NULL;
		smpool_free_handle(m_pool);
		smpool_free_handle(m_ir_ptr_pool);
	}

	void build_lt();

	void clean_livebbs();
	void compute_local_liveness(IR_BB * bb, IR_DU_MGR & du_mgr);
	void compute_global_liveness();
	void compute_liveness();

	BITSET * get_livein_mds(IR_BB * bb);
	BITSET * get_liveout_mds(IR_BB * bb);
	BITSET * get_def_mds(IR_BB * bb);
	BITSET * get_use_mds(IR_BB * bb);
	MD_LT * gen_mdlt(MD * md);

	void dump();
	void dump_mdlt();
	void dump_occ_list(LIST<IR*> & occs, DT_MGR * dm);
	void dump_access2(TTAB<IR*> & access, DT_MGR * dm);
	void dump_access(TMAP<MD const*, IR*> & access, DT_MGR * dm);

	//Return true if 'ir' can be promoted.
	//Note ir must be memory reference.
	virtual bool is_promotable(IR const* ir) const
	{
		IS_TRUE0(ir->is_memory_ref());
		//I think the reference that may throw is promotable.
		return !IR_has_sideeffect(ir);
	}

	virtual CHAR const* get_opt_name() const { return "Register Promotion"; }
	OPT_TYPE get_opt_type() const { return OPT_RP; }

	virtual bool perform(OPT_CTX & oc);
};
#endif
