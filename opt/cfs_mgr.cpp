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
#include "cfs_mgr.h"

//
//START CFS_MGR
//
CFS_INFO * CFS_MGR::new_cfs_info(IR_TYPE irtype)
{
	CFS_INFO * ci = (CFS_INFO*)xmalloc(sizeof(CFS_INFO));
	CFS_INFO_cfs_type(ci) = irtype;
	switch (irtype) {
	case IR_IF:
		{
			CFS_INFO_true_body(ci) = m_bs_mgr.create();
			CFS_INFO_false_body(ci) = m_bs_mgr.create();
		}
		break;
	case IR_DO_LOOP:
	case IR_WHILE_DO:
	case IR_DO_WHILE:
		CFS_INFO_loop_body(ci) = m_bs_mgr.create();
		break;
	default:
		IS_TRUE0(0);
	}
	return ci;
}


void CFS_MGR::set_map_ir2cfsinfo(IR * ir, CFS_INFO * ci)
{
	m_map_ir2cfsinfo.set(IR_id(ir), ci);
}


CFS_INFO * CFS_MGR::map_ir2cfsinfo(IR * ir)
{
	return m_map_ir2cfsinfo.get(IR_id(ir));
}


//Record IR list into 'irset'.
void CFS_MGR::record_ir_stmt(IR * ir_list, BITSET & irset)
{
	while (ir_list != NULL) {
		irset.bunion(IR_id(ir_list));
		ir_list = IR_next(ir_list);
	}
}


ABS_NODE * CFS_MGR::new_abs_node(ABS_TYPE ty)
{
	ABS_NODE * a = (ABS_NODE*)xmalloc(sizeof(ABS_NODE));
	ABS_NODE_type(a) = ty;
	return a;
}


void CFS_MGR::dump_indent(UINT indent)
{
	while (indent != 0) {
		fprintf(g_tfile, " ");
		indent--;
	}
}


void CFS_MGR::dump_abs_tree(ABS_NODE * an)
{
	fprintf(g_tfile, "\n##############\nDUMP ABS_NODE TREE\n##############\n");
	dump_abs_tree(an, 0);
}


void CFS_MGR::dump_abs_tree(ABS_NODE * an, UINT indent)
{
	while (an != NULL) {
		switch (ABS_NODE_type(an)) {
		case ABS_BB:
			fprintf(g_tfile, "\n"); dump_indent(indent);
			fprintf(g_tfile, "BB%d", IR_BB_id(ABS_NODE_bb(an)));
			break;
		case ABS_LOOP:
			fprintf(g_tfile, "\n"); dump_indent(indent);
			fprintf(g_tfile, "LOOP: HEAD=BB%d", IR_BB_id(ABS_NODE_loop_head(an)));
			dump_abs_tree(ABS_NODE_loop_body(an), indent + 4);
			break;
		case ABS_IF:
			fprintf(g_tfile, "\n"); dump_indent(indent);
			fprintf(g_tfile, "IF: HEAD=BB%d", IR_BB_id(ABS_NODE_if_head(an)));
			if (ABS_NODE_true_body(an) != NULL) {
				fprintf(g_tfile, "\n"); dump_indent(indent);
				fprintf(g_tfile, "TRUE_BODY:");
				dump_abs_tree(ABS_NODE_true_body(an), indent + 4);
			}
			if (ABS_NODE_false_body(an) != NULL) {
				fprintf(g_tfile, "\n"); dump_indent(indent);
				fprintf(g_tfile, "FALSE_BODY:");
				dump_abs_tree(ABS_NODE_false_body(an), indent + 4);
			}
			break;
		}
		an = ABS_NODE_next(an);
	}
}


ABS_NODE * CFS_MGR::map_bb2abs(IR_BB const* bb)
{
	return m_map_bb2abs.get(IR_BB_id(bb));
}


void CFS_MGR::set_map_bb2abs(IR_BB const* bb, ABS_NODE * abs)
{
	m_map_bb2abs.set(IR_BB_id(bb), abs);
}


ABS_NODE * CFS_MGR::construct_abs_loop(
						IN IR_BB * entry,
						IN ABS_NODE * parent,
						IN BITSET * cur_region,
						IN GRAPH & cur_graph,
						IN OUT BITSET & visited)
{
	IS_TRUE0(cur_region == NULL || cur_region->is_contain(IR_BB_id(entry)));
	IR_CFG * cfg = m_ru->get_cfg();
	LI<IR_BB> * li = cfg->map_bb2li(entry);
	IS_TRUE0(li != NULL && LI_loop_head(li) == entry);

	ABS_NODE * node = new_abs_node(ABS_LOOP);
	set_map_bb2abs(entry, node);
	ABS_NODE_parent(node) = parent;
	ABS_NODE_loop_head(node) = entry;
	IR_BB * body_start;
	cfg->get_loop_two_kids(entry, NULL, &body_start);
	IS_TRUE0(body_start != NULL);

	CFS_INFO * ci = map_ir2cfsinfo(cfg->get_last_xr(entry));
	IS_TRUE0(ci != NULL && CFS_INFO_head(ci) == entry);

	IS_TRUE0(CFS_INFO_loop_body(ci)->is_contain(*LI_bb_set(li)));
	BITSET loc_visited;
	ABS_NODE_loop_body(node) = construct_abs_tree(body_start, node,
												  LI_bb_set(li),
												  cur_graph, loc_visited);
	visited.bunion(loc_visited);
	visited.bunion(IR_BB_id(entry));
	return node;
}


//'cur_region' covered 'entry'.
ABS_NODE * CFS_MGR::construct_abs_if(
						IN IR_BB * entry,
						IN ABS_NODE * parent,
						IN GRAPH & cur_graph,
						IN OUT BITSET & visited)
{
	ABS_NODE * node = new_abs_node(ABS_IF);
	set_map_bb2abs(entry, node);
	ABS_NODE_parent(node) = parent;
	ABS_NODE_if_head(node) = entry;

	IR_BB * true_body, * false_body;
	IR_CFG * cfg = m_ru->get_cfg();
	cfg->get_if_three_kids(entry, &true_body, &false_body, NULL);
	CFS_INFO * ci = map_ir2cfsinfo(cfg->get_last_xr(entry));
	IS_TRUE0(ci != NULL && CFS_INFO_head(ci) == entry);

	BITSET loc_visited;
	ABS_NODE_true_body(node) = construct_abs_tree(true_body, node, CFS_INFO_true_body(ci), cur_graph, loc_visited);
	visited.bunion(loc_visited);
	loc_visited.clean();
	ABS_NODE_false_body(node) = construct_abs_tree(false_body, node, CFS_INFO_false_body(ci), cur_graph, loc_visited);
	visited.bunion(loc_visited);
	visited.bunion(IR_BB_id(entry));
	return node;
}


ABS_NODE * CFS_MGR::construct_abs_bb(IN IR_BB * bb, IN ABS_NODE * parent)
{
	ABS_NODE * node = new_abs_node(ABS_BB);
	set_map_bb2abs(bb, node);
	ABS_NODE_parent(node) = parent;
	ABS_NODE_bb(node) = bb;
	return node;
}


ABS_NODE * CFS_MGR::construct_abs_tree(
						IN IR_BB * entry,
						IN ABS_NODE * parent,
						IN BITSET * cur_region,
						IN GRAPH & cur_graph,
						IN OUT BITSET & visited)
{
	IR_CFG * cfg = m_ru->get_cfg();
	ABS_NODE * lst = NULL;
	IR_BB * bb = entry;
	GRAPH g;
	g.clone(cur_graph);
	VERTEX * next = NULL;
	VERTEX * v;
	if (cur_region != NULL) {
		if (cur_region->get_elem_count() == 0) {
			visited.clean();
			return NULL;
		}
		INT c;
		for (v = g.get_first_vertex(c); v != NULL; v = next) {
			next = g.get_next_vertex(c);
			if (cur_region->is_contain(VERTEX_id(v))) {
				continue;
			}
			g.remove_vertex(v);
		}
	}
	BITSET loc_visited;
	while (bb != NULL &&
		   (cur_region == NULL ||
		    cur_region->is_contain(IR_BB_id(bb)))) {
		ABS_NODE * node = NULL;
		loc_visited.clean();
		LI<IR_BB> * li = cfg->map_bb2li(bb);
		if (li != NULL) {
			node = construct_abs_loop(bb, parent, LI_bb_set(li),
									  g, loc_visited);
		} else {
			IR * last_xr = cfg->get_last_xr(bb);
			if (last_xr != NULL && //'bb' is branching node of IF.
				last_xr->is_cond_br()) {
				IS_TRUE0(map_ir2cfsinfo(last_xr) != NULL);

				/* There might not exist ipdom.
				e.g:
					if (x) //BB1
						return 1;
					return 2;

					BB1 does not have a ipdom.
				*/
				UINT ipdom = ((DGRAPH*)cfg)->get_ipdom(IR_BB_id(bb));
				IS_TRUE(ipdom > 0, ("bb does not have ipdom"));
				node = construct_abs_if(bb, parent, g, loc_visited);
			} else {
				node = construct_abs_bb(bb, parent);
				loc_visited.bunion(IR_BB_id(bb));
			}
		}
		insertbefore_one(&lst, lst, node);

		visited.bunion(loc_visited);
		//Remove visited vertex.
		next = NULL;
		INT c;
		for (v = g.get_first_vertex(c); v != NULL; v = next) {
			next = g.get_next_vertex(c);
			if (!loc_visited.is_contain(VERTEX_id(v))) {
				continue;
			}
			g.remove_vertex(v);
		}

		IR_BB * cand = NULL;
		for (v = g.get_first_vertex(c); v != NULL; v = g.get_next_vertex(c)) {
			if (g.get_in_degree(v) == 0) {
				IS_TRUE(cand == NULL, ("multiple immediate-post-dominators"));
				cand = cfg->get_bb(VERTEX_id(v));
			}
		}

		if (cand == NULL) {
			//Cannot find leading BB, there might be exist cycle in graph.
			bb = cfg->get_ipdom(bb);
		} else {
			bb = cand;
		}

		if (parent != NULL && bb == ABS_NODE_bb(parent)) {
			//Here control-flow is cyclic.
			break;
		}
	}
	lst = reverse_list(lst);
	return lst;
}


//Construct Control Flow Structure.
ABS_NODE * CFS_MGR::construct_abstract_cfs()
{
	IR_CFG * cfg = m_ru->get_cfg();
	IR_BB * entry = cfg->get_entry_list()->get_head();
	IS_TRUE(cfg->get_entry_list()->get_elem_count() == 1, ("CFG should be single-entry"));
	BITSET visited;
	ABS_NODE * a = construct_abs_tree(entry, NULL, NULL, *(GRAPH*)cfg, visited);
	//dump_abs_tree(a);
	return a;
}
//END CFS_MGR
