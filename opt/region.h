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
#ifndef __REGION_H__
#define __REGION_H__

class IPA;
class CFS_MGR;
class TARG_MACH;
class IR_DU_MGR;
class IR_AA;
class IR_EXPR_TAB;

//Make sure IR_ICALL is the largest ir.
#define MAX_OFFSET_AT_FREE_TABLE	(sizeof(CICALL) - sizeof(IR))

typedef enum {
	RU_UNDEF = 0,

	//Region is black box with
	//Black box is single entry, single exit.
	RU_BLX,

	//Sub region is the region which contains a list of IRs,
	//Sub region must be single entry, multiple exits.
	RU_SUB,

	//Region is exception region.
	//Exception is single entry, multiple exits.
	RU_EH,

	//Region is function unit
	//Function unit is single entry, multiple exits.
	RU_FUNC,

	//Region is whole program spectrum.
	//Program region is single entry, multiple exits.
	RU_PROGRAM,
} REGION_TYPE;

class REGION;
class CALLG;

//
//START REGION_MGR
//
//Region Manager is the top level manager.
#define RU_MGR_label_count(r)		((r)->m_label_count)
#define RU_MGR_tm(r)				((r)->m_tm)
#define RU_MGR_var_mgr(r)			((r)->m_var_mgr)
class REGION_MGR {
friend class REGION;
protected:
	SVECTOR<REGION*> m_id2ru;
	BITSET_MGR m_bs_mgr;
	MD_SET m_global_var_mds;
	SYM_TAB m_sym_tab;
	DT_MGR m_dt_mgr;
	TARG_MACH * m_tm;
	VAR_MGR * m_var_mgr;
	MD * m_str_md;
	MD_SYS * m_md_sys;
	CALLG * m_callg;
	UINT m_ru_count;
	UINT m_label_count;
	bool m_is_regard_str_as_same_md;
public:
	explicit REGION_MGR() : m_sym_tab(0)
	{
		m_ru_count = 0;
		m_tm = NULL;
		m_label_count = 1;
		m_var_mgr = NULL;
		m_md_sys = NULL;
		m_is_regard_str_as_same_md = true;
		m_str_md = NULL;
		m_callg = NULL;
		m_sym_tab.init(64);
	}
	virtual ~REGION_MGR();
	inline SYM * add_to_symtab(CHAR const* s) { return m_sym_tab.add(s); }
	void dump();
	void delete_region(UINT id);

	inline BITSET_MGR * get_bs_mgr() { return &m_bs_mgr; }
	virtual REGION * get_region(UINT id);
	inline UINT get_num_of_region() const { return m_ru_count; }
	REGION * get_top_region();

	//TODO: remove this useless function get_global_var_mds() and
	//related variable.
	inline MD_SET const* get_global_var_mds() { return &m_global_var_mds; }

	inline VAR_MGR * get_var_mgr() { return m_var_mgr; }
	MD const* get_dedicated_str_md();
	inline TARG_MACH * get_tm() const { return m_tm; }
	inline MD_SYS * get_md_sys() { return m_md_sys; }
	inline SYM_TAB * get_sym_tab() { return &m_sym_tab; }
	inline DT_MGR * get_dm() { return &m_dt_mgr; }
	CALLG * get_callg() const { return m_callg; }

	void register_global_mds();

	//Initialize VAR_MGR structure. It is the first thing after you
	//declared a REGION_MGR.
	inline void init_var_mgr()
	{
		m_var_mgr = new_var_mgr();
		IS_TRUE0(m_var_mgr);
		m_md_sys = new MD_SYS(m_var_mgr);
	}
	CALLG * init_cg(REGION * top);
	virtual REGION * new_region(REGION_TYPE rt);
	virtual VAR_MGR * new_var_mgr();
	IPA * new_ipa();

	inline void set_tm(TARG_MACH * tm) { m_tm = tm; }
	virtual void process_func(IN REGION * ru);
	virtual void process();

	virtual void register_region(REGION * ru);
};
//END REGION_MGR


//Analysis Instrument.
//Record Data structure for IR analysis and transformation.
#define ANA_INS_ru_mgr(a)	((a)->m_ru_mgr)
class ANA_INS {
	bool verify_var(VAR_MGR * vm, VAR * v);
public:
	REGION * m_ru;
	UINT m_pr_count; //counter of IR_PR.

	SMEM_POOL * m_pool;
	SMEM_POOL * m_du_pool;
	LIST<REGION*> * m_inner_region_lst; //indicate the inner regions.

	//Indicate a list of IR.
	IR * m_ir_list;

	LIST<IR const*> * m_call_list; //record CALL/ICALL in region.

	IR_CFG * m_ir_cfg; //CFG of region.
	IR_AA * m_ir_aa; //alias analyzer.
	IR_DU_MGR * m_ir_du_mgr; //DU manager.
	REGION_MGR * m_ru_mgr; //REGION manager.

	/* Record vars defined in current region,
	include the subregion defined variables.
	All LOCAL vars in the tab will be deleted during region destructing. */
	VAR_TAB m_ru_var_tab;
	IR * m_free_tab[MAX_OFFSET_AT_FREE_TABLE + 1];
	SVECTOR<VAR*> m_prno2var; //map prno to related VAR.
	SVECTOR<IR*> m_ir_vector; //record IR which have allocated.
	BITSET_MGR m_bs_mgr;
	SDBITSET_MGR m_sbs_mgr;
	MD_SET_MGR m_mds_mgr;
	MD_SET_HASH m_mds_hash;
	LIST<DU*> m_free_du_list;
	IR_BB_MGR m_ir_bb_mgr; //Allocate the basic block.
	IR_BB_LIST m_ir_bb_list; //record a list of basic blocks.

	#ifdef _DEBUG_
	BITSET m_has_been_freed_irs;
	#endif

public:
	explicit ANA_INS(REGION * ru);
	~ANA_INS();

	UINT count_mem();
};


//REGION referrence info.
#define REF_INFO_maydef(ri)		((ri)->may_def_mds)
#define REF_INFO_mustdef(ri)		((ri)->must_def_mds)
#define REF_INFO_mayuse(ri)		((ri)->may_use_mds)
class REF_INFO {
public:
	MD_SET * must_def_mds; //Record the MD set for REGION usage
	MD_SET * may_def_mds; //Record the MD set for REGION usage
	MD_SET * may_use_mds; //Record the MD set for REGION usage

	UINT count_mem()
	{
		UINT c = must_def_mds->count_mem();
		c += may_def_mds->count_mem();
		c += may_use_mds->count_mem();
		c += sizeof(MD_SET*) * 3;
		return c;
	}
};


//
//START REGION
//
//Record unique id in REGION_MGR.
#define RU_id(r)					((r)->id)
#define RU_type(r)					((r)->m_ru_type)
#define RU_parent(r)				((r)->m_parent)

//Record analysis data structure for code region.
#define RU_ana(r)					((r)->m_u1.m_ana_ins)

//Record the binary data for black box region.
#define RU_blx_data(r)				((r)->m_u1.m_blx_data)

//Set to true if REGION is expected to be inlined.
#define RU_is_exp_inline(r)			((r)->m_u2.s1.is_expect_inline)

//True value means MustDef, MayDef, MayUse are available.
#define RU_is_du_valid(r)			((r)->m_u2.s1.is_du_valid)

//Record memory reference for region.
#define RU_ref_info(r)				((r)->m_ref_info)

//True if PR is unique which do not consider PR's data type.
#define RU_is_pr_unique_for_same_no(r)	((r)->m_u2.s1.is_pr_unique_for_same_no)

class REGION {
friend class REGION_MGR;
protected:
	union {
		ANA_INS * m_ana_ins; //Analysis instrument.
		void * m_blx_data; //Black box data.
	} m_u1;

	void * xmalloc(UINT size)
	{
		IS_TRUE(RU_ana(this)->m_pool != NULL, ("pool does not initialized"));
		void * p = smpool_malloc_h(size, RU_ana(this)->m_pool);
		IS_TRUE0(p != NULL);
		memset(p, 0, size);
		return p;
	}

public:
	REF_INFO * m_ref_info; //record use/def info if REGION has.
	REGION_TYPE m_ru_type; //region type.
	UINT id; //region unique id.
	VAR * m_var; //record VAR if RU has.
	REGION * m_parent; //record parent region.
	union {
		struct {
			BYTE is_expect_inline:1; //see above macro declaration.
			BYTE is_du_valid:1;  //see above macro declaration.

			/* Set to true if there is single map between
			PR and MD. If PR may corresponde to multiple
			MD, set it to false.

			e.g: If one use same pr with different type U8 and U32,
			there will have two mds refer to pr1(U8) and pr1(U32),
			MD2->pr1(U8), MD8->pr1(U32). */
			BYTE is_pr_unique_for_same_no:1;
		} s1;
		BYTE s1b1;
	} m_u2;

public:
	explicit REGION(REGION_TYPE rt, REGION_MGR * rm) { init(rt, rm); }
	virtual ~REGION() { destroy(); }

	//Add var which used inside current or inner REGION.
	//Once the region destructing, all local vars are deleted.
	void add_to_var_tab(VAR * v)
	{ RU_ana(this)->m_ru_var_tab.append(v); }

	//Add irs into IR list of current region.
	void add_to_ir_list(IR * irs)
	{ add_next(&RU_ana(this)->m_ir_list, irs); }

	//The function generates new MD for given pr.
	//It should be called if new PR generated in optimzations.
	inline MD const* alloc_ref_for_pr(IR * pr)
	{
		MD const* md = gen_md_for_pr(pr);
		pr->set_ref_md(md, this);
		pr->clean_ref_mds();
		return md;
	}

	inline DU * alloc_du()
	{
		DU * du = RU_ana(this)->m_free_du_list.remove_head();
		if (du == NULL) {
			du = (DU*)smpool_malloc_h_const_size(sizeof(DU),
									RU_ana(this)->m_du_pool);
			memset(du, 0, sizeof(DU));
		}
		return du;
	}

	IR * build_continue();
	IR * build_break();
	IR * build_case(IR * casev_exp, LABEL_INFO * case_br_lab);
	IR * build_do_loop(IR * det, IR * init, IR * step, IR * loop_body);
	IR * build_do_while(IR * det, IR * loop_body);
	IR * build_while_do(IR * det, IR * loop_body);
	IR * build_if(IR * det, IR * true_body, IR * false_body);
	IR * build_switch(IR * vexp, IR * case_list, IR * body,
					  LABEL_INFO * default_lab);
	IR * build_pr_dedicated(UINT prno, UINT tyid);
	IR * build_pr(UINT tyid);
	UINT build_prno(UINT tyid);
	IR * build_branch(bool is_true_br, IR * det, LABEL_INFO * lab);
	IR * build_id(IN VAR * var_info);
	IR * build_ilabel();
	IR * build_label(LABEL_INFO * li);
	IR * build_cvt(IR * exp, UINT tgt_tyid);
	IR * build_goto(LABEL_INFO * li);
	IR * build_igoto(IR * vexp, IR * case_list);
	IR * build_pointer_op(IR_TYPE irt, IR * lchild, IR * rchild);
	IR * build_cmp(IR_TYPE irt, IR * lchild, IR * rchild);
	IR * build_judge(IR * exp);
	IR * build_binary_op_simp(IR_TYPE irt, UINT tyid,
							  IR * lchild, IR * rchild);
	IR * build_binary_op(IR_TYPE irt, UINT tyid,
						 IN IR * lchild, IN IR * rchild);
	IR * build_unary_op(IR_TYPE irt, UINT tyid, IN IR * opnd);
	IR * build_logical_not(IR * opnd0);
	IR * build_logical_op(IR_TYPE irt, IR * opnd0, IR * opnd1);
	IR * build_imm_int(HOST_INT v, UINT tyid);
	IR * build_imm_fp(HOST_FP fp, UINT tyid);
	IR * build_lda(IR * lda_base);
	IR * build_load(VAR * var, UINT tyid);
	IR * build_load(VAR * var);
	IR * build_iload(IR * base, UINT tyid);
	IR * build_iload(IR * base, UINT ofst, UINT tyid);
	IR * build_store(VAR * lhs, IR * rhs);
	IR * build_store(VAR * lhs, UINT tyid, IR * rhs);
	IR * build_store(VAR * lhs, UINT tyid, UINT ofst, IR * rhs);
	IR * build_store_pr(UINT prno, UINT tyid, IR * rhs);
	IR * build_store_pr(UINT tyid, IR * rhs);
	IR * build_istore(IR * lhs, IR * rhs, UINT tyid);
	IR * build_istore(IR * lhs, IR * rhs, UINT ofst, UINT tyid);
	IR * build_string(SYM * strtab);
	IR * build_array(IR * base, IR * sublist, UINT tyid,
					 UINT elem_tyid, UINT dims, TMWORD const* elem_num);
	IR * build_return(IR * ret_exp);
	IR * build_select(IR * det, IR * true_exp, IR * false_exp, UINT tyid);
	IR * build_phi(UINT prno, UINT tyid, UINT num_opnd);
	IR * build_region(REGION_TYPE rt, VAR * ru_var);
	IR * build_icall(IR * callee, IR * param_list,
					UINT result_prno = 0, UINT tyid = 0);
	IR * build_call(VAR * callee, IR * param_list,
					UINT result_prno = 0, UINT tyid = 0);

	IR * construct_ir_list(bool clean_ir_list);
	void construct_ir_bb_list();
	HOST_INT calc_int_val(IR_TYPE ty, HOST_INT v0, HOST_INT v1);
	double calc_float_val(IR_TYPE ty, double v0, double v1);
	UINT count_mem();
	void check_valid_and_recompute(OPT_CTX * oc, ...);

	virtual void destroy();
	IR * dup_ir(IR const* ir);
	IR * dup_irs(IR const* ir);
	IR * dup_irs_list(IR const* ir);

	void dump_all_stmt();
	void dump_all_ir();
	void dump_var_tab(INT indent = 0);
	void dump_free_tab();
	void dump_mem_usage(FILE * h);
	void dump_bb_usage(FILE * h);

	void free_mds(MD_SET * mds) { get_mds_mgr()->free(mds); }
	void free_ir(IR * ir);
	void free_irs(IR * ir);
	void free_irs_list(IR * ir);
	void free_irs_list(IR_LIST & irs);
	IR * fold_const_float_una(IR * ir, bool & change);
	IR * fold_const_float_bin(IR * ir, bool & change);
	IR * fold_const_int_una(IR * ir, bool & change);
	IR * fold_const_int_bin(IR * ir, bool & change);
	IR * fold_const(IR * ir, bool & change);
	CHAR * format_label_name(IN LABEL_INFO * lab, OUT CHAR * buf);

	UINT get_irt_size(IR * ir)
	{
		#ifdef CONST_IRT_SZ
		return IR_irt_size(ir);
		#else
		return IRTSIZE(IR_type(ir));
		#endif
	}
	SMEM_POOL * get_pool() { return RU_ana(this)->m_pool; }
	inline MD_SYS * get_md_sys() { return get_ru_mgr()->get_md_sys(); }
	inline DT_MGR * get_dm() { return get_ru_mgr()->get_dm(); }

	inline LIST<REGION*> * get_inner_region_list()
	{
		if (RU_ana(this)->m_inner_region_lst == NULL) {
			RU_ana(this)->m_inner_region_lst = new LIST<REGION*>();
		}
		return RU_ana(this)->m_inner_region_lst;
	}
	inline TARG_MACH * get_tm() const { return RU_MGR_tm(get_ru_mgr()); }
	UINT get_pr_count() const { return RU_ana(this)->m_pr_count; }
	VAR * get_ru_var() const { return m_var; }

	inline REGION_MGR * get_ru_mgr() const
	{
		IS_TRUE0(RU_type(this) == RU_FUNC || RU_type(this) == RU_PROGRAM);
		IS_TRUE0(RU_ana(this));
		return ANA_INS_ru_mgr(RU_ana(this));
	}
	IR * get_ir_list() const { return RU_ana(this)->m_ir_list; }

	VAR_MGR * get_var_mgr() const { return RU_MGR_var_mgr(get_ru_mgr()); }

	VAR_TAB * get_var_tab() { return &RU_ana(this)->m_ru_var_tab; }
	BITSET_MGR * get_bs_mgr() { return &RU_ana(this)->m_bs_mgr; }
	SDBITSET_MGR * get_sbs_mgr() { return &RU_ana(this)->m_sbs_mgr; }
	MD_SET_MGR * get_mds_mgr() { return &RU_ana(this)->m_mds_mgr; }
	IR_BB_LIST * get_bb_list() { return &RU_ana(this)->m_ir_bb_list; }
	IR_BB_MGR * get_bb_mgr() { return &RU_ana(this)->m_ir_bb_mgr; }

	//Get MD_SET_HASH.
	MD_SET_HASH * get_mds_hash() { return &RU_ana(this)->m_mds_hash; }

	//Return IR pointer via the unique IR_id.
	inline IR * get_ir(UINT irid)
	{
		IS_TRUE0(RU_ana(this)->m_ir_vector.get(irid));
		return RU_ana(this)->m_ir_vector.get(irid);
	}

	//Return the vector that record all allocated IRs.
	SVECTOR<IR*> * get_ir_vec() { return &RU_ana(this)->m_ir_vector; }

	//Return IR_BB pointer via the unique IR_BB_id.
	IR_BB * get_bb(UINT bbid)
	{ return RU_ana(this)->m_ir_cfg->get_bb(bbid); }

	//Return IR_CFG.
	IR_CFG * get_cfg() const { return RU_ana(this)->m_ir_cfg; }

	//Return alias analysis module.
	IR_AA * get_aa() const { return RU_ana(this)->m_ir_aa; }

	//Return DU info manager.
	IR_DU_MGR * get_du_mgr() const { return RU_ana(this)->m_ir_du_mgr; }

	CHAR const* get_ru_name() const;
	REGION * get_func_unit();

	//Allocate and return a list of call, that indicate all IR_CALL in
	//current REGION.
	inline LIST<IR const*> * get_call_list()
	{
		if (RU_ana(this)->m_call_list == NULL) {
			RU_ana(this)->m_call_list = new LIST<IR const*>();
		}
		return RU_ana(this)->m_call_list;
	}

	//Compute the most conservative MD reference information.
	void gen_default_ref_info();

	//Get the MustUse of REGION.
	inline MD_SET * get_must_def()
	{
		if (m_ref_info != NULL) { return REF_INFO_mustdef(m_ref_info); }
		return NULL;
	}

	//Get the MayDef of REGION.
	inline MD_SET * get_may_def()
	{
		if (m_ref_info != NULL) { return REF_INFO_maydef(m_ref_info); }
		return NULL;
	}

	//Get the MayUse of REGION.
	inline MD_SET * get_may_use()
	{
		if (m_ref_info != NULL) { return REF_INFO_mayuse(m_ref_info); }
		return NULL;
	}

	//Allocate a internal LABEL_INFO that is not declared by compiler user.
	LABEL_INFO * gen_ilab();

	//Allocate a LABEL_INFO accroding to given 'labid'.
	LABEL_INFO * gen_ilab(UINT labid);

	//Allocate MD for IR_PR.
	MD * gen_md_for_pr(IR const* ir);
	MD * gen_md_for_pr(UINT prno, UINT tyid);

	//Allocate MD for IR_ST.
	MD * gen_md_for_st(IR const* ir);

	//Allocate MD for IR_ID.
	MD * gen_md_for_var(IR const* ir);

	//Allocate MD for IR_LD.
	MD * gen_md_for_ld(IR const* ir)
	{ return gen_md_for_var(ir); }

	//Return the tyid for array index, the default is unsigned 32bit.
	inline UINT get_target_machine_array_index_tyid()
	{
		return get_dm()->get_simplex_tyid_ex(get_dm()->
					get_uint_dtype(WORD_LENGTH_OF_TARGET_MACHINE));
	}

	//Perform high level optmizations.
	virtual bool high_process(OPT_CTX & oc);

	//Initialze REGION.
	void init(REGION_TYPE rt, REGION_MGR * rm);

	//Allocate and initialize REGION reference structure.
	void init_ref_info()
	{
		if (m_ref_info != NULL) { return; }
		m_ref_info = (REF_INFO*)xmalloc(sizeof(REF_INFO));
		if (REF_INFO_maydef(m_ref_info) == NULL) {
			REF_INFO_maydef(m_ref_info) = get_mds_mgr()->create();
		}
		if (REF_INFO_mustdef(m_ref_info) == NULL) {
			REF_INFO_mustdef(m_ref_info) = get_mds_mgr()->create();
		}
		if (REF_INFO_mayuse(m_ref_info) == NULL) {
			REF_INFO_mayuse(m_ref_info) = get_mds_mgr()->create();
		}
	}

	//Allocate and initialize control flow graph.
	virtual IR_CFG * init_cfg(OPT_CTX & oc);

	//Allocate and initialize alias analysis.
	virtual IR_AA * init_aa(OPT_CTX & oc);

	//Allocate and initialize def-use manager.
	virtual IR_DU_MGR * init_du(OPT_CTX & oc);

	//Invert condition for relation operation.
	virtual void invert_cond(IR ** cond);

	bool is_safe_to_optimize(IR const* ir);

	//Check and insert data type CVT if it is necessary.
 	IR * insert_cvt(IR * parent, IR * kid, bool & change);
	void insert_cvt_for_binop(IR * ir, bool & change);

	/* Return true if the tree height is not great than 2.
	e.g: tree a + b is lowest height , but a + b + c is not.
	Note that if ARRAY or ILD still not be lowered at the moment, regarding
	it as a whole node. e.g: a[1][2] + b is the lowest height. */
	bool is_lowest_heigh(IR const* ir) const;
	bool is_lowest_heigh_exp(IR const* ir) const;

	//Return true if REGION name is equivalent to 'n'.
	bool is_ru_name_equ(CHAR const* n) const
	{ return strcmp(get_ru_name(), n) == 0; }

	//Perform middle level IR optimizations which are implemented
	//accroding to control flow info and data flow info.
	virtual bool middle_process(OPT_CTX & oc);

	//Map from prno to related VAR.
	VAR * map_pr2var(UINT prno)
	{ return RU_ana(this)->m_prno2var.get(prno); }

	//Allocate IR_BB.
	IR_BB * new_bb();

	//Allcoate PASS_MGR.
	virtual PASS_MGR * new_pass_mgr() { return new PASS_MGR(this); }

	//Allocate an IR.
	IR * new_ir(IR_TYPE irt);

	//Allocate IR_AI.
	inline IR_AI * new_ai()
	{
		IR_AI * ai = (IR_AI*)xmalloc(sizeof(IR_AI));
		IS_TRUE0(ai);
		ai->init();
		return ai;
	}

	//Peephole optimizations.
	IR * refine_band(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_bor(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_cvt(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_land(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_lor(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_xor(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_add(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_sub(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_mul(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_rem(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_div(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_ne(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_eq(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_mod(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_call(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_icall(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_switch(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_return(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_br(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_select(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_branch(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_array(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_neg(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_not(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_bin_op(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_lda(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_ld(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_ild_1(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_ild_2(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_ild(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_det(IR * ir_list, bool & change, RF_CTX & rc);
	IR * refine_store(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_istore(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_ir(IR * ir, bool & change, RF_CTX & rc);
	IR * refine_ir_list(IR * ir_list, bool & change, RF_CTX & rc);
	bool refine_ir_stmt_list(IN OUT BBIR_LIST & ir_list, RF_CTX & rc);
	bool refine_ir_bb_list(IN OUT IR_BB_LIST * ir_bb_list, RF_CTX & rc);
	IR * reassociation(IR * ir, bool & change);
	bool reconstruct_ir_bb_list(OPT_CTX & oc);
	void remove_inner_region(REGION * ru);

	C<IR_BB*> * split_irs(IR * irs, IR_BB_LIST * bbl, C<IR_BB*> * ctbb);
	IR * simplify_kids(IR * ir, SIMP_CTX * cont);
	IR * simplify_store(IR * ir, SIMP_CTX * cont);
	IR * simplify_store_pr(IR * ir, SIMP_CTX * cont);
	IR * simplify_setepr(IR * ir, SIMP_CTX * ctx);
	IR * simplify_getepr(IR * ir, SIMP_CTX * ctx);
	IR * simplify_istore(IR * ir, SIMP_CTX * cont);
	IR * simplify_call(IR * ir, SIMP_CTX * cont);
	IR * simplify_if (IR * ir, SIMP_CTX * cont);
	IR * simplify_while_do(IR * ir, SIMP_CTX * cont);
	IR * simplify_do_while (IR * ir, SIMP_CTX * cont);
	IR * simplify_do_loop(IR * ir, SIMP_CTX * cont);
	IR * simplify_det(IR * ir, SIMP_CTX * cont);
	IR * simplify_judge_exp(IR * ir, SIMP_CTX * cont);
	IR * simplify_select(IR * ir, SIMP_CTX * cont);
	IR * simplify_switch (IR * ir, SIMP_CTX * cont);
	IR * simplify_igoto(IR * ir, SIMP_CTX * cont);
	IR * simplify_array_addr_exp(IR * ir, SIMP_CTX * cont);
	IR * simplify_array(IR * ir, SIMP_CTX * cont);
	IR * simplify_exp(IR * ir, SIMP_CTX * cont);
	IR * simplify_stmt(IR * ir, SIMP_CTX * cont);
	IR * simplify_stmt_list(IR * ir, SIMP_CTX * cont);
	void simplify_bb(IR_BB * bb, SIMP_CTX * cont);
	void simplify_bb_list(IR_BB_LIST * bbl, SIMP_CTX * cont);
	IR * simplify_logical_not(IN IR * ir, SIMP_CTX * cont);
	IR * simplify_logical_or_falsebr(IN IR * ir, IN LABEL_INFO * tgt_label);
	IR * simplify_logical_or_truebr(IN IR * ir, IN LABEL_INFO * tgt_label);
	IR * simplify_logical_or(IN IR * ir, SIMP_CTX * cont);
	IR * simplify_logical_and_truebr(IN IR * ir, IN LABEL_INFO * tgt_label);
	IR * simplify_logical_and_falsebr(IN IR * ir, IN LABEL_INFO * tgt_label);
	IR * simplify_logical_and(IN IR * ir, IN LABEL_INFO * label,
							  SIMP_CTX * cont);
	IR * simplify_logical_det(IR * ir, SIMP_CTX * cont);
	IR * simplify_lda(IR * ir, SIMP_CTX * cont);
	void set_irt_size(IR * ir, UINT irt_sz)
	{
		#ifdef CONST_IRT_SZ
		IR_irt_size(ir) = irt_sz;
		#endif
	}
	void set_map_pr2var(UINT prno, VAR * pr_var)
	{ RU_ana(this)->m_prno2var.set(prno, pr_var); }

	void set_ru_var(VAR * v) { m_var = v; }
	void set_ir_list(IR * irs) { RU_ana(this)->m_ir_list = irs; }
	void set_blx_data(void * d) { RU_blx_data(this) = d; }
	IR * strength_reduce(IN OUT IR * ir, IN OUT bool & change);
	LIST<IR const*> * scan_call_list();

	void prescan(IR const* ir);
	bool partition_region();
	virtual void process(); //Entry to process region-unit.

	bool verify_bbs(IR_BB_LIST & bbl);
	bool verify_ir_inplace();
};
//END REGION
#endif
