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
#ifndef _IR_DU_
#define	_IR_DU_

/* Util Functions supplied by IR_DU_MGR
* These functions manipulate the reference of IR.
  IR may reference MD, or MD_SET, or both MD and MD_SET.

	compute_overlap_def_mds
	compute_overlap_use_mds
	collect_may_use
	collect_may_use_recur
	copy_ir_tree_du_info
	change_use
	change_def
	get_may_def
	get_must_use
	get_may_use
	get_must_def
	get_effect_ref
	get_effect_def_md
	get_exact_def_md
	get_effect_use_md
	get_exact_use_md
	get_exact_and_unique_def
	free_mds
	free_duset_and_refs
	map_call2mayuse
	is_may_def
	is_may_kill
	is_must_kill
	is_exact_unique_def
	is_call_may_def

* These functions manipulate the DU chain.

	build_du_chain
	free_duset_and_refs
	copy_duset
	get_du_c
	get_du_alloc
	free_du_info
	is_du_exist
	union_use
	union_useset
	union_def
	union_defset
	remove_du_chain
	remove_expired_du_chain
	remove_def
	remove_use_out_from_defset
	remove_def_out_from_useset
	remove_ir_out_from_du_mgr
*/

//Mapping from MD to IR list.
typedef  HMAP<UINT, SBITSETC*> MDID2IRLIST_MAP;

//Mapping from IR to index.
typedef HMAP<IR const*, UINT, HASH_FUNC_BASE2<IR const*> > IR2UINT;


class IR_DU_MGR;

//Mapping from MD to IR list, and to be responsible for
//allocating and destroy LIST<IR*> objects.
class MDID2IRS : public TMAP<UINT, SBITSETC*> {
	REGION * m_ru;
	MD_SYS * m_md_sys;
	DT_MGR * m_dm;
	IR_DU_MGR * m_du;
	SDBITSET_MGR * m_sbs_mgr;
	TMAP_ITER<UINT, SBITSETC*> m_iter;
	SBITSETC m_global_md;

	//Indicate if there exist stmt which only have MayDef.
	bool m_has_stmt_which_only_have_maydef;
public:
	explicit MDID2IRS(REGION * ru);
	~MDID2IRS();

	void append(MD const* md, IR const* ir)
	{
		IS_TRUE0(ir);
		append(MD_id(md), IR_id(ir));
	}
	void append(MD const* md, UINT irid)
	{
		append(MD_id(md), irid);
	}
	void append(UINT mdid, IR * ir)
	{
		IS_TRUE0(ir);
		append(mdid, IR_id(ir));
	}
	void append(UINT mdid, UINT irid);
	void append_to_global_md(UINT irid);
	void append_to_allmem_md(UINT irid);

	void clean();
	void dump();

	bool has_ineffect_def_stmt() const
	{ return m_has_stmt_which_only_have_maydef; }

	void set(UINT mdid, IR * ir);
	void set_mark_stmt_only_has_maydef()
	{ m_has_stmt_which_only_have_maydef = true; }
};


typedef SBITSETC REFSET;


/* Ud chains describe all of the might uses of the prior DEFINITION of md.
Du chains describe all effective USEs of once definition of md.
e.g:
		d1:a=   d2:a=   d3:a=
			  \   	|     	/
			      b = a
			  /            \
 			d4: =b         d5: =b
		Ud chains:  a use def d1,d2,d3 stmt
		Du chains:  b's value will be used by d4,d5 stmt
If ir is stmt, this class indicate the USE expressions set.
If ir is expression, this class indicate the DEF stmt set. */

//Mapping from IR to DU_SET.
typedef HMAP<IR*, DU_SET*> IR2DU_MAP;
typedef TMAP<IR const*, MD_SET const*> IR2MDS;


//Def|Use information manager.
#define COMP_EXP_RECOMPUTE				1 //Recompute DU
										  //completely needs POINT-TO info.
#define COMP_EXP_UPDATE_DU				2
#define COMP_EXP_COLLECT_MUST_USE		4

#define SOL_UNDEF					0
#define SOL_AVAIL_REACH_DEF			1  //available reach-definition.
#define SOL_REACH_DEF				2  //may reach-definition.
#define SOL_AVAIL_EXPR				4  //live expr.
#define SOL_RU_REF					8  //region's def/use mds.
#define SOL_REF						16 //referrenced mds.
class IR_DU_MGR {
	friend class MDID2IRS;
	friend class DU_SET;
protected:
	REGION * m_ru;
	DT_MGR * m_dm;
	IR_AA * m_aa;
	IR_CFG * m_cfg;
	MD_SYS * m_md_sys;
	SMEM_POOL * m_pool;
	MD_SET_MGR * m_mds_mgr;
	MD_SET_HASH * m_mds_hash;
	SDBITSET_MGR * m_sbs_mgr;

	/* Set to true if there is single map between
	PR and MD. If PR may corresponde to multiple
	MD, set it to false.

	e.g: If one use same pr with different type U8 and U32,
	there will have two mds refer to pr1(U8) and pr1(U32),
	MD2->pr1(U8), MD8->pr1(U32). */
	BYTE m_is_pr_unique_for_same_no:1;

	/* Set to true if stmt may have multiple referrence MD.
	The DU manager access the first result MD via
	get_must_use()/get_must_def(), and access the rest of MD via
	get_may_use()/get_may_def().

	NOTE: Build DU chain one must consider the may-referrence MD_SET
	if the flag is true.*/
	BYTE m_is_stmt_has_multi_result:1;

	/* Indicate whether compute the DU chain for PR.
	This flag is often set to false if PR SSA has constructed to reduce
	compilation time and memory consumption.
	If the flag is true, reach-def will not be computed. */
	BYTE m_is_compute_pr_du_chain:1;

	IR2MDS m_call2mayuse; //record call site mayuse MD_SET.

	CIR_ITER m_citer; //for tmp use.
	CIR_ITER m_citer2; //for tmp use.
	IR_ITER m_iter; //for tmp use.
	IR_ITER m_iter2; //for tmp use.
	MD_SET m_tmp_mds; //for tmp use.
	TMAP_ITER<MD*, MD*> m_tab_iter;

	//Used to cache overlapping MD_SET for specified MD.
	TMAP<MD const*, MD_SET const*> m_cached_overlap_mdset;

	//Indicate whether MD_SET is cached for specified MD.
	SBITSETC m_is_cached_mdset;

	//Used by DU chain.
	BITSET * m_is_init;
	MDID2IRS * m_md2irs;

	OPT_CTX * m_oc;

	/* Available reach-def computes the definitions
	which must be the last definition of result variable,
	but it may not reachable meanwhile.
	e.g:
		BB1:
		a=1  //S1
		*p=3
		a=4  //S2
		goto BB3

		BB2:
		a=2 //S3
		goto BB3

		BB3:
		f(a)

		Here we do not known where p pointed to.
		The available-reach-def of BB3 is {S1, S3}

	Compare to available reach-def, reach-def computes the definition
	which may-live at each BB.
	e.g:
		BB1:
		a=1  //S1
		*p=3
		goto BB3

		BB2:
		a=2 //S2
		goto BB3

		BB3:
		f(a)

		Here we do not known where p pointed to.
		The reach-def of BB3 is {S1, S2} */
	SVECTOR<DBITSETC*> m_bb_avail_in_reach_def; //avail reach-in def of IR STMT
	SVECTOR<DBITSETC*> m_bb_avail_out_reach_def; //avail reach-out def of IR STMT
	SVECTOR<DBITSETC*> m_bb_in_reach_def; //reach-in def of IR STMT
	SVECTOR<DBITSETC*> m_bb_out_reach_def; //reach-out def of IR STMT
	SVECTOR<DBITSETC*> m_bb_may_gen_def; //generated-def of IR STMT
	SVECTOR<DBITSETC*> m_bb_must_gen_def; //generated-def of IR STMT
	SVECTOR<DBITSETC*> m_bb_must_killed_def; //must-killed def of IR STMT
	SVECTOR<DBITSETC*> m_bb_may_killed_def; //may-killed def of IR STMT

	BVEC<DBITSETC*> m_bb_gen_ir_expr; //generate IR EXPR
	BVEC<DBITSETC*> m_bb_killed_ir_expr; //killed IR EXPR
	BVEC<DBITSETC*> m_bb_availin_ir_expr; //available in IR EXPR
	BVEC<DBITSETC*> m_bb_availout_ir_expr; //available out IR EXPR
protected:
	void add_overlapped_exact_md(OUT MD_SET * mds, MD const* x,
								 MD_ITER & mditer);

	bool build_local_du_chain(IR const* stmt, IR const* exp,
							MD const* expmd, DU_SET * expdu,
							C<IR*> * ct);
	void build_chain_for_md(IR const* exp, MD const* expmd, DU_SET * expdu);
	void build_chain_for_mds(IR const* exp,  MD const* expmd,
							MD_SET const& expmds, DU_SET * expdu);
	void build_chain_for_must(IR const* exp, MD const* expmd, DU_SET * expdu);

	bool check_call_mds_in_hash();
	bool check_is_truely_dep(MD const* mustdef, MD_SET const* maydef,
							IR const* def, IR const* use);
	UINT check_is_local_killing_def(IR const* stmt, IR const* exp,
									C<IR*> * expct);
	UINT check_is_nonlocal_killing_def(IR const* stmt, IR const* exp);
	inline bool can_be_live_expr_cand(IR const* ir) const;
	void comp_array_lhs_ref(IR * ir);
	MD_SET * compute_istore_memaddr(IN IR * ir,
									IN OUT MD_SET & ret_mds, UINT flag);
	void compute_exp(IR * ir, MD_SET * ret_mds, UINT flag);
	void check_and_build_chain_recur(IR * stmt, IR * exp, C<IR*> * ct);
	void check_and_build_chain(IR * stmt, C<IR*> * ct);
	void collect_must_use_for_lda(IR const* lda, OUT MD_SET * ret_mds);

	void comp_mustdef_for_region(IR const* ir, MD_SET * bb_mustdefmds,
								MD_ITER & mditer);
 	void comp_bb_maydef(IR const* ir, MD_SET * bb_maydefmds,
						DBITSETC * maygen_stmt);
	void comp_bb_mustdef(IR const* ir, MD_SET * bb_mustdefmds,
						DBITSETC * mustgen_stmt, MD_ITER & mditer);
	void comp_mustdef_maydef_mayuse(OUT SVECTOR<MD_SET*> * mustdef,
									OUT SVECTOR<MD_SET*> * maydef,
									OUT MD_SET * mayuse,
									UINT flag);

	bool for_avail_reach_def(UINT bbid, LIST<IR_BB*> & preds,
							 LIST<IR_BB*> * lst);
	bool for_reach_def(UINT bbid, LIST<IR_BB*> & preds, LIST<IR_BB*> * lst);
	bool for_avail_expr(UINT bbid, LIST<IR_BB*> & preds, LIST<IR_BB*> * lst);
	inline void * xmalloc(size_t size)
	{
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p);
		memset(p, 0, size);
		return p;
	}

	bool has_single_def_to_md(DU_SET const& defset, MD const* md) const;

	void init_md2irs(IR_BB * bb);
	void infer_region(IR * ir, bool ruinfo_avail, IN MD_SET * tmp);
	void infer_ist(IR * ir);
	void infer_st(IR * ir);
	void infer_stpr(IR * ir);
	void infer_phi(IR * ir);
	void infer_call(IR * ir, IN MD_SET * tmp, IN MD2MDS * mx);
	bool ir_is_exp(UINT irid);
	bool ir_is_stmt(UINT irid);

	void solve(DBITSETC const* expr_universe, UINT const flag);
	void reset_local(bool clean_member);
	void reset_global(bool clean_member);
	void reset_reach_in(bool clean_member);
	void reset_reach_out(bool clean_member);
	void update_def(IR * ir);
	void update_region(IR * ir);
public:
	explicit IR_DU_MGR(REGION * ru);
	~IR_DU_MGR();

	//Build DU chain : def->use.
	void build_du_chain(IR * def, IR * use)
	{
		IS_TRUE0(def && def->is_stmt() && use && use->is_exp());
		get_du_alloc(def)->add_use(use, *m_sbs_mgr);
		get_du_alloc(use)->add_def(def, *m_sbs_mgr);
	}

	/* Compute the MD_SET that might overlap ones which 'ir' defined.
	e.g: int A[100], there are two referrence of array A: A[i], A[j]
		A[i] might overlap A[j].

	'tmp': regard as input data, compute overlapped MD of its element.

	NOTE: Be careful 'mds' would be modified. */
	void compute_overlap_def_mds(IR * ir, bool recompute)
	{ compute_overlap_use_mds(ir, recompute); }

	void compute_overlap_use_mds(IR * ir, bool recompute);
	void collect_must_use(IR const* ir, OUT MD_SET & mustuse);
	void compute_gen_for_bb(IN IR_BB * bb, IN OUT DBITSETC & expr_univers,
							MD_SET & tmp);
	void compute_bb_du(IR_BB * bb, MD_SET & tmp1, MD_SET & tmp2);
	void comp_ref();
	void comp_kill(SVECTOR<MD_SET*> const* mustdefs,
				   SVECTOR<MD_SET*> const* maydefs,
				   UINT flag);
	void comp_ir_expr(OUT DBITSETC * expr_universe,
					  SVECTOR<MD_SET*> const* maydefmds);
	void compute_du_chain(IN OUT OPT_CTX & oc);
	void comp_ru_du(SVECTOR<MD_SET*> const* mustdefmds,
					SVECTOR<MD_SET*> const* maydefmds,
					MD_SET const* mayusemds);

	//Clean all DU-Chain and Defined/Used-MD reference info.
	//Return the DU structure if has to facilitate other free or destroy process.
	inline DU * free_duset_and_refs(IR * ir)
	{
		DU * du = ir->get_du();
		if (du == NULL) { return NULL; }

		if (DU_duset(du) != NULL) {
			//Free DU_SET back to SEG_MGR, or it will
			//complain and make an assertion.
			IS_TRUE0(m_sbs_mgr);
			m_sbs_mgr->free_sbitsetc(DU_duset(du));
			DU_duset(du) = NULL;
		}
		DU_mds(du) = NULL;
		DU_md(du) = NULL;
		return du;
	}

	//DU chain and Memory Object reference operation.
	void copy_ir_tree_du_info(IR * to, IR const* from, bool copy_du_info);

	//Count the memory usage to IR_DU_MGR.
	UINT count_mem();
	UINT count_mem_duset();
	UINT count_mem_local_data(DBITSETC * expr_univers,
							SVECTOR<MD_SET*> * maydef_mds,
							SVECTOR<MD_SET*> * mustdef_mds,
							MD_SET * mayuse_mds,
							MD_SET mds_arr_for_must[],
							MD_SET mds_arr_for_may[],
							UINT elemnum);

	//Collect must and may memory reference.
	void collect_may_use_recur(IR const* ir, MD_SET & may_use, bool comp_pr);

	//Collect may memory reference.
	void collect_may_use(IR const* ir, MD_SET & may_use, bool comp_pr);

	//DU chain operation.
	//Copy DU_SET from 'src' to 'tgt'. src and tgt must
	//both to be either stmt or expression.
	void copy_duset(IR * tgt, IR const* src)
	{
		//tgt and src should either both be stmt or both be exp.
		IS_TRUE0(!(tgt->is_stmt() ^ src->is_stmt()));
		DU_SET const* srcduinfo = get_du_c(src);
		if (srcduinfo == NULL) {
			DU_SET * tgtduinfo = tgt->get_duset();
			if (tgtduinfo != NULL) {
				tgtduinfo->clean(*m_sbs_mgr);
			}
			return;
		}

		DU_SET * tgtduinfo = get_du_alloc(tgt);
		tgtduinfo->copy(*srcduinfo, *m_sbs_mgr);
	}

	//DU chain operation.
	//Copy DU_SET from 'srcset' to 'tgt'.
	//If srcset is empty, then clean tgt's duset.
	inline void copy_duset(IR * tgt, DU_SET const* srcset)
	{
		if (srcset == NULL) {
			DU_SET * tgtduinfo = tgt->get_duset();
			if (tgtduinfo != NULL) {
				tgtduinfo->clean(*m_sbs_mgr);
			}
			return;
		}

		DU_SET * tgtduinfo = get_du_alloc(tgt);
		tgtduinfo->copy(*srcset, *m_sbs_mgr);
	}

	//Copy and maintain tgt DU chain.
	void copy_du_chain(IR * tgt, IR * src)
	{
		copy_duset(tgt, src);
		DU_SET const* from_du = get_du_c(src);

		DU_ITER di;
		for (UINT i = from_du->get_first(&di);
			 di != NULL; i = from_du->get_next(i, &di)) {
			IR const* ref = get_ir(i);
			//ref may be stmt or exp.

			DU_SET * ref_defuse_set = ref->get_duset();
			if (ref_defuse_set == NULL) { continue; }
			ref_defuse_set->add(IR_id(tgt), *m_sbs_mgr);
		}
	}

	/* DU chain operation.
	Change Def stmt from 'from' to 'to'.
	'to': copy to stmt's id.
	'from': copy from stmt's id.
	'useset': each element is USE, it is the USE expression set of 'from'.
	e.g:
		from->USE change to to->USE */
	void change_def(UINT to, UINT from, DU_SET * useset_of_to,
					DU_SET * useset_of_from, SDBITSET_MGR * m)
	{
		IS_TRUE0(get_ir(from)->is_stmt() && get_ir(to)->is_stmt() &&
				 useset_of_to && useset_of_from && m);
		DU_ITER di;
		for (UINT i = useset_of_from->get_first(&di);
			 di != NULL; i = useset_of_from->get_next(i, &di)) {
			IR const* exp = get_ir(i);
			IS_TRUE0(exp->is_memory_ref());

			DU_SET * defset = exp->get_duset();
			if (defset == NULL) { continue; }
			defset->diff(from, *m_sbs_mgr);
			defset->bunion(to, *m_sbs_mgr);
		}
		useset_of_to->bunion(*useset_of_from, *m);
		useset_of_from->clean(*m);
	}

	/* DU chain operation.
	Change Def stmt from 'from' to 'to'.
	'to': target stmt.
	'from': source stmt.
	e.g:
		from->USE change to to->USE */
	inline void change_def(IR * to, IR * from, SDBITSET_MGR * m)
	{
		IS_TRUE0(to && from && to->is_stmt() && from->is_stmt());
		DU_SET * useset_of_from = from->get_duset();
		if (useset_of_from == NULL) { return; }

		DU_SET * useset_of_to = get_du_alloc(to);
		change_def(IR_id(to), IR_id(from), useset_of_to, useset_of_from, m);
	}

	/* DU chain operation.
	Change Use expression from 'from' to 'to'.
	'to': indicate the target expression which copy to.
	'from': indicate the source expression which copy from.
	'defset': it is the DEF stmt set of 'from'.
	e.g:
		DEF->from change to DEF->to */
	void change_use(UINT to, UINT from, DU_SET * defset_of_to,
					DU_SET * defset_of_from, SDBITSET_MGR * m)
	{
		IS_TRUE0(get_ir(from)->is_exp() && get_ir(to)->is_exp() &&
				 defset_of_from && defset_of_to && m);
		DU_ITER di;
		for (UINT i = defset_of_from->get_first(&di);
			 di != NULL; i = defset_of_from->get_next(i, &di)) {
			IR * stmt = get_ir(i);
			IS_TRUE0(stmt->is_stmt());
			DU_SET * useset = stmt->get_duset();
			if (useset == NULL) { continue; }
			useset->diff(from, *m_sbs_mgr);
			useset->bunion(to, *m_sbs_mgr);
		}
		defset_of_to->bunion(*defset_of_from, *m);
		defset_of_from->clean(*m);
	}

	/* DU chain operation.
	Change Use expression from 'from' to 'to'.
	'to': indicate the exp which copy to.
	'from': indicate the expression which copy from.
	e.g:
		DEF->from change to DEF->to */
	inline void change_use(IR * to, IR const* from, SDBITSET_MGR * m)
	{
		IS_TRUE0(to && from && to->is_exp() && from->is_exp());
		DU_SET * defset_of_from = from->get_duset();
		if (defset_of_from == NULL) { return; }

		DU_SET * defset_of_to = get_du_alloc(to);
		change_use(IR_id(to), IR_id(from), defset_of_to, defset_of_from, m);
	}

	void destroy();
	void dump_mem_usage_for_set();
	void dump_mem_usage_for_ir_du_ref();
	void dump_set(bool is_bs = false);
	void dump_ir_ref(IN IR * ir, UINT indent);
	void dump_ir_list_ref(IN IR * ir, UINT indent);
	void dump_bb_ref(IN IR_BB * bb, UINT indent);
	void dump_ref(UINT indent = 4);
	void dump_du_chain();
	void dump_du_chain2();
	void dump_bb_du_chain2(UINT bbid);
	void dump_bb_du_chain2(IR_BB * bb);
	void dump_du_graph(CHAR const* name = NULL, bool detail = true);

	CHAR const* get_opt_name() const { return "DEF USE info manager"; }

	//DU chain operation.
	DU_SET * get_du_alloc(IR * ir);
	DU_SET const* get_du_c(IR const* ir) const //Get DU_SET.
	{
		DU * du = ir->get_du();
		if (du == NULL) { return NULL; }
		return DU_duset(du);
	}

	//Get set to BB.
	DBITSETC * get_may_gen_def(UINT bbid);
	DBITSETC * get_must_gen_def(UINT bbid);
	DBITSETC * get_avail_in_reach_def(UINT bbid);
	DBITSETC * get_avail_out_reach_def(UINT bbid);
	DBITSETC * get_in_reach_def(UINT bbid);
	DBITSETC * get_out_reach_def(UINT bbid);
	DBITSETC * get_must_killed_def(UINT bbid);
	DBITSETC * get_may_killed_def(UINT bbid);
	DBITSETC * get_gen_ir_expr(UINT bbid);
	DBITSETC * get_killed_ir_expr(UINT bbid);
	DBITSETC * get_availin_expr(UINT bbid);
	DBITSETC * get_availout_expr(UINT bbid);

	SDBITSET_MGR * get_sbs_mgr() { return m_sbs_mgr; }

	//Return the MustDef MD.
	MD const* get_must_def(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_stmt());
		return ir->get_ref_md();
	}

	MD const* get_effect_def_md(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_stmt());
		return ir->get_effect_ref();
	}

	//Return exact MD if ir defined.
	inline MD const* get_exact_def_md(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_stmt());
		MD const* md = get_effect_def_md(ir);
		if (md == NULL || !md->is_exact()) { return NULL; }
		return md;
	}

	//Return the MayDef MD set.
	MD_SET const* get_may_def(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_stmt());
		return ir->get_ref_mds();
	}

	//Return the MustUse MD.
	MD const* get_must_use(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_exp());
		return ir->get_effect_ref();
	}

	//Return the MayUse MD set.
	MD_SET const* get_may_use(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_exp());
		return ir->get_ref_mds();
	}

	//Return exact MD if ir defined.
	inline MD const* get_exact_use_md(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_exp());
		MD const* md = get_effect_use_md(ir);
		return md != NULL && md->is_exact() ? md : NULL;
	}

	MD const* get_effect_use_md(IR const* ir)
	{
		IS_TRUE0(ir && ir->is_exp());
		return ir->get_effect_ref();
	}

	IR const* get_exact_and_unique_def(IR const* exp);
	IR * get_ir(UINT irid);

	void free_duset_and_refmds();
	void free_du_info();

	bool is_compute_pr_du_chain() const { return m_is_compute_pr_du_chain; }
	inline bool is_du_exist(IR const* def, IR const* use) const
	{
		IS_TRUE0(def->is_stmt() && use->is_exp());
		DU_SET const* du = get_du_c(def);
		if (du == NULL) { return false; }
		return du->is_contain(IR_id(use));
	}
	bool is_pr_unique_for_same_no() const { return m_is_pr_unique_for_same_no; }
	bool is_stmt_multi_result() const { return m_is_stmt_has_multi_result; }
	bool is_stpr_may_def(IR const* def, IR const* use, bool is_recur);
	bool is_call_may_def(IR const* def, IR const* use, bool is_recur);
	bool is_may_def(IR const* def, IR const* use, bool is_recur);
	bool is_may_kill(IR const* def1, IR const* def2);
	bool is_must_kill(IR const* def1, IR const* def2);
	bool is_exact_unique_def(IR const* def, IR const* exp);
	IR * find_dom_def(IR const* exp, IR const* exp_stmt,
					  DU_SET const* expdu, bool omit_self);
	IR const* find_killing_local_def(IR const* stmt, C<IR*> * ct,
									IR const* exp, MD const* md);

	//Get call site mayuse MD_SET.
	MD_SET const* map_call2mayuse(IR const* ir)
	{
		IS_TRUE0(ir->is_call());
		return m_call2mayuse.get(ir);
	}

	void set_map_call2mayuse(IR const* ir, MD_SET const* mds);

	void set_compute_pr_du_chain(bool compute)
	{ m_is_compute_pr_du_chain = compute; }

	void set_stmt_multi_result(bool is_multi_result)
	{ m_is_stmt_has_multi_result = is_multi_result; }

	void sort_stmt_in_bfs(IN SLIST<IR const*> & stmts,
						  OUT SLIST<IR const*> & outs,
						  SVECTOR<UINT> & bfs_order);

	/* DU chain operations.
	Set 'use' to be USE of 'stmt'.
	e.g: given stmt->{u1, u2}, the result will be stmt->{u1, u2, use} */
	inline void union_use(IR * stmt, IN IR * use)
	{
		IS_TRUE0(stmt && stmt->is_stmt());
		if (use == NULL) return;
		IS_TRUE0(use->is_exp());
		get_du_alloc(stmt)->add_use(use, *m_sbs_mgr);
	}

	/* DU chain operations.
	Set 'exp' to be USE of stmt which is held in 'stmtset'.
	e.g: given stmt and its use chain is {u1, u2}, the result will be
	stmt->{u1, u2, exp} */
	inline void union_use(DU_SET const* stmtset, IR * exp)
	{
		IS_TRUE0(stmtset && exp && exp->is_exp());
		DU_ITER di;
		for (INT i = stmtset->get_first(&di);
			 i >= 0; i = stmtset->get_next(i, &di)) {
			IR * d = m_ru->get_ir(i);
			IS_TRUE0(d->is_stmt());
			get_du_alloc(d)->add_use(exp, *m_sbs_mgr);
		}
	}

	/* DU chain operation.
	Set element in 'set' as USE to stmt.
	e.g: given set is {u3, u4}, and stmt->{u1},
	=>
	stmt->{u1, u1, u2} */
	inline void union_useset(IR * stmt, IN SBITSETC const* set)
	{
		IS_TRUE0(stmt->is_stmt());
		if (set == NULL) { return; }
		get_du_alloc(stmt)->bunion(*set, *m_sbs_mgr);
	}

	/* DU chain operation.
	Set element in 'set' as DEF to ir.
	e.g: given set is {d1, d2}, and {d3}->ir,
	=>
	{d1, d2, d3}->ir */
	inline void union_defset(IR * ir, IN SBITSETC const* set)
	{
		IS_TRUE0(ir->is_exp());
		if (set == NULL) { return; }
		get_du_alloc(ir)->bunion(*set, *m_sbs_mgr);
	}

	/* DU chain operation.
	Set 'def' to be DEF of 'ir'.
	e.g: given def is d1, and {d2}->ir,
	the result will be {d1, d2}->ir */
	inline void union_def(IR * ir, IN IR * def)
	{
		IS_TRUE0(ir->is_exp());
		if (def == NULL) return;
		IS_TRUE0(def->is_stmt());
		get_du_alloc(ir)->add_def(def, *m_sbs_mgr);
	}


	/* DU chain operations.
	Set 'stmt' to be DEF of each expressions which is held in 'expset'.
	e.g: given stmt and its use-chain is {u1, u2}, the result will be
	stmt->{u1, u2, use} */
	inline void union_def(DU_SET const* expset, IR * stmt)
	{
		IS_TRUE0(expset && stmt && stmt->is_stmt());
		DU_ITER di;
		for (INT i = expset->get_first(&di);
			 i >= 0; i = expset->get_next(i, &di)) {
			IR * u = m_ru->get_ir(i);
			IS_TRUE0(u->is_exp());
			get_du_alloc(u)->add_def(stmt, *m_sbs_mgr);
		}
	}

	/* DU chain operation.
	Cut off the chain bewteen 'def' and 'use'.
	The related function is build_du_chain(). */
	inline void remove_du_chain(IR const* def, IR const* use)
	{
		IS_TRUE0(def->is_stmt() && use->is_exp());
		DU_SET * useset = def->get_duset();
		if (useset != NULL) { useset->remove(IR_id(use), *m_sbs_mgr); }

		DU_SET * defset= use->get_duset();
		if (defset != NULL) { defset->remove(IR_id(def), *m_sbs_mgr); }
	}

	//DU chain operations.
	//See function definition.
	bool remove_expired_du_chain(IR * stmt);
	void remove_def(IR const* ir, IR const* def);
	void remove_use_out_from_defset(IR * stmt);
	void remove_def_out_from_useset(IR * def);
	void remove_ir_out_from_du_mgr(IR * ir);

	bool verify_du_ref();
	bool verify_du_chain();
	bool verify_du_info(IR const* ir);
	bool verify_livein_exp();

	bool perform(IN OUT OPT_CTX & oc, UINT flag = SOL_AVAIL_REACH_DEF|
													 SOL_AVAIL_EXPR|
													 SOL_REACH_DEF|
													 SOL_REF|
													 SOL_RU_REF);
};
#endif
