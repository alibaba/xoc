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
#ifndef _IR_CFG_H_
#define _IR_CFG_H_


typedef TMAP<LABEL_INFO*, IR_BB*> LAB2BB;


/*
NOTICE:
1. For accelerating perform operation of each vertex, e.g
   compute dominator, please try best to add vertex with
   topological order.
*/
class IR_CFG : public CFG<IR_BB, IR> {
protected:
	SVECTOR<IR_BB*> m_bb_vec;
	LAB2BB m_lab2bb;
	REGION * m_ru;
	DT_MGR * m_dm;
public:
	IR_CFG(CFG_SHAPE cs, IR_BB_LIST * bbl, REGION * ru,
		   UINT edge_hash_size = 16, UINT vertex_hash_size = 16);
	virtual ~IR_CFG() {}

	//Add LABEL to bb, and establish map between label and bb.
	void add_lab(IR_BB * src, LABEL_INFO * li)
	{
		src->add_label(li);

		//Set label->bb map.
		m_lab2bb.aset(li, src);
	}

	/* Add new IR_BB into CFG, but the BB list should be modified
	out of this function.
	Namely, you should use 'insert_bb_between()' to insert BB into list.
	And you must consider the right insertion. */
	inline void add_bb(IR_BB * bb)
	{
		IS_TRUE0(bb && m_bb_vec.get(IR_BB_id(bb)) == NULL);
		IS_TRUE(IR_BB_id(bb) != 0, ("bb id should start at 1"));
		m_bb_vec.set(IR_BB_id(bb), bb);
		add_vertex(IR_BB_id(bb));
	}

	virtual void cf_opt();
	void compute_dom_idom(IN OUT OPT_CTX & oc, BITSET const* uni = NULL);
	void compute_pdom_ipdom(IN OUT OPT_CTX & oc, BITSET const* uni = NULL);
	virtual void compute_entry_and_exit(bool comp_entry, bool comp_exit)
	{
		IS_TRUE0(comp_entry | comp_exit);
		if (comp_entry) { m_entry_list.clean(); }
		if (comp_exit) { m_exit_list.clean(); }

		for (IR_BB * bb = m_bb_list->get_head();
			 bb != NULL; bb = m_bb_list->get_next()) {
			VERTEX * vex = get_vertex(bb->id);
			IS_TRUE(vex, ("No vertex corresponds to BB%d", bb->id));
			if (comp_entry) {
				if (VERTEX_in_list(vex) == NULL &&
					(is_ru_entry(bb) || is_exp_handling(bb))) {
					m_entry_list.append_tail(bb);
				}
			}
			if (comp_exit) {
				if (VERTEX_out_list(vex) == NULL) {
					m_exit_list.append_tail(bb);
					IR_BB_is_exit(bb) = true;
				}
			}
		}
	}

	virtual void dump_vcg(CHAR const* name)
	{ CFG<IR_BB, IR>::dump_vcg(name); }

	void dump_vcg(CHAR const* name = NULL, bool detail = true,
				  bool dump_eh = true);
	void dump_dot(CHAR const* name = NULL, bool detail = true);

	void erase();

	void find_tgt_bb_list(IN IR * ir, OUT LIST<IR_BB*> & tgt_bbs);
	IR_BB * find_bb_by_label(LABEL_INFO * lab);
	void find_bbs(IR * ir, LIST<IR_BB*> & tgtlst);

	LABEL_INFO * get_xr_label(IR * ir) { return ir->get_label(); }
	LIST<LABEL_INFO*> * get_label_list(IR_BB * bb) { return &IR_BB_lab_list(bb); }

	virtual bool if_opt(IR_BB * bb);
	bool is_ru_entry(IR_BB * bb) { return IR_BB_is_entry(bb); }
	bool is_ru_exit(IR_BB * bb) { return  IR_BB_is_exit(bb); }
	virtual bool is_exp_handling(IR_BB const* bb) const
	{ return bb->is_exp_handling(); }
	virtual bool is_exp_jumpo(IR_BB const* bb) const
	{
		C<IR*> * ct;
		IR * x = IR_BB_ir_list(const_cast<IR_BB*>(bb)).get_tail(&ct);
		if (x != NULL && IR_may_throw(x)) {
			return true;
		}
		return false;
	}
	void insert_bb_between(IN IR_BB * from, IN C<IR_BB*> * from_ct,
						   IN IR_BB * to, IN C<IR_BB*> * to_ct,
						   IN IR_BB * newbb);

	//Return the first operation of 'bb'.
	IR * get_first_xr(IR_BB * bb)
	{
		IS_TRUE0(bb && m_bb_vec.get(IR_BB_id(bb)));
		return IR_BB_first_ir(bb);
	}

	//Return the last operation of 'bb'.
	IR * get_last_xr(IR_BB * bb)
	{
		IS_TRUE0(bb && m_bb_vec.get(IR_BB_id(bb)));
		return IR_BB_last_ir(bb);
	}
	INT get_bb_num() { return get_vertex_num(); }
	IR_BB_LIST * get_bb_list() { return m_bb_list; }
	LAB2BB * get_lab2bb_map() { return &m_lab2bb; }
	IR_BB * get_bb(INT id)
	{
		IS_TRUE0(id >= 0);
		return m_bb_vec.get(id);
	}
	virtual bool goto_opt(IR_BB * bb);

	//Find natural loop and scan loop body to find call and early exit, etc.
	void loop_analysis(OPT_CTX & oc);

	virtual void remove_bb(C<IR_BB*> * bbct)
	{
		IS_TRUE0(bbct);
		IS_TRUE0(m_bb_list->in_list(bbct));
		m_bb_vec.set(IR_BB_id(C_val(bbct)), NULL);
		m_bb_list->remove(bbct);
		remove_vertex(IR_BB_id(C_val(bbct)));
	}

	//We only remove 'bb' from CF graph, vector and bb-list.
	virtual void remove_bb(IR_BB * bb)
	{
		IS_TRUE0(bb);
		m_bb_vec.set(IR_BB_id(bb), NULL);
		m_bb_list->remove(bb);
		remove_vertex(IR_BB_id(bb));
	}
	void remove_xr(IR_BB * bb, IR * ir);
	bool remove_tramp_edge();
	bool remove_tramp_bb();
	bool remove_redundant_branch();

	virtual void set_rpo(IR_BB * bb, INT order) { IR_BB_rpo(bb) = order; }
	virtual void union_labs(IR_BB * src, IR_BB * tgt);
};
#endif
