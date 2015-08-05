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
#ifndef _IR_EXPR_TAB_
#define	_IR_EXPR_TAB_

class IR_EXPR {
public:
	UINT id;
	IR * registered_ir;
	MD_SET * referred_mds;
	IR_EXPR * next;
	IR_EXPR * prev;
	IR_LIST occ_list;

	IR_EXPR()
	{
		id = 0;
		next = prev = NULL;
		registered_ir = NULL;
		referred_mds = NULL;
	}

	~IR_EXPR() {}
};
#define IR_EXPR_id(i)				(i)->id
#define IR_EXPR_ir(i)				(i)->registered_ir
#define IR_EXPR_referred_mds(i)		(i)->referred_mds
#define IR_EXPR_next(i)				(i)->next
#define IR_EXPR_prev(i)				(i)->prev
#define IR_EXPR_occ_list(i)			(i)->occ_list


/* IR Expression Table, scanning statement to
evaluate the hash value of expression.
Compute LIVE IN and LIVE OUT IR expressions for each BB. */
#define IR_EXPR_TAB_LEVEL1_HASH_BUCKET	256
#define IR_EXPR_TAB_LEVEL2_HASH_BUCKET	128
class IR_EXPR_TAB : public IR_OPT {
	UINT m_expr_count; //the encode-number expression.
	REGION * m_ru;
	DT_MGR * m_dm;
	BVEC<IR_EXPR*> m_ir_expr_vec;

	//Record allocated object. used by destructor.
	SLIST<IR_EXPR*> m_ir_expr_lst;

	CIR_ITER m_iter; //for tmp use.
	SMEM_POOL * m_pool;
	SMEM_POOL * m_sc_pool;
	IR_EXPR ** m_level1_hash_tab[IR_EXPR_TAB_LEVEL1_HASH_BUCKET];
	SVECTOR<IR_EXPR*> m_map_ir2ir_expr;
	MD_SET_MGR m_md_set_mgr; //alloca MS_SET.
	BITSET_MGR m_bs_mgr;

	void * xmalloc(INT size);
	inline UINT compute_hash_key(IR const* ir);
	inline UINT compute_hash_key_for_tree(IR * ir);
	inline IR_EXPR * encode_istore_memaddr(IN IR * ir, IN IR_BB * bb)
	{
		IS_TRUE0(IR_type(IR_parent(ir)) == IR_IST);
		if (IR_type(ir) == IR_ARRAY) {
			return NULL;
		}
		return encode_expr(ir, bb);
	}
public:
	IR_EXPR_TAB(REGION * ru);
	~IR_EXPR_TAB();

	IR_EXPR * append_expr(IR * ir);
	void clean_occ_list();
	UINT count_mem();

	void dump_ir_expr_tab();
	IR_EXPR * encode_expr(IN IR * ir, IN IR_BB * bb);
	void encode_bb(IR_BB * bb);

	IR_EXPR * find_expr(IR * ir);

	IR_EXPR * get_expr(UINT id) { return m_ir_expr_vec.get(id); }
	BVEC<IR_EXPR*> * get_expr_vec() { return &m_ir_expr_vec; }

	IR_EXPR * map_ir2ir_expr(IR const* ir);
	IR_EXPR * new_ir_expr();

	IR * remove_occ(IR * occ);
	void remove_occs(IR * ir);
	IR_EXPR * remove_expr(IR * ir);
	void reperform(IN OUT OPT_CTX & oc);

	void set_map_ir2ir_expr(IR const* ir, IR_EXPR * ie);

	OPT_TYPE get_opt_type() const { return OPT_EXPR_TAB; }

	virtual CHAR const* get_opt_name() const { return "IR Expr Tabel"; }

	virtual bool perform(IN OUT OPT_CTX & oc);
};
#endif
