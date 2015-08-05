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

//IR_CFG
IR_CFG::IR_CFG(CFG_SHAPE cs, IR_BB_LIST * bbl, REGION * ru,
			   UINT edge_hash_size, UINT vertex_hash_size)
	: CFG<IR_BB, IR>(bbl, edge_hash_size, vertex_hash_size)
{
	m_ru = ru;
	m_dm = ru->get_dm();
	set_bs_mgr(ru->get_bs_mgr());
	if (m_bb_list->get_elem_count() == 0) {
		return;
	}

	//Add BB into graph.
	IS_TRUE0(m_bb_vec.get_last_idx() == -1);
	for (IR_BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {
		m_bb_vec.set(IR_BB_id(bb), bb);
		add_vertex(IR_BB_id(bb));
		for (LABEL_INFO * li = IR_BB_lab_list(bb).get_head();
			 li != NULL; li = IR_BB_lab_list(bb).get_next()) {
			m_lab2bb.set(li, bb);
			if (LABEL_INFO_is_catch_start(li)) {
				IR_BB_is_catch_start(bb) = true;
			}
			if (LABEL_INFO_is_unreachable(li)) {
				IR_BB_is_unreachable(bb) = true;
			}
		}
	}
	switch (cs) {
	case C_SESE:
		{
			//Make sure the unique entry per cfg.
			IR_BB * entry = m_ru->new_bb();
			IR_BB_is_entry(entry) = 1;
			IR_BB_is_fallthrough(entry) = 1;
			add_bb(entry);
			m_bb_list->append_head(entry);
			m_entry_list.append_tail(entry);

			/* Create logical exit BB.
			NOTICE: In actually, the logical exit BB is ONLY
			used to solve diverse dataflow equations, whereas
			considering the requirement of ENTRY BB, EXIT BB. */
			IR_BB * exit = m_ru->new_bb();
			IR_BB_is_fallthrough(exit) = 1;
			add_bb(exit);
			m_bb_list->append_tail(exit);
			m_exit_list.append_tail(exit);
			break;
		}
	case C_SEME:
		{
			//Create entry BB.
			IR_BB * entry = m_ru->new_bb();
			IR_BB_is_entry(entry) = 1;
			//IR_BB_is_fallthrough(entry) = 1;
			add_bb(entry);
			m_bb_list->append_head(entry);
			m_entry_list.append_tail(entry);

			/* Collect exit BB.
			for (IR_BB * bb = m_bb_list->get_head();
				 bb != NULL; bb = m_bb_list->get_next()) {
				if (IR_BB_is_func_exit(bb)) {
					m_exit_list.append_tail(bb);
				}
			} */
			break;
		}
	case C_MEME:
	case C_MESE:
		/* One should mark all entries and exits after cfg contructed.
		Multiple entry and exit are deprecated, it might confuse
		some optimizations. */
		IS_TRUE(0, ("TODO"));
		break;
	default: IS_TRUE(0, ("strang shape of CFG"));
	}
}


//Control flow optimization
void IR_CFG::cf_opt()
{
	bool change = true;
	while (change) {
		change = false;
		IR_BB_LIST * bbl = get_bb_list();
		for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
			change = goto_opt(bb);
			if (change) { break; }
			change = if_opt(bb);
			if (change) { break; }
		}
	}
}


void IR_CFG::find_tgt_bb_list(IN IR * ir, OUT LIST<IR_BB*> & tgt_bbs)
{
	IS_TRUE0(IR_type(ir) == IR_SWITCH);
	tgt_bbs.clean();
	if (m_bb_list == NULL) return;
	IR * casev_list = SWITCH_case_list(ir);
	if (SWITCH_deflab(ir) != NULL) {
		IR_BB * tbb = find_bb_by_label(SWITCH_deflab(ir));
		IS_TRUE0(tbb);
		tgt_bbs.append_tail(tbb);
	}
	if (casev_list != NULL) {
		for (IR * casev = casev_list;
			 casev != NULL; casev = IR_next(casev)) {
			IR_BB * tbb = find_bb_by_label(CASE_lab(casev));
			IS_TRUE0(tbb);
			tgt_bbs.append_tail(tbb);
		}
	}
}


//Find bb that 'lab' attouchemented.
void IR_CFG::find_bbs(IR * ir, LIST<IR_BB*> & tgtlst)
{
	IS_TRUE0(ir->is_indirect_br());
	for (IR * c = IGOTO_case_list(ir); c != NULL; c = IR_next(c)) {
		IS_TRUE0(IR_type(c) == IR_CASE);
		IR_BB * bb = m_lab2bb.get(CASE_lab(c));
		IS_TRUE0(bb); //no bb is correspond to lab.
		tgtlst.append_tail(bb);

		#ifdef _DEBUG_
		bool find = false;
		for (LABEL_INFO * li = IR_BB_lab_list(bb).get_head();
			 li != NULL; li = IR_BB_lab_list(bb).get_next()) {
			if (is_same_label(CASE_lab(c), li)) {
				find = true;
				break;
			}
		}
		IS_TRUE0(find);
		#endif
	}
}


//Find natural loop and scan loop body to find call and early exit, etc.
void IR_CFG::loop_analysis(OPT_CTX & oc)
{
	m_ru->check_valid_and_recompute(&oc, OPT_DOM, OPT_UNDEF);
	find_loop();
	collect_loop_info();
	OPTC_is_loopinfo_valid(oc) = true;
}


//Find bb that 'lab' attouchemented.
IR_BB * IR_CFG::find_bb_by_label(LABEL_INFO * lab)
{
	IR_BB * bb = m_lab2bb.get(lab);
	IS_TRUE(bb, ("no bb correspond to this label."));
	#ifdef _DEBUG_
	bool find = false;
	for (LABEL_INFO * li = IR_BB_lab_list(bb).get_head();
		 li != NULL; li = IR_BB_lab_list(bb).get_next()) {
		if (is_same_label(lab, li)) {
			find = true;
			break;
		}
	}
	IS_TRUE0(find);
	#endif
	return bb;
}


void IR_CFG::insert_bb_between(IN IR_BB * from,
							   IN C<IR_BB*> * from_ct,
							   IN IR_BB * to,
							   IN C<IR_BB*> * to_ct,
							   IN IR_BB * newbb)
{
	/* Revise BB list, note that 'from' is either fall-through to 'to',
	or jumping to 'to'.	*/
	IR_BB_LIST * bblst = get_bb_list();

	//First, processing edge if 'from'->'to' is fallthrough.
	C<IR_BB*> * tmp_ct = from_ct;
	if (IR_BB_is_fallthrough(from) && bblst->get_next(&tmp_ct) == to) {
		bblst->insert_after(newbb, from);
		insert_vertex_between(IR_BB_id(from), IR_BB_id(to), IR_BB_id(newbb));
		IR_BB_is_fallthrough(newbb) = 1;
		return;
	}

	//Second, from->to is jump-edge.
	LIST<IR_BB*> preds;
	get_preds(preds, to);
	IS_TRUE(preds.find(from), ("'from' is not pred of 'to'"));
	C<IR_BB*> * pred_ct = NULL;
	for (IR_BB * pred = preds.get_head(&pred_ct);
		 pred != NULL; pred = preds.get_next(&pred_ct)) {
		C<IR_BB*> * tmp_ct = to_ct;
		if (IR_BB_is_fallthrough(pred) && bblst->get_prev(&tmp_ct) == pred) {
			/* Given 'to' has a fallthrough in-edge. Insert a tmp BB
			e.g:
				from->bb1->bb2->to, all edges are fallthrough
				from->to, jump-edge
				bb1->to, jump-edge

			Here we need to revise the fallthrough-edge 'bb2->to',
			the result is from->bb1->bb2->tmp_tramp_bb, all
			edges are fallthrough tmp_tramp_bb->to, jump-edge
				from->to, jump-edge
				bb1->to, jump-edge

				bb2->tmp_tramp_bb, tmp_tramp_bb->to, both are jump-edge.
				ir-list of tmp_tramp_bb is:
					goto L1:

				ir-list of 'to' is:
					L1:
					...
					...
			*/
			IR_BB * tmp_tramp_bb = m_ru->new_bb();
			LABEL_INFO * li = m_ru->gen_ilab();
			IR * goto_ir = m_ru->build_goto(li);
			IR_BB_ir_list(tmp_tramp_bb).append_tail(goto_ir);
			to->add_label(li);
			m_lab2bb.set(li, to);
			add_bb(tmp_tramp_bb);
			bblst->insert_after(tmp_tramp_bb, pred);

			insert_vertex_between(IR_BB_id(pred),
								  IR_BB_id(to),
								  IR_BB_id(tmp_tramp_bb));

			//Fallthrough edge has been broken, insert 'newbb' before 'to'.
			break;
		}
	} //end for

	//Revise the target LABEL of last XR in 'from'.
	IR * last_xr_of_from = get_last_xr(from);
	LABEL_INFO * from_tgt_li = get_xr_label(last_xr_of_from);
	IS_TRUE0(from_tgt_li && find_bb_by_label(from_tgt_li) == to);
	IS_TRUE0(last_xr_of_from->get_label() != NULL);

	LABEL_INFO * li = m_ru->gen_ilab();
	last_xr_of_from->set_label(li);

	newbb->add_label(li);
	m_lab2bb.set(li, newbb);

	//When we get here, there are NOT any fallthrough in-edges of 'to' exist.
	bblst->insert_before(newbb, to_ct);
	insert_vertex_between(IR_BB_id(from), IR_BB_id(to), IR_BB_id(newbb));
	IR_BB_is_fallthrough(newbb) = 1;
}


//Merge LABEL from 'src' to 'tgt'.
void IR_CFG::union_labs(IR_BB * src, IR_BB * tgt)
{
	tgt->union_labs(src);

	//Set label2bb map.
	for (LABEL_INFO * li = IR_BB_lab_list(tgt).get_head();
		 li != NULL; li = IR_BB_lab_list(tgt).get_next()) {
		m_lab2bb.aset(li, tgt);
	}
}


/* Remove trampoline BB.
e.g: bb1->bb2->bb3
	stmt of bb2 is just 'goto bb3', and bb3 is the NEXT BB of bb2 in BB-LIST.
	Then bb2 is tramp BB.
Return true if at least one tramp BB removed.

ALGO:
	for each pred of BB
		if (pred is fallthrough && prev of BB == pred)
			remove edge pred->BB.
			add edge pred->BB's next.
			continue;
		end if
		duplicate LABEL_INFO from BB to BB's next.
		revise LABEL_INFO of pred to new target BB.
		remove edge pred->BB.
		add edge pred->BB's next.
	end for
*/
bool IR_CFG::remove_tramp_bb()
{
	bool removed = false;
	C<IR_BB*> * ct;
	LIST<IR_BB*> succs;
	LIST<IR_BB*> preds;
	for (IR_BB * bb = m_bb_list->get_head(&ct);
		 bb != NULL; bb = m_bb_list->get_next(&ct)) {
		if (is_exp_handling(bb)) { continue; }
		IR * br = get_first_xr(bb);
		if (br != NULL &&
			br->is_uncond_br() &&
			IR_BB_ir_num(bb) == 1) {
			/* CASE: Given pred1->bb, fallthrough edge,
				and pred2->bb, jumping edge.
				bb:
					goto L1

				next of bb:
					L1:
					...
					...
			Remove bb and revise CFG. */
			get_succs(succs, bb);
			C<IR_BB*> * tmp_bb_ct = ct;
			IR_BB * next = m_bb_list->get_next(&tmp_bb_ct);
			IS_TRUE0(succs.get_elem_count() == 1);
			if (next == NULL || //bb may be the last BB in bb-list.
				next != succs.get_head()) {
				continue;
			}

			tmp_bb_ct = ct;
			IR_BB * prev = m_bb_list->get_prev(&tmp_bb_ct);
			preds.clean(); //use list because cfg may be modify.
			get_preds(preds, bb);
			for (IR_BB * pred = preds.get_head();
				 pred != NULL; pred = preds.get_next()) {
				if (IR_BB_is_fallthrough(pred) && prev == pred) {
					remove_edge(pred, bb);
					add_edge(IR_BB_id(pred), IR_BB_id(next));
					continue;
				}

				union_labs(bb, next);

				//Revise branch target LABEL_INFO of xr in 'pred'.
				IR * last_xr_of_pred = get_last_xr(pred);
				LABEL_INFO * pred_tgt_li = get_xr_label(last_xr_of_pred);
				IS_TRUE0(pred_tgt_li != NULL &&
						 find_bb_by_label(pred_tgt_li) == bb);

				IS_TRUE0(last_xr_of_pred->get_label() != NULL);
				last_xr_of_pred->set_label(get_xr_label(br));
				remove_edge(pred, bb);
				add_edge(IR_BB_id(pred), IR_BB_id(next));
			} //end for each pred of BB.
			remove_bb(bb);
			removed = true;
			bb = m_bb_list->get_head(&ct); //reprocessing BB list.
		} //end if ending XR of BB is uncond-branch.
	} //end for each BB
	return removed;
}


/* Remove trampoline edge.
e.g: bb1->bb2->bb3
stmt of bb2 is just 'goto bb3', then bb1->bb2 is tramp edge.
And the resulting edges are bb1->bb3, bb2->bb3 respectively.
Return true if at least one tramp edge removed. */
bool IR_CFG::remove_tramp_edge()
{
	bool removed = false;
	C<IR_BB*> * ct;
	for (IR_BB * bb = m_bb_list->get_head(&ct);
		 bb != NULL; bb = m_bb_list->get_next(&ct)) {
		if (IR_BB_ir_num(bb) == 0) { continue; }
		if (IR_BB_ir_num(bb) != 1) { continue; }
		IR * last_xr = get_first_xr(bb);
		if (last_xr->is_uncond_br() &&
			!bb->is_attach_dedicated_lab()) {
			//stmt is GOTO.
			LABEL_INFO * tgt_li = get_xr_label(last_xr);
			IS_TRUE0(tgt_li != NULL);
			LIST<IR_BB*> preds; //use list because cfg may be modify.
			get_preds(preds, bb);
			for (IR_BB * pred = preds.get_head();
				 pred != NULL; pred = preds.get_next()) {
				if (pred == bb) {
					//bb's pred is itself.
					continue;
				}
				if (IR_BB_ir_num(pred) == 0) {
					continue;
				}
				IR * last_xr_of_pred = get_last_xr(pred);
				if (!pred->is_bb_down_boundary(last_xr_of_pred)) {
					/* CASE: pred->bb, pred is fallthrough-BB.
						pred is:
							a=b+1
						bb is:
							L1:
							goto L2
					=>
						pred is:
							a=b+1
							goto L2
						bb is:
							L1:
							goto L2
					*/
					IR_BB_ir_list(pred).append_tail(m_ru->dup_irs(last_xr));
					IR_BB_is_fallthrough(pred) = 0;
					remove_edge(pred, bb);
					IR_BB * new_tgt = find_bb_by_label(tgt_li);
					IS_TRUE0(new_tgt != NULL);
					add_edge(IR_BB_id(pred), IR_BB_id(new_tgt));
					removed = true;
					continue;
				} //end if

				if (last_xr_of_pred->is_uncond_br()) {
					/* CASE: pred->bb,
						pred is:
							goto L1
						bb is:
							L1:
							goto L2
					=>
						pred to be:
							goto L2
						bb is:
							L1:
							goto L2
					*/
					LABEL_INFO * pred_tgt_li = get_xr_label(last_xr_of_pred);
					IS_TRUE0(pred_tgt_li != NULL &&
							 find_bb_by_label(pred_tgt_li) == bb);

					IS_TRUE0(last_xr_of_pred->get_label() != NULL);
					GOTO_lab(last_xr_of_pred) = tgt_li;
					remove_edge(pred, bb);
					IR_BB * new_tgt = find_bb_by_label(tgt_li);
					IS_TRUE0(new_tgt != NULL);
					add_edge(IR_BB_id(pred), IR_BB_id(new_tgt));
					removed = true;
					continue;
				} //end if

				if (last_xr_of_pred->is_cond_br()) {
					/* CASE: pred->f, pred->bb, and pred->f is fall through edge.
						pred is:
							truebr/falsebr L1

						f is:
							...
							...

						bb is:
							L1:
							goto L2
					=>
						pred is:
							truebr/falsebr L2

						f is:
							...
							...

						bb is:
							L1:
							goto L2
					*/
					C<IR_BB*> * prev_of_bb = ct;
					if (m_bb_list->get_prev(&prev_of_bb) == pred) {
						//Can not remove jumping-edge if 'bb' is
						//fall-through successor of 'pred'.
						continue;
					}

					LABEL_INFO * pred_tgt_li = get_xr_label(last_xr_of_pred);
					IS_TRUE0(pred_tgt_li != NULL &&
							 find_bb_by_label(pred_tgt_li) == bb);

					IS_TRUE0(last_xr_of_pred->get_label() != NULL);
					BR_lab(last_xr_of_pred) = tgt_li;
					remove_edge(pred, bb);
					IR_BB * new_tgt = find_bb_by_label(tgt_li);
					IS_TRUE0(new_tgt != NULL);
					add_edge(IR_BB_id(pred), IR_BB_id(new_tgt));
					removed = true;
					continue;
				} //end if
			}
		} //end if xr is uncond branch.
	} //end for	each BB
	return removed;
}


bool IR_CFG::remove_redundant_branch()
{
	bool removed = CFG<IR_BB, IR>::remove_redundant_branch();
	C<IR_BB*> * ct;
	IR_DU_MGR * dumgr = m_ru->get_du_mgr();
	LIST<IR_BB*> succs;
	for (IR_BB * bb = m_bb_list->get_head(&ct);
		 bb != NULL; bb = m_bb_list->get_next(&ct)) {
		IR * last_xr = get_last_xr(bb);
		if (last_xr != NULL && last_xr->is_cond_br()) {
			IR * det = BR_det(last_xr);
			IS_TRUE0(det != NULL);
			bool always_true = (IR_is_const(det) &&
								det->is_int(m_dm) &&
								CONST_int_val(det) != 0) ||
								det->is_str(m_dm);
			bool always_false = IR_is_const(det) &&
								det->is_int(m_dm) &&
								CONST_int_val(det) == 0;
			if ((IR_type(last_xr) == IR_TRUEBR && always_true) ||
				(IR_type(last_xr) == IR_FALSEBR && always_false)) {
				//Substitute cond_br with 'goto'.
				LABEL_INFO * tgt_li = get_xr_label(last_xr);
				IS_TRUE0(tgt_li != NULL);
				IR_BB_ir_list(bb).remove_tail();
				if (dumgr != NULL) {
					dumgr->remove_ir_out_from_du_mgr(last_xr);
				}
				m_ru->free_irs(last_xr);
				IR * uncond_br = m_ru->build_goto(tgt_li);
				IR_BB_ir_list(bb).append_tail(uncond_br);

				//Remove fallthrough edge, leave branch edge.
				get_succs(succs, bb);
				C<IR_BB*> * tmp_ct = ct;
				m_bb_list->get_next(&tmp_ct);
				for (IR_BB * s = succs.get_head();
					 s != NULL; s = succs.get_next()) {
					if (s == C_val(tmp_ct)) {
						//Remove branch edge, leave fallthrough edge.
						remove_edge(bb, s);
					}
				}
				removed = true;
			} else if ((IR_type(last_xr) == IR_TRUEBR && always_false) ||
					   (IR_type(last_xr) == IR_FALSEBR && always_true)) {
				IR * r = IR_BB_ir_list(bb).remove_tail();
				if (dumgr != NULL) {
					dumgr->remove_ir_out_from_du_mgr(r);
				}
				m_ru->free_irs(r);

				//Remove branch edge, leave fallthrough edge.
				get_succs(succs, bb);
				C<IR_BB*> * tmp_ct = ct;
				m_bb_list->get_next(&tmp_ct);
				for (IR_BB * s = succs.get_head();
					 s != NULL; s = succs.get_next()) {
					if (s != C_val(tmp_ct)) {
						remove_edge(bb, s);
					}
				}
				removed = true;
			}
		}
	} //for each BB
	return removed;
}


void IR_CFG::dump_dot(CHAR const* name, bool detail)
{
	if (g_tfile == NULL) { return; }
	if (name == NULL) {
		name = "graph_cfg.dot";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!", name));

	//Print comment
	FILE * old = g_tfile;
	g_tfile = h;
	fprintf(h, "\n/*");
	for (IR_BB * bb = m_bb_list->get_head();
		 bb != NULL; bb = m_bb_list->get_next()) {
		fprintf(h, "\n--- BB%d ----", IR_BB_id(bb));
		dump_bbs(m_bb_list);
		//fprintf(h, "\n\t%s", dump_ir_buf(ir, buf));
	}
	fprintf(h, "\n*/\n");
	fprintf(h, "digraph G {\n");

	//Print node
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		INT id = VERTEX_id(v);
		if (detail) {
			IR_BB * bb = get_bb(id);
			fprintf(h, "\nnode%d [shape = Mrecord, label=\"{   BB%d | ",
						IR_BB_id(bb), IR_BB_id(bb));
			for (IR * ir = IR_BB_first_ir(bb);
				 ir != NULL; ir = IR_BB_next_ir(bb)) {
				dump_ir(ir, m_dm); //TODO: implement dump_ir_buf();
				fprintf(h, " | ");
			}
			fprintf(h, "}\"];");
		} else {
			fprintf(h, "\nnode%d [shape = Mrecord, label=\"{BB%d}\"];",
						VERTEX_id(v), VERTEX_id(v));
		}
	}

	//Print edge
	for (EDGE const* e = m_edges.get_first(c);
		 e != NULL;  e = m_edges.get_next(c)) {
		fprintf(h, "\nnode%d->node%d[label=\"%s\"]",
					VERTEX_id(EDGE_from(e)), VERTEX_id(EDGE_to(e)), "");
	}
	g_tfile = old;
	fprintf(h,"\n}\n");
	fclose(h);
}


void IR_CFG::dump_vcg(CHAR const* name, bool detail, bool dump_eh)
{
	if (g_tfile == NULL) { return; }
	if (name == NULL) {
		name = "graph_cfg.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h != NULL, ("%s create failed!!!",name));
	FILE * old = NULL;

	//Print comment
	//fprintf(h, "\n/*");
	//old = g_tfile;
	//g_tfile = h;
	//dump_bbs(m_bb_list);
	//g_tfile = old;
	//fprintf(h, "\n*/\n");

	//Print graph structure description.
	fprintf(h, "graph: {"
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
			  "node.borderwidth: 2\n"
			  "node.color: lightcyan\n"
			  "node.textcolor: black\n"
			  "node.bordercolor: blue\n"
			  "edge.color: darkgreen\n");

	//Print REGION name.
	fprintf(h,
			"\nnode: {title:\"\" shape:rhomboid color:turquoise "
			"borderwidth:0 fontname:\"Courier Bold\" "
			"scaling:2 label:\"REGION:%s\" }", m_ru->get_ru_name());

	//Print node.
	old = g_tfile;
	g_tfile = h;

	C<IR_BB*> * bbct;
	UINT vertical_order = 1;
	for (IR_BB * bb = m_bb_list->get_head(&bbct);
		 bb != NULL; bb = m_bb_list->get_next(&bbct)) {
		INT id = IR_BB_id(bb);
		VERTEX * v = get_vertex(id);
		IS_TRUE(v, ("bb is not in cfg"));
		CHAR const* shape = "box";
		if (IR_BB_is_catch_start(bb)) {
			shape = "uptrapezoid";
		}

		CHAR const* font = "courB";
		INT scale = 1;
		CHAR const* color = "gold";
		if (IR_BB_is_entry(bb) || IR_BB_is_exit(bb)) {

			font = "Times Bold";
			scale = 2;
			color = "cyan";
		}

		if (detail) {
			fprintf(h,
				"\nnode: {title:\"%d\" vertical_order:%d shape:%s color:%s "
				"fontname:\"%s\" scaling:%d label:\"",
				id, vertical_order++, shape, color, font, scale);
			fprintf(h, "   BB%d ", id);
			if (VERTEX_rpo(v) != 0) {
				fprintf(h, " rpo:%d ", VERTEX_rpo(v));
			}

			IR_BB * bb = get_bb(id);
			IS_TRUE0(bb != NULL);

			for (LABEL_INFO * li = IR_BB_lab_list(bb).get_head();
				 li != NULL; li = IR_BB_lab_list(bb).get_next()) {
				switch (LABEL_INFO_type(li)) {
				case L_CLABEL:
					fprintf(h, CLABEL_STR_FORMAT, CLABEL_CONT(li));
					break;
				case L_ILABEL:
					fprintf(h, ILABEL_STR_FORMAT, ILABEL_CONT(li));
					break;
				case L_PRAGMA:
					fprintf(h, "%s", SYM_name(LABEL_INFO_pragma(li)));
					break;
				default: IS_TRUE0(0);
				}
				if (LABEL_INFO_is_try_start(li) ||
					LABEL_INFO_is_try_end(li) ||
					LABEL_INFO_is_catch_start(li)) {
					fprintf(g_tfile, "(");
					if (LABEL_INFO_is_try_start(li)) {
						fprintf(g_tfile, "try_start,");
					}
					if (LABEL_INFO_is_try_end(li)) {
						fprintf(g_tfile, "try_end,");
					}
					if (LABEL_INFO_is_catch_start(li)) {
						fprintf(g_tfile, "catch_start");
					}
					fprintf(g_tfile, ")");
				}
				fprintf(g_tfile, " ");
			}
			fprintf(h, "\n");
			for (IR * ir = IR_BB_first_ir(bb);
				 ir != NULL; ir = IR_BB_next_ir(bb)) {
				//fprintf(h, "%s\n", dump_ir_buf(ir, buf));

				//TODO: implement dump_ir_buf();
				dump_ir(ir, m_dm, NULL, true, false);
			}
			fprintf(h, "\"}");
		} else {
			fprintf(h,
					"\nnode: {title:\"%d\" vertical_order:%d shape:%s color:%s "
					"fontname:\"%s\" scaling:%d label:\"%d",
					id, vertical_order++, shape, color, font, scale, id);
			if (VERTEX_rpo(v) != 0) {
				fprintf(h, " rpo:%d", VERTEX_rpo(v));
			}
			fprintf(h, "\" }");
		}
	}

	//Print edge
	INT c;
	for (EDGE * e = m_edges.get_first(c); e != NULL; e = m_edges.get_next(c)) {
		VERTEX * from = EDGE_from(e);
		VERTEX * to = EDGE_to(e);
		EI * ei = (EI*)EDGE_info(e);
		if (ei == NULL) {
			fprintf(h,
					"\nedge: { sourcename:\"%d\" targetname:\"%d\" "
					" thickness:4 color:darkred }",
					VERTEX_id(EDGE_from(e)), VERTEX_id(EDGE_to(e)));
		} else if (EI_is_eh(ei)) {
			if (dump_eh) {
				fprintf(h,
						"\nedge: { sourcename:\"%d\" targetname:\"%d\" "
						"linestyle:dotted color:lightgrey }",
						VERTEX_id(EDGE_from(e)), VERTEX_id(EDGE_to(e)));
			}
		} else {
			IS_TRUE(0, ("unsupport EDGE_INFO"));
		}
	}
	g_tfile = old;
	fprintf(h, "\n}\n");
	fclose(h);
}


void IR_CFG::compute_dom_idom(IN OUT OPT_CTX & oc, BITSET const* uni)
{
	START_TIMER_AFTER();
	IS_TRUE0(OPTC_is_cfg_valid(oc));
	IS_TRUE(m_entry_list.get_elem_count() == 1,
			("ONLY support SESE or SEME"));

	m_ru->check_valid_and_recompute(&oc, OPT_RPO, OPT_UNDEF);
	LIST<IR_BB*> * bblst = get_bblist_in_rpo();
	IS_TRUE0(bblst->get_elem_count() == m_ru->get_bb_list()->get_elem_count());

	LIST<VERTEX const*> vlst;
	for (IR_BB * bb = bblst->get_head(); bb != NULL; bb = bblst->get_next()) {
		IS_TRUE0(IR_BB_id(bb) != 0);
		vlst.append_tail(get_vertex(IR_BB_id(bb)));
	}

	//DGRAPH::compute_dom(&vlst, uni);
	//DGRAPH::compute_idom();

	bool f = DGRAPH::compute_idom2(vlst);
	IS_TRUE0(f);

	f = DGRAPH::compute_dom2(vlst);
	IS_TRUE0(f);

	OPTC_is_dom_valid(oc) = true;
	END_TIMER_AFTER("Compute Dom, IDom");
}


void IR_CFG::compute_pdom_ipdom(IN OUT OPT_CTX & oc, BITSET const* uni)
{
	START_TIMER("Compute PDom,IPDom");
	IS_TRUE0(OPTC_is_cfg_valid(oc));

	m_ru->check_valid_and_recompute(&oc, OPT_RPO, OPT_UNDEF);
	LIST<IR_BB*> * bblst = get_bblist_in_rpo();
	IS_TRUE0(bblst->get_elem_count() == m_ru->get_bb_list()->get_elem_count());

	LIST<VERTEX const*> vlst;
	for (IR_BB * bb = bblst->get_tail(); bb != NULL; bb = bblst->get_prev()) {
		IS_TRUE0(IR_BB_id(bb) != 0 && get_vertex(IR_BB_id(bb)));
		vlst.append_tail(get_vertex(IR_BB_id(bb)));
	}

	bool f = false;
	if (uni != NULL) {
		f = DGRAPH::compute_pdom(&vlst, uni);
	} else {
		f = DGRAPH::compute_pdom(&vlst);
	}
	IS_TRUE0(f);

	f = DGRAPH::compute_ipdom();
	IS_TRUE0(f);

	OPTC_is_pdom_valid(oc) = true;
	END_TIMER();
}


void IR_CFG::remove_xr(IR_BB * bb, IR * ir)
{
	IR_DU_MGR * dumgr = m_ru->get_du_mgr();
	if (dumgr != NULL) {
		dumgr->remove_ir_out_from_du_mgr(ir);
	}
	ir = IR_BB_ir_list(bb).remove(ir);
	m_ru->free_irs(ir);
}
