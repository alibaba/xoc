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
#ifndef _IR_SSA_H_
#define _IR_SSA_H_

class IR_SSA_MGR;

//Dominace Frontier manager
class DF_MGR {
	IR_SSA_MGR * m_ssa_mgr;
	BITSET_MGR m_bs_mgr;
	SVECTOR<BITSET*> m_df_vec;
public:
	DF_MGR(IR_SSA_MGR * sm);
	void clean();
	void build(DGRAPH & g);
	void dump(DGRAPH & g);

	//Return the BB set controlled by bbid.
	BITSET const* get_df_ctrlset_c(UINT bbid) const
	{ return m_df_vec.get(bbid); }

	//Get the BB set controlled by v.
	BITSET * get_df_ctrlset(VERTEX const* v);

	inline void rebuild(DGRAPH & g) { clean(); build(g); }
};


//SSA_GRAPH
class SSA_GRAPH : GRAPH {
	REGION * m_ru;
	IR_SSA_MGR * m_ssa_mgr;
	TMAP<UINT, VP*> m_vdefs;
public:
	SSA_GRAPH(REGION * ru, IR_SSA_MGR * ssamgr);
	void dump(IN CHAR const* name = NULL, bool detail = true);
};


typedef SVECTOR<SVECTOR<VP*>*> BB2VP;


//Perform SSA based optimizations.
class IR_SSA_MGR : public IR_OPT {
protected:
	REGION * m_ru;
	SMEM_POOL * m_vp_pool;
	DT_MGR * m_dm;
	IR_CFG * m_cfg;
	SEG_MGR * m_seg_mgr;
	PASS_MGR * m_pm; //Pass Manager.
	bool m_is_ssa_construct;
	UINT m_vp_count;

	//Set true to indicate SSA optimization must maintain DU chains which
	//built by IR_DU_MGR.
	bool m_need_maintain_du_chain;

	IR_ITER m_iter; //for tmp use.

	//Record versions for each PRs.
	UINT2VPVEC m_map_prno2vp_vec;

	//Record version stack during renaming.
	UINT2VPSTACK m_map_prno2stack;
	SVECTOR<VP*> m_vp_vec;
	SVECTOR<UINT> m_max_version; //record version number counter for pr.

	//Record the duplicated IR* to each prno.
	//Be used to generate phi for each prno.
	SVECTOR<IR*> m_prno2ir;

protected:
	inline void init(REGION * ru, PASS_MGR * pm)
	{
		if (m_vp_pool != NULL) { return; }

		IS_TRUE0(ru && pm);
		m_ru = ru;

		m_dm = ru->get_dm();
		IS_TRUE0(m_dm);

		m_pm = pm;
		IS_TRUE0(ru->get_sbs_mgr());
		m_seg_mgr = ru->get_sbs_mgr()->get_seg_mgr();
		IS_TRUE0(m_seg_mgr);

		m_cfg = ru->get_cfg();
		IS_TRUE(m_cfg, ("cfg is not available."));

		m_vp_count = 1;
		m_is_ssa_construct = false;
		m_need_maintain_du_chain = true;
		m_vp_pool = smpool_create_handle(sizeof(VP)*2, MEM_CONST_SIZE);
	}

	void clean()
	{
		m_ru = NULL;
		m_dm = NULL;
		m_pm = NULL;
		m_seg_mgr = NULL;
		m_cfg = NULL;
		m_vp_count = 1;
		m_is_ssa_construct = false;
		m_need_maintain_du_chain = true;
		m_vp_pool = NULL;
	}
	void clean_pr_ssainfo();
	void construct_du_chain();
	void clean_prno2stack();
	void collect_defed_pr(IN IR_BB * bb, OUT SBITSET & mustdef_pr);
	void compute_effect(IN OUT BITSET & effect_prs,
						IN BITSET & defed_prs,
						IN IR_BB * bb,
						IN PRDF & live_mgr,
						IN SVECTOR<BITSET*> & pr2defbb);

	void destruct_bb(IR_BB * bb, IN OUT bool & insert_stmt_after_call);
	void destruction_in_dom_tree_order(IR_BB * root, GRAPH & domtree);

	void handle_bb(IR_BB * bb, IN SBITSET & defed_prs,
				   IN OUT BB2VP & bb2vp, IN LIST<IR*> & lst);

	SSTACK<VP*> * map_prno2vpstack(UINT prno);
	inline IR * map_prno2ir(UINT prno)
	{ return m_prno2ir.get(prno); }

	VP * new_vp()
	{
		IS_TRUE(m_vp_pool != NULL, ("not init"));
		VP * p = (VP*)smpool_malloc_h_const_size(sizeof(VP), m_vp_pool);
		IS_TRUE0(p);
		memset(p, 0, sizeof(VP));
		return p;
	}

	void refine_phi(LIST<IR_BB*> & wl);
	void rename_dfs(IN BITSET & effect_prs);
	void rename(SBITSET & effect_prs,
				SVECTOR<SBITSET*> & defed_prs_vec,
				GRAPH & domtree);
	void rename_bb(IR_BB * bb);
	void rename_in_dom_tree_order_dfs(IR_BB * bb, GRAPH & dtree,
									  BITSET & effect_prs);
	void rename_in_dom_tree_order(IR_BB * root, GRAPH & dtree,
								  SVECTOR<SBITSET*> & defed_prs_vec);

	void strip_version_for_bblist();
	void strip_version_for_all_vp();
	bool strip_phi(IR * phi, C<IR*> * phict);
	void strip_specified_vp(VP * vp);
	void strip_stmt_version(IR * stmt, BITSET & visited);

	void place_phi_for_pr(UINT prno, IN LIST<IR_BB*> * defbbs,
						  DF_MGR & dfm, BITSET & visited, LIST<IR_BB*> & wl);
	void place_phi(OPT_CTX & oc, IN DF_MGR & dfm, IN OUT SBITSET & effect_prs,
				   SDBITSET_MGR & bs_mgr, SVECTOR<SBITSET*> & defed_prs_vec,
				   LIST<IR_BB*> & wl);

public:
	IR_SSA_MGR(REGION * ru, PASS_MGR * pm) { clean(); }
	~IR_SSA_MGR() { destroy(false); }

	void build_df(OUT DF_MGR & dfm);

	//is_reinit: this function is invoked in reinit().
	void destroy(bool is_reinit)
	{
		if (m_vp_pool == NULL) { return; }

		IS_TRUE(!m_is_ssa_construct,
			("Still in ssa mode, you should out of SSA before the destruction."));

		for (INT i = 0; i <= m_map_prno2vp_vec.get_last_idx(); i++) {
			SVECTOR<VP*> * vpv = m_map_prno2vp_vec.get(i);
			if (vpv != NULL) { delete vpv; }
		}

		clean_prno2stack();

		for (INT i = 0; i <= m_vp_vec.get_last_idx(); i++) {
			VP * v = m_vp_vec.get(i);
			if (v != NULL) { v->destroy(); }
		}

		if (is_reinit) {
			m_map_prno2vp_vec.clean();
			m_vp_vec.clean();
			m_max_version.clean();
			m_prno2ir.clean();
		}

		//Do not free irs in m_prno2ir.
		smpool_free_handle(m_vp_pool);
		m_vp_pool = NULL;
	}

	void destruction(DOM_TREE & domtree);
	void destruction_in_bblist_order();
	void dump();
	void dump_all_vp(bool have_renamed);
	CHAR * dump_vp(IN VP * v, OUT CHAR * buf);
	void dump_ssa_graph(CHAR * name = NULL);

	void construction(OPT_CTX & oc, REGION * ru);
	void construction(OPT_CTX & oc, DOM_TREE & domtree);
	UINT count_mem();

	inline SVECTOR<VP*> const* get_vp_vec() const { return &m_vp_vec; }
	inline VP * get_vp(UINT id) const { return m_vp_vec.get(id); }

	IR * init_vp(IN IR * ir);
	void insert_phi(UINT prno, IN IR_BB * bb);

	//Return true if PR ssa is constructed.
	bool is_ssa_construct() const { return m_is_ssa_construct; }

	//Return true if phi is redundant.
	bool is_redundant_phi(IR const* phi, OUT IR ** common_def) const;

	//Allocate VP and ensure it is unique according to 'version' and 'prno'.
	VP * new_vp(UINT prno, UINT version)
	{
		IS_TRUE0(prno > 0);
		SVECTOR<VP*> * vec = m_map_prno2vp_vec.get(prno);
		if (vec == NULL) {
			vec = new SVECTOR<VP*>();
			m_map_prno2vp_vec.set(prno, vec);
		}

		VP * v = vec->get(version);
		if (v != NULL) {
			return v;
		}

		IS_TRUE(m_seg_mgr, ("SSA manager is not initialized"));
		v = new_vp();
		v->init_no_clean(m_seg_mgr);
		VP_prno(v) = prno;
		VP_ver(v) = version;
		SSA_id(v) = m_vp_count++;
		SSA_def(v) = NULL;
		vec->set(version, v);
		m_vp_vec.set(SSA_id(v), v);
		return v;
	}


	//Allocate SSAINFO for specified PR indicated by 'prno'.
	inline SSAINFO * new_ssainfo(UINT prno)
	{
		IS_TRUE0(prno > 0);
		return (SSAINFO*)new_vp(prno, 0);
	}

	void reinit(REGION * ru, PASS_MGR * pm)
	{
		destroy(true);
		init(ru, pm);
	}

	//Set current PR to be in SSA form or not.
	//This flag will direct the behavior of optimizations.
	//If SSA constructed, dDU mananger will not compute any information for PR.
	void set_ssa_construct(bool construct)
	{ m_is_ssa_construct = construct; }

	bool verify_phi_sanity(bool is_vpinfo_avail);
	bool verify_vp_prno();
	bool verify_ssainfo();

	virtual CHAR const* get_opt_name() const
	{ return "SSA Optimization Manager"; }

	OPT_TYPE get_opt_type() const { return OPT_SSA_MGR; }

	bool perform_in_ssa_mode(OPT_CTX & oc, REGION * ru);
	bool perform_ssa_opt(OPT_CTX & oc);
};
#endif
