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
#include "callg.h"

namespace xoc {

//
//START CallGraph
//
void CallGraph::computeEntryList(List<CALL_NODE*> & elst)
{
	elst.clean();
	INT c;
	for (Vertex * v = get_first_vertex(c);
		 v != NULL; v = get_next_vertex(c)) {
		if (VERTEX_in_list(v) == NULL) {
			CALL_NODE * cn = m_cnid2cn_map.get(VERTEX_id(v));
			ASSERT0(cn != NULL);
			elst.append_tail(cn);
		}
	}
}


void CallGraph::computeExitList(List<CALL_NODE*> & elst)
{
	elst.clean();
	INT c;
	for (Vertex * v = get_first_vertex(c);
		 v != NULL; v = get_next_vertex(c)) {
		if (VERTEX_out_list(v) == NULL) {
			CALL_NODE * cn = m_cnid2cn_map.get(VERTEX_id(v));
			ASSERT0(cn != NULL);
			elst.append_tail(cn);
		}
	}
}


void CallGraph::dump_vcg(CHAR const* name, INT flag)
{
	if (name == NULL) {
		name = "graph_call_graph.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	ASSERT(h != NULL, ("%s create failed!!!",name));

	//Print comment
	fprintf(h, "\n/*");
	FILE * old = g_tfile;
	g_tfile = h;
	//....
	g_tfile = old;
	fprintf(h, "\n*/\n");

	//Print graph structure description.
	fprintf(h, "graph: {"
			  "title: \"Graph\"\n"
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

	//Print node
	old = g_tfile;
	g_tfile = h;
	INT c;
	for (Vertex * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		INT id = VERTEX_id(v);
		CALL_NODE * cn = m_cnid2cn_map.get(id);
		ASSERT0(cn != NULL);
		fprintf(h, "\nnode: { title:\"%d\" shape:box color:gold "
				   "fontname:\"courB\" label:\"", id);
		fprintf(h, "FUNC(%d):%s\n", CN_id(cn), SYM_name(CN_sym(cn)));

		//
		fprintf(h, "\n");
		if (HAVE_FLAG(flag, CALLG_DUMP_IR) && CN_ru(cn) != NULL) {
			g_indent = 0;
			IR * irs = CN_ru(cn)->get_ir_list();
			for (; irs != NULL; irs = IR_next(irs)) {
				//fprintf(h, "%s\n", dump_ir_buf(ir, buf));
				//TODO: implement dump_ir_buf();
				dump_ir(irs, m_dm, NULL, true, false);
			}
		}
		//

		fprintf(h, "\"}");
	}

	//Print edge
	for (Edge * e = m_edges.get_first(c); e != NULL; e = m_edges.get_next(c)) {
		Vertex * from = EDGE_from(e);
		Vertex * to = EDGE_to(e);
		fprintf(h, "\nedge: { sourcename:\"%d\" targetname:\"%d\" %s}",
				VERTEX_id(from), VERTEX_id(to),  "");
	}
	g_tfile = old;
	fprintf(h, "\n}\n");
	fclose(h);

}


//Insure CALL_NODE for function is unique.
//Do NOT modify ir' content.
CALL_NODE * CallGraph::newCallNode(IR * ir)
{
	ASSERT0(ir->is_call());
	SYM * name = VAR_name(CALL_idinfo(ir));
	CALL_NODE * cn  = m_sym2cn_map.get(name);
	if (cn != NULL) return cn;

	cn = newCallNode();
	CN_sym(cn) = name;
	CN_id(cn) = m_cn_count++;
	m_sym2cn_map.set(name, cn);
	return cn;
}


//To guarantee CALL_NODE of function is unique.
CALL_NODE * CallGraph::newCallNode(Region * ru)
{
	ASSERT0(REGION_type(ru) == RU_FUNC);
	SYM * name = VAR_name(ru->get_ru_var());
	CALL_NODE * cn = m_sym2cn_map.get(name);
	if (cn != NULL) {
		if (CN_ru(cn) == NULL) {
			CN_ru(cn) = ru;
			m_ruid2cn_map.set(REGION_id(ru), cn);
		}
		ASSERT(CN_ru(cn) == ru, ("more than 2 ru with same name"));
		return cn;
	}

	cn = newCallNode();
	CN_sym(cn) = name;
	CN_id(cn) = m_cn_count++;
	CN_ru(cn) = ru;
	m_sym2cn_map.set(name, cn);
	m_ruid2cn_map.set(REGION_id(ru), cn);
	return cn;
}


//Build call graph.
void CallGraph::build(Region * top)
{
	ASSERT0(REGION_type(top) == RU_PROGRAM);
	IR * irs = top->get_ir_list();
	List<CALL_NODE*> ic;
	while (irs != NULL) {
		if (irs->is_region()) {
			Region * ru = REGION_ru(irs);
			ASSERT0(ru && REGION_type(ru) == RU_FUNC);
			CALL_NODE * cn = newCallNode(ru);
			add_node(cn);
			List<IR const*> * call_list = ru->get_call_list();
			if (call_list->get_elem_count() == 0) {
				irs = IR_next(irs);
				continue;
			}

			C<IR const*> * ct;
			for (call_list->get_head(&ct);
				 ct != NULL; ct = call_list->get_next(ct)) {
				IR const* ir = ct->val();
				ASSERT0(ir && ir->is_calls_stmt());

				if (ir->is_call()) {
					//Direct call.
					CALL_NODE * cn2 = newCallNode(const_cast<IR*>(ir));
					add_node(cn2);
					addEdge(CN_id(cn), CN_id(cn2));
				} else {
					//Indirect call.
					CALL_NODE * cn3 = newCallNode(const_cast<IR*>(ir));
					ic.append_tail(cn3);
					add_node(cn3);
				}
			}
		}
		irs = IR_next(irs);
	}

	C<CALL_NODE*> * ct;
	for (ic.get_head(&ct); ct != NULL; ct = ic.get_next(ct)) {
		CALL_NODE * i = ct->val();
		ASSERT0(i);
		INT c;
		for (Vertex * v = get_first_vertex(c);
			 v != NULL; v = get_next_vertex(c)) {
			CALL_NODE * j = map_id2cn(VERTEX_id(v));
			ASSERT0(j);
			addEdge(CN_id(i), CN_id(j));
		}
	}
	//dump_vcg(m_ru_mgr->get_dt_mgr());
	set_bs_mgr(m_ru_mgr->get_bs_mgr());
}
//END CallGraph

} //namespace xoc
