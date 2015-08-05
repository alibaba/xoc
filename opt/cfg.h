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
#ifndef _CFG_H_
#define _CFG_H_

//Sort Sequence
typedef enum {
	SEQ_UNDEF = 0,
	SEQ_DFS,   //depth first sort
	SEQ_BFS,   //breadth first sort
	SEQ_TOPOL,   //topological sort
} SEQ_TYPE;


//CFG Shape
typedef enum {
	C_UNDEF = 0,
	C_SESE,    //single entry, single exit
	C_SEME,    //single entry, multi exit
	C_MEME,    //multi entry, multi exit
	C_MESE,    //multi entry, single exit
} CFG_SHAPE;


//CFG Edge Info.
#define EI_is_eh(ei)	((ei)->is_eh)
class EI {
public:
	BYTE is_eh:1; //true if edge describe eh edge.
};


/*
NOTICE:
1. For accelerating perform operation of each vertex, e.g
   compute dominator, please try best to add vertex with
   topological order.
*/
template <class BB, class XR> class CFG : public DGRAPH {
protected:
	LI<BB> * m_loop_info; //Loop information
	LIST<BB*> * m_bb_list;
	SVECTOR<LI<BB>*> m_map_bb2li;
	LIST<BB*> m_entry_list; //CFG GRAPH ENTRY list
	LIST<BB*> m_exit_list; //CFG GRAPH ENTRY list
	LIST<BB*> m_rpo_bblst; //record BB in reverse-post-order.
	SEQ_TYPE m_bb_sort_type;
	BITSET_MGR * m_bs_mgr;
	SMEM_POOL * m_pool;
	UINT m_li_count; //counter to loop.
	BYTE m_has_eh_edge:1;

	void _remove_unreach_bb(UINT id, BITSET & visited);
	void _remove_unreach_bb2(UINT id, BITSET & visited);
	void _sort_by_dfs(LIST<BB*> & new_bbl, BB * bb,
					  SVECTOR<bool> & visited);
	inline bool _is_loop_head(LI<BB> * li, BB * bb);

	inline void _collect_loop_info(LI<BB> * li);
	void _dump_loop_tree(LI<BB> * looplist, UINT indent, FILE * h);
	bool _insert_into_loop_tree(LI<BB> ** lilist, LI<BB> * loop);
	void compute_rpo_core(IN OUT BITSET & is_visited,
						  IN VERTEX * v, IN OUT INT & order);
	void * xmalloc(size_t size)
	{
		IS_TRUE0(m_pool);
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p);
		memset(p, 0, size);
		return p;
	}
public:
	CFG(LIST<BB*> * bb_list, UINT edge_hash_size = 16, UINT vertex_hash_size = 16)
		: DGRAPH(edge_hash_size, vertex_hash_size)
	{
		IS_TRUE0(bb_list);
		m_bb_list = bb_list;
		m_loop_info = NULL;
		m_bs_mgr = NULL;
		m_li_count = 1;
		m_pool = smpool_create_handle(sizeof(EI) * 4, MEM_COMM);
		set_dense(true);
	}

	virtual ~CFG()
	{ smpool_free_handle(m_pool); }

	virtual void add_break_out_loop_node(BB * loop_head, BITSET & body_set);
	void rebuild(OPT_CTX & oc)
	{
		erase();
		UINT newsz =
			MAX(16, get_nearest_power_of_2(m_bb_list->get_elem_count()));
		resize(newsz, newsz);
		build(oc);

		//One should call remove_empty_bb() immediately after this function , because
		//rebuilding cfg may generate redundant empty bb, it
		//disturb the computation of entry and exit.
	}

	void build(OPT_CTX & oc);
	void build_eh();

	void clean_loop_info();
	bool compute_dom(BITSET const* uni)
	{
		IS_TRUE(m_entry_list.get_elem_count() == 1,
				("ONLY support SESE or SEME"));
		BB const* root = m_entry_list.get_head();
		LIST<VERTEX const*> vlst;
		this->compute_rpo_norec(get_vertex(root->id), vlst);
		return DGRAPH::compute_dom(&vlst, uni);
	}

	//CFG may have multiple exit. The method computing pdom is not
	//different with dom.
	bool compute_pdom(BITSET const* uni)
	{
		//IS_TRUE(m_exit_list.get_elem_count() == 1,
		//		 ("ONLY support SESE or SEME"));
		BB const* root = m_entry_list.get_head();
		IS_TRUE0(root);
		return DGRAPH::compute_pdom_by_rpo(get_vertex(root->id), uni);
	}

	//Compute the entry bb.
	//Only the function entry and try and catch entry BB can be CFG entry.
	virtual void compute_entry_and_exit(bool comp_entry, bool comp_exit)
	{
		IS_TRUE0(comp_entry | comp_exit);
		if (comp_entry) { m_entry_list.clean(); }
		if (comp_exit) { m_exit_list.clean(); }

		for (BB * bb = m_bb_list->get_head();
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
				}
			}
		}
	}

	//Collecting loop info .e.g has-call, has-goto.
	void collect_loop_info()
	{ _collect_loop_info(m_loop_info); }

	virtual void chain_pred_succ(INT vid, bool is_single_pred_succ = false);
	void compute_rpo(OPT_CTX & oc);
	UINT count_mem() const
	{
		UINT count = 0;
		count += sizeof(m_loop_info);
		count += sizeof(m_bb_list); //do NOT count up BBs in bb_list.
		count += m_map_bb2li.count_mem();
		count += m_entry_list.count_mem();
		count += m_exit_list.count_mem();
		count += sizeof(BYTE);
		count += sizeof(m_bb_sort_type);
		count += sizeof(m_bs_mgr);
		return count;
	}

	virtual void dump_loop(FILE * h)
	{
		if (h == NULL) return;
		fprintf(h, "\n==---- DUMP Natural Loop Tree ----==");
		_dump_loop_tree(m_loop_info, 0, h);
		fflush(h);
	}

	virtual void dump_vcg(CHAR const* name = NULL);

	bool find_loop();

	//Find the target bb list.
	virtual void find_tgt_bb_list(IN XR * xr, OUT LIST<BB*> & tgt_bbs)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	//Return the last operation of 'bb'.
	virtual BB * find_bb_by_label(LABEL_INFO * lab_xr)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return NULL;
	}

	virtual void find_bbs(IN XR * xr, OUT LIST<BB*> & tgtlst)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	UINT get_loop_num() const { return m_li_count - 1; }
	void get_preds(IN OUT LIST<BB*> & preds, IN BB const* v);
	void get_succs(IN OUT LIST<BB*> & succs, IN BB const* v);
	virtual LIST<BB*> * get_entry_list() { return &m_entry_list; }
	virtual LIST<BB*> * get_exit_list() { return &m_exit_list; }
	virtual BB * get_last_exit();

	//Return the last operation of 'bb'.
	virtual XR * get_last_xr(BB * bb)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return NULL;
	}

	virtual XR * get_first_xr(BB * bb)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return NULL;
	}

	virtual LABEL_INFO * get_xr_label(XR * xr)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return NULL;
	}

	virtual LIST<LABEL_INFO*> * get_label_list(BB * bb)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return NULL;
	}

	LIST<BB*> * get_bblist_in_rpo() { return &m_rpo_bblst; }
	virtual BB * get_bb(INT id)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return NULL;
	}

	virtual BB * get_fallthrough_bb(BB * bb)
	{
		IS_TRUE0(bb);
		C<BB*> * ct;
		IS_TRUE0(m_bb_list->find(bb, &ct));
		m_bb_list->find(bb, &ct);
		BB * res = m_bb_list->get_next(&ct);
		return res;
	}

	virtual BB * get_target_bb(BB * bb)
	{
		IS_TRUE0(bb);
		XR * xr = get_last_xr(bb);
		IS_TRUE0(xr != NULL);
		LABEL_INFO * lab = get_xr_label(xr);
		IS_TRUE0(lab != NULL);
		BB * target = find_bb_by_label(lab);
		IS_TRUE0(target != NULL);
		return target;
	}

	BB * get_idom(BB * bb)
	{
		IS_TRUE0(bb != NULL);
		return get_bb(DGRAPH::get_idom(bb->id));
	}

	BB * get_ipdom(BB * bb)
	{
		IS_TRUE0(bb != NULL);
		return get_bb(DGRAPH::get_ipdom(bb->id));
	}

	LI<BB> * get_loop_info() { return m_loop_info; }
	void get_if_three_kids(BB * bb, BB ** true_body,
						   BB ** false_body, BB ** sibling);
	void get_loop_two_kids(IN BB * bb, OUT BB ** sibling, OUT BB ** body_root);

	bool has_eh_edge() const { return m_has_eh_edge; }

	void identify_natural_loop(UINT x, UINT y, IN OUT BITSET & loop,
								LIST<INT> & tmp);

	virtual bool is_cfg_entry(UINT bbid)
	{ return GRAPH::is_graph_entry(get_vertex(bbid)); }

	//Return true if bb is exit BB of CFG.
	virtual bool is_cfg_exit(UINT bbid)
	{ return GRAPH::is_graph_exit(get_vertex(bbid)); }

	//Return true if bb is entry BB of function.
	virtual bool is_ru_entry(BB * bb)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return false;
	}

	//Return true if bb is exit BB of function.
	virtual bool is_ru_exit(BB * bb)
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return false;
	}

	//Return true if bb may throw exception.
	virtual bool is_exp_jumpo(BB const* bb) const
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return false;
	}
	//Return true if bb is exception handling.
	virtual bool is_exp_handling(BB const* bb) const
	{
		IS_TRUE(0, ("Target Dependent Code"));
		return false;
	}

	virtual bool is_loop_head(BB * bb)
	{ return _is_loop_head(m_loop_info, bb); }

	virtual LI<BB> * map_bb2li(BB * bb)
	{ return m_map_bb2li.get(bb->id); }

	//void remove_edge(BB * bb, XR * xr);
	virtual void remove_xr(BB * bb, XR * xr)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	virtual void remove_bb(BB * bb)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	virtual void remove_bb(C<BB*> * bbct)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	void remove_edge(BB * from, BB * to)
	{
		EDGE * e = GRAPH::get_edge(from->id, to->id);
		IS_TRUE0(e != NULL);
		GRAPH::remove_edge(e);
	}

	bool remove_empty_bb(OPT_CTX & oc);
	bool remove_unreach_bb();
	bool remove_redundant_branch();

	/* Insert unconditional branch to revise fall through bb.
	e.g: Given bblist is bb1-bb2-bb3-bb4, bb4 is exit-BB,
	and flow edges are: bb1->bb2->bb3->bb4, bb1->bb3,
	where bb1->bb2, bb2->bb3, bb3->bb4 are fallthrough edge.

	Assuming the reordered bblist is bb1-bb3-bb4-bb2, the
	associated flow edges are
	bb1->bb3->bb4, bb2->bb3.
	It is obviously that converting bb1->bb3 to be fallthrough,
	and converting bb1->bb2 to be conditional branch, converting
	bb2->bb3 to be unconditional branch. */
	void revise_fallthrough(LIST<BB*> & new_bbl)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	void sort_by_dfs();
	void sort_by_bfs();
	void sort_by_topol();

	/* Set entry and exit node.
	Entry node for computing of dominators of each node
	Entry node for computing of post-dominators of each node. */
	void set_sese(BB * entry, BB * exit)
	{
		IS_TRUE0(entry >= 0 && exit >= 0);
		m_entry_list.clean();
		m_exit_list.clean();
		m_entry_list.append_head(entry);
		m_exit_list.append_head(exit);
	}

	void set_bs_mgr(BITSET_MGR * bs_mgr)
	{
		m_bs_mgr = bs_mgr;
		DGRAPH::set_bs_mgr(bs_mgr);
	}

	virtual void set_rpo(BB * bb, INT order)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	virtual void union_labs(BB * src, BB * tgt)
	{ IS_TRUE(0, ("Target Dependent Code")); }

	bool verify_rmbb(IN CDG * cdg, OPT_CTX & oc);
	bool verify()
	{
		//The entry node must not have predecessors.
		for (BB * bb = m_entry_list.get_head();
			 bb != NULL; bb = m_entry_list.get_next()) {

			VERTEX * vex = get_vertex(bb->id);
			IS_TRUE0(vex && get_in_degree(vex) == 0);
		}

		//The exit node must not have successors.
		for (BB * bb = m_exit_list.get_head();
			 bb != NULL; bb = m_exit_list.get_next()) {
			VERTEX * vex = get_vertex(bb->id);
			IS_TRUE0(vex && get_out_degree(vex) == 0);
		}
		return true;
	}
};


/* Find and Return LOOP_SIBLING and BODY_ROOT.
e.g:
	LOOP
		BODY_ROOT
	END_LOOP
	LOOP_SIBLING
*/
template <class BB, class XR>
void CFG<BB, XR>::get_loop_two_kids(IN BB * bb, OUT BB ** sibling,
									OUT BB ** body_root)
{
	LI<BB> * li = map_bb2li(bb);
	IS_TRUE0(li != NULL && LI_loop_head(li) == bb);
	LIST<BB*> succs;
	get_succs(succs, bb);
	IS_TRUE0(succs.get_elem_count() == 2);
	BB * s = succs.get_head();
	if (sibling != NULL) {
		*sibling = li->inside_loop(s->id) ? succs.get_tail() : s;
	}

	if (body_root != NULL) {
		*body_root = li->inside_loop(s->id) ? s : succs.get_tail();
	}
}


/* Find and Return TRUE_BODY, FALSE_BODY, IF_SIBLING.
e.g:
	IF
		TRUE_BODY
	ELSE
		FALSE_BODY
	END_IF
	IF_SIBLING
*/
template <class BB, class XR>
void CFG<BB, XR>::get_if_three_kids(BB * bb, BB ** true_body,
									BB ** false_body, BB ** sibling)
{
	if (true_body != NULL || false_body != NULL) {
		UINT ipdom = DGRAPH::get_ipdom(bb->id);
		IS_TRUE(ipdom > 0, ("bb does not have ipdom"));
		BB * fallthrough_bb = get_fallthrough_bb(bb);
		BB * target_bb = get_target_bb(bb);
		XR * xr = get_last_xr(bb);
		IS_TRUE0(xr != NULL && xr->is_cond_br());
		if (xr->is_truebr()) {
			if (true_body != NULL) {
				if (ipdom == target_bb->id) {
					*true_body = NULL;
				} else {
					*true_body = target_bb;
				}
			}
			if (false_body != NULL) {
				if (ipdom == fallthrough_bb->id) {
					*false_body = NULL;
				} else {
					*false_body = fallthrough_bb;
				}
			}
		} else {
			IS_TRUE0(xr->is_falsebr());
			if (true_body != NULL) {
				if (ipdom == fallthrough_bb->id) {
					*true_body = NULL;
				} else {
					*true_body = fallthrough_bb;
				}
			}
			if (false_body != NULL) {
				if (ipdom == target_bb->id) {
					*false_body = NULL;
				} else {
					*false_body = target_bb;
				}
			}
		} //end if
	}
	if (sibling != NULL) {
		UINT ipdom = DGRAPH::get_ipdom(bb->id);
		IS_TRUE(ipdom > 0, ("bb does not have ipdom"));
		*sibling = get_bb(ipdom);
	}
}


template <class BB, class XR>
void CFG<BB, XR>::_dump_loop_tree(LI<BB> * looplist, UINT indent, FILE * h)
{
	while (looplist != NULL) {
		fprintf(h, "\n");
		for (UINT i = 0; i < indent; i++) { fprintf(h, " "); }
		fprintf(h, "LOOP HEAD:BB%d, BODY:",
					LI_loop_head(looplist)->id);
		for (INT i = LI_bb_set(looplist)->get_first(); i != -1;
			 i = LI_bb_set(looplist)->get_next(i)) {
			fprintf(h, "%d,", i);
		}
		_dump_loop_tree(LI_inner_list(looplist), indent + 2, h);
		looplist = LI_next(looplist);
	}
}


template <class BB, class XR>
bool CFG<BB, XR>::verify_rmbb(IN CDG * cdg, OPT_CTX & oc)
{
	IS_TRUE(cdg, ("DEBUG: verify need cdg."));
	C<BB*> * ct, * next_ct;
	LIST<BB*> succs;
	bool is_cfg_valid = OPTC_is_cfg_valid(oc);
	for (m_bb_list->get_head(&ct), next_ct = ct;
		 ct != NULL; ct = next_ct) {
		m_bb_list->get_next(&next_ct);
		BB * bb = C_val(ct);
		BB * next_bb = NULL;
		if (next_ct != NULL) {
			next_bb = C_val(next_ct);
		}
		if (get_last_xr(bb) == NULL && !is_ru_entry(bb) &&
			!is_exp_handling(bb)) {
			if (next_bb == NULL) {
				continue;
			}
			if (is_cfg_valid) {
				get_succs(succs, bb);
				/* CASE:
					BB1
					LOOP_HEADER(BB2)
						LOOP_BODY(BB3)
					ENDLOOP
					BB5

				There are edges: BB1->BB2->BB3, BB2->BB5, BB3->BB2
				Where BB3->BB2 is back edge.
				When we remove BB2, add edge BB3->BB5 */
				if (succs.get_elem_count() > 1) {
					for (BB * succ = succs.get_head();
						 succ != NULL; succ = succs.get_next()) {
						if (succ == next_bb || succ == bb) {
							continue;
						}
						EDGE * e = get_edge(bb->id, succ->id);
						if (EDGE_info(e) != NULL &&
							EI_is_eh((EI*)EDGE_info(e))) {
							continue;
						}
						if (!cdg->is_cd(bb->id, succ->id)) {
							//bb should not be empty, need goto.
							IS_TRUE0(0);
						}
					}
				}
			} //end if
		} //end if
	} //end for each bb
	return true;
}


//Remove empty bb, and merger label info.
template <class BB, class XR>
bool CFG<BB, XR>::remove_empty_bb(OPT_CTX & oc)
{
	START_TIMER("Remove Empty BB");
	C<BB*> * ct, * next_ct;
	bool doit = false;
	LIST<BB*> succs;
	LIST<BB*> preds;
	bool is_cfg_valid = OPTC_is_cfg_valid(oc);
	for (m_bb_list->get_head(&ct), next_ct = ct;
		 ct != NULL; ct = next_ct) {
		m_bb_list->get_next(&next_ct);
		BB * bb = C_val(ct);
		BB * next_bb = NULL;
		if (next_ct != NULL) {
			next_bb = C_val(next_ct);
		}

		/* TODO: confirm if this is right:
			is_ru_exit() need to update if cfg
			changed or ir_bb_list reconstruct.
			e.g: void m(bool r, bool y)
				{
					bool l;
					l = y || r;
					return 0;
				}
		After init_cfg(), there are 2 bb, BB1 and BB3.
		While IR_LOR simpilified, new bb generated, then func-exit bb flag
		need to update. */
		if (get_last_xr(bb) == NULL && !is_ru_entry(bb) &&
			!is_exp_handling(bb)) {
			/* Do not replace is_ru_exit/entry() to is_cfg_exit/entry().
			Some redundant cfg has multi bb which
			satifies cfg-entry condition. */
			if (next_bb == NULL) {
				//bb is the last empty bb.
				IS_TRUE0(next_ct == NULL);
				if (get_label_list(bb)->get_elem_count() == 0 &&
					!is_ru_exit(bb)) {
					remove_bb(ct);
					doit = true;
				}
				continue;
			}

			union_labs(bb, next_bb);
			if (is_cfg_valid) {
				get_preds(preds, bb);
				get_succs(succs, bb);
				for (BB * pred = preds.get_head();
					 pred != NULL; pred = preds.get_next()) {
					if (get_vertex(pred->id) == NULL) {
						continue;
					}
					add_edge(pred->id, next_bb->id);
				}

				for (BB * succ = succs.get_head();
					 succ != NULL; succ = succs.get_next()) {
					if (get_vertex(succ->id) == NULL || succ == next_bb) {
						continue;
					}
					C<BB*> * tmp_ct;
					m_bb_list->find(succ, &tmp_ct);
					IS_TRUE0(tmp_ct != NULL);
					m_bb_list->get_prev(&tmp_ct);
					if (tmp_ct != NULL) {
						BB * prev_bb_of_succ = C_val(tmp_ct);
						IS_TRUE0(prev_bb_of_succ != NULL);
						XR * last_xr = get_last_xr(prev_bb_of_succ);
						if (last_xr == NULL ||
							(!last_xr->is_cond_br() &&
							 !last_xr->is_uncond_br() &&
							 !last_xr->is_return())) {
							//Add fall-through edge.
							add_edge(prev_bb_of_succ->id, succ->id);
						} else if (last_xr != NULL &&
								   last_xr->is_uncond_br() &&
								   !last_xr->is_indirect_br()) {
							/*
							BB1:
								...
								goto L1  <--- redundant branch

							BB2:
								L1:
							*/
							BB * tgt_bb =
								find_bb_by_label(get_xr_label(last_xr));
							IS_TRUE0(tgt_bb != NULL);
							if (tgt_bb == succ) {
								add_edge(prev_bb_of_succ->id, succ->id);
							}
						}
					} //end if (tmp_ct != NULL...
				} //end for each succ
				remove_bb(bb);
				doit = true;
			} //end if
		} //end if
	} //end for each bb
	END_TIMER();
	return doit;
}


//Remove redundant branch edge.
template <class BB, class XR>
bool CFG<BB, XR>::remove_redundant_branch()
{
	START_TIMER("Remove Redundant Branch");
	C<BB*> * ct, * next_ct;
	bool doit = false;
	for (m_bb_list->get_head(&ct), next_ct = ct;
		 ct != NULL; ct = next_ct) {
		m_bb_list->get_next(&next_ct);
		BB * bb = C_val(ct);
		BB * next_bb = NULL; //next_bb is fallthrough BB.
		if (next_ct != NULL) {
			next_bb = C_val(next_ct);
		}
		XR * xr = get_last_xr(bb);
		if (xr == NULL) {
			/* Although bb is empty, it may have some labels attached,
			which may have dedicated usage. Do not remove it for
			convservative purpose. */
			IS_TRUE(is_cfg_entry(bb->id) || is_cfg_exit(bb->id) ||
					get_label_list(bb)->get_elem_count() != 0,
					("should call remove_empty_bb() first."));
			continue;
		}
		if (xr->is_cond_br()) {
			/* CASE:
				BB1:
				falsebr L0 //S1

				BB2:
				L0  //S2
				... //S3

			S1 is redundant branch. */
			VERTEX * v = get_vertex(bb->id);
			EDGE_C * el = VERTEX_out_list(v);
			EDGE_C * last_el = NULL;
			bool find = false; //find different succ.
			while (el != NULL) {
				last_el = el;
				BB * succ = get_bb(VERTEX_id(EDGE_to(EC_edge(el))));
				if (succ != next_bb) {
					find = true;
					break;
				}
				el = EC_next(el);
			}
			if (!find) {
				el = last_el;
				while (EC_prev(el) != NULL) {
					EDGE_C * tmp = el;
					el = EC_prev(el);
					GRAPH::remove_edge(EC_edge(tmp));
				}
				remove_xr(bb, xr);
				doit = true;
			}
		} else if (xr->is_uncond_br() && !xr->is_indirect_br()) {
			/*
			BB1:
				...
				goto L1  <--- redundant branch

			BB2:
				L1:
			*/
			BB * tgt_bb = find_bb_by_label(get_xr_label(xr));
			IS_TRUE0(tgt_bb != NULL);
			if (tgt_bb == next_bb) {
				remove_xr(bb, xr);
				doit = true;
			}
		}
	}
	END_TIMER();
	return doit;
}


template <class BB, class XR>
void CFG<BB, XR>::_sort_by_dfs(LIST<BB*> & new_bbl, BB * bb,
							   SVECTOR<bool> & visited)
{
	if (bb == NULL) return;
	visited.set(bb->id, true);
	new_bbl.append_tail(bb);
	LIST<BB* > succs;
	get_succs(succs, bb);
	for (BB * succ = succs.get_head();
		 succ != NULL; succ = succs.get_next()) {
		if (!visited.get(succ->id)) {
			_sort_by_dfs(new_bbl, succ, visited);
		}
	}
	return;
}


/* Sort BBs in order of DFS.
NOTICE:
	Be careful use this function. Because we will emit IR in terms of
	the order which 'm_bbl' holds. */
template <class BB, class XR>
void CFG<BB, XR>::sort_by_dfs()
{
	LIST<BB*> new_bbl;
	SVECTOR<bool> visited;
	for (BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {
		if (!visited.get(bb->id)) {
			_sort_by_dfs(new_bbl, bb, visited);
		}
	}
	#ifdef _DEBUG_
	for (BB * bbc = m_bb_list->get_head();
		 bbc != NULL; bbc = m_bb_list->get_next()) {
		IS_TRUE(visited.get(bbc->id),
				("unreachable BB, call remove_unreach_bb()"));
	}
	#endif
	revise_fallthrough(new_bbl);
	m_bb_list->copy(new_bbl);
	m_bb_sort_type = SEQ_DFS;
}


/* Sort BBs in order of BFS.
NOTICE:
	Be careful use this function. Because we will emit IR in terms of
	the order which 'm_bbl' holds. */
template <class BB, class XR>
void CFG<BB, XR>::sort_by_bfs()
{
	LIST<BB*> foottrip;
	LIST<BB*> new_bbl;
	LIST<BB*> succs;
	SVECTOR<bool> visited;
	for (BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {
		if (!visited.get(bb->id)) {
			visited.set(bb->id, true);
			new_bbl.append_tail(bb);
			foottrip.append_head(bb);
			while (foottrip.get_elem_count() > 0) {
				BB * tail = foottrip.remove_tail();
				get_succs(succs, bb);
				for (BB * succ = succs.get_head();
					 succ != NULL; succ = succs.get_next()) {
					if (!visited.get(succ->id)) {
						visited.set(succ->id, true);
						new_bbl.append_tail(succ);
						foottrip.append_head(succ);
					}
				}
			}
		}
	}
	#ifdef _DEBUG_
	for (BB * bbc = m_bb_list->get_head();
		 bbc != NULL; bbc = m_bb_list->get_next()) {
		IS_TRUE(visited.get(bbc->id),
				("unreachable BB, call remove_unreach_bb()"));
	}
	#endif
	revise_fallthrough(new_bbl);
	m_bb_list->copy(new_bbl);
	m_bb_sort_type = SEQ_BFS;
}


/* Sort BBs in order of topology.
NOTICE:
	Be careful use this function.
	Because we will emit IR in terms of
	the order which 'm_bbl' holds. */
template <class BB, class XR>
void CFG<BB, XR>::sort_by_topol()
{
	GRAPH g;
	g.clone(*this);
	LIST<BB*> new_bbl;
	LIST<VERTEX*> succ;
	INT c;
	while (g.get_vertex_num() > 0) {
		LIST<VERTEX*> header;
		for (VERTEX * vex = g.get_first_vertex(c);
			 vex != NULL; vex = g.get_next_vertex(c)) {
			if (g.get_in_degree(vex) == 0) {
				header.append_tail(vex);
			}
		}
		if (header.get_elem_count() == 0) {
			header.append_tail(succ.get_head());
		}
		succ.clean();
		IS_TRUE(header.get_elem_count() > 0, ("No entry BB"));
		for (VERTEX * v = header.get_head();
			 v != NULL; v = header.get_next()) {
			EDGE_C * el = VERTEX_out_list(v);
			if (el) {
				//record the first succ.
				succ.append_tail(EDGE_to(EC_edge(el)));
			}
			g.remove_vertex(v);
			new_bbl.append_tail(get_bb(VERTEX_id(v)));
		}
	}
	revise_fallthrough(new_bbl);
	m_bb_list->copy(new_bbl);
	m_bb_sort_type = SEQ_TOPOL;
}


template <class BB, class XR>
void CFG<BB, XR>::_remove_unreach_bb2(UINT id, BITSET & visited)
{
	visited.bunion(id);
	EDGE_C * el = VERTEX_out_list(get_vertex(id));
	while (el != NULL) {
		UINT succ = VERTEX_id(EDGE_to(EC_edge(el)));
		if (!visited.is_contain(succ)) {
			_remove_unreach_bb(succ, visited);
		}
		el = EC_next(el);
	}
}


template <class BB, class XR>
void CFG<BB, XR>::_remove_unreach_bb(UINT id, BITSET & visited)
{
	LIST<VERTEX*> wl;
	IS_TRUE0(get_vertex(id));
	wl.append_tail(get_vertex(id));
	visited.bunion(id);

	VERTEX * v;
	while ((v = wl.remove_head()) != NULL) {
		EDGE_C * el = VERTEX_out_list(v);
		while (el != NULL) {
			VERTEX * succv = EDGE_to(EC_edge(el));
			if (!visited.is_contain(VERTEX_id(succv))) {
				wl.append_tail(succv);
				visited.bunion(VERTEX_id(succv));
			}
			el = EC_next(el);
		}
	}
}


/* Perform DFS to seek for unreachable BB, removing the 'dead-BB', and
free its ir-list. Return true if some dead-BB removed. */
template <class BB, class XR>
bool CFG<BB, XR>::remove_unreach_bb()
{
	bool removed = false;
	IS_TRUE0(m_bb_list);
	if (m_bb_list->get_elem_count() == 0) { return false; }

	START_TIMER("Remove Unreach BB");

	//There is only one entry point.
	BITSET visited;
	visited.bunion(m_bb_list->get_elem_count());
	visited.diff(m_bb_list->get_elem_count());

	IS_TRUE(m_entry_list.get_elem_count() > 0,
			("call compute_entry_and_exit first"));

	if (m_entry_list.get_elem_count() == 1) {
		BB * entry = m_entry_list.get_head();
		IS_TRUE(entry != NULL, ("need entry bb"));
		if (!visited.is_contain(entry->id)) {
			_remove_unreach_bb(entry->id, visited);
		}
	} else { //multiple entries
		for (BB * bb = m_entry_list.get_head();
			 bb != NULL; bb = m_entry_list.get_next()) {
			if (!visited.is_contain(bb->id)) {
				_remove_unreach_bb(bb->id, visited);
			}
		}
	}

	BB * next;
	C<BB*> * ct;
	for (BB * bb = m_bb_list->get_head(&ct); bb != NULL; bb = next) {
		next = m_bb_list->get_next(&ct);
		if (!visited.is_contain(bb->id)) {
			IS_TRUE(!is_exp_handling(bb),
				("For conservative purpose, exception handler is reserved."));

			remove_bb(bb);

			removed = true;
		}
	}

	END_TIMER();
	return removed;
}


//Chains all predessors and successors.
template <class BB, class XR>
void CFG<BB, XR>::chain_pred_succ(INT vid, bool is_single_pred_succ)
{
	VERTEX * v = get_vertex(vid);
	IS_TRUE0(v);
	EDGE_C * pred_lst = VERTEX_in_list(v);
	EDGE_C * succ_lst = VERTEX_out_list(v);
	if (is_single_pred_succ) {
		IS_TRUE(get_in_degree(v) <= 1 &&
				get_out_degree(v) <= 1,
				("BB only has solely pred and succ."));
	}
	while (pred_lst != NULL) {
		INT from = VERTEX_id(EDGE_from(EC_edge(pred_lst)));
		EDGE_C * tmp_succ_lst = succ_lst;
		while (tmp_succ_lst != NULL) {
			INT to = VERTEX_id(EDGE_to(EC_edge(tmp_succ_lst)));
			add_edge(from, to);
			tmp_succ_lst = EC_next(tmp_succ_lst);
		}
		pred_lst = EC_next(pred_lst);
	}
}


//Return all successors.
template <class BB, class XR>
void CFG<BB, XR>::get_succs(IN OUT LIST<BB*> & succs, IN BB const* v)
{
	IS_TRUE0(v);
	VERTEX * vex = get_vertex(v->id);
	EDGE_C * el = VERTEX_out_list(vex);
	succs.clean();
	while (el != NULL) {
		INT succ = VERTEX_id(EDGE_to(EC_edge(el)));
		succs.append_tail(get_bb(succ));
		el = EC_next(el);
	}
}


//Return the latest EXIT BB if there are multiple exits.
template <class BB, class XR>
BB * CFG<BB, XR>::get_last_exit()
{
	if (m_entry_list.get_elem_count() != 1) {
		return NULL;
	}
	BB * entry = m_entry_list.get_head();
	C<BB*> * ct;
	for (BB * bb = m_exit_list.get_head(&ct);
		 bb != NULL; bb = m_exit_list.get_next(&ct)) {
		if (get_pdom_set(entry->id)->is_contain(bb->id)) {
			return bb;
		}
	}
	return NULL;
}


//Return all predecessors.
template <class BB, class XR>
void CFG<BB, XR>::get_preds(IN OUT LIST<BB*> & preds, IN BB const* v)
{
	IS_TRUE0(v);
	VERTEX * vex = get_vertex(v->id);
	EDGE_C * el = VERTEX_in_list(vex);
	preds.clean();
	while (el != NULL) {
		INT pred = VERTEX_id(EDGE_from(EC_edge(el)));
		preds.append_tail(get_bb(pred));
		el = EC_next(el);
	}
}


//Construct cfg.
//Append exit bb if necessary when cfg is constructed.
template <class BB, class XR>
void CFG<BB, XR>::build(OPT_CTX & oc)
{
	m_entry_list.clean();
	m_exit_list.clean();

	BB * next = NULL;
	IS_TRUE(m_bb_list, ("bb_list is emt"));
	C<BB*> * holder = NULL;
	for (BB * bb = m_bb_list->get_head(&holder); bb != NULL; bb = next) {
		next = m_bb_list->get_next(&holder);

		XR * last = get_last_xr(bb);
		if (last == NULL) {
			/* Remove empty bb after CFG done.
			IS_TRUE(bb->is_bb_exit(), ("Should be removed!"));
			Add fall-through edge.
			The last bb may not be terminated by 'return' stmt. */
			if (next != NULL && !next->is_unreachable()) {
				add_edge(bb->id, next->id);
			}
			continue;
		}

		//Check bb boundary
		if (last->is_call()) {
			//Add fall-through edge
			//The last bb may not be terminated by 'return' stmt.
			if (next != NULL && !next->is_unreachable()) {
				add_edge(bb->id, next->id);
			}
		} else if (last->is_cond_br()) {
			//Add fall-through edge
			//The last bb may not be terminated by 'return' stmt.
			if (next != NULL && !next->is_unreachable()) {
				add_edge(bb->id, next->id);
			}
			//Add edge between source BB and target BB.
			BB * target_bb = find_bb_by_label(get_xr_label(last));
			IS_TRUE(target_bb != NULL, ("target cannot be NULL"));
			add_edge(bb->id, target_bb->id);
		} else if (last->is_multicond_br()) {
			//Add fall-through edge
			//The last bb may not be terminated by 'return' stmt.
			if (next != NULL && !next->is_unreachable()) {
				add_edge(bb->id, next->id);
			}

			//Add edge between source BB and multi-target BBs.
			LIST<BB*> tgt_bbs;
			find_tgt_bb_list(last, tgt_bbs);
			for (BB * tbb = tgt_bbs.get_head();
				 tbb != NULL; tbb = tgt_bbs.get_next()) {
				add_edge(bb->id, tbb->id);
			}
		} else if (last->is_uncond_br()) {
			if (last->is_indirect_br()) {
				LIST<BB*> tgtlst;
				find_bbs(last, tgtlst);
				for (BB * t = tgtlst.get_head();
					 t != NULL; t = tgtlst.get_next()) {
					add_edge(bb->id, t->id);
				}
			} else {
				//Add edge between source BB and target BB.
				BB * target_bb = find_bb_by_label(get_xr_label(last));
				IS_TRUE(target_bb != NULL, ("target cannot be NULL"));
				add_edge(bb->id, target_bb->id);
			}
		} else if (!last->is_return() && !last->is_terminate()) {
			//Add fall-through edge.
			//The last bb may not end by 'return' stmt.
			if (next != NULL && !next->is_unreachable()) {
				add_edge(bb->id, next->id);
			}
		}
	}
	OPTC_is_cfg_valid(oc) = true;
}


//Construct eh edge after cfg is built.
template <class BB, class XR>
void CFG<BB, XR>::build_eh()
{
	IS_TRUE(m_bb_list, ("bb_list is emt"));
	LIST<BB*> jumpo;
	LIST<BB*> ehl;
	BB * e = NULL;
	for (BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {
		if (is_ru_entry(bb)) {
			IS_TRUE(e == NULL, ("multi entries"));
			e = bb;
			//jumpo.append_tail(bb);
		}

		if (is_exp_handling(bb)) {
			ehl.append_tail(bb);
		}

		if (is_exp_jumpo(bb)) {
			jumpo.append_tail(bb);
		}
	}

	if (ehl.get_elem_count() == 0) { return; }

	for (BB * b = ehl.get_head(); b != NULL; b = ehl.get_next()) {
		for (BB * a = jumpo.get_head(); a != NULL; a = jumpo.get_next()) {
			EDGE * e = add_edge(a->id, b->id);
			EDGE_info(e) = xmalloc(sizeof(EI));
			EI_is_eh((EI*)EDGE_info(e)) = true;
			m_has_eh_edge = true;
		}
	}
}


template <class BB, class XR>
void CFG<BB, XR>::_collect_loop_info(LI<BB> * li)
{
	if (li == NULL) { return; }
	LI<BB> * subli = LI_inner_list(li);
	while (subli != NULL) {
		_collect_loop_info(subli);
		LI_has_call(li) = LI_has_call(subli);
		LI_has_early_exit(li) = LI_has_early_exit(subli);
		subli = LI_next(subli);
	}

	/* A BB list is used in the CFG to describing sparse node layout.
	In actually, such situation is rarely happen.
	So we just use 'bb_set' for now. (see loop.h) */
	BITSET * bbset = LI_bb_set(li);
	IS_TRUE0(bbset != NULL);
	for (INT id = bbset->get_first(); id != -1; id = bbset->get_next(id)) {
		BB * bb = get_bb(id);
		IS_TRUE0(bb != NULL);
		if (bb->is_bb_has_call()) {
			LI_has_call(li) = true;
		}
	}
}


//Insert loop into loop tree.
template <class BB, class XR>
bool CFG<BB, XR>::_insert_into_loop_tree(LI<BB> ** lilist, LI<BB>* loop)
{
	IS_TRUE0(lilist != NULL && loop != NULL);
	if (*lilist == NULL) {
		*lilist = loop;
		return true;
	}

	LI<BB> * li = *lilist, * cur = NULL;
	while (li != NULL) {
		cur = li;
		li = LI_next(li);
		if (LI_bb_set(cur)->is_contain(*LI_bb_set(loop))) {
			if (_insert_into_loop_tree(&LI_inner_list(cur), loop)) {
				return true;
			}
		} else if (LI_bb_set(loop)->is_contain(*LI_bb_set(cur))) {
			remove(lilist, cur);
			_insert_into_loop_tree(&LI_inner_list(loop), cur);
			IS_TRUE(LI_inner_list(loop), ("illegal loop tree"));
		} //end if
	} //end while
	add_next(lilist, loop);
	return true;
}


/* Add BB which is break-point of loop into loop.
e.g:
	for (i)
		if (i < 10)
			foo(A);
		else
			foo(B);
			goto L1;
		endif
	endfor
	...
	L1:

where foo(B) and goto L1 are in BBx, and BBx
should belong to loop body. */
template <class BB, class XR>
void CFG<BB, XR>::add_break_out_loop_node(BB * loop_head, BITSET & body_set)
{
	for (INT i = body_set.get_first(); i >= 0; i = body_set.get_next(i)) {
		if (i == (INT)loop_head->id) { continue; }
		VERTEX * v = get_vertex(i);
		IS_TRUE0(v);
		EDGE_C * out = VERTEX_out_list(v);
		UINT c = 0;
		while (out != NULL) {
			c++;
			if (c >= 2) {
				break;
			}
			out = EC_next(out);
		}
		if (c < 2) { continue; }
		while (out != NULL) {
			UINT succ = VERTEX_id(EDGE_to(EC_edge(out)));
			if (!body_set.is_contain(succ)) {
				BB * p = get_bb(succ);
				IS_TRUE0(p);
				XR * xr = get_last_xr(p);
				if (xr == NULL || xr->is_uncond_br()) {
					body_set.bunion(succ);
				}
			}
			out = EC_next(out);
		}
	}
}


template <class BB, class XR>
void CFG<BB, XR>::clean_loop_info()
{
	for (BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {
		LI<BB> * li = m_map_bb2li.get(bb->id);
		if (li != NULL) {
			m_bs_mgr->free(LI_bb_set(li));
			m_map_bb2li.set(bb->id, NULL);
		}
	}
	m_loop_info = NULL;
}


//Find natural loops.
//NOTICE: DOM set of BB must be avaiable.
template <class BB, class XR>
bool CFG<BB, XR>::find_loop()
{
	clean_loop_info();
	LIST<INT> tmp;
	TMAP<BB*, LI<BB>*> head2li;
	for (BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {

		//Access each sussessor of bb.
		VERTEX * vex = get_vertex(bb->id);
		IS_TRUE0(vex);
		for (EDGE_C * el = VERTEX_out_list(vex); el != NULL; el = EC_next(el)) {
			BB * succ = get_bb(VERTEX_id(EDGE_to(EC_edge(el))));
			IS_TRUE0(succ);

			BITSET * dom = m_dom_set.get(bb->id);
			IS_TRUE(dom, ("should compute dominator first"));
			if (!dom->is_contain(succ->id)) { continue; }

			/* If the SUCC is one of the DOMINATOR of bb, then it
			indicates a back-edge.
			Edge:bb->succ is a back-edge, each back-edge descripts a
			natural loop. */
			BITSET * loop = m_bs_mgr->create();
			identify_natural_loop(bb->id, succ->id, *loop, tmp);

			//Handle some special cases.
			add_break_out_loop_node(succ, *loop);

			//Loop may have multiple back edges.
			LI<BB> * li = head2li.get(succ);
			if (li != NULL) {
				//Multiple natural loops have the same loop header.
				LI_bb_set(li)->bunion(*loop);
				continue;
			}

			li = (LI<BB>*)xmalloc(sizeof(LI<BB>));
			LI_id(li) = m_li_count++;
			LI_bb_set(li) = loop;
			LI_loop_head(li) = succ;
			m_map_bb2li.set(succ->id, li);
			_insert_into_loop_tree(&m_loop_info, li);
			head2li.set(succ, li);
		}
	}
	return true;
}


//Back edge: y dominate x, back-edge is : x->y
template <class BB, class XR>
void CFG<BB, XR>::identify_natural_loop(UINT x, UINT y, IN OUT BITSET & loop,
										LIST<INT> & tmp)
{
	//Both x,y are node in loop.
	loop.bunion(x);
	loop.bunion(y);
	if (x != y) {
		tmp.clean();
		tmp.append_head(x);
		while (tmp.get_elem_count() != 0) {
			/* Bottom-up scanning and starting with 'x'
			to handling each node till 'y'.
			All nodes in the path among from 'x' to 'y'
			are belong to natural loop. */
			INT bb = tmp.remove_tail();
			EDGE_C const* ec = VERTEX_in_list(get_vertex(bb));
			while (ec != NULL) {
				INT pred = VERTEX_id(EDGE_from(EC_edge(ec)));
				if (!loop.is_contain(pred)) {
					//If pred is not a member of loop,
					//add it into list to handle.
					loop.bunion(pred);
					tmp.append_head(pred);
				}
				ec = EC_next(ec);
			}
		}
	}
}


template <class BB, class XR>
bool CFG<BB, XR>::_is_loop_head(LI<BB> * li, BB * bb)
{
	if (li == NULL) { return false; }
	LI<BB> * t = li;
	while (t != NULL) {
		IS_TRUE(LI_loop_head(t) != NULL, ("loop info absent loophead bb"));
		if (LI_loop_head(t) == bb) {
			return true;
		}
		t = LI_next(t);
	}
	return _is_loop_head(LI_inner_list(li), bb);
}


template <class BB, class XR>
void CFG<BB, XR>::dump_vcg(CHAR const* name)
{
	if (!name) {
		name = "graph_cfg.vcg";
	}
	unlink(name);
	FILE * hvcg = fopen(name, "a+");
	IS_TRUE(hvcg, ("%s create failed!!!", name));
	fprintf(hvcg, "graph: {"
			  "title: \"GRAPH\"\n"
			  "shrink:  15\n"
			  "stretch: 27\n"
			  "layout_downfactor: 1\n"
			  "layout_upfactor: 1\n"
			  "layout_nearfactor: 1\n"
			  "layout_splinefactor: 70\n"
			  "spreadlevel: 1\n"
			  "treefactor: 0.500000\n"
			  "node_alignment: center\n"
			  "orientation: top_to_bottom\n"
			  "late_edge_labels: no\n"
			  "display_edge_labels: yes\n"
			  "dirty_edge_labels: no\n"
			  "finetuning: no\n"
			  "nearedges: no\n"
			  "splines: yes\n"
			  "ignoresingles: no\n"
			  "straight_phase: no\n"
			  "priority_phase: no\n"
			  "manhatten_edges: no\n"
			  "smanhatten_edges: no\n"
			  "port_sharing: no\n"
			  "crossingphase2: yes\n"
			  "crossingoptimization: yes\n"
			  "crossingweight: bary\n"
			  "arrow_mode: free\n"
			  "layoutalgorithm: mindepthslow\n"
			  "node.borderwidth: 3\n"
			  "node.color: lightcyan\n"
			  "node.textcolor: darkred\n"
			  "node.bordercolor: red\n"
			  "edge.color: darkgreen\n");

	//Print node
	for (BB * bb = m_bb_list->get_head();
		 bb != NULL;  bb = m_bb_list->get_next()) {
		if (is_ru_entry(bb) || is_ru_exit(bb)) {
			fprintf(hvcg,
					"\nnode: { title:\"%d\" label:\"%d\" "
					"shape:hexagon color:blue "
					"fontname:\"courB\" textcolor:white}",
					bb->id, bb->id);
		} else {
			fprintf(hvcg,
					"\nnode: { title:\"%d\" label:\"%d\" "
					"shape:circle color:gold}",
					bb->id, bb->id);
		}
	}

	//Print edge
	INT c;
	for (EDGE * e = m_edges.get_first(c);
		 e != NULL;  e = m_edges.get_next(c)) {
		EI * ei = (EI*)EDGE_info(e);
		if (ei == NULL) {
			fprintf(hvcg,
					"\nedge: { sourcename:\"%d\" targetname:\"%d\" }",
					VERTEX_id(EDGE_from(e)), VERTEX_id(EDGE_to(e)));
		} else if (EI_is_eh(ei)) {
			fprintf(hvcg,
					"\nedge: { sourcename:\"%d\" targetname:\"%d\" linestyle:dotted }",
					VERTEX_id(EDGE_from(e)), VERTEX_id(EDGE_to(e)));
		} else {
			IS_TRUE0(0);
		}
	}
	fprintf(hvcg, "\n}\n");
	fclose(hvcg);
}


template <class BB, class XR>
void CFG<BB, XR>::compute_rpo_core(IN OUT BITSET & is_visited,
								   IN VERTEX * v,
								   IN OUT INT & order)
{
	is_visited.bunion(VERTEX_id(v));
	EDGE_C * el = VERTEX_out_list(v);
	while (el != NULL) {
		VERTEX * succ = EDGE_to(EC_edge(el));
		IS_TRUE(get_bb(VERTEX_id(succ)) != NULL,
				("without bb corresponded"));
		if (!is_visited.is_contain(VERTEX_id(succ))) {
			compute_rpo_core(is_visited, succ, order);
		}
		el = EC_next(el);
	}
	set_rpo(get_bb(VERTEX_id(v)), order);
	order--;
}


//Compute rev-post-order.
template <class BB, class XR>
void CFG<BB, XR>::compute_rpo(OPT_CTX & oc)
{
	START_TIMER("Compute Rpo");
	if (m_bb_list->get_elem_count() == 0) { return; }

	#ifdef _DEBUG_
	//Only for verify.
	for (BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {
		set_rpo(bb, -1);
	}
	#endif

	BITSET is_visited;
	BB * entry = NULL;
	for (BB * e = m_entry_list.get_head(); e != NULL;
		 e = m_entry_list.get_next()) {
		if (is_ru_entry(e)) {
			IS_TRUE(entry == NULL, ("multiple func entry"));
			entry = e;

			#ifndef _DEBUG_
			//We iterate each BB in entry list for verification
			//only in DEBUG mode.
			break;
			#endif
		}
	}

	#ifdef RECURSIVE_ALGO
	INT order = m_bb_list->get_elem_count();
	compute_rpo_core(is_visited, get_vertex(entry->id), order);
	#else
	LIST<VERTEX const*> vlst;
	compute_rpo_norec(get_vertex(entry->id), vlst);
	#endif

	m_rpo_bblst.clean();
	for (VERTEX const* v = vlst.get_head(); v != NULL; v = vlst.get_next()) {
		BB * bb = get_bb(VERTEX_id(v));
		IS_TRUE0(bb);
		set_rpo(bb, VERTEX_rpo(v));
		m_rpo_bblst.append_tail(bb);
	}
	OPTC_is_rpo_valid(oc) = true;
	END_TIMER();
}
#endif
