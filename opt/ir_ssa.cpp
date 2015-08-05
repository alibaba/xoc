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
#include "comopt.h"

//#define TRAVERSE_IN_DOM_TREE_ORDER

//
//START DF_MGR
//
DF_MGR::DF_MGR(IR_SSA_MGR * sm)
{
	m_ssa_mgr = sm;
}


//Get the BB set where 'v' is the dominate frontier of them.
BITSET * DF_MGR::get_df_ctrlset(VERTEX const* v)
{
	BITSET * df = m_df_vec.get(VERTEX_id(v));
	if (df == NULL) {
		df = m_bs_mgr.create();
		m_df_vec.set(VERTEX_id(v), df);
	}
	return df;
}


void DF_MGR::clean()
{
	for (INT i = 0; i <= m_df_vec.get_last_idx(); i++) {
		BITSET * df = m_df_vec.get(i);
		if (df != NULL) {
			df->clean();
		}
	}
}


//g: dump dominance frontier to graph.
void DF_MGR::dump(DGRAPH & g)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP Dominator Frontier Control Set ----==\n");
	INT c;
	for (VERTEX const* v = g.get_first_vertex(c);
		 v != NULL; v = g.get_next_vertex(c)) {
		UINT vid = VERTEX_id(v);
		fprintf(g_tfile, "\nVEX%d DF controlled: ", VERTEX_id(v));
		BITSET const* df = m_df_vec.get(vid);
		if (df != NULL) {
			for (INT i = df->get_first(); i >= 0; i = df->get_next(i)) {
				fprintf(g_tfile, "%d,", i);
			}
		}
	}
	fflush(g_tfile);
}


//This function compute dominance frontier to graph g.
void DF_MGR::build(DGRAPH & g)
{
	INT c;
	for (VERTEX const* v = g.get_first_vertex(c);
		 v != NULL; v = g.get_next_vertex(c)) {
		BITSET const* v_dom = g.get_dom_set(v);
		IS_TRUE0(v_dom != NULL);
		UINT vid = VERTEX_id(v);

		//Access each preds
		EDGE_C const* ec = VERTEX_in_list(v);
		while (ec != NULL) {
			VERTEX const* pred = EDGE_from(EC_edge(ec));
			BITSET * pred_df = get_df_ctrlset(pred);
			if (pred == v || g.get_idom(vid) != VERTEX_id(pred)) {
				pred_df->bunion(vid);
			}

			BITSET const* pred_dom = g.get_dom_set(pred);
			IS_TRUE0(pred_dom != NULL);
			for (INT i = pred_dom->get_first();
				 i >= 0; i = pred_dom->get_next(i)) {
				if (!v_dom->is_contain(i)) {
					VERTEX const* pred_dom_v = g.get_vertex(i);
					IS_TRUE0(pred_dom_v != NULL);
					get_df_ctrlset(pred_dom_v)->bunion(vid);
				}
			}
			ec = EC_next(ec);
		}
	}
	//dump(g);
}
//END DF_MGR



//
//START SSA_GRAPH
//
SSA_GRAPH::SSA_GRAPH(REGION * ru, IR_SSA_MGR * ssamgr)
{
	IS_TRUE0(ru && ssamgr);
	m_ru = ru;
	m_ssa_mgr = ssamgr;
	SVECTOR<VP*> const* vp_vec = ssamgr->get_vp_vec();
	UINT inputcount = 1;
	for (INT i = 1; i <= vp_vec->get_last_idx(); i++) {
		VP * v = vp_vec->get(i);
		IS_TRUE0(v != NULL);
		IR * def = SSA_def(v);
		if (def == NULL) {
			IS_TRUE0(VP_ver(v) == 0);
			UINT vdef = 0xffffFFFF - inputcount;
			inputcount++;
			add_vertex(vdef);
			m_vdefs.set(vdef, v);

			//May be input parameters.
			SSAUSE_ITER vit;
			for (INT i = SSA_uses(v).get_first(&vit);
		 		 vit != NULL; i = SSA_uses(v).get_next(i, &vit)) {
				IR * use = m_ru->get_ir(i);
				IS_TRUE0(use->is_pr());
				add_edge(vdef, IR_id(use->get_stmt()));
			}
		} else {
			IS_TRUE0(def->is_stmt());
			add_vertex(IR_id(def));
			SSAUSE_ITER vit;
			for (INT i = SSA_uses(v).get_first(&vit);
				 vit != NULL; i = SSA_uses(v).get_next(i, &vit)) {
				IR * use = m_ru->get_ir(i);
				IS_TRUE0(use->is_pr());
				add_edge(IR_id(def), IR_id(use->get_stmt()));
			}
		}
	}
}


void SSA_GRAPH::dump(IN CHAR const* name, bool detail)
{
	if (name == NULL) {
		name = "graph_ssa_graph.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h != NULL, ("%s create failed!!!",name));

	//Print comment
	fprintf(h, "\n/*");
	FILE * old = g_tfile;
	g_tfile = h;
	dump_bbs(m_ru->get_bb_list());
	g_tfile = old;
	fprintf(h, "\n*/\n");

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

	//Print node
	old = g_tfile;
	g_tfile = h;
	LIST<IR const*> lst;
	DT_MGR * dm = m_ru->get_dm();
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		VP * vp = m_vdefs.get(VERTEX_id(v));
		if (vp != NULL) {
			//Print virtual def for parameter.
			fprintf(h,
					"\nnode: { title:\"%u\" shape:hexagon fontname:\"courB\" "
					"color:lightgrey label:\" param:P%uV%u \"}",
					VERTEX_id(v), VP_prno(vp), VP_ver(vp));
			continue;
		}

		IR * def = m_ru->get_ir(VERTEX_id(v));
		IS_TRUE0(def != NULL);
		IR * res = def->get_pr_results();
		if (res != NULL) {
			fprintf(h, "\nnode: { title:\"%d\" shape:box fontname:\"courB\" "
						"color:gold label:\"", IR_id(def));
			for (IR * r = res; r != NULL; r = IR_next(r)) {
				VP * vp = (VP*)r->get_ssainfo();
				fprintf(h, "P%uV%u ", VP_prno(vp), VP_ver(vp));
			}
			fprintf(h, " <-- ");
		} else {
			fprintf(h, "\nnode: { title:\"%u\" shape:box fontname:\"courB\" "
						"color:gold label:\" <-- ",
					IR_id(def));
		}

		lst.clean();
		for (IR const* opnd = ir_iter_init_c(def, lst);
			 opnd != NULL; opnd = ir_iter_next_c(lst)) {
			 if (!def->is_rhs(opnd) || IR_type(opnd) != IR_PR) {
				 continue;
			 }
			 VP * use_vp = (VP*)PR_ssainfo(opnd);
			 fprintf(h, "P%dV%d, ", VP_prno(use_vp), VP_ver(use_vp));
		}
		if (detail) {
			//TODO: implement dump_ir_buf();
			dump_ir(def, dm, NULL, true, false);
		}
		fprintf(h, "\"}");
	}

	//Print edge
	for (EDGE * e = m_edges.get_first(c); e != NULL; e = m_edges.get_next(c)) {
		VERTEX * from = EDGE_from(e);
		VERTEX * to = EDGE_to(e);
		fprintf(h, "\nedge: { sourcename:\"%u\" targetname:\"%u\" %s}",
				VERTEX_id(from), VERTEX_id(to),  "");
	}
	g_tfile = old;
	fprintf(h, "\n}\n");
	fclose(h);
}
//END SSA_GRAPH



//
//START IR_SSA_MGR
//
UINT IR_SSA_MGR::count_mem()
{
	UINT count = 0;
	count += smpool_get_pool_size_handle(m_vp_pool);
	count += m_map_prno2vp_vec.count_mem();
	count += m_map_prno2stack.count_mem();
	count += sizeof(m_vp_count);
	count += m_vp_vec.count_mem();
	count += m_max_version.count_mem();
	count += m_prno2ir.count_mem();
	count += sizeof(m_ru);
	return count;
}


//Clean version stack.
void IR_SSA_MGR::clean_prno2stack()
{
	for (INT i = 0; i <= m_map_prno2stack.get_last_idx(); i++) {
		SSTACK<VP*> * s = m_map_prno2stack.get(i);
		if (s != NULL) { delete s; }
	}
	m_map_prno2stack.clean();
}


//Dump ssa du stmt graph.
void IR_SSA_MGR::dump_ssa_graph(CHAR * name)
{
	SSA_GRAPH sa(m_ru, this);
	sa.dump(name, true);
}


CHAR * IR_SSA_MGR::dump_vp(IN VP * v, OUT CHAR * buf)
{
	sprintf(buf, "P%dV%d", VP_prno(v), VP_ver(v));
	return buf;
}


//This function dumps VP structure and SSA DU info.
//have_renamed: set true if PRs have been renamed in construction.
void IR_SSA_MGR::dump_all_vp(bool have_renamed)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP IR_SSA_MGR:VP_DU ----==\n");

	SVECTOR<VP*> const* vp_vec = get_vp_vec();
	for (INT i = 1; i <= vp_vec->get_last_idx(); i++) {
		VP * v = vp_vec->get(i);
		IS_TRUE0(v != NULL);
		fprintf(g_tfile, "\nid%d:pr%dver%d: ", SSA_id(v), VP_prno(v), VP_ver(v));
		IR * def = SSA_def(v);
		if (VP_ver(v) != 0 && !have_renamed) {
			//After renaming, version is meaningless.
			IS_TRUE0(def);
		}
		if (def != NULL) {
			IS_TRUE0(def->is_stmt());

			if (def->is_stpr()) {
				fprintf(g_tfile, "DEF:st pr%d,id%d",
						def->get_prno(), IR_id(def));
			} else if (def->is_phi()) {
				fprintf(g_tfile, "DEF:phi pr%d,id%d",
						def->get_prno(), IR_id(def));
			} else if (def->is_call()) {
				fprintf(g_tfile, "DEF:call");
				if (def->has_return_val()) {
					fprintf(g_tfile, " pr%d,id%d", def->get_prno(), IR_id(def));
				}
			} else {
				IS_TRUE(0, ("not def stmt of PR"));
			}
		} else {
			fprintf(g_tfile, "DEF:---");
		}

		fprintf(g_tfile, "\tUSE:");

		SSAUSE_ITER vit;
		INT nexti = 0;
		for (INT i = SSA_uses(v).get_first(&vit); vit != NULL; i = nexti) {
			nexti = SSA_uses(v).get_next(i, &vit);
			IR * use = m_ru->get_ir(i);
			IS_TRUE0(use->is_pr());

			fprintf(g_tfile, "(pr%d,id%d)", use->get_prno(), IR_id(use));

			if (nexti >= 0) {
				fprintf(g_tfile, ",");
			}
		}
	}
	fflush(g_tfile);
}


void IR_SSA_MGR::dump()
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n\n==---- DUMP Result of IR_SSA_MGR ----==\n");

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	LIST<IR const*> lst;
	LIST<IR const*> opnd_lst;
	for (IR_BB * bb = bbl->get_head();
		 bb != NULL; bb = bbl->get_next()) {
		fprintf(g_tfile, "\n--- BB%d ---", IR_BB_id(bb));
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			g_indent = 4;
			dump_ir(ir, m_dm);
			lst.clean();
			opnd_lst.clean();
			//Find IR_PR.
			bool find = false;
			for (IR const* opnd = ir_iter_init_c(ir, lst);
				opnd != NULL;
				opnd = ir_iter_next_c(lst)) {
				if (ir->is_rhs(opnd) && IR_type(opnd) == IR_PR) {
					opnd_lst.append_tail(opnd);
				}
				if (IR_type(opnd) == IR_PR) {
					find = true;
				}
			}
			if (!find) { continue; }

			IR * res = ir->get_pr_results();
			fprintf(g_tfile, "\nVP:");
			if (res != NULL) {
				for (IR * r = res; r != NULL; r = IR_next(r)) {
					VP * vp = (VP*)PR_ssainfo(r);
					IS_TRUE0(vp);
					fprintf(g_tfile, "p%dv%d ", VP_prno(vp), VP_ver(vp));
				}
			} else {
				fprintf(g_tfile, "--");
			}
			fprintf(g_tfile, " <= ");
			if (opnd_lst.get_elem_count() != 0) {
				UINT i = 0, n = opnd_lst.get_elem_count() - 1;
				for (IR const* opnd = opnd_lst.get_head(); opnd != NULL;
					 opnd = opnd_lst.get_next(), i++) {
					VP * vp = (VP*)PR_ssainfo(opnd);
					IS_TRUE0(vp);
					fprintf(g_tfile, "p%dv%d",
							VP_prno(vp), VP_ver(vp));
					if (i < n) { fprintf(g_tfile, ","); }
				}
			} else {
				fprintf(g_tfile, "--");
			}
		}
	}
	fflush(g_tfile);
}


//Build dominance frontier.
void IR_SSA_MGR::build_df(OUT DF_MGR & dfm)
{
	dfm.build((DGRAPH&)*m_cfg);
}


//Initialize VP for each PR.
IR * IR_SSA_MGR::init_vp(IN IR * ir)
{
	IR * prres = NULL;

	//Process result.
	if (ir->is_stpr()) {
		prres = ir;
		STPR_ssainfo(ir) = new_vp(STPR_no(ir), 0);
		if (m_prno2ir.get(STPR_no(ir)) == NULL) {
			m_prno2ir.set(STPR_no(ir), ir);
		}
	} else if (ir->is_phi()) {
		prres = ir;
		PHI_ssainfo(ir) = new_vp(PHI_prno(ir), 0);
		if (m_prno2ir.get(PHI_prno(ir)) == NULL) {
			m_prno2ir.set(PHI_prno(ir), ir);
		}
	} else if (ir->is_call()) {
		prres = ir;
		CALL_ssainfo(prres) = new_vp(CALL_prno(ir), 0);
		if (m_prno2ir.get(CALL_prno(ir)) == NULL) {
			m_prno2ir.set(CALL_prno(ir), ir);
		}
	} else {
		IS_TRUE0(ir->get_pr_results() == NULL);
	}

	//Process opnd.
	m_iter.clean();
	for (IR * kid = ir_iter_init(ir, m_iter);
		 kid != NULL; kid = ir_iter_next(m_iter)) {
		if (ir->is_rhs(kid) && kid->is_pr()) {
			PR_ssainfo(kid) = new_vp(PR_no(kid), 0);

			//SSA_uses(PR_ssainfo(kid)).append(kid);
			if (m_prno2ir.get(PR_no(kid)) == NULL) {
				m_prno2ir.set(PR_no(kid), kid);
			}
		}
	}
	return prres;
}


//'mustdef_pr': record PRs which defined in 'bb'.
void IR_SSA_MGR::collect_defed_pr(IN IR_BB * bb, OUT SBITSET & mustdef_pr)
{
	for (IR * ir = IR_BB_first_ir(bb); ir != NULL; ir = IR_BB_next_ir(bb)) {
		//Generate VP for non-phi stmt.
		IR * res = init_vp(ir);
		for (IR * r = res; r != NULL; r = IR_next(r)) {
			mustdef_pr.bunion(r->get_prno());
		}
	}
}


void IR_SSA_MGR::insert_phi(UINT prno, IN IR_BB * bb)
{
	UINT num_opnd = m_cfg->get_in_degree(m_cfg->get_vertex(IR_BB_id(bb)));
	IR * pr = m_prno2ir.get(prno);
	IS_TRUE0(pr);

	//Here each operand and result of phi set to same type.
	//They will be revised to correct type during renaming.
	IR * phi = m_ru->build_phi(pr->get_prno(), IR_dt(pr), num_opnd);

	m_ru->alloc_ref_for_pr(phi);

	for (IR * opnd = PHI_opnd_list(phi); opnd != NULL; opnd = IR_next(opnd)) {
		opnd->copy_ref(phi, m_ru);
	}

	IR_BB_ir_list(bb).append_head(phi);

	init_vp(phi);
}


//Insert phi for PR.
//defbbs: record BBs which defined the PR identified by 'prno'.
void IR_SSA_MGR::place_phi_for_pr(UINT prno, IN LIST<IR_BB*> * defbbs,
								  DF_MGR & dfm, BITSET & visited,
								  LIST<IR_BB*> & wl)
{
	visited.clean();
	wl.clean();
	for (IR_BB * defbb = defbbs->get_head();
		 defbb != NULL; defbb = defbbs->get_next()) {
		wl.append_tail(defbb);
		visited.bunion(IR_BB_id(defbb));
	}

	while (wl.get_elem_count() != 0) {
		IR_BB * bb = wl.remove_head();

		//Each basic block in dfcs is in dominance frontier of 'bb'.
		BITSET const* dfcs = dfm.get_df_ctrlset_c(IR_BB_id(bb));
		if (dfcs == NULL) { continue; }

		for (INT i = dfcs->get_first(); i >= 0; i = dfcs->get_next(i)) {
			if (visited.is_contain(i)) {
				//Already insert phi for 'prno' into BB i.
				//TODO:ensure the phi for same PR does NOT be
				//inserted multiple times.
				continue;
			}

			visited.bunion(i);

			IR_BB * ibb = m_ru->get_bb(i);
			IS_TRUE0(ibb);

			//Redundant phi will be removed during refine_phi().
			insert_phi(prno, ibb);

			wl.append_tail(ibb);
		}
	}
}


void IR_SSA_MGR::compute_effect(IN OUT BITSET & effect_prs,
								IN BITSET & defed_prs,
								IN IR_BB * bb,
								IN PRDF & live_mgr,
								IN SVECTOR<BITSET*> & pr2defbb)
{
	for (INT i = defed_prs.get_first(); i >= 0; i = defed_prs.get_next(i)) {
		BITSET * bbs = pr2defbb.get(i);
		if (bbs == NULL) {
			bbs = new BITSET();
			pr2defbb.set(i, bbs);
		}
		bbs->bunion(IR_BB_id(bb));

		IR * pr = map_prno2ir(i);
		if (live_mgr.get_liveout(IR_BB_id(bb))->is_contain(i)) {
			effect_prs.bunion(i);
		}
	}
}


/* Return true if phi is redundant, otherwise return false.
If all opnds have same defintion or defined by current phi,
the phi is redundant.
common_def: record the common_def if the definition of all opnd is the same. */
bool IR_SSA_MGR::is_redundant_phi(IR const* phi, OUT IR ** common_def) const
{
	IS_TRUE0(phi->is_phi());

	VP * vp = (VP*)PHI_ssainfo(phi);
	IS_TRUE0(vp);
	if (SSA_uses(vp).get_elem_count() == 0) { return true; }

	#define DUMMY_DEF_ADDR	0x1234
	IR * def = NULL;
	bool same_def = true; //indicate all DEF of operands are the same stmt.
	for (IR const* opnd = PHI_opnd_list(phi);
		 opnd != NULL; opnd = IR_next(opnd)) {
		IS_TRUE0(opnd->is_phi_opnd());

		if (!opnd->is_pr()) { continue; }

		SSAINFO const* si = PR_ssainfo(opnd);
		IS_TRUE0(si);

		if (SSA_def(si) != NULL) {
			if (def == NULL) {
				def = SSA_def(si);
			} else if (def != SSA_def(si) && def != phi) {
				same_def = false;
				break;
			}
		} else {
			//Assign def a dummy value to inidcate the region live-in PR.
			def = (IR*)DUMMY_DEF_ADDR;
		}
	}

	IS_TRUE0(common_def);
	if (def == (IR*)DUMMY_DEF_ADDR) {
		*common_def = NULL;
	} else {
		*common_def = def;
	}
	return same_def;
}


//Place phi and assign the v0 for each PR.
//'effect_prs': record the pr which need to versioning.
void IR_SSA_MGR::place_phi(OPT_CTX & oc,
						   IN DF_MGR & dfm,
						   OUT SBITSET & effect_prs,
						   SDBITSET_MGR & bs_mgr,
						   SVECTOR<SBITSET*> & defed_prs_vec,
						   LIST<IR_BB*> & wl)
{
	START_TIMERS("SSA: Place phi", t2);
	//Record the defbb list for each pr.
	IR_BB_LIST * bblst = m_ru->get_bb_list();
	SVECTOR<LIST<IR_BB*>*> pr2defbb(bblst->get_elem_count()); //for local used.

	//SBITSET defed_prs;
	for (IR_BB * bb = bblst->get_head(); bb != NULL; bb = bblst->get_next()) {
		//defed_prs.clean();
		SBITSET * bs = bs_mgr.create_sbitset();
		defed_prs_vec.set(IR_BB_id(bb), bs);
		collect_defed_pr(bb, *bs);
		//compute_effect(effect_prs, tmp, bb, live_mgr, pr2defbb);

		//Regard all defined PR as effect, and they will be versioned later.
		effect_prs.bunion(*bs);

		//Record which BB defined these effect prs.
		SC<SEG*> * cur;
		for (INT i = bs->get_first(&cur); i >= 0; i = bs->get_next(i, &cur)) {
			LIST<IR_BB*> * bbs = pr2defbb.get(i);
			if (bbs == NULL) {
				bbs = new LIST<IR_BB*>();
				pr2defbb.set(i, bbs);
			}
			bbs->append_tail(bb);
		}
	}

	//Place phi for lived effect prs.
	wl.clean();
	BITSET visited((bblst->get_elem_count()/8)+1);
	SC<SEG*> * cur;
	for (INT i = effect_prs.get_first(&cur);
		 i >= 0; i = effect_prs.get_next(i, &cur)) {
		place_phi_for_pr(i, pr2defbb.get(i), dfm, visited, wl);
	}
	END_TIMERS(t2);

	//Free local used objects.
	for (INT i = 0; i <= pr2defbb.get_last_idx(); i++) {
		LIST<IR_BB*> * bbs = pr2defbb.get(i);
		if (bbs == NULL) { continue; }
		delete bbs;
	}
}


//Rename vp from current version to the top-version on stack if it exist.
void IR_SSA_MGR::rename_bb(IN IR_BB * bb)
{
 	for (IR * ir = IR_BB_first_ir(bb);
		 ir != NULL; ir = IR_BB_next_ir(bb)) {
		if (IR_type(ir) != IR_PHI) {
			//Rename opnd, not include phi.
			//Walk through rhs expression IR tree to rename IR_PR's VP.
			m_iter.clean();
			for (IR * opnd = ir_iter_init(ir, m_iter);
				 opnd != NULL; opnd = ir_iter_next(m_iter)) {
				if (!ir->is_rhs(opnd) || !opnd->is_pr()) {
					continue;
				}

				//Get the top-version on stack.
				SSTACK<VP*> * vs = map_prno2vpstack(PR_no(opnd));
				IS_TRUE0(vs);
				VP * topv = vs->get_top();
				if (topv == NULL) {
					//prno has no top-version, it has no def, may be parameter.
					IS_TRUE0(PR_ssainfo(opnd));
					IS_TRUE(VP_ver((VP*)PR_ssainfo(opnd)) == 0,
							("parameter only has first version"));
					continue;
				}

				/*
				e.g: pr1 = pr2(vp1)
					vp1 will be renamed to vp2, so vp1 does not
					occur in current IR any more.
				*/
				VP * curv = (VP*)PR_ssainfo(opnd);
				IS_TRUE0(curv && VP_prno(curv) == PR_no(opnd));

				//Set newest version VP uses current opnd.
				if (VP_ver(topv) == 0) {
					//Each opnd only meet once.
					IS_TRUE0(curv == topv);
					IS_TRUE0(!SSA_uses(topv).find(opnd));
					SSA_uses(topv).append(opnd);
				} else if (curv != topv) {
					//curv may be ver0.
					//Current ir does not refer the old version VP any more.
					IS_TRUE0(VP_ver(curv) == 0 || SSA_uses(curv).find(opnd));
					IS_TRUE0(VP_ver(curv) == 0 || SSA_def(curv) != NULL);
					IS_TRUE0(!SSA_uses(topv).find(opnd));

					SSA_uses(curv).remove(opnd);
					PR_ssainfo(opnd) = topv;
					SSA_uses(topv).append(opnd);
				}
			}
		}

		//Rename result, include phi.
		IR * res = ir->get_pr_results();
		for (IR * r = res; r != NULL; r = IR_next(r)) {
			if (r->is_stpr() || r->is_phi()) {
				IS_TRUE0(IR_prev(r) == NULL && IR_next(r) == NULL);
			}

			VP * resvp = (VP*)r->get_ssainfo();
			IS_TRUE0(resvp);
			IS_TRUE0(VP_prno(resvp) == r->get_prno());
			UINT prno = VP_prno(resvp);
			UINT maxv = m_max_version.get(prno);
			VP * new_v = new_vp(prno, maxv + 1);
			m_max_version.set(prno, maxv + 1);

			map_prno2vpstack(prno)->push(new_v);
			res->set_ssainfo(new_v);
			SSA_def(new_v) = ir;
		}
	}
}


SSTACK<VP*> * IR_SSA_MGR::map_prno2vpstack(UINT prno)
{
	SSTACK<VP*> * stack = m_map_prno2stack.get(prno);
	if (stack == NULL) {
		stack = new SSTACK<VP*>();
		m_map_prno2stack.set(prno, stack);
	}
	return stack;
}


void IR_SSA_MGR::handle_bb(IR_BB * bb, SBITSET & defed_prs,
						   IN OUT BB2VP & bb2vp, IN LIST<IR*> & lst)
{
	IS_TRUE0(bb2vp.get(IR_BB_id(bb)) == NULL);
	SVECTOR<VP*> * ve_vec = new SVECTOR<VP*>();
	bb2vp.set(IR_BB_id(bb), ve_vec);

	SC<SEG*> * cur;
	for (INT prno = defed_prs.get_first(&cur);
		 prno >= 0; prno = defed_prs.get_next(prno, &cur)) {
		VP * ve = map_prno2vpstack(prno)->get_top();
		IS_TRUE0(ve);
		ve_vec->set(VP_prno(ve), ve);
	}
	rename_bb(bb);

	//Rename PHI opnd in successor BB.
	LIST<IR_BB*> succs;
	m_cfg->get_succs(succs, bb);
	if (succs.get_elem_count() > 0) {
		//Replace the jth opnd of PHI with 'topv' which in bb's successor.
		LIST<IR_BB*> preds;
		for (IR_BB * succ = succs.get_head();
			 succ != NULL; succ = succs.get_next()) {
			//Compute which predecessor 'bb' is with respect to its successor.
			m_cfg->get_preds(preds, succ);
			UINT idx = 0; //the index of corresponding predecessor.
			IR_BB * p;
			for (p = preds.get_head();
				 p != NULL; p = preds.get_next(), idx++) {
				if (p == bb) {
					break;
				}
			}
			IS_TRUE0(p != NULL);

			//Replace opnd of PHI of 'succ' with top SSA version.
			C<IR*> * ct;
			for (IR * ir = IR_BB_ir_list(succ).get_head(&ct);
				 ir != NULL; ir = IR_BB_ir_list(succ).get_next(&ct)) {
				if (IR_type(ir) != IR_PHI) {
					break;
				}

				//Update version for same PR.
				IR * opnd = PHI_opnd_list(ir);
				UINT j = 0;
				while (opnd != NULL && j < idx) {
					opnd = IR_next(opnd);
					j++;
				}
				IS_TRUE0(j == idx && opnd);
				VP * old = (VP*)PR_ssainfo(opnd);
				IS_TRUE0(old != NULL);
				VP * topv = map_prno2vpstack(VP_prno(old))->get_top();
				IS_TRUE0(topv != NULL);

				SSA_uses(old).remove(opnd);

				if (SSA_def(topv) != NULL) {
					//topv might be zero version.
					IR * defres = SSA_def(topv)->get_res_pr(VP_prno(topv));
					IS_TRUE0(defres);

					IR * new_opnd = m_ru->build_pr_dedicated(
											defres->get_prno(), IR_dt(defres));
					new_opnd->copy_ref(defres, m_ru);
					replace(&PHI_opnd_list(ir), opnd, new_opnd);
					IR_parent(new_opnd) = ir;
					m_ru->free_irs(opnd);
					opnd = new_opnd;

					//Phi should have same type with opnd.
					IR_dt(ir) = IR_dt(defres);
				}

				PR_ssainfo(opnd) = topv;

				//Add version0 opnd here, means opnd will be add to ver0
				//use-list if topv is version0. So one does not need to
				//add version0 at place_phi.
				SSA_uses(topv).append(opnd);
			}
		}
	}
}


//defed_prs_vec: for each BB, indicate PRs which has been defined.
void IR_SSA_MGR::rename_in_dom_tree_order(IR_BB * root, GRAPH & domtree,
										  SVECTOR<SBITSET*> & defed_prs_vec)
{
	SSTACK<IR_BB*> stk;
	UINT n = m_ru->get_bb_list()->get_elem_count();
	BITSET visited(n / BIT_PER_BYTE);
	BB2VP bb2vp(n);
	IR_BB * v;
	stk.push(root);
	LIST<IR*> lst; //for tmp use.
	while ((v = stk.get_top()) != NULL) {
		if (!visited.is_contain(IR_BB_id(v))) {
			visited.bunion(IR_BB_id(v));
			SBITSET * defed_prs = defed_prs_vec.get(IR_BB_id(v));
			IS_TRUE0(defed_prs);
			handle_bb(v, *defed_prs, bb2vp, lst);
		}

		VERTEX const* bbv = domtree.get_vertex(IR_BB_id(v));
		EDGE_C const* c = VERTEX_out_list(bbv);
		bool all_visited = true;
		while (c != NULL) {
			VERTEX * dom_succ = EDGE_to(EC_edge(c));
			if (dom_succ == bbv) { continue; }
			if (!visited.is_contain(VERTEX_id(dom_succ))) {
				IS_TRUE0(m_cfg->get_bb(VERTEX_id(dom_succ)));
				all_visited = false;
				stk.push(m_cfg->get_bb(VERTEX_id(dom_succ)));
				break;
			}
			c = EC_next(c);
		}

		if (all_visited) {
			stk.pop();

			//Do post-processing while all kids of BB has been processed.
			SVECTOR<VP*> * ve_vec = bb2vp.get(IR_BB_id(v));
			IS_TRUE0(ve_vec);
			SBITSET * defed_prs = defed_prs_vec.get(IR_BB_id(v));
			IS_TRUE0(defed_prs);

			SC<SEG*> * cur;
			for (INT i = defed_prs->get_first(&cur);
				 i >= 0; i = defed_prs->get_next(i, &cur)) {
				SSTACK<VP*> * vs = map_prno2vpstack(i);
				IS_TRUE0(vs->get_bottom() != NULL);
				VP * ve = ve_vec->get(VP_prno(vs->get_top()));
				while (vs->get_top() != ve) {
					vs->pop();
				}
			}
			bb2vp.set(IR_BB_id(v), NULL);
			delete ve_vec;
		}
	}

	#ifdef _DEBUG_
	for (INT i = 0; i <= bb2vp.get_last_idx(); i++) {
		IS_TRUE0(bb2vp.get(i) == NULL);
	}
	#endif
}


//Do renaming in dfs-preorder at dominator tree.
void IR_SSA_MGR::rename_in_dom_tree_order_dfs(IN IR_BB * bb,
											  IN GRAPH & dom_tree,
											  IN BITSET & effect_prs)
{
	SVECTOR<VP*> ve_vec;
	for (INT i = effect_prs.get_first(); i >= 0;
		 i = effect_prs.get_next(i)) {
		VP * ve = map_prno2vpstack(i)->get_top();
		IS_TRUE0(ve);
		ve_vec.set(VP_prno(ve), ve);
	}

	rename_bb(bb);

	//Rename PHI opnd in successor BB.
	LIST<IR_BB*> succs;
	m_cfg->get_succs(succs, bb);
	if (succs.get_elem_count() > 0) {
		for (IR_BB * succ = succs.get_head(); succ != NULL;
			 succ = succs.get_next()) {

			LIST<IR_BB*> preds;
			m_cfg->get_preds(preds, succ);
			UINT idx = 0; //the index of corresponding predecessor.
			IR_BB * p;
			for (p = preds.get_head(); p != NULL; p = preds.get_next(), idx++) {
				if (p == bb) {
					break;
				}
			}
			IS_TRUE0(p != NULL);

			//Replace opnd of PHI of 'succ' with top SSA version.
			C<IR*> * ct;
			for (IR * ir = IR_BB_ir_list(succ).get_head(&ct); ir != NULL;
				 ir = IR_BB_ir_list(succ).get_next(&ct)) {
				if (IR_type(ir) != IR_PHI) {
					break;
				}

				IR * opnd = PHI_opnd_list(ir);
				UINT j = 0;
				while (opnd != NULL && j < idx) {
					opnd = IR_next(opnd);
					j++;
				}
				IS_TRUE0(j == idx && opnd);
				VP * old = (VP*)PR_ssainfo(opnd);
				IS_TRUE0(old != NULL);
				SSA_uses(old).remove(opnd);
				VP * topv = map_prno2vpstack(VP_prno(old))->get_top();
				PR_ssainfo(opnd) = topv;
				SSA_uses(topv).append(opnd);
			}
		}

		VERTEX * bbv = dom_tree.get_vertex(IR_BB_id(bb));
		EDGE_C * c = VERTEX_out_list(bbv);
		while (c != NULL) {
			VERTEX * dom_succ = EDGE_to(EC_edge(c));
			if (dom_succ == bbv) { continue; }
			rename_in_dom_tree_order_dfs(m_cfg->get_bb(VERTEX_id(dom_succ)),
									  dom_tree, effect_prs);
			c = EC_next(c);
		}
	}

	for (INT i = effect_prs.get_first(); i >= 0;
		 i = effect_prs.get_next(i)) {
		SSTACK<VP*> * vs = map_prno2vpstack(i);
		IS_TRUE0(vs->get_bottom() != NULL);
		VP * ve = ve_vec.get(VP_prno(vs->get_top()));
		while (vs->get_top() != ve) {
			vs->pop();
		}
	}
}


//Rename variables.
void IR_SSA_MGR::rename_dfs(IN BITSET & effect_prs)
{
	IR_BB_LIST * bblst = m_ru->get_bb_list();
	if (bblst->get_elem_count() == 0) { return; }

	GRAPH dtree;
	m_cfg->get_dom_tree(dtree);
	LIST<IR_BB*> * l = m_cfg->get_entry_list();
	IS_TRUE(l->get_elem_count() == 1, ("illegal region"));
	IR_BB * entry = l->get_head();
	for (INT prno = effect_prs.get_first();
		 prno >= 0; prno = effect_prs.get_next(prno)) {
		VP * vp = new_vp(prno, 0);
		map_prno2vpstack(prno)->push(vp);
	}
	rename_in_dom_tree_order_dfs(entry, dtree, effect_prs);
}


//Rename variables.
void IR_SSA_MGR::rename(SBITSET & effect_prs,
						SVECTOR<SBITSET*> & defed_prs_vec,
						GRAPH & domtree)
{
	START_TIMERS("SSA: Rename", t);
	IR_BB_LIST * bblst = m_ru->get_bb_list();
	if (bblst->get_elem_count() == 0) { return; }

	LIST<IR_BB*> * l = m_cfg->get_entry_list();
	IS_TRUE(l->get_elem_count() == 1, ("illegal region"));
	IR_BB * entry = l->get_head();

	SC<SEG*> * cur;
	for (INT prno = effect_prs.get_first(&cur);
		 prno >= 0; prno = effect_prs.get_next(prno, &cur)) {
		VP * vp = new_vp(prno, 0);
		map_prno2vpstack(prno)->push(vp);
	}
	IS_TRUE0(entry);
	rename_in_dom_tree_order(entry, domtree, defed_prs_vec);
	END_TIMERS(t);
}


void IR_SSA_MGR::destruct_bb(IR_BB * bb, IN OUT bool & insert_stmt_after_call)
{
	C<IR*> * ct, * next_ct;
	IR_BB_ir_list(bb).get_head(&next_ct);
	ct = next_ct;
	for (; ct != NULL; ct = next_ct) {
		IR_BB_ir_list(bb).get_next(&next_ct);
		IR * ir = C_val(ct);
		if (IR_type(ir) != IR_PHI) {
			if (insert_stmt_after_call) {
				continue;
			} else {
				break;
			}
		}

		insert_stmt_after_call |= strip_phi(ir, ct);
		IR_BB_ir_list(bb).remove(ct);
		m_ru->free_irs(ir);
	}
}


void IR_SSA_MGR::destruction_in_dom_tree_order(IR_BB * root, GRAPH & domtree)
{
	SSTACK<IR_BB*> stk;
	UINT n = m_ru->get_bb_list()->get_elem_count();
	BITSET visited(n / BIT_PER_BYTE);
	BB2VP bb2vp(n);
	IR_BB * v;
	stk.push(root);
	bool insert_stmt_after_call = false;
	while ((v = stk.get_top()) != NULL) {
		if (!visited.is_contain(IR_BB_id(v))) {
			visited.bunion(IR_BB_id(v));
			destruct_bb(v, insert_stmt_after_call);
		}

		VERTEX * bbv = domtree.get_vertex(IR_BB_id(v));
		IS_TRUE(bbv, ("dom tree is invalid."));

		EDGE_C * c = VERTEX_out_list(bbv);
		bool all_visited = true;
		while (c != NULL) {
			VERTEX * dom_succ = EDGE_to(EC_edge(c));
			if (dom_succ == bbv) { continue; }
			if (!visited.is_contain(VERTEX_id(dom_succ))) {
				IS_TRUE0(m_cfg->get_bb(VERTEX_id(dom_succ)));
				all_visited = false;
				stk.push(m_cfg->get_bb(VERTEX_id(dom_succ)));
				break;
			}
			c = EC_next(c);
		}

		if (all_visited) {
			stk.pop();
			//Do post-processing while all kids of BB has been processed.
		}
	}
}


/* This function perform SSA destruction via scanning BB in preorder
traverse dominator tree.
Return true if inserting copy at the head of fallthrough BB
of current BB's predessor. */
void IR_SSA_MGR::destruction(DOM_TREE & domtree)
{
	START_TIMER_FMT();

	IR_BB_LIST * bblst = m_ru->get_bb_list();
	if (bblst->get_elem_count() == 0) { return; }

	LIST<IR_BB*> * l = m_cfg->get_entry_list();
	IS_TRUE(l->get_elem_count() == 1, ("illegal region"));
	IR_BB * entry = l->get_head();
	IS_TRUE0(entry);

	destruction_in_dom_tree_order(entry, domtree);

	clean_pr_ssainfo();

	m_is_ssa_construct = false;

	END_TIMER_FMT(("SSA: destruction in dom tree order"));
}


/* Return true if inserting copy at the head of fallthrough BB
of current BB's predessor.
Note that do not free phi at this function, it will be freed
by user. */
bool IR_SSA_MGR::strip_phi(IR * phi, C<IR*> * phict)
{
	IR_BB * bb = phi->get_bb();
	IS_TRUE0(bb);

	VERTEX const* vex = m_cfg->get_vertex(IR_BB_id(bb));
	IS_TRUE0(vex);

	//Temprarory RP to hold the result of PHI.
	IR * phicopy = m_ru->build_pr(IR_dt(phi));
	phicopy->set_ref_md(m_ru->gen_md_for_pr(PR_no(phicopy),
								IR_dt(phicopy)), m_ru);
	phicopy->clean_ref_mds();

	bool insert_stmt_after_call = false;

	IR * opnd = PHI_opnd_list(phi);
	IS_TRUE0(opnd->is_pr());

	VP * phivp = (VP*)PHI_ssainfo(phi);
	IS_TRUE0(phivp);

	for (EDGE_C * el = VERTEX_in_list(vex); el != NULL;
		 el = EC_next(el), opnd = IR_next(opnd)) {
		INT pred = VERTEX_id(EDGE_from(EC_edge(el)));

		IR * opndcopy = m_ru->dup_irs(opnd);
		if (opndcopy->is_pr()) {
			opndcopy->copy_ref(opnd, m_ru);
		}

		//The copy will be inserted into related predecessor.
		IR * store_to_phicopy = m_ru->build_store_pr(PR_no(phicopy),
									IR_dt(phicopy), opndcopy);
		store_to_phicopy->copy_ref(phicopy, m_ru);

		if (m_need_maintain_du_chain) {
			IS_TRUE0(m_ru->get_du_mgr());
			//Add DU chain for IR_DU_MGR between phi opnd and the
			//real definition. There may be many real definition
			//for PR in non-SSA mode.
			DU_SET const* opnd_duset = opnd->get_duset_c();
			if (opnd_duset != NULL) {
				//We are accessing BB via Dom Tree top down.
				//The prior PHI of current phi opnd should already striped.
				DU_ITER dui;
				for (INT i = opnd_duset->get_first(&dui);
					 dui != NULL; opnd_duset->get_next(i, &dui)) {
				 	IR * defstmt = m_ru->get_ir(i);
					IS_TRUE0(defstmt->is_stmt() && !defstmt->is_phi());
					IS_TRUE0(defstmt->is_exact_def(opndcopy->get_ref_md()));
					m_ru->get_du_mgr()->build_du_chain(defstmt, opndcopy);
				}
				m_ru->get_du_mgr()->remove_use_out_from_defset(opnd);
			}

			IS_TRUE0(store_to_phicopy->is_exact_def(phicopy->get_ref_md()));
			m_ru->get_du_mgr()->build_du_chain(store_to_phicopy, phicopy);
		}

		IR_BB * p = m_cfg->get_bb(pred);
		IS_TRUE0(p);
		IR * plast = IR_BB_last_ir(p);
		if (plast != NULL && plast->is_call()) {
			IR_BB * fallthrough = m_cfg->get_fallthrough_bb(p);

			/* fallthrough BB may not exist if the last ir has terminate
			attribute. That will an may-execution path to other bb if
			the last ir may throw exception. */
			IS_TRUE(fallthrough, ("invalid control flow."));
			IR_BB_ir_list(fallthrough).append_head(store_to_phicopy);
			insert_stmt_after_call = true;
		} else {
			IR_BB_ir_list(p).append_tail_ex(store_to_phicopy);
		}

		//Remove the SSA DU chain between opnd and its DEF stmt.
		IS_TRUE0(PR_ssainfo(opnd));
		SSA_uses(PR_ssainfo(opnd)).remove(opnd);
	}

	IR * substitue_phi = m_ru->build_store_pr(PHI_prno(phi),
											IR_dt(phi), phicopy);
	substitue_phi->copy_ref(phi, m_ru);

	if (m_need_maintain_du_chain) {
		IS_TRUE0(m_ru->get_du_mgr());

		//Add DU chain for IR_DU_MGR between COPY and the real USE.
		SSAUSE_ITER vi;
		for (INT i = SSA_uses(phivp).get_first(&vi);
			 i >= 0; i = SSA_uses(phivp).get_next(i, &vi)) {
			IR * exp = m_ru->get_ir(i);
			IS_TRUE0(exp->is_pr());

			IS_TRUE(exp->get_stmt() != phi, ("phi reference itself"));

			IS_TRUE0(substitue_phi->is_exact_def(exp->get_ref_md()));

			m_ru->get_du_mgr()->build_du_chain(substitue_phi, exp);
		}

		m_ru->get_du_mgr()->remove_def_out_from_useset(phi);
	}

	SSA_def(phivp) = NULL;

	IR_BB_ir_list(bb).insert_before(substitue_phi, phict);

	PHI_ssainfo(phi) = NULL;

	return insert_stmt_after_call;
}


/* This function verify def/use information of PHI stmt.
If vpinfo is available, the function also check VP_prno of phi operands.
is_vpinfo_avail: set true if VP information is available. */
bool IR_SSA_MGR::verify_phi_sanity(bool is_vpinfo_avail)
{
	IR_BB_LIST * bblst = m_ru->get_bb_list();
	LIST<IR_BB*> lst;
	for (IR_BB * bb = bblst->get_head();
		 bb != NULL; bb = bblst->get_next()) {
		m_cfg->get_preds(lst, bb);

		UINT c = lst.get_elem_count();

		for (IR * ir = IR_BB_first_ir(bb); ir != NULL; ir = IR_BB_next_ir(bb)) {
			if (IR_type(ir) != IR_PHI) { continue; }

			//Check phi result.
			VP * resvp = (VP*)PHI_ssainfo(ir);
			IS_TRUE(!is_vpinfo_avail ||
					VP_prno(resvp) == PHI_prno(ir),
					("prno unmatch"));

			//Check the number of phi opnds.
			UINT i = 0;
			IR * opnd = PHI_opnd_list(ir);
			while (opnd != NULL) {
				IS_TRUE0(opnd->is_pr());
				IS_TRUE(!is_vpinfo_avail ||
						VP_prno((VP*)PR_ssainfo(opnd)) == PR_no(opnd),
						("prno is unmatch"));

				//Ver0 is input parameter, and it has no def.
				//IS_TRUE0(VP_ver(PR_ssainfo(opnd)) > 0);

				i++;
				opnd = IR_next(opnd);
			}
			IS_TRUE(i == c, ("the num of opnd unmatch"));

			//Check VP uses.
			SSAUSE_ITER vit;
			for (INT i = SSA_uses(resvp).get_first(&vit);
				 vit != NULL; i = SSA_uses(resvp).get_next(i, &vit)) {
				IR * use = m_ru->get_ir(i);
				IS_TRUE0(use->is_pr());

				IS_TRUE(PR_no(use) == PHI_prno(ir), ("prno is unmatch"));

				SSAINFO * use_ssainfo = PR_ssainfo(use);
				IS_TRUE0(use_ssainfo);

				IS_TRUE0(SSA_def(use_ssainfo) == ir);
			}
		}
	}
	return true;
}


/* Check the consistency for IR_PR if VP_prno == PR_no.
This function only can be invoked immediately
after rename(), because refine_phi() might clobber VP information, that leads
VP_prno() to be invalid. */
bool IR_SSA_MGR::verify_vp_prno()
{
	CIR_ITER ii;
	IR_BB_LIST * bblst = m_ru->get_bb_list();
	C<IR_BB*> * ct;
	for (IR_BB * bb = bblst->get_head(&ct);
		 bb != NULL; bb = bblst->get_next(&ct)) {
	 	for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			ii.clean();
			for (IR const* opnd = ir_iter_init_c(ir, ii);
				 opnd != NULL; opnd = ir_iter_next_c(ii)) {
				if (opnd->is_pr()) {
					IS_TRUE0(PR_no(opnd) == VP_prno((VP*)PR_ssainfo(opnd)));
				}
			}
	 	}
 	}
	return true;
}


bool IR_SSA_MGR::verify_ssainfo()
{
	//Check version for each vp.
	BITSET defset;
	for (INT i = 1; i <= m_vp_vec.get_last_idx(); i++) {
		VP * v = m_vp_vec.get(i);
		IS_TRUE0(v != NULL);
		IR * def = SSA_def(v);
		if (def == NULL) {
			//ver0 used to indicate the REGION live-in PR. It may be a parameter.
			IS_TRUE(VP_ver(v) == 0, ("Nondef vp's version must be 0"));
		} else {
			IS_TRUE(VP_ver(v) != 0, ("version can not be 0"));
			IS_TRUE0(def->is_stmt());

			IS_TRUE(!defset.is_contain(IR_id(def)),
					("DEF for each pr+version must be unique."));
			defset.bunion(IR_id(def));
		}

		IR const* respr = NULL;
		UINT defprno = 0;
		if (def != NULL) {
			respr = def->get_res_pr(VP_prno(v));
			IS_TRUE0(respr);

			defprno = respr->get_prno();
			IS_TRUE0(defprno > 0);
		}

		SSAUSE_ITER vit;
		UINT opndprno = 0;
		for (INT i = SSA_uses(v).get_first(&vit);
			 vit != NULL; i = SSA_uses(v).get_next(i, &vit)) {
			IR * use = m_ru->get_ir(i);

			IS_TRUE0(use->is_pr() || use->is_const());

			if (use->is_pr()) {
				if (opndprno == 0) {
					opndprno = PR_no(use);
				} else {
					//All opnd should have same PR no.
					IS_TRUE0(opndprno == PR_no(use));
				}

				//Each USE of current VP must be defined by same stmt.
				IS_TRUE0(PR_ssainfo(use) == v);
			}
		}

		if (opndprno != 0 && defprno != 0) {
			//Def should have same PR no with USE.
			IS_TRUE0(opndprno == defprno);
		}
	}
	return true;
}


//This function perform SSA destruction via scanning BB in sequential order.
void IR_SSA_MGR::destruction_in_bblist_order()
{
	IR_BB_LIST * bblst = m_ru->get_bb_list();
	if (bblst->get_elem_count() == 0) { return; }

	C<IR_BB*> * bbct;
	bool insert_stmt_after_call = false;
	for (IR_BB * bb = bblst->get_head(&bbct);
		 bb != NULL; bb = bblst->get_next(&bbct)) {
		destruct_bb(bb, insert_stmt_after_call);
	}

	//Clean SSA info to avoid unnecessary abort or assert.
	clean_pr_ssainfo();
	m_is_ssa_construct = false;
}


void IR_SSA_MGR::clean_pr_ssainfo()
{
	IR_BB_LIST * bblst = m_ru->get_bb_list();
	for (IR_BB * bb = bblst->get_head();
		 bb != NULL; bb = bblst->get_next()) {
		for (IR * ir = IR_BB_ir_list(bb).get_head();
			 ir != NULL; ir = IR_BB_ir_list(bb).get_next()) {
			m_iter.clean();
			for (IR * x = ir_iter_init(ir, m_iter);
				 x != NULL; x = ir_iter_next(m_iter)) {
				IS_TRUE(!x->is_phi(), ("phi should have been striped."));

				if (x->is_read_pr() || x->is_write_pr()) {
					x->set_ssainfo(NULL);
				}
			}
		}
	}
}


static void revise_phi_dt(IR * phi, REGION * ru)
{
	IS_TRUE0(phi->is_phi());
	//The data type of phi is set to be same type as its USE.
	SSAINFO * irssainfo = PHI_ssainfo(phi);
	IS_TRUE0(irssainfo);
	IS_TRUE(SSA_uses(irssainfo).get_elem_count() > 0,
			("phi has no use, it is redundant at all."));

	SSAUSE_ITER si = NULL;
	INT i = SSA_uses(irssainfo).get_first(&si);
	IS_TRUE0(si && i >= 0);
	IS_TRUE0(ru->get_ir(i)->is_pr());
	IS_TRUE0(PR_no(ru->get_ir(i)) == PHI_prno(phi));

	IR_dt(phi) = IR_dt(ru->get_ir(i));
}


//This function revise phi data type, and remove redundant phi.
//wl: work list for temporary used.
void IR_SSA_MGR::refine_phi(LIST<IR_BB*> & wl)
{
	START_TIMERS("SSA: Refine phi", t);

	IR_BB_LIST * bblst = m_ru->get_bb_list();
	C<IR_BB*> * ct;

	wl.clean();
	for (IR_BB * bb = bblst->get_head(&ct);
		 bb != NULL; bb = bblst->get_next(&ct)) {
		wl.append_tail(bb);
	}

	IR_BB * bb = NULL;
	while ((bb = wl.remove_head()) != NULL) {
		C<IR*> * irct = NULL;
		C<IR*> * nextirct = NULL;
		for (IR_BB_ir_list(bb).get_head(&nextirct), irct = nextirct;
			 irct != NULL; irct = nextirct) {
			IR_BB_ir_list(bb).get_next(&nextirct);

			IR * ir = C_val(irct);

			if (!ir->is_phi()) { break; }

			IR * common_def = NULL;
			if (is_redundant_phi(ir, &common_def)) {
				for (IR const* opnd = PHI_opnd_list(ir);
					 opnd != NULL; opnd = IR_next(opnd)) {
					IS_TRUE0(opnd->is_phi_opnd());

					if (!opnd->is_pr()) { continue; }

					SSAINFO * si = PR_ssainfo(opnd);
					IS_TRUE(si, ("Miss SSAINFO."));

					SSA_uses(si).remove(opnd);

					if (SSA_def(si) == NULL || !SSA_def(si)->is_phi()) {
						continue;
					}

					IR_BB * defbb = SSA_def(si)->get_bb();

					IS_TRUE(defbb, ("defbb does not belong to any BB"));

					wl.append_tail(defbb);
				}

				SSAINFO * curphi_ssainfo = PHI_ssainfo(ir);
				IS_TRUE0(curphi_ssainfo);
				IS_TRUE0(SSA_def(curphi_ssainfo) == ir);

				if (common_def != NULL && ir != common_def) {
					//All operands of PHI are defined by same alternative stmt,
					//just call it common_def. Replace the SSA_def of
					//current SSAINFO to the common_def.

					IS_TRUE0(common_def->get_res_pr(PHI_prno(ir)));
					IS_TRUE(common_def->get_res_pr(PHI_prno(ir))->get_prno()
							== PHI_prno(ir), ("not same PR"));

					IR * respr = common_def->get_res_pr(PHI_prno(ir));
					IS_TRUE0(respr);

					SSAINFO * commdef_ssainfo = respr->get_ssainfo();
					IS_TRUE0(commdef_ssainfo);

					SSA_uses(commdef_ssainfo).bunion(SSA_uses(curphi_ssainfo));

					SSAUSE_ITER si = NULL;
					for (INT i = SSA_uses(curphi_ssainfo).get_first(&si);
						 si != NULL;
						 i = SSA_uses(curphi_ssainfo).get_next(i, &si)) {
						IR * use = m_ru->get_ir(i);
						IS_TRUE0(use->is_pr());

						SSAINFO * use_ssainfo = PR_ssainfo(use);
						IS_TRUE0(PR_ssainfo(use) &&
								 PR_ssainfo(use) == curphi_ssainfo);

						PR_ssainfo(use) = commdef_ssainfo;
					}
				}

				((VP*)curphi_ssainfo)->clean_member();
				curphi_ssainfo->clean_du();

				IR_BB_ir_list(bb).remove(irct);

				m_ru->free_ir(ir);

				continue;
			}

			revise_phi_dt(ir, m_ru);
		}
	}

	END_TIMERS(t);
}


//This function revise phi data type, and remove redundant phi.
void IR_SSA_MGR::strip_version_for_bblist()
{
	START_TIMERS("SSA: Strip version", t);

	IR_BB_LIST * bblst = m_ru->get_bb_list();
	if (bblst->get_elem_count() == 0) { return; }

	C<IR_BB*> * ct;

	BITSET visited;

	//Ensure the first allocation of bitset could
	//accommodata the last vp id.
	visited.bunion(m_vp_count);
	visited.diff(m_vp_count);

	for (IR_BB * bb = bblst->get_head(&ct);
		 bb != NULL; bb = bblst->get_next(&ct)) {
		C<IR*> * irct = NULL;
		C<IR*> * nextirct = NULL;
		for (IR_BB_ir_list(bb).get_head(&nextirct), irct = nextirct;
			 irct != NULL; irct = nextirct) {
			IR_BB_ir_list(bb).get_next(&nextirct);

			IR * ir = C_val(irct);
			strip_stmt_version(ir, visited);
		}
	}

	END_TIMERS(t);
}


//Return the replaced one.
static IR * replace_res_pr(IR * stmt, UINT oldprno, UINT newprno, UINT newprdt)
{
	//Permit newprdt to be VOID_TY.
	IS_TRUE0(newprno > 0);

	//Replace stmt PR and DATA_TYPE info.
	switch (IR_type(stmt)) {
	case IR_STPR:
		IS_TRUE0(STPR_no(stmt) == oldprno);
		STPR_no(stmt) = newprno;
		IR_dt(stmt) = newprdt;
		return stmt;
	case IR_SETEPR:
		IS_TRUE0(SETEPR_no(stmt) == oldprno);
		SETEPR_no(stmt) = newprno;
		IR_dt(stmt) = newprdt;
		return stmt;
	case IR_GETEPR:
		IS_TRUE0(GETEPR_no(stmt) == oldprno);
		GETEPR_no(stmt) = newprno;
		IR_dt(stmt) = newprdt;
		return stmt;
	case IR_PHI:
		IS_TRUE0(PHI_prno(stmt) == oldprno);
		PHI_prno(stmt) = newprno;
		IR_dt(stmt) = newprdt;
		return stmt;
	case IR_CALL:
	case IR_ICALL:
		IS_TRUE0(CALL_prno(stmt) == oldprno);
		CALL_prno(stmt) = newprno;
		IR_dt(stmt) = newprdt;
		return stmt;
	default: IS_TRUE0(0);
	}
	return NULL;
}


//Strip specified VP's version.
void IR_SSA_MGR::strip_specified_vp(VP * vp)
{
	IR * def = SSA_def(vp);
	if (def == NULL) { return; }

	IS_TRUE0(VP_ver(vp) != 0);

	IR * res = def->get_res_pr(VP_prno(vp));
	IS_TRUE(res, ("Stmt does not modified PR%d", VP_prno(vp)));

	UINT newprdt = IR_dt(res);
	UINT newprno = m_ru->build_prno(newprdt);

	//newpr->copy_ref(res, m_ru);
	if (m_ru->get_du_mgr() != NULL) {
		m_ru->get_du_mgr()->remove_def_out_from_useset(res);
	}

	IR * replaced_one = replace_res_pr(def, VP_prno(vp), newprno, newprdt);
	IS_TRUE0(replaced_one);

	MD const* md = m_ru->gen_md_for_pr(newprno, newprdt);
	replaced_one->set_ref_md(md, m_ru);
	replaced_one->clean_ref_mds();

	SSAUSE_ITER vit;
	for (INT i = SSA_uses(vp).get_first(&vit);
		 vit != NULL; i = SSA_uses(vp).get_next(i, &vit)) {
		IR * use = m_ru->get_ir(i);
		IS_TRUE0(use->is_pr());
		if (m_ru->get_du_mgr() != NULL) {
			m_ru->get_du_mgr()->remove_use_out_from_defset(use);
		}

		PR_no(use) = newprno;
		//Keep the data type of reference unchanged.
		//IR_dt(use) = newprdt;
		use->set_ref_md(md, m_ru);
	}

	//Set VP prno to the new prno to avoid verify_vp() assert.
	//However, checking VP_prno after strip_version, I think, is dispensable.
	VP_prno(vp) = newprno;
}


void IR_SSA_MGR::strip_stmt_version(IR * stmt, BITSET & visited)
{
	IS_TRUE0(stmt->is_stmt());

	if (stmt->is_write_pr() || stmt->is_call_has_retval()) {
		VP * vp = (VP*)stmt->get_ssainfo();
		IS_TRUE0(vp);

		if (!visited.is_contain(SSA_id(vp))) {
			IS_TRUE0(VP_ver(vp) != 0);

			//Avoid restriping again.
			visited.bunion(SSA_id(vp));

			strip_specified_vp(vp);
		}
	}

	//Process operand.
	m_iter.clean();
	for (IR const* k = ir_iter_rhs_init(stmt, m_iter);
		 k != NULL; k = ir_iter_rhs_next(m_iter)) {
		if (!k->is_pr()) { continue; }

		VP * vp = (VP*)k->get_ssainfo();
		IS_TRUE0(vp);

		if (!visited.is_contain(SSA_id(vp))) {
			//Version may be zero if there is not any DEF for k.
			//IS_TRUE0(VP_ver(vp) != 0);

			//Avoid restriping again.
			visited.bunion(SSA_id(vp));

			strip_specified_vp(vp);
		}
	}
}


/* Do striping for all VP recorded.

We do not try to strip version for all VP, because the information of VP
during striping will not be maintained and the relationship between
VP_prno and the concrete occurrence PR may be invalid and
that making the process assert. */
void IR_SSA_MGR::strip_version_for_all_vp()
{
	for (INT i = 1; i <= m_vp_vec.get_last_idx(); i++) {
		VP * v = m_vp_vec.get(i);
		IS_TRUE0(v != NULL);
		strip_specified_vp(v);
	}
}


//Construct DU chain which need by IR_DU_MGR.
//This function will build the DU_SET for PHI and its USE.
void IR_SSA_MGR::construct_du_chain()
{
	for (INT i = 1; i <= m_vp_vec.get_last_idx(); i++) {
		VP * v = m_vp_vec.get(i);
		IS_TRUE0(v != NULL);
		IR * def = SSA_def(v);
		if (def == NULL) { continue; }

		IS_TRUE0(def->is_stmt());

		SSAUSE_ITER vit;
		for (INT i = SSA_uses(v).get_first(&vit);
			 vit != NULL; i = SSA_uses(v).get_next(i, &vit)) {
			IR * use = m_ru->get_ir(i);
			IS_TRUE0(use->is_pr());
			IS_TRUE0(def->is_exact_def(use->get_ref_md()));
			m_ru->get_du_mgr()->build_du_chain(def, use);
		}
	}
}


//Do optimizations in SSA mode.
bool IR_SSA_MGR::perform_ssa_opt(IN OUT OPT_CTX & oc)
{
	LIST<IR_OPT*> opt_list;
	//opt_list.append_tail(m_pm->register_opt(OPT_CCP));

	bool change;
	bool do_ssa_opt = false;
	UINT count = 0;
	do {
		count++;
		change = false;
		for (IR_OPT * opt = opt_list.get_head();
			 opt != NULL; opt = opt_list.get_next()) {
			CHAR const* optn = opt->get_opt_name();
			IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));
			if (!opt->perform(oc)) {
				continue;
			}
			do_ssa_opt = true;
			change = true;
			IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));
			count++;
		}
	} while (change && count < 20);
	IS_TRUE0(!change);
	return do_ssa_opt;
}



void IR_SSA_MGR::construction(OPT_CTX & oc, REGION * ru)
{
	reinit(ru, OPTC_pass_mgr(oc));
	m_ru->check_valid_and_recompute(&oc, OPT_DOM, OPT_UNDEF);

	//Extract dominate tree of CFG.
	START_TIMERS("SSA: Extract Dom Tree", t4);
	DOM_TREE domtree;
	m_cfg->get_dom_tree(domtree);
	END_TIMERS(t4);

	construction(oc, domtree);

	m_is_ssa_construct = true;
}


void IR_SSA_MGR::construction(OPT_CTX & oc, DOM_TREE & domtree)
{
	IS_TRUE0(m_ru);

	START_TIMERS("SSA: DF Manager", t1);
	DF_MGR dfm(this);
	build_df(dfm);
	//dfm.dump((DGRAPH&)*m_cfg);
	END_TIMERS(t1);

	LIST<IR_BB*> wl;
	SDBITSET_MGR sm;
	SBITSET effect_prs(sm.get_seg_mgr());
	SVECTOR<SBITSET*> defed_prs_vec;

	place_phi(oc, dfm, effect_prs, sm, defed_prs_vec, wl);

	rename(effect_prs, defed_prs_vec, domtree);

	IS_TRUE0(verify_phi_sanity(true) && verify_vp_prno());

	refine_phi(wl);

	//Clean version stack after renaming.
	clean_prno2stack();

	//Recompute the map if ssa needs reconstruct.
	m_prno2ir.clean();

	//dump();

	IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));

	strip_version_for_bblist();

	IS_TRUE0(verify_phi_sanity(false) && verify_ssainfo());

	if (m_ru->get_du_mgr() != NULL) {
		IS_TRUE0(m_ru->get_du_mgr()->verify_du_ref());
	}

	if (m_need_maintain_du_chain && m_ru->get_du_mgr() != NULL) {
		IS_TRUE0(m_ru->get_du_mgr()->verify_du_chain());
	}

	m_is_ssa_construct = true;
}


/* Process region in ssa mode, include steps:
1. Transform to ssa mode.
2. Perform optimizations only for PR.
3. Transform to nonssa mode. */
bool IR_SSA_MGR::perform_in_ssa_mode(OPT_CTX & oc, REGION * ru)
{
	construction(oc, ru);

	//dump_ssa_graph();
	//dump_all_vp(true);
	bool change = perform_ssa_opt(oc);

	if (m_need_maintain_du_chain) {
		IS_TRUE0(m_ru->get_du_mgr());
		construct_du_chain();
	}

	#ifdef TRAVERSE_IN_DOM_TREE_ORDER
	DOM_TREE domtree;
	m_ru->check_valid_and_recompute(&oc, OPT_DOM, OPT_UNDEF);
	domtree.erase();
	m_cfg->get_dom_tree(domtree);
	destruction(domtree);
	#else
	destruction_in_bblist_order();
	#endif

	if (change) {
		OPTC_is_expr_tab_valid(oc) = false; //New PR generated.
		OPTC_is_aa_valid(oc) = false; //Point To info changed.
	}

	//DU chain for IR_DU_MGR should be maintained at destruction.
	//DU ref must be maintained during each optimizations.
	IS_TRUE0(m_ru->get_du_mgr()->verify_du_ref());
	IS_TRUE0(m_ru->get_du_mgr()->verify_du_chain());
	IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));
	return change;
}
//END IR_SSA_MGR
