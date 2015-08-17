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


typedef TMap<LabelInfo*, IRBB*> LAB2BB;


/*
NOTICE:
1. For accelerating perform operation of each vertex, e.g
   compute dominator, please try best to add vertex with
   topological order.
*/
class IR_CFG : public CFG<IRBB, IR> {
protected:
	Vector<IRBB*> m_bb_vec;
	LAB2BB m_lab2bb;
	Region * m_ru;
	TypeMgr * m_dm;
public:
	IR_CFG(CFG_SHAPE cs, BBList * bbl, Region * ru,
		   UINT edge_hash_size = 16, UINT vertex_hash_size = 16);
	COPY_CONSTRUCTOR(IR_CFG);
	virtual ~IR_CFG() {}

	//Add LABEL to bb, and establish map between label and bb.
	void add_lab(IRBB * src, LabelInfo * li)
	{
		src->addLabel(li);

		//Set label->bb map.
		m_lab2bb.aset(li, src);
	}

	/* Add new IRBB into CFG, but the BB list should be modified
	out of this function.
	Namely, you should use 'insertBBbetween()' to insert BB into list.
	And you must consider the right insertion. */
	inline void add_bb(IRBB * bb)
	{
		ASSERT0(bb && m_bb_vec.get(BB_id(bb)) == NULL);
		ASSERT(BB_id(bb) != 0, ("bb id should start at 1"));
		m_bb_vec.set(BB_id(bb), bb);
		addVertex(BB_id(bb));
	}

	virtual void cf_opt();
	void computeDomAndIdom(IN OUT OptCTX & oc, BitSet const* uni = NULL);
	void computePdomAndIpdom(IN OUT OptCTX & oc, BitSet const* uni = NULL);
	virtual void computeEntryAndExit(bool comp_entry, bool comp_exit)
	{
		ASSERT0(comp_entry | comp_exit);
		if (comp_entry) { m_entry_list.clean(); }
		if (comp_exit) { m_exit_list.clean(); }

		for (IRBB * bb = m_bb_list->get_head();
			 bb != NULL; bb = m_bb_list->get_next()) {
			Vertex * vex = get_vertex(bb->id);
			ASSERT(vex, ("No vertex corresponds to BB%d", bb->id));
			if (comp_entry) {
				if (VERTEX_in_list(vex) == NULL &&
					(is_ru_entry(bb) || is_exp_handling(bb))) {
					m_entry_list.append_tail(bb);
				}
			}
			if (comp_exit) {
				if (VERTEX_out_list(vex) == NULL) {
					m_exit_list.append_tail(bb);
					BB_is_exit(bb) = true;
				}
			}
		}
	}

	virtual void dump_vcg(CHAR const* name)
	{ CFG<IRBB, IR>::dump_vcg(name); }

	void dump_vcg(CHAR const* name = NULL, bool detail = true,
				  bool dump_eh = true);
	void dump_dot(CHAR const* name = NULL, bool detail = true);

	void erase();

	void findTargetBBOfMulticondBranch(IN IR * ir,
										OUT List<IRBB*> & tgt_bbs);
	IRBB * findBBbyLabel(LabelInfo * lab);
	void findTargetBBOfIndirectBranch(IR * ir, OUT List<IRBB*> & tgtlst);

	virtual bool if_opt(IRBB * bb);
	bool is_ru_entry(IRBB * bb) { return BB_is_entry(bb); }
	bool is_ru_exit(IRBB * bb) { return  BB_is_exit(bb); }
	virtual bool is_exp_handling(IRBB const* bb) const
	{ return bb->is_exp_handling(); }
	virtual bool is_exp_jumpo(IRBB const* bb) const
	{
		C<IR*> * ct;
		IR * x = BB_irlist(const_cast<IRBB*>(bb)).get_tail(&ct);
		if (x != NULL && IR_may_throw(x)) {
			return true;
		}
		return false;
	}
	void insertBBbetween(IN IRBB * from, IN C<IRBB*> * from_ct,
						   IN IRBB * to, IN C<IRBB*> * to_ct,
						   IN IRBB * newbb);

	//Return the first operation of 'bb'.
	IR * get_first_xr(IRBB * bb)
	{
		ASSERT0(bb && m_bb_vec.get(BB_id(bb)));
		return BB_first_ir(bb);
	}

	//Return the last operation of 'bb'.
	IR * get_last_xr(IRBB * bb)
	{
		ASSERT0(bb && m_bb_vec.get(BB_id(bb)));
		return BB_last_ir(bb);
	}

	Region * get_ru() { return m_ru; }
	UINT get_bb_num() const { return get_vertex_num(); }
	BBList * get_bb_list() { return m_bb_list; }
	LAB2BB * get_lab2bb_map() { return &m_lab2bb; }
	IRBB * get_bb(UINT id) const { return m_bb_vec.get(id); }
	virtual bool goto_opt(IRBB * bb);

	//Find natural loop and scan loop body to find call and early exit, etc.
	void LoopAnalysis(OptCTX & oc);

	//Compute which predecessor is pred to bb.
	//e.g: If pred is the first predecessor, return 0.
	//pred: BB id of predecessor.
	UINT WhichPred(IRBB const* pred, IRBB const* bb) const
	{
		Vertex * bb_vex = get_vertex(BB_id(bb));
		ASSERT0(bb_vex);

		UINT n = 0;
		bool find = false;
		for (EdgeC * in = VERTEX_in_list(bb_vex);
			 in != NULL; in = EC_next(in)) {
			Vertex * local_pred_vex = EDGE_from(EC_edge(in));
			if (VERTEX_id(local_pred_vex) == BB_id(pred)) {
				find = true;
				break;
			}
			n++;
		}

		CK_USE(find); //pred should be a predecessor of bb.

		return n;
	}

	virtual void remove_bb(C<IRBB*> * bbct)
	{
		ASSERT0(bbct);
		ASSERT0(m_bb_list->in_list(bbct));
		m_bb_vec.set(BB_id(C_val(bbct)), NULL);
		m_bb_list->remove(bbct);
		removeVertex(BB_id(C_val(bbct)));
	}

	//We only remove 'bb' from CF graph, vector and bb-list.
	virtual void remove_bb(IRBB * bb)
	{
		ASSERT0(bb);
		m_bb_vec.set(BB_id(bb), NULL);
		m_bb_list->remove(bb);
		removeVertex(BB_id(bb));
	}
	void remove_xr(IRBB * bb, IR * ir);
	bool removeTrampolinEdge();
	bool removeTrampolinBB();
	bool removeRedundantBranch();

	virtual void set_rpo(IRBB * bb, INT order) { BB_rpo(bb) = order; }
	virtual void unionLabels(IRBB * src, IRBB * tgt);

	/* Perform miscellaneous control flow optimizations.
	Include remove dead bb which is unreachable, remove empty bb as many
	as possible, simplify and remove the branch like "if (x==x)", remove
	the trampolin branch. */
	bool performMiscOpt(OptCTX & oc);
};
#endif
