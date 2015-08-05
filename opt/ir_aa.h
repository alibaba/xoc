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
#ifndef _IR_ALIAS_ANALYSIS_H_
#define _IR_ALIAS_ANALYSIS_H_

class IR_CFG;

//PT_PAIR
#define PP_id(pp)		((pp)->id)
#define PP_from(pp)		((pp)->from)
#define PP_to(pp)		((pp)->to)
class PT_PAIR {
public:
	UINT id;
	MD const* from;
	MD const* to;
};


//PT_PAIR_SET
typedef BITSET PT_PAIR_SET;


//MD Addendum
#define MDA_md(m)		((m)->md)
#define MDA_mds(m)		((m)->mds)
class MDA {
public:
	MD const* md; //record single MD for IR such like, PR, LD, ST, ID.
	MD_SET * mds; //record multpile MD for IR such like, ILD, IST, ARRAY.
};


//PP_SET_MGR
class PP_SET_MGR {
protected:
	SMEM_POOL * m_pool;
	SLIST<PT_PAIR_SET*> m_free_pp_set;
	SLIST<PT_PAIR_SET*> m_pp_set_list;
public:
	PP_SET_MGR()
	{
		m_pool = smpool_create_handle(sizeof(SC<PT_PAIR_SET*>) * 4,
									  MEM_CONST_SIZE);
		m_free_pp_set.set_pool(m_pool);
		m_pp_set_list.set_pool(m_pool);
	}

	~PP_SET_MGR()
	{
		SC<PT_PAIR_SET*> * sct;
		for (PT_PAIR_SET * pps = m_pp_set_list.get_head(&sct);
			 pps != NULL; pps = m_pp_set_list.get_next(&sct)) {
			delete pps;
		}
		smpool_free_handle(m_pool);
	}

	UINT count_mem();

	void free(PT_PAIR_SET * pps)
	{
		pps->clean();
		m_free_pp_set.append_head(pps);
	}

	inline PT_PAIR_SET * new_pps()
	{
		PT_PAIR_SET * pps = m_free_pp_set.remove_head();
		if (pps == NULL) {
			pps = new PT_PAIR_SET();
			m_pp_set_list.append_head(pps);
		}
		return pps;
	}
};


//PT_PAIR_MGR
class PT_PAIR_MGR {
	TMAP<UINT, TMAP<UINT, PT_PAIR*>*>  m_from_tmap;
	SVECTOR<PT_PAIR*> m_id2pt_pair;
	SMEM_POOL * m_pool_pt_pair; //pool of PT_PAIR
	SMEM_POOL * m_pool_tmap; //pool of TMAP<UINT, PT_PAIR*>
	UINT m_pp_count;

	inline PT_PAIR * xmalloc_pt_pair()
	{
		PT_PAIR * p = (PT_PAIR*)smpool_malloc_h_const_size(sizeof(PT_PAIR),
															m_pool_pt_pair);
		IS_TRUE0(p);
		memset(p, 0, sizeof(PT_PAIR));
		return p;
	}

	inline TMAP<UINT, PT_PAIR*> * xmalloc_tmap()
	{
		TMAP<UINT, PT_PAIR*> * p =
			(TMAP<UINT, PT_PAIR*>*)smpool_malloc_h_const_size(
												sizeof(TMAP<UINT, PT_PAIR*>),
												m_pool_tmap);
		IS_TRUE0(p);
		memset(p, 0, sizeof(TMAP<UINT, PT_PAIR*>));
		return p;
	}
public:
	PT_PAIR_MGR()
	{
		m_pool_pt_pair = NULL;
		m_pool_tmap = NULL;
		init();
	}
	~PT_PAIR_MGR() { destroy();	}

	void init()
	{
		if (m_pool_pt_pair != NULL) { return; }
		m_pp_count = 1;
		m_pool_pt_pair = smpool_create_handle(sizeof(PT_PAIR),
											MEM_CONST_SIZE);
		m_pool_tmap = smpool_create_handle(sizeof(TMAP<UINT, PT_PAIR*>),
											MEM_CONST_SIZE);
	}

	void destroy()
	{
		if (m_pool_pt_pair == NULL) { return; }

		TMAP_ITER<UINT, TMAP<UINT, PT_PAIR*>*> ti;
		TMAP<UINT, PT_PAIR*> * v = NULL;
		for (m_from_tmap.get_first(ti, &v);
			 v != NULL; m_from_tmap.get_next(ti, &v)) {
			v->destroy();
		}

		smpool_free_handle(m_pool_pt_pair);
		smpool_free_handle(m_pool_tmap);
		m_pool_pt_pair = NULL;
		m_pool_tmap = NULL;
	}

	inline void clobber()
	{
		destroy();
		m_from_tmap.clean();
		m_id2pt_pair.clean();
	}

	//Get PT PAIR from 'id'.
	PT_PAIR * get(UINT id) { return m_id2pt_pair.get(id); }

	PT_PAIR * add(MD const* from, MD const* to);

	UINT count_mem() const;
};


//IRAA_CTX
#define AC_is_mds_mod(c)		((c)->u1.s1.is_mds_modify)
#define AC_is_lda_base(c)		((c)->u1.s1.is_lda_base)
#define AC_has_comp_lda(c)		((c)->u1.s1.has_comp_lda)
#define AC_comp_pt(c)			((c)->u1.s1.comp_pt)
class AA_CTX {
public:
	union {
		struct {
			//Transfer flag bottom up to indicate whether new
			//MDS generate while processing kids.
			UINT is_mds_modify:1; //Analysis context for IR_AA.

			/* Transfer flag top down to indicate the Parent or Grandfather
			IR (or Parent of grandfather ...) is IR_LDA/IR_ARRAY.
			This flag tell the current process whether if we are processing
			the base of LDA/ARRAY. */
			UINT is_lda_base:1; //Set true to indicate we are
								//analysing the base of LDA.

			/* Transfer flag bottom up to indicate whether we has taken address
			of ID.
			e.g: If p=q,q->x; We would propagate q and get p->x.
				But if p=&a; We should get p->a, with offset is 0.
				Similarly, if p=(&a)+n+m+z; We also get p->a,
				but with offset is INVALID. */
			UINT has_comp_lda:1;

			/* Transfer flag top down to indicate that we need
			current function to compute the MD that the IR
			expression may pointed to.
			e.g: Given (p+1), we want to know the expression pointed to.
			Presumedly, p->&a[0], we can figure out MD that dereferencing
			the expression *(p+1) is a[1]. */
			UINT comp_pt:1;
		} s1;
		UINT i1;
	} u1;

	AA_CTX() { clean(); }
	AA_CTX(AA_CTX const& ic) { copy(ic); }

	inline void copy(AA_CTX const& ic) { u1.i1 = ic.u1.i1; }

	//Only copy top down flag.
	inline void copy_top_down_flag(AA_CTX const& ic)
	{
		AC_comp_pt(this) = AC_comp_pt(&ic);
		AC_is_lda_base(this) = AC_is_lda_base(&ic);
	}

	void clean() { u1.i1 = 0; }

	//Clean these flag when processing each individiual IR trees.
	inline void clean_bottom_up_flag()
	{
		AC_has_comp_lda(this) = 0;
		AC_is_mds_mod(this) = 0;
	}

	//Collect the bottom-up flag and use them to direct parent action.
	//Clean these flag when processing each individiual IR trees.
	inline void copy_bottom_up_flag(AA_CTX const& ic)
	{
		AC_has_comp_lda(this) = AC_has_comp_lda(&ic);
		AC_is_mds_mod(this) = AC_is_mds_mod(&ic);
	}
};


typedef TMAP<UINT, MD*> VARID2MD;
typedef TMAP<IR const*, MD*> IR2HEAPOBJ;


/* Alias Analysis.
1. Compute the Memory Address Set for IR expression.
2. Compute the POINT TO Set for individual memory address.

NOTICE:
Heap is a unique object. That means the whole
HEAP is modified/referrenced if a LOAD/STORE operates
MD that describes variable belongs to HEAP. */
class IR_AA {
protected:
	IR_CFG * m_cfg;
	VAR_MGR * m_var_mgr;
	DT_MGR * m_dm;
	REGION * m_ru;
	MD_SYS * m_md_sys;
	SMEM_POOL * m_pool;
	MD_SET_MGR * m_mds_mgr; //MD_SET manager.
	MD_SET_HASH * m_mds_hash; //MD_SET hash table.

	//This is a dummy global variable.
	//It is used used as a placeholder if there
	//is not any effect global variable.
	MD * m_dummy_global;

	IR2HEAPOBJ m_ir2heapobj;
	SVECTOR<PT_PAIR_SET*> m_in_pp_set;
	SVECTOR<PT_PAIR_SET*> m_out_pp_set;
	VARID2MD m_varid2md;
	PT_PAIR_MGR m_pt_pair_mgr;
	BITSET m_is_visit;

	//This class contains those variables that can be referenced by
	//pointers (address-taken variables)
	MD_SET const* m_hashed_maypts; //initialized by init_may_ptset()

	//Analysis context. Record MD->MD_SET for each BB.
	SVECTOR<MD2MDS*> m_md2mds_vec;
	BITSET m_id2heap_md_map;
	MD2MDS m_unique_md2mds;

	//If the flag is true, flow sensitive analysis is performed.
	//Or perform flow insensitive.
	BYTE m_flow_sensitive:1;

	//If the flag is true, PR may have relative to multiple MD.
	BYTE m_is_pr_unique_for_same_no:1;

	//Set flag to indicate that SSA du chain has been available.
	BYTE m_is_ssa_available:1;

protected:
	MD * alloc_heap_obj(IR * ir);
	MD const* assign_string_const(IN IR * ir, IN OUT MD_SET * mds,
								  IN OUT AA_CTX * ic);
	MD const* assign_string_identifier(IN IR * ir, IN OUT MD_SET * mds,
									   IN OUT AA_CTX * ic);
	MD const* assign_id_md(IN IR * ir, IN OUT MD_SET * mds,
						   IN OUT AA_CTX * ic);
	MD const* assign_ld_md(IN IR * ir, IN OUT MD_SET * mds,
						   IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	MD const* assign_pr_md(IN IR * ir, IN OUT MD_SET * mds,
						   IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);

	MD const* alloc_id_md(IR * ir);
	MD const* alloc_ld_md(IR * ir);
	MD const* alloc_st_md(IR * ir);
	MD const* alloc_pr_md(IR * ir);
	MD const* alloc_phi_md(IR * phi);
	MD const* alloc_stpr_md(IR * ir);
	MD const* alloc_call_pr_md(IR * ir);
	MD const* alloc_setepr_md(IR * ir);
	MD const* alloc_getepr_md(IR * ir);
	MD const* alloc_string_md(IR * ir);

	bool evaluate_from_lda(IR const* ir);

	void init_entry_ptset(PT_PAIR_SET ** ptset_arr);
	void init_vpts(VAR * v, MD2MDS * mx, MD_ITER & iter);
	void infer_pt_arith(IR const* ir, IN OUT MD_SET & mds,
						IN OUT MD_SET & opnd0_mds,
						IN OUT AA_CTX * opnd0_ic,
						IN OUT MD2MDS * mx);
	void infer_stv(IN IR * ir, IN IR * rhs, MD const* lhs_md,
				   IN AA_CTX * ic, IN MD2MDS * mx);
	void infer_istv(IN IR * ir, IN AA_CTX * ic,
					IN MD2MDS * mx);

	void infer_array_inf(INT ofst, bool is_ofst_pred, UINT md_size,
						 MD_SET const& in, OUT MD_SET & out);
	void infer_array_ldabase(IR * ir, IR * array_base,
							 bool is_ofst_pred, UINT ofst,
							 OUT MD_SET & mds, IN OUT AA_CTX * ic,
							 IN OUT MD2MDS * mx);
	void infer_exp(IR * ir, IN OUT MD_SET & mds,
				   IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);

	void process_lda(IR * ir, IN OUT MD_SET & mds,
					 IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	void process_array_lda_base(IR * ir, IN OUT MD_SET & mds,
								IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	void process_cvt(IR const* ir, IN OUT MD_SET & mds,
					 IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	void process_getepr(IR * ir, IN OUT MD_SET & mds,
						IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	void process_setepr(IR * ir, IN MD2MDS * mx);
	void process_getepr(IR * ir, IN MD2MDS * mx);
	void process_ild(IR * ir, IN OUT MD_SET & mds,
					 IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	void process_pointer_arith(IR * ir, IN OUT MD_SET & mds,
							   IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	void process_array(IR * ir, IN OUT MD_SET & mds,
					   IN OUT AA_CTX * ic, IN OUT MD2MDS * mx);
	void process_cst(IR * ir, IN OUT MD_SET & mds,
					 IN OUT AA_CTX * ic);
	void process_st(IN IR * ir, IN OUT MD2MDS * mx);
	void process_stpr(IN IR * ir, IN MD2MDS * mx);
	void process_ist(IN IR * ir, IN OUT MD2MDS * mx);
	void process_phi(IN IR * ir, IN MD2MDS * mx);
	void process_call_sideeffect(IN OUT MD2MDS & mx, bool by_addr,
								MD_SET const& by_addr_mds);
	void process_call(IN IR * ir, IN OUT MD2MDS * mx);
	void process_return(IN IR * ir, IN MD2MDS * mx);
	void process_region(IR const* ir, IN MD2MDS * mx);
	void predict_array_ptrbase(IR * ir,
							   IR * array_base,
							   bool is_ofst_predicable,
							   UINT ofst,
							   OUT MD_SET & mds,
							   OUT bool mds_is_may_pt,
							   IN OUT AA_CTX * ic,
							   IN OUT MD2MDS * mx);
	void pt2md2mds(PT_PAIR_SET const& pps, IN PT_PAIR_MGR & pt_pair_mgr,
				   IN OUT MD2MDS * ctx);

	void revise_md_size(IN OUT MD_SET & mds, UINT size);

	void md2mds2pt(OUT PT_PAIR_SET & pps, IN PT_PAIR_MGR & pt_pair_mgr,
				   IN MD2MDS * mx);

	inline void * xmalloc(size_t size)
	{
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p);
		memset(p, 0, size);
		return p;
	}
public:
	explicit IR_AA(REGION * ru);
	virtual ~IR_AA();

	//Attemp to compute the type based may point to MD set.
	//Return true if this function find the point-to MD set, otherwise
	//return false.
	virtual MD * comp_point_to_via_type(IR const* ir)
	{ return NULL; }

	void clean();
	void clean_point_to(MD const* md, IN OUT MD2MDS & ctx)
	{ ctx.set(MD_id(md), NULL); }

	void compute_flowsenpt(LIST<IR_BB*> const& bbl);
	void compute_stmt_pt(IR_BB const* bb,
						 IN OUT MD2MDS * mx);
	void compute_flowinsenpt();
	void compute_may_point_to(IR * pointer, IN MD2MDS * mx, OUT MD_SET & mds);
	void compute_may_point_to(IR * pointer, OUT MD_SET & mds);
	UINT count_mem();
	UINT count_md2mds_mem();

	void dump_md2mds(IN MD2MDS * mx, bool dump_ptg);
	void dump_md2mds(MD const* md, IN MD2MDS * mx);
	void dump_ir_point_to(IN IR * ir, bool dump_kid, IN MD2MDS * mx);
	void dump_bb_ir_point_to(IR_BB * bb, bool dump_kid);
	void dump_all_ir_point_to(bool dump_kid);
	void dump_pps(PT_PAIR_SET & pps);
	void dump_bb_in_out_pt_set();
	void dump_all_md2mds(bool dump_pt_graph);
	void dump_may_point_to();

	void elem_bunion_pt(MD_SET const& mds, IN MD_SET & in_set, IN MD2MDS * mx);
	void elem_bunion_pt(MD_SET const& mds, IN MD * in_elem, IN MD2MDS * mx);
	void elem_copy_pt(MD_SET const& mds, IN MD_SET & in_set, IN MD2MDS * mx);
	void elem_copy_pt_maypts(MD_SET const& mds, IN MD2MDS * mx);
	void elem_copy_union_pt(MD_SET const& mds, IN MD_SET & pt_set,
							IN MD2MDS * mx);
	void elem_clean_pt(MD_SET const& mds, IN MD2MDS * mx);
	void elem_clean_exact_pt(MD_SET const& mds, IN MD2MDS * mx);

	CHAR const* get_opt_name() const { return "Alias Analysis"; }
	OPT_TYPE get_opt_type() const { return OPT_AA; }
	inline IR_CFG * get_cfg() const { return m_cfg; }

	inline PT_PAIR_SET * get_in_pp_set(IR_BB const* bb)
	{
		PT_PAIR_SET * pps = m_in_pp_set.get(IR_BB_id(bb));
		IS_TRUE(pps, ("IN set is not yet initialized for BB%d", IR_BB_id(bb)));
		return pps;
	}

	inline PT_PAIR_SET * get_out_pp_set(IR_BB const* bb)
	{
		PT_PAIR_SET * pps = m_out_pp_set.get(IR_BB_id(bb));
		IS_TRUE(pps, ("OUT set is not yet initialized for BB%d", IR_BB_id(bb)));
		return pps;
	}

	//For given MD2MDS, return the MD_SET that 'md' pointed to.
	//ctx: context of point-to analysis.
	inline MD_SET const* get_point_to(MD const* md, MD2MDS & ctx) const
	{
		IS_TRUE0(md);
		return get_point_to(MD_id(md), ctx);
	}

	//For given MD2MDS, return the MD_SET that 'md' pointed to.
	//ctx: context of point-to analysis.
	inline MD_SET const* get_point_to(UINT mdid, MD2MDS & ctx) const
	{ return ctx.get(mdid); }

	//Return the MemoryAddr set for 'ir' may describe.
	MD_SET const* get_may_addr(IR const* ir)
	{ return ir->get_ref_mds(); }

	//Return the MemoryAddr for 'ir' must be.
	MD const* get_must_addr(IR const* ir)
	{ return ir->get_ref_md(); }

	MD2MDS * get_unique_md2mds() { return &m_unique_md2mds; }

	bool is_flow_sensitive() const { return m_flow_sensitive; }
	bool is_valid_stmt_to_aa(IR * ir);

	bool is_all_mem(MD const* md) const
	{ return (MD_id(md) == MD_ALL_MEM) ? true : false; }

	bool is_heap_mem(MD const* md) const
	{ //return (MD_id(md) == MD_HEAP_MEM) ? true : false;
		return false;
	}

	bool is_pr_unique_for_same_no() const { return m_is_pr_unique_for_same_no; }
	void init_may_ptset();

	void clean_context(OPT_CTX & oc);
	void destroy_context(OPT_CTX & oc);

	void set_must_addr(IR * ir, MD const* md)
	{
		IS_TRUE0(ir != NULL && md);
		ir->set_ref_md(md, m_ru);
	}
	void set_may_addr(IR * ir, MD_SET const* mds)
	{
		IS_TRUE0(ir && mds && !mds->is_empty());
		ir->set_ref_mds(mds, m_ru);
	}

	//For given MD2MDS, set the point-to set to 'md'.
	//ctx: context of point-to analysis.
	inline void set_point_to(MD const* md, MD2MDS & ctx, MD_SET const* ptset)
	{
		IS_TRUE0(md && ptset);
		IS_TRUE(m_mds_hash->find(*ptset), ("ptset should be in hash"));
		ctx.set(MD_id(md), ptset);
	}

	//Set pointer points to 'target'.
	inline void set_point_to_unique_md(MD const* pointer, MD2MDS & ctx,
										MD const* target)
	{
		IS_TRUE0(pointer && target);
		MD_SET * tmp = m_mds_mgr->create();
		tmp->bunion(target);
		MD_SET const* hashed = m_mds_hash->append(*tmp);
		set_point_to(pointer, ctx, hashed);
		m_mds_mgr->free(tmp);
	}

	//Set pointer points to 'target_set'.
	inline void set_point_to_set(MD const* pointer, MD2MDS & ctx,
								MD_SET const& target_set)
	{
		IS_TRUE0(pointer);
		MD_SET const* hashed = m_mds_hash->append(target_set);
		set_point_to(pointer, ctx, hashed);
	}

	//Set pointer points to MD set by appending a new element 'target'.
	inline void set_point_to_set_add_md(MD const* pointer, MD2MDS & ctx,
										MD const* target)
	{
		MD_SET * tmp = m_mds_mgr->create();
		tmp->bunion(target);

		MD_SET const* pts = get_point_to(pointer, ctx);
		if (pts != NULL) {
			tmp->bunion(*pts);
		}

		MD_SET const* hashed = m_mds_hash->append(*tmp);
		set_point_to(pointer, ctx, hashed);
		m_mds_mgr->free(tmp);
	}

	//Set pointer points to MD set by appending a MD set.
	inline void set_point_to_set_add_set(MD const* pointer, MD2MDS & ctx,
										MD_SET const& set)
	{
		MD_SET const* pts = get_point_to(pointer, ctx);
		if (pts == NULL) {
			MD_SET const* hashed = m_mds_hash->append(set);
			set_point_to(pointer, ctx, hashed);
			return;
		}

		MD_SET * tmp = m_mds_mgr->create();
		tmp->copy(*pts);
		tmp->bunion(set);

		MD_SET const* hashed = m_mds_hash->append(*tmp);
		set_point_to(pointer, ctx, hashed);
		m_mds_mgr->free(tmp);
	}

	//Set 'md' points to whole memory.
	void set_pt_all_mem(MD const* md, OUT MD2MDS & ctx)
	{ set_point_to_set_add_md(md, ctx, m_md_sys->get_md(MD_ALL_MEM)); }

	//Set md points to whole global memory.
	void set_pt_global_mem(MD const* md, OUT MD2MDS & ctx)
	{ set_point_to_set_add_md(md, ctx, m_md_sys->get_md(MD_GLOBAL_MEM)); }

	void set_flow_sensitive(bool is_sensitive)
	{ m_flow_sensitive = is_sensitive; }

	void set_ssa_available(bool is_avail)
	{ m_is_ssa_available = is_avail; }

	//Function return the POINT-TO pair for each BB.
	//Only used in flow-sensitive analysis.
	MD2MDS * map_bb_to_md2mds(IR_BB const* bb) const
	{ return m_md2mds_vec.get(IR_BB_id(bb)); }

	//Function return the POINT-TO pair for each BB.
	//Only used in flow-sensitive analysis.
	inline MD2MDS * alloc_md2mds_for_bb(IR_BB const* bb)
	{
		MD2MDS * mx = m_md2mds_vec.get(IR_BB_id(bb));
		if (mx == NULL) {
			mx = (MD2MDS*)xmalloc(sizeof(MD2MDS));
			mx->init();
			m_md2mds_vec.set(IR_BB_id(bb), mx);
		}
		return mx;
	}

	void mark_may_alias(IN IR_BB * bb, IN MD2MDS * mx);
	bool verify_ir(IR * ir);
	bool verify();
	bool perform(OPT_CTX & oc);
};
#endif
