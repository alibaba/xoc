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
#include "ltype.h"
#include "comf.h"
#include "smempool.h"
#include "sstl.h"
#include "bs.h"
#include "sgraph.h"

//Expect for unique vertex in graphic depitction
#define ALWAYS_VERTEX_UNIQUE

//
//START EDGE_HASH
//
EDGE * EDGE_HASH::create(OBJTY v)
{
	EDGE * t = (EDGE*)v;
	VERTEX * from = m_g->get_vertex(VERTEX_id(EDGE_from(t)));
	VERTEX * to = m_g->get_vertex(VERTEX_id(EDGE_to(t)));
	IS_TRUE0(from && to);
	t = m_g->new_edge_impl(from, to);
	return t;
}
//END EDGE_HASH


//DONT CALL Constructor directly.
GRAPH::GRAPH(UINT edge_hash_size, UINT vex_hash_size) :
	m_edges(edge_hash_size), m_vertices(vex_hash_size)
{
	m_edge_hash_size = edge_hash_size;
	m_vex_hash_size = vex_hash_size;
	m_ec_pool = NULL;
	init();
}


GRAPH::GRAPH(GRAPH & g) :
	m_edges(g.m_edge_hash_size), m_vertices(g.m_vex_hash_size)
{
	m_edge_hash_size = g.m_edge_hash_size;
	m_vex_hash_size = g.m_vex_hash_size;
	m_ec_pool = NULL;
	init();
	clone(g);
}


void GRAPH::init()
{
	if (m_ec_pool != NULL) { return; }

	m_ec_pool = smpool_create_handle(sizeof(EDGE_C), MEM_CONST_SIZE);
	IS_TRUE(m_ec_pool, ("create mem pool failed"));

	m_vertex_pool = smpool_create_handle(sizeof(VERTEX) * 4, MEM_CONST_SIZE);
	IS_TRUE(m_vertex_pool, ("create mem pool failed"));

	m_edge_pool = smpool_create_handle(sizeof(EDGE) * 4, MEM_CONST_SIZE);
	IS_TRUE(m_edge_pool, ("create mem pool failed"));

	//If m_edges already initialized, this call will do nothing.
	m_edges.init(this, m_edge_hash_size);

	//If m_vertices already initialized, this call will do nothing.
	m_vertices.init(m_vex_hash_size);
	m_is_unique = true;
	m_is_direction = true;
	m_dense_vertex = NULL;
}


/* Reconstruct vertex hash table, and edge hash table with new bucket size.
'vertex_hash_sz': new vertex table size to be resized.
'edge_hash_sz': new edge table size to be resized.
NOTE: mem pool should have been initialized. */
void GRAPH::resize(UINT vertex_hash_sz, UINT edge_hash_sz)
{
	IS_TRUE0(m_ec_pool);
	IS_TRUE(m_vertices.get_elem_count() == 0, ("graph is not empty"));
	IS_TRUE(m_edges.get_elem_count() == 0, ("graph is not empty"));

	if (m_vex_hash_size != vertex_hash_sz) {
		m_vertices.destroy();
		m_vertices.init(vertex_hash_sz);
		m_vex_hash_size = vertex_hash_sz;
	}

	if (m_edge_hash_size != edge_hash_sz) {
		m_edges.destroy();
		m_edges.init(this, edge_hash_sz);
		m_edge_hash_size = edge_hash_sz;
	}
}


UINT GRAPH::count_mem() const
{
	UINT count = 0;
	count += sizeof(BYTE);
	count += sizeof(m_edge_hash_size);
	count += sizeof(m_vex_hash_size);
	count += m_edges.count_mem();
	count += m_vertices.count_mem();
	count += m_e_free_list.count_mem();
	count += m_el_free_list.count_mem();
	count += m_v_free_list.count_mem();
	count += smpool_get_pool_size_handle(m_ec_pool);
	count += smpool_get_pool_size_handle(m_vertex_pool);
	count += smpool_get_pool_size_handle(m_edge_pool);
	if (m_dense_vertex != NULL) {
		count += m_dense_vertex->count_mem();
	}
	return count;
}


void GRAPH::destroy()
{
	if (m_ec_pool == NULL) return;
	m_edges.destroy();
	m_vertices.destroy();

	//Set if edge and vertex would not be redundantly.
	m_is_unique = false;
	m_is_direction = false; //Set if graph is direction.
	m_e_free_list.clean(); //edge free list
	m_el_free_list.clean(); //edge-list free list
	m_v_free_list.clean(); //vertex free list

	smpool_free_handle(m_ec_pool);
	m_ec_pool = NULL;

	smpool_free_handle(m_vertex_pool);
	m_vertex_pool = NULL;

	smpool_free_handle(m_edge_pool);
	m_edge_pool = NULL;

	if (m_dense_vertex != NULL) {
		delete m_dense_vertex;
		m_dense_vertex = NULL;
	}
}


//Erasing graph, include all nodes and edges,
//except for mempool, freelist.
void GRAPH::erase()
{
	IS_TRUE(m_ec_pool != NULL, ("Graph must be initialized before clone."));
	//Collect edge, vertex data structure into free-list
	//for allocation on demand.
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		 remove_vertex(v);
	}
	#ifdef _DEBUG_
	if (m_dense_vertex != NULL) {
		for (INT i = 0; i <= m_dense_vertex->get_last_idx(); i++) {
			//Should be removed in remove_vertex().
			IS_TRUE0(m_dense_vertex->get(i) == NULL);
		}
	}
	#endif
}


/* Sort vertice by rporder, and update rpo of vertex.
Record sorted vertex into vlst in incremental order of rpo.
NOTE: rpo start at 1, and 0 means undefined. */
void GRAPH::compute_rpo_norec(VERTEX * root, OUT LIST<VERTEX const*> & vlst)
{
	IS_TRUE0(root && is_graph_entry(root));
	BITSET is_visited;
	SSTACK<VERTEX*> stk;
	stk.push(root);
	VERTEX * v;
	UINT order = get_vertex_num();
	while ((v = stk.get_top()) != NULL) {
		is_visited.bunion(VERTEX_id(v));
		EDGE_C * el = VERTEX_out_list(v);
		bool find = false; //find unvisited kid.
		while (el != NULL) {
			VERTEX * succ = EDGE_to(EC_edge(el));
			if (!is_visited.is_contain(VERTEX_id(succ))) {
				stk.push(succ);
				find = true;
				break;
			}
			el = EC_next(el);
		}
		if (!find) {
			stk.pop();
			vlst.append_head(v);
			VERTEX_rpo(v) = order--;
		}
	}
	IS_TRUE0(order == 0);
}


bool GRAPH::clone(GRAPH & src)
{
	erase();
	m_is_unique = src.m_is_unique;
	m_is_direction = src.m_is_direction;

	//Clone vertices
	INT c;
	for (VERTEX * srcv = src.get_first_vertex(c);
		 srcv != NULL; srcv = src.get_next_vertex(c)) {
		VERTEX * v = add_vertex(VERTEX_id(srcv));

		/*
		Calls inherited class method.
		Vertex info of memory should allocated by inherited class method
		*/
		if (VERTEX_info(srcv) != NULL) {
			VERTEX_info(v) = clone_vertex_info(srcv);
		}
	}

	//Clone edges
	for (EDGE * srce = src.get_first_edge(c);
		 srce != NULL; srce = src.get_next_edge(c)) {
		EDGE * e = add_edge(VERTEX_id(EDGE_from(srce)),
							VERTEX_id(EDGE_to(srce)));

		/*
		Calls inherited class method.
		Edge info of memory should allocated by inherited class method
		*/
		if (EDGE_info(srce) != NULL) {
			EDGE_info(e) = clone_edge_info(srce);
		}
	}
	return true;
}


//NOTE: Do NOT use 0 as vertex id.
VERTEX * GRAPH::new_vertex(UINT vid)
{
	IS_TRUE(m_vertex_pool, ("not yet initialized."));
	IS_TRUE(vid != 0, ("Do not use 0 as vertex id"));

	VERTEX * vex = NULL;

	#ifndef ALWAYS_VERTEX_UNIQUE
	if (m_is_unique)
	#endif
	{
		VERTEX * v = m_vertices.find((OBJTY)(size_t)vid);
		if (v != NULL) {
			return v;
		}
	}

	vex = m_v_free_list.get_free_elem();
	if (vex == NULL) {
		vex = new_vertex();
	}
	VERTEX_id(vex) = vid;

	if (m_dense_vertex != NULL) {
		IS_TRUE0(m_dense_vertex->get(vid) == NULL);
		m_dense_vertex->set(vid, vex);
	}
	return vex;
}


EDGE * GRAPH::new_edge(VERTEX * from, VERTEX * to)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (from == NULL || to == NULL) return NULL;
	EDGE teste;
	VERTEX testfrom, testto;
	if (m_is_unique) {
		VERTEX_id(&testfrom) = VERTEX_id(from);
		VERTEX_id(&testto) = VERTEX_id(to);
		EDGE_from(&teste) = &testfrom;
		EDGE_to(&teste) = &testto;
		if (m_is_direction) {
			return m_edges.append((OBJTY)&teste, NULL);
		}

		EDGE * e = NULL;
		if (m_edges.find(&teste, &e)) {
			IS_TRUE0(e);
			return e;
		}

		//Both check from->to and to->from
		EDGE_from(&teste) = &testto;
		EDGE_to(&teste) = &testfrom;
		return m_edges.append((OBJTY)&teste, NULL);
	}
	return m_edges.append(new_edge_impl(from, to));
}


//Reverse edge direction
EDGE * GRAPH::rev_edge(EDGE * e)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	IS_TRUE(m_is_direction, ("graph is indirection"));
	void * einfo = EDGE_info(e);
	VERTEX * from = EDGE_from(e);
	VERTEX * to = EDGE_to(e);
	remove_edge(e);
	e = add_edge(VERTEX_id(to), VERTEX_id(from));
	EDGE_info(e) = einfo;
	return e;
}


//Reverse all edge direction
void GRAPH::rev_edges()
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	IS_TRUE(m_is_direction, ("graph is indirection"));
	LIST<EDGE*> list;
	EDGE * e;
	INT c;
	for (e = get_first_edge(c); e != NULL; e = get_next_edge(c)) {
		list.append_tail(e);
	}
	for (e = list.get_head(); e != NULL; e = list.get_next()) {
		rev_edge(e);
	}
}


/*
Insert 'newv' between 'v1' and 'v2'.
e.g: given edge v1->v2, the result is v1->newv->v2.
Return edge v1->newv, newv->v2.
*/
void GRAPH::insert_vertex_between(IN VERTEX * v1, IN VERTEX * v2,
								  IN VERTEX * newv, OUT EDGE ** e1,
								  OUT EDGE ** e2)
{
	EDGE * e = get_edge(v1, v2);
	remove_edge(e);
	EDGE * tmpe1 = add_edge(v1, newv);
	EDGE * tmpe2 = add_edge(newv, v2);
	if (e1 != NULL) *e1 = tmpe1;
	if (e2 != NULL) *e2 = tmpe2;
}


/*
Insert 'newv' between 'v1' and 'v2'.
e.g: given edge v1->v2, the result is v1->newv->v2.
Return edge v1->newv, newv->v2.

NOTICE: newv must be node in graph.
*/
void GRAPH::insert_vertex_between(UINT v1, UINT v2, UINT newv,
								OUT EDGE ** e1, OUT EDGE ** e2)
{
	VERTEX * pv1 = get_vertex(v1);
	VERTEX * pv2 = get_vertex(v2);
	VERTEX * pnewv = get_vertex(newv);
	IS_TRUE0(pv1 != NULL && pv2 != NULL && pnewv != NULL);
	insert_vertex_between(pv1, pv2, pnewv, e1, e2);
}


EDGE * GRAPH::remove_edge(EDGE * e)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (e == NULL) return NULL;
	VERTEX * from = EDGE_from(e);
	VERTEX * to = EDGE_to(e);

	//remove out of out-list of 'from'
	EDGE_C * el = VERTEX_out_list(from);
	while (el != NULL) {
		if (EC_edge(el) == e) {	break; }
		el = EC_next(el);
	}
	IS_TRUE(el != NULL, ("can not find out-edge, it is illegal graph"));
	remove(&VERTEX_out_list(from), el);
	m_el_free_list.add_free_elem(el);

	//remove out of in-list of 'to'
	el = VERTEX_in_list(to);
	while (el != NULL) {
		if (EC_edge(el) == e) break;
		el = EC_next(el);
	}
	IS_TRUE(el != NULL, ("can not find in-edge, it is illegal graph"));
	remove(&VERTEX_in_list(to), el);
	m_el_free_list.add_free_elem(el);

	//remove edge out of edge-hash
	e = m_edges.removed(e);
	m_e_free_list.add_free_elem(e);
	return e;
}


VERTEX * GRAPH::remove_vertex(VERTEX * vex)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (vex == NULL) return NULL;
	EDGE_C * el = VERTEX_out_list(vex);
	//remove all out-edge
	while (el != NULL) {
		EDGE_C * tmp = el;
		el = EC_next(el);
		remove_edge(EC_edge(tmp));
	}

	//remove all in-edge
	el = VERTEX_in_list(vex);
	while (el != NULL) {
		EDGE_C * tmp = el;
		el = EC_next(el);
		remove_edge(EC_edge(tmp));
	}
	vex = m_vertices.removed(vex);
	m_v_free_list.add_free_elem(vex);
	if (m_dense_vertex != NULL) {
		m_dense_vertex->set(VERTEX_id(vex), NULL);
	}
	return vex;
}


/*
Return all neighbors of 'vid' on graph.
Return false if 'vid' is not on graph.

'ni_list': record the neighbours of 'vid'.
	Note that this function ensure each neighbours in ni_list is unique.
*/
bool GRAPH::get_neighbor_list(OUT LIST<UINT> & ni_list, UINT vid) const
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	UINT degree = 0;

	//Ensure VERTEX_HASH::find is readonly.
	GRAPH * pthis = const_cast<GRAPH*>(this);
	VERTEX * vex  = pthis->get_vertex(vid);
	if (vex == NULL) { return false; }

	EDGE_C * el = VERTEX_in_list(vex);
	while (el != NULL) {
		INT v = VERTEX_id(EDGE_from(EC_edge(el)));
		if (!ni_list.find(v)) {
			ni_list.append_tail(v);
		}
		el = EC_next(el);
	}

	el = VERTEX_out_list(vex);
	while (el != NULL) {
		INT v = VERTEX_id(EDGE_to(EC_edge(el)));
		if (!ni_list.find(v)) {
			ni_list.append_tail(v);
		}
		el = EC_next(el);
	}
	return true;
}


/*
Return all neighbors of 'vid' on graph.
Return false if 'vid' is not on graph.

'niset': record the neighbours of 'vid'.
	Note that this function ensure each neighbours in niset is unique.
	Using sparse bitset is faster than list in most cases.
*/
bool GRAPH::get_neighbor_set(OUT SBITSET & niset, UINT vid) const
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	UINT degree = 0;

	//Ensure VERTEX_HASH::find is readonly.
	GRAPH * pthis = const_cast<GRAPH*>(this);
	VERTEX * vex  = pthis->get_vertex(vid);
	if (vex == NULL) { return false; }

	EDGE_C * el = VERTEX_in_list(vex);
	while (el != NULL) {
		INT v = VERTEX_id(EDGE_from(EC_edge(el)));
		niset.bunion(v);
		el = EC_next(el);
	}

	el = VERTEX_out_list(vex);
	while (el != NULL) {
		INT v = VERTEX_id(EDGE_to(EC_edge(el)));
		niset.bunion(v);
		el = EC_next(el);
	}
	return true;
}


UINT GRAPH::get_degree(VERTEX const* vex) const
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (vex == NULL) return 0;
	return get_in_degree(vex) + get_out_degree(vex);
}


UINT GRAPH::get_in_degree(VERTEX const* vex) const
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (vex == NULL) { return 0; }
	UINT degree = 0;
	EDGE_C * el = VERTEX_in_list(vex);
	while (el != NULL) {
		degree++;
		el = EC_next(el);
	}
	return degree;
}


UINT GRAPH::get_out_degree(VERTEX const* vex) const
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (vex == NULL) { return 0; }

	UINT degree = 0;
	EDGE_C * el = VERTEX_out_list(vex);
	while (el != NULL) {
		degree++;
		el = EC_next(el);
	}
	return degree;
}


EDGE * GRAPH::get_edge(VERTEX const* from, VERTEX const* to)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (from == NULL || to == NULL) { return NULL; }

	EDGE_C * el = VERTEX_out_list(from);
	while (el != NULL) {
		EDGE * e = EC_edge(el);
		if (EDGE_from(e) == from && EDGE_to(e) == to) {
			return e;
		}
		if (!m_is_direction &&
			(EDGE_from(e) == to && EDGE_to(e) == from)) {
			return e;
		}
		el = EC_next(el);
	}

	if (!m_is_direction) {
		EDGE_C * el = VERTEX_out_list(to);
		while (el != NULL) {
			EDGE * e = EC_edge(el);
			if (EDGE_from(e) == to && EDGE_to(e) == from) {
				return e;
			}
			el = EC_next(el);
		}
	}
	return NULL;
}


EDGE * GRAPH::get_edge(UINT from, UINT to)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	VERTEX * fp = get_vertex(from);
	VERTEX * tp = get_vertex(to);
	return get_edge(fp, tp);
}


bool GRAPH::is_equal(GRAPH & g)
{
	if (get_vertex_num() != g.get_vertex_num() ||
		get_edge_num() != g.get_edge_num()) {
		return false;
	}

	BITSET vs;
	INT c;
	for (VERTEX * v1 = get_first_vertex(c);
		 v1 != NULL; v1 = get_next_vertex(c)) {
		VERTEX * v2 = g.get_vertex(VERTEX_id(v1));
		if (v2 == NULL) {
			return false;
		}

		vs.clean();
		EDGE_C * el = VERTEX_out_list(v1);
		EDGE * e = NULL;
		UINT v1_succ_n = 0;
		if (el == NULL) {
			if (VERTEX_out_list(v2) != NULL) {
				return false;
			}
			continue;
		}

		for (e = EC_edge(el); e != NULL; el = EC_next(el),
			 e = el ? EC_edge(el) : NULL) {
			vs.bunion(VERTEX_id(EDGE_to(e)));
			v1_succ_n++;
		}

		UINT v2_succ_n = 0;
		el = VERTEX_out_list(v2);
		for (e = EC_edge(el); e != NULL; el = EC_next(el),
			 e = el ? EC_edge(el) : NULL) {
			v2_succ_n++;
			if (!vs.is_contain(VERTEX_id(EDGE_to(e)))) {
				return false;
			}
		}

		if (v1_succ_n != v2_succ_n) {
			return false;
		}
	}
	return true;
}


//Is there exist a path connect 'from' and 'to'.
bool GRAPH::is_reachable(VERTEX * from, VERTEX * to)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	IS_TRUE(from != NULL && to != NULL, ("parameters cannot be NULL"));
	EDGE_C * el = VERTEX_out_list(from);
	EDGE * e = NULL;
	if (el == NULL) { return false; }

	//Walk through each succ of 'from'
	for (e = EC_edge(el); e != NULL; el = EC_next(el),
		 e = el ? EC_edge(el):NULL) {
		VERTEX * succ = EDGE_to(e);
		if (VERTEX_id(succ) == VERTEX_id(to)) {
			return true;
		} else {
			if (is_reachable(succ, to)) {
				return true;
			}
		}
	} //end for
	return false;
}


/*
Sort graph vertices in topological order.
'vex_vec': record nodes with topological sort.
NOTE: current graph will be empty at function return.
	If one need to keep the graph unchanged, clone graph
	as a tmpgraph and operate on the tmpgraph.
	e.g: GRAPH org;
		And org must be unchanged,
		GRAPH tmp(org);
		tmp.sort_in_toplog_order(...)
*/
bool GRAPH::sort_in_toplog_order(OUT SVECTOR<UINT> & vex_vec, bool is_topdown)
{
	IS_TRUE(m_ec_pool != NULL, ("Graph still not yet initialize."));
	if (get_vertex_num() == 0) {
		return true;
	}
	LIST<VERTEX*> vlst;
	UINT pos = 0;
	vex_vec.clean();
	vex_vec.grow(get_vertex_num());
	while (this->get_vertex_num() != 0) {
		vlst.clean();
		VERTEX * v;
		INT c;
		for (v = this->get_first_vertex(c);
			 v != NULL; v = this->get_next_vertex(c)) {
			if (is_topdown) {
				if (VERTEX_in_list(v) == NULL) {
					vlst.append_tail(v);
				}
			} else if (VERTEX_out_list(v) == NULL) {
				vlst.append_tail(v);
			}
		}
		if (vlst.get_elem_count() == 0 && this->get_vertex_num() != 0) {
			IS_TRUE(0, ("exist cycle in graph"));
			return false;
		}
		for (v = vlst.get_head(); v != NULL; v = vlst.get_next()) {
			vex_vec.set(pos, VERTEX_id(v));
			pos++;
			this->remove_vertex(v);
		}
	}
	return true;
}


//Remove all edges between v1 and v2.
void GRAPH::remove_edges_between(VERTEX * v1, VERTEX * v2)
{
	EDGE_C * ec = VERTEX_out_list(v1);
	while (ec != NULL) {
		EDGE_C * next = EC_next(ec);
		EDGE * e = EC_edge(ec);
		if ((EDGE_from(e) == v1 && EDGE_to(e) == v2) ||
			(EDGE_from(e) == v2 && EDGE_to(e) == v1)) {
			remove_edge(e);
		}
		ec = next;
	}

	ec = VERTEX_in_list(v1);
	while (ec != NULL) {
		EDGE_C * next = EC_next(ec);
		EDGE * e = EC_edge(ec);
		if ((EDGE_from(e) == v1 && EDGE_to(e) == v2) ||
			(EDGE_from(e) == v2 && EDGE_to(e) == v1)) {
			remove_edge(e);
		}
		ec = next;
	}
}


/*
Remove transitive edge.
e.g: Given edges of G, there are v1->v2->v3, v1->v3, then v1->v3 named
transitive edge.

Algo:
	INPUT: Graph with N edges.
	1. Sort vertices in topological order.
	2. Associate each edges with indicator respective,
	   and recording them in one matrix(N*N)
		e.g: e1:v0->v2, e2:v1->v2, e3:v0->v1
			  0   1    2
			0 --  e3   e1
			1 --  --   e2
			2 --  --   --

	3. Scan vertices according to toplogical order,
	   remove all edges which the target-node has been
	   marked at else rows.
		e.g: There are dependence edges: v0->v1, v0->v2.
		 If v1->v2 has been marked, we said v0->v2 is removable,
		 and the same goes for the rest of edges.
*/
void GRAPH::remove_transitive_edge()
{
	BITSET_MGR bs_mgr;
	SVECTOR<UINT> vex_vec;
	sort_in_toplog_order(vex_vec, true);
	SVECTOR<UINT> vid2pos_in_bitset_map; //Map from VERTEX-ID to BITSET.
	INT i;

	//Mapping vertex id to its position in 'vex_vec'.
	for (i = 0; i <= vex_vec.get_last_idx(); i++) {
		vid2pos_in_bitset_map.set(vex_vec.get(i), i);
	}

	//Associate each edges with indicator respective.
	UINT vex_num = get_vertex_num();
	SVECTOR<BITSET*> edge_indicator; //container of bitset.
	INT c;
	for (EDGE * e = m_edges.get_first(c); e != NULL; e = m_edges.get_next(c)) {
		UINT from = VERTEX_id(EDGE_from(e));
		UINT to = VERTEX_id(EDGE_to(e));

		UINT frompos = vid2pos_in_bitset_map.get(from);
		BITSET * bs = edge_indicator.get(frompos);
		if (bs == NULL) {
			bs = bs_mgr.create();
			edge_indicator.set(frompos, bs);
		}

		//Each from-vertex is associated with
		//a bitset to record all to-vertices.
		bs->bunion(vid2pos_in_bitset_map.get(to));
	} //end for each of edge

	//Scanning vertices in topological order.
	for (i = 0; i < vex_vec.get_last_idx(); i++) {
		//Get the successor vector.
		BITSET * bs = edge_indicator.get(i);
		if (bs != NULL && bs->get_elem_count() >= 2) {
			//Do NOT remove the first edge. Position in bitset
			//has been sorted in topological order.
			for (INT pos_i = bs->get_first(); pos_i >= 0;
				 pos_i = bs->get_next(pos_i)) {
				INT kid_from_vid = vex_vec.get(pos_i);
				INT kid_from_pos = vid2pos_in_bitset_map.get(kid_from_vid);

				//Get bitset that 'pos_i' associated.
				BITSET * kid_from_bs = edge_indicator.get(kid_from_pos);
				if (kid_from_bs != NULL) {
					for (INT pos_j = bs->get_next(pos_i); pos_j >= 0;
						 pos_j = bs->get_next(pos_j)) {
						if (kid_from_bs->is_contain(pos_j)) {
							//The edge 'i->pos_j' is redundant.
							INT to_vid = vex_vec.get(pos_j);
							UINT src_vid = vex_vec.get(i);
							remove_edge(get_edge(src_vid, to_vid));
							bs->diff(pos_j);
						}
					}
				} //end if
			} //end for
		} //end if
	} //end for each vertex
}


//Return true if 'succ' is successor of 'v'.
bool GRAPH::is_succ(VERTEX * v, VERTEX * succ)
{
	EDGE_C * e = VERTEX_out_list(v);
	while (e != NULL) {
		if (EDGE_to(EC_edge(e)) == succ) {
			return true;
		}
		e = EC_next(e);
	}
	return false;
}


//Return true if 'pred' is predecessor of 'v'.
bool GRAPH::is_pred(VERTEX * v, VERTEX * pred)
{
	EDGE_C * e = VERTEX_in_list(v);
	while (e != NULL) {
		if (EDGE_from(EC_edge(e)) == pred) {
			return true;
		}
		e = EC_next(e);
	}
	return false;
}


void GRAPH::dump_dot(CHAR const* name)
{
	if (name == NULL) {
		name = "graph.dot";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!", name));

	fprintf(h, "digraph G {\n");
	//Print node
	INT c;
	for (VERTEX const* v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		fprintf(h, "\nnode%d [shape = Mrecord, label=\"{BB%d}\"];",
				VERTEX_id(v), VERTEX_id(v));
	}

	//Print edge
	for (EDGE const* e = m_edges.get_first(c);
		 e != NULL; e = m_edges.get_next(c)) {
		fprintf(h, "\nnode%d->node%d[label=\"%s\"]",
					VERTEX_id(EDGE_from(e)),
					VERTEX_id(EDGE_to(e)),
					"");
	}
	fprintf(h, "\n}\n");
	fclose(h);
}


void GRAPH::dump_vcg(CHAR const* name)
{
	IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
	if (name == NULL) {
		name = "graph.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!",name));
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

	//Print node
	INT c;
	for (VERTEX const* v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		fprintf(h, "\nnode: { title:\"%d\" label:\"%d\" "
				"shape:circle fontname:\"courB\" color:gold}",
				VERTEX_id(v), VERTEX_id(v));
	}

	//Print edge
	for (EDGE const* e = m_edges.get_first(c);
		 e != NULL; e = m_edges.get_next(c)) {
		fprintf(h, "\nedge: { sourcename:\"%d\" targetname:\"%d\" %s}",
				VERTEX_id(EDGE_from(e)),
				VERTEX_id(EDGE_to(e)),
				m_is_direction ? "" : "arrowstyle:none" );
	}
	fprintf(h, "\n}\n");
	fclose(h);
}


//
//START DGRAPH
//
DGRAPH::DGRAPH(UINT edge_hash_size, UINT vex_hash_size) :
	GRAPH(edge_hash_size, vex_hash_size)
{
	m_bs_mgr = NULL;
}


DGRAPH::DGRAPH(DGRAPH & g) : GRAPH(g)
{
	m_bs_mgr = g.m_bs_mgr;
	if (m_bs_mgr != NULL) {
		clone_bs(g);
	}
}


bool DGRAPH::clone_bs(DGRAPH & src)
{
	IS_TRUE0(m_bs_mgr != NULL);
	INT c;
	for (VERTEX * srcv = src.m_vertices.get_first(c);
		 srcv != NULL; srcv = src.m_vertices.get_next(c)) {
		INT src_vid = VERTEX_id(srcv);
		VERTEX * tgtv = get_vertex(src_vid);
		IS_TRUE0(tgtv != NULL);

		get_dom_set(tgtv)->copy(*src.get_dom_set(srcv));
		get_pdom_set(tgtv)->copy(*src.get_pdom_set(srcv));
	} //end for each vertices
	m_idom_set.copy(src.m_idom_set);
	m_ipdom_set.copy(src.m_ipdom_set);
	return true;
}


UINT DGRAPH::count_mem() const
{
	UINT count = 0;
	count += m_dom_set.count_mem();
	count += m_pdom_set.count_mem(); //record post-dominator-set of each vertex.
	count += m_idom_set.count_mem(); //immediate dominator.
	count += m_ipdom_set.count_mem(); //immediate post dominator.
	count += sizeof(m_bs_mgr); //Do NOT count up the bitset in BS_MGR.
	return count;
}


/* Vertices should have been sorted in topological order.
And we access them by reverse-topological order.
'vlst': compute dominator for vertices in vlst if it
	is not empty or else compute all graph.
'uni': universe. */
bool DGRAPH::compute_dom(LIST<VERTEX const*> const* vlst, BITSET const* uni)
{
	LIST<VERTEX const*> tmpvlst;
	LIST<VERTEX const*> * pvlst = &tmpvlst;
	if (vlst != NULL) {
		//Here one must guarantee pvlst would not be modified.
		pvlst = const_cast<LIST<VERTEX const*>*>(vlst);
	} else {
		INT c;
		for (VERTEX const* u = get_first_vertex(c);
			 u != NULL; u = get_next_vertex(c)) {
			pvlst->append_tail(u);
		}
	}

	BITSET const* luni = NULL;
	if (uni != NULL) {
		luni = uni;
	} else {
		BITSET * x = new BITSET();
		C<VERTEX const*> * ct;
		for (VERTEX const* u = pvlst->get_head(&ct);
			 u != NULL; u = pvlst->get_next(&ct)) {
			x->bunion(VERTEX_id(u));
		}
		luni = x;
	}

	//Initialize dom-set for each BB.
	C<VERTEX const*> * ct;
	for (VERTEX const* v = pvlst->get_head(&ct);
		 v != NULL; v = pvlst->get_next(&ct)) {
		if (is_graph_entry(v)) {
			BITSET * dom = get_dom_set(v);
			dom->clean();
			dom->bunion(VERTEX_id(v));
		} else {
			get_dom_set(v)->copy(*luni);
		}
	}

	//DOM[entry] = {entry}
	//DOM[n] = {n} กศ { กษ(DOM[pred] of predecessor of 'n') }
	bool change = true;
	BITSET tmp;
	UINT count = 0;
	while (change && count < 10) {
		count++;
		change = false;
		C<VERTEX const*> * ct;
		for (VERTEX const* v = pvlst->get_head(&ct);
			 v != NULL; v = pvlst->get_next(&ct)) {
			UINT vid = VERTEX_id(v);
			if (is_graph_entry(v)) {
				continue;
			} else {
				//Access each preds
				EDGE_C * ec = VERTEX_in_list(v);
				while (ec != NULL) {
					VERTEX * pred = EDGE_from(EC_edge(ec));
					if (ec == VERTEX_in_list(v)) {
						tmp.copy(*m_dom_set.get(VERTEX_id(pred)));
					} else {
						tmp.intersect(*m_dom_set.get(VERTEX_id(pred)));
					}
					ec = EC_next(ec);
				}
				tmp.bunion(vid);

				BITSET * dom = m_dom_set.get(VERTEX_id(v));
				if (!dom->is_equal(tmp)) {
					dom->copy(tmp);
					change = true;
				}
			} //end else
		} //end for
	}//end while
	IS_TRUE0(!change);

	if (uni == NULL && luni != NULL) {
		delete luni;
	}
	return true;
}


/* Vertices should have been sorted in topological order.
And we access them by reverse-topological order.
'vlst': compute dominator for vertices in vlst if it
	is not empty or else compute all graph.
'uni': universe. */
bool DGRAPH::compute_dom3(LIST<VERTEX const*> const* vlst, BITSET const* uni)
{
	LIST<VERTEX const*> tmpvlst;
	LIST<VERTEX const*> * pvlst = &tmpvlst;
	if (vlst != NULL) {
		//Here one must guarantee pvlst would not be modified.
		pvlst = const_cast<LIST<VERTEX const*>*>(vlst);
	} else {
		INT c;
		for (VERTEX const* u = get_first_vertex(c);
			 u != NULL; u = get_next_vertex(c)) {
			pvlst->append_tail(u);
		}
	}

	//Initialize dom-set for each BB.
	C<VERTEX const*> * ct;
	for (VERTEX const* v = pvlst->get_head(&ct);
		 v != NULL; v = pvlst->get_next(&ct)) {
		if (is_graph_entry(v)) {
			BITSET * dom = get_dom_set(v);
			dom->clean();
			dom->bunion(VERTEX_id(v));
		} else {
			get_dom_set(v)->clean();
		}
	}

	//DOM[entry] = {entry}
	//DOM[n] = {n} กศ { กษ(DOM[pred] of predecessor of 'n') }
	bool change = true;
	BITSET tmp;
	UINT count = 0;
	while (change && count < 10) {
		count++;
		change = false;
		C<VERTEX const*> * ct;
		for (VERTEX const* v = pvlst->get_head(&ct);
			 v != NULL; v = pvlst->get_next(&ct)) {
			UINT vid = VERTEX_id(v);
			if (is_graph_entry(v)) {
				continue;
			} else {
				//Access each preds
				EDGE_C * ec = VERTEX_in_list(v);
				UINT meet = 0;
				while (ec != NULL) {
					VERTEX * pred = EDGE_from(EC_edge(ec));
					BITSET * ds = m_dom_set.get(VERTEX_id(pred));
					if (ds->is_empty()) {
						ec = EC_next(ec);
						continue;
					}

					if (meet == 0) {
						tmp.copy(*ds);
					} else {
						tmp.intersect(*ds);
					}
					meet++;
					ec = EC_next(ec);
				}

				if (meet == 0) { tmp.clean(); }
				tmp.bunion(vid);

				BITSET * dom = m_dom_set.get(VERTEX_id(v));
				if (!dom->is_equal(tmp)) {
					dom->copy(tmp);
					change = true;
				}
			} //end else
		} //end for
	} //end while
	IS_TRUE0(!change);
	return true;
}


/* Compute post-dominator according to rpo.
root: root node of graph.
uni: universe.
Note you should use this function carefully, it may be expensive, because that
the function does not check if RPO is available, namely, it will always
compute the RPO. */
bool DGRAPH::compute_pdom_by_rpo(VERTEX * root, BITSET const* uni)
{
	LIST<VERTEX const*> vlst;
	compute_rpo_norec(root, vlst);
	vlst.reverse();

	bool res = false;
	if (uni == NULL) {
		res = compute_pdom(&vlst);
	} else {
		res = compute_pdom(&vlst, uni);
	}
	IS_TRUE0(res);
	return true;
}


//Vertices should have been sorted in topological order.
//We access them by reverse-topological order.
bool DGRAPH::compute_pdom(LIST<VERTEX const*> const* vlst)
{
	IS_TRUE0(vlst);
	BITSET uni;
	C<VERTEX const*> * ct;
	for (VERTEX const* u = vlst->get_head(&ct);
		 u != NULL; u = vlst->get_next(&ct)) {
		uni.bunion(VERTEX_id(u));
	}
	return compute_pdom(vlst, &uni);
}


/* Vertices should have been sorted in topological order.
And we access them by reverse-topological order.
vlst: vertex list.
uni: universe. */
bool DGRAPH::compute_pdom(LIST<VERTEX const*> const* vlst, BITSET const* uni)
{
	IS_TRUE0(vlst && uni);

	//Initialize pdom for each bb
	C<VERTEX const*> * ct;
	for (VERTEX const* v = vlst->get_head(&ct);
		 v != NULL; v = vlst->get_next(&ct)) {
		if (is_graph_exit(v)) {
			BITSET * pdom = get_pdom_set(v);
			pdom->clean();
			pdom->bunion(VERTEX_id(v));
		} else {
			get_pdom_set(v)->copy(*uni);
		}
	}

	//PDOM[exit] = {exit}
	//PDOM[n] = {n} U {กษ(PDOM[succ] of each succ of n)}
	bool change = true;
	BITSET tmp;
	UINT count = 0;
	while (change && count < 10) {
		count++;
		change = false;
		C<VERTEX const*> * ct;
		for (VERTEX const* v = vlst->get_head(&ct);
			 v != NULL; v = vlst->get_next(&ct)) {
			UINT vid = VERTEX_id(v);
			if (is_graph_exit(v)) {
				continue;
			} else {
				tmp.clean();
				//Access each succs
				EDGE_C * ec = VERTEX_out_list(v);
				while (ec != NULL) {
					VERTEX * succ = EDGE_to(EC_edge(ec));
					if (ec == VERTEX_out_list(v)) {
						tmp.copy(*m_pdom_set.get(VERTEX_id(succ)));
					} else {
						tmp.intersect(*m_pdom_set.get(VERTEX_id(succ)));
					}
					ec = EC_next(ec);
				}
				tmp.bunion(vid);

				BITSET * pdom = m_pdom_set.get(VERTEX_id(v));
				if (!pdom->is_equal(tmp)) {
					pdom->copy(tmp);
					change = true;
				}
			}
		} //end for
	} // end while

	IS_TRUE0(!change);
	return true;
}


//This function need idom to be avaiable.
//NOTE: set does NOT include node itself.
bool DGRAPH::compute_dom2(LIST<VERTEX const*> const& vlst)
{
	C<VERTEX const*> * ct;
	BITSET avail;
	for (VERTEX const* v = vlst.get_head(&ct);
		 v != NULL; v = vlst.get_next(&ct)) {
		BITSET * doms = get_dom_set(VERTEX_id(v));
		doms->clean();
		IS_TRUE0(doms);
		for (UINT idom = get_idom(VERTEX_id(v));
			 idom != 0; idom = get_idom(idom)) {
			if (avail.is_contain(idom)) {
				BITSET const* idom_doms = get_dom_set(idom);
				doms->copy(*idom_doms);
				doms->bunion(idom);
				break;
			}
			doms->bunion(idom);
		}
		avail.bunion(VERTEX_id(v));
	}
	return true;
}


/* Vertices should have been sorted in rpo.
'vlst': a list of vertex which sort in rpo order.

NOTE:
	1. The root node has better to be the first one in 'vlst'.
	2. Do not use '0' as vertex id, it is used as Undefined.
	3. Entry does not have idom.
*/
bool DGRAPH::compute_idom2(LIST<VERTEX const*> const& vlst)
{
	bool change = true;

	//Initialize idom-set for each BB.
	m_idom_set.clean();
	UINT nentry = 0;
	while (change) {
		change = false;
		//Access with topological order.
		C<VERTEX const*> * ct;
		for (VERTEX const* v = vlst.get_head(&ct);
			 v != NULL; v = vlst.get_next(&ct)) {
			INT cur_id = VERTEX_id(v);
			if (is_graph_entry(v)) {
				m_idom_set.set(VERTEX_id(v), VERTEX_id(v));
				nentry++;
				continue;
			}

			//Access each preds
			EDGE_C const* ec = VERTEX_in_list(v);
			UINT meet = 0;
			VERTEX const* idom = NULL;
			while (ec != NULL) {
				VERTEX const* pred = EDGE_from(EC_edge(ec));
				UINT pid = VERTEX_id(pred);

				if (m_idom_set.get(pid) == 0) {
					ec = EC_next(ec);
					continue;
				}

				if (idom == NULL) {
					idom = pred;
					ec = EC_next(ec);
					continue;
				}

				VERTEX const* j = pred;
				VERTEX const* k = idom;
				while (j != k) {
					while (VERTEX_rpo(j) > VERTEX_rpo(k)) {
						j = get_vertex(m_idom_set.get(VERTEX_id(j)));
						IS_TRUE0(j);
						if (is_graph_entry(j)) {
							break;
						}
					}
					while (VERTEX_rpo(j) < VERTEX_rpo(k)) {
						k = get_vertex(m_idom_set.get(VERTEX_id(k)));
						IS_TRUE0(k);
						if (is_graph_entry(k)) {
							break;
						}
					}
					if (is_graph_entry(j) && is_graph_entry(k)) {
						break;
					}
 				}

				if (j != k) {
					//Multi entries.
					IS_TRUE0(is_graph_entry(j) && is_graph_entry(k));
					idom = NULL;
					break;
				}

				idom = j;
				ec = EC_next(ec);
			}

			if (idom == NULL) {
				if (m_idom_set.get(VERTEX_id(v)) != 0) {
					m_idom_set.set(VERTEX_id(v), 0);
					change = true;
				}
			} else if ((UINT)m_idom_set.get(VERTEX_id(v)) != VERTEX_id(idom)) {
				m_idom_set.set(VERTEX_id(v), VERTEX_id(idom));
				change = true;
			}
		} //end for
	}

	C<VERTEX const*> * ct;
	for (VERTEX const* v = vlst.get_head(&ct);
		 v != NULL; v = vlst.get_next(&ct)) {
		if (is_graph_entry(v)) {
			m_idom_set.set(VERTEX_id(v), 0);
			nentry--;
			if (nentry == 0) { break; }
		}
	}
	return true;
}


//NOTE: Entry does not have idom.
bool DGRAPH::compute_idom()
{
	bool change = true;

	//Initialize idom-set for each BB.
	m_idom_set.clean();

	//Access with topological order.
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		INT cur_id = VERTEX_id(v);
		if (is_graph_entry(v)) {
			continue;
		} else if (m_dom_set.get(cur_id)->get_elem_count() >= 2) {
			BITSET * p = m_dom_set.get(cur_id);
			IS_TRUE(p != NULL, ("should compute dom first"));
			if (p->get_elem_count() == 1) {
				//There is no idom if 'dom' set only contain itself.
				IS_TRUE0(m_idom_set.get(cur_id) == 0);
				continue;
			}
			p->diff(cur_id);

			#ifdef MAGIC_METHOD
			INT i;
			INT idom = -1;
			for (i = p->get_first(); i != -1; i = p->get_next(i)) {
				if (m_dom_set.get(i)->is_equal(*p)) {
					IS_TRUE0(m_idom_set.get(cur_id) == 0);
					m_idom_set.set(cur_id, i);
					break;
				}
			}
			IS_TRUE(i != -1, ("not find idom?"));
			#else
			INT i;
			for (i = tmp.get_first(); i != -1; i = tmp.get_next(i)) {
				for (INT j = tmp.get_first(); j != -1; j = tmp.get_next(j)) {
					if (i == j) {
						continue;
					}
					if (m_dom_set.get(j)->is_contain(i)) {
						tmp.diff(i);
						//Search 'tmp' over again.
						i = tmp.get_first();
						j = i;
					}
				}
			}
			i = tmp.get_first();
			IS_TRUE(i != -1, ("cannot find idom of BB:%d", cur_id));
			IS_TRUE(m_idom_set.get(cur_id) == 0, ("recompute idom for BB:%d", cur_id));
			m_idom_set.set(cur_id, i);
			#endif
			p->bunion(cur_id);
		} //end else if
	} //end for
	return true;
}


//NOTE: Exit does not have idom.
bool DGRAPH::compute_ipdom()
{
	bool change = true;

	//Initialize ipdom-set for each BB.
	m_ipdom_set.clean();

	//Processing in reverse-topological order.
	INT c;
	for (VERTEX const* v = m_vertices.get_last(c);
		 v != NULL; v = m_vertices.get_prev(c)) {
		INT cur_id = VERTEX_id(v);
		if (is_graph_exit(v)) {
			continue;
		} else if (m_pdom_set.get(cur_id)->get_elem_count() > 1) {
			BITSET * p = m_pdom_set.get(cur_id);
			IS_TRUE(p != NULL, ("should compute pdom first"));
			if (p->get_elem_count() == 1) {
				//There is no ipdom if 'pdom' set only contain itself.
				IS_TRUE0(m_ipdom_set.get(cur_id) == 0);
				continue;
			}

			p->diff(cur_id);
			INT i;
			for (i = p->get_first(); i != -1; i = p->get_next(i)) {
				if (m_pdom_set.get(i)->is_equal(*p)) {
					IS_TRUE0(m_ipdom_set.get(cur_id) == 0);
					m_ipdom_set.set(cur_id, i);
					break;
				}
			}
			//IS_TRUE(i != -1, ("not find ipdom")); //Not find.
			p->bunion(cur_id);
		} //end else if
	} //end for
	return true;
}


//'dom': output dominator tree.
void DGRAPH::get_dom_tree(OUT GRAPH & dom)
{
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		UINT vid = VERTEX_id(v);
		dom.add_vertex(vid);
		if (m_idom_set.get(vid) != 0) {
			dom.add_edge(m_idom_set.get(vid), vid);
		}
	}
}


//'pdom': output post-dominator tree.
void DGRAPH::get_pdom_tree(OUT GRAPH & pdom)
{
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		UINT vid = VERTEX_id(v);
		pdom.add_vertex(vid);
		if (m_ipdom_set.get(vid) != 0) { //id of bb starting at 1.
			pdom.add_edge(m_ipdom_set.get(vid), vid);
		}
	}
}


/* Dump dom set, pdom set, idom, ipdom.
'dump_dom_tree': set to be true to dump dominate
	tree, and post dominate Tree. */
void DGRAPH::dump_dom(FILE * h, bool dump_dom_tree)
{
	if (h == NULL) return;
	fprintf(h, "\n\n\n\n==---- DUMP DOM/PDOM/IDOM/IPDOM ----==");
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		INT vid = VERTEX_id(v);
		fprintf(h, "\nVERTEX(%d) dom: ", vid);
		BITSET * bs;
		if ((bs = m_dom_set.get(vid)) != NULL) {
			for (INT id = bs->get_first(); id != -1 ; id = bs->get_next(id)) {
				if (id != vid) {
					fprintf(h, "%d ", id);
				}
			}
		}
		fprintf(h, "\n     pdom: ");
		if ((bs = m_pdom_set.get(vid)) != NULL) {
			for (INT id = bs->get_first(); id != -1; id = bs->get_next(id)) {
				if (id != vid) {
					fprintf(h, "%d ", id);
				}
			}
		}
		if (m_idom_set.get(vid) != 0) {
			fprintf(h, "\n     idom: %d", m_idom_set.get(vid));
		} else {
			fprintf(h, "\n");
		}

		if (m_ipdom_set.get(vid) != 0) {
			fprintf(h, "\n     ipdom: %d", m_ipdom_set.get(vid));
		} else {
			fprintf(h, "\n");
		}
	} //end for each vertices

	fprintf(h, "\n");
	fflush(h);

	if (dump_dom_tree) {
		GRAPH dom;
		get_dom_tree(dom);
		dom.dump_vcg("graph_dom_tree.vcg");
		dom.erase();
		get_pdom_tree(dom);
		dom.dump_vcg("graph_pdom_tree.vcg");
	}
}


void DGRAPH::sort_dom_tree_in_preorder(IN GRAPH & dom_tree, IN VERTEX * root,
									   OUT LIST<VERTEX*> & lst)
{
	IS_TRUE0(root);
	BITSET is_visited;
	is_visited.bunion(VERTEX_id(root));
	lst.append_tail(root);

	VERTEX * v;
	SSTACK<VERTEX*> stk;
	stk.push(root);
	while ((v = stk.pop()) != NULL) {
		if (!is_visited.is_contain(VERTEX_id(v))) {
			is_visited.bunion(VERTEX_id(v));
			stk.push(v);
			//The only place to process vertex.
			lst.append_tail(v);
		}

		//Visit children.
		EDGE_C * el = VERTEX_out_list(v);
		VERTEX * succ;
		while (el != NULL) {
			succ = EDGE_to(EC_edge(el));
			if (!is_visited.is_contain(VERTEX_id(succ))) {
				stk.push(v);
				stk.push(succ);
				break;
			}
			el = EC_next(el);
		}
	}
}


/*
'order_buf': record the bfs-order for each vertex.

NOTE: BFS does NOT keep the sequence if you are going to
access vertex in lexicographic order.
*/
void DGRAPH::sort_in_bfs_order(SVECTOR<UINT> & order_buf, VERTEX * root,
							   BITSET & visit)
{
	LIST<VERTEX*> worklst;
	worklst.append_tail(root);
	UINT order = 1;
	while (worklst.get_elem_count() > 0) {
		VERTEX * sv = worklst.remove_head();
		order_buf.set(VERTEX_id(sv), order);
		order++;
		visit.bunion(VERTEX_id(sv));
		EDGE_C * el = VERTEX_out_list(sv);
		while (el != NULL) {
			VERTEX * to = EDGE_to(EC_edge(el));
			if (visit.is_contain(VERTEX_id(to))) {
				el = EC_next(el);
				continue;
			}
			worklst.append_tail(to);
			el = EC_next(el);
		}
	}
}


void DGRAPH::sort_dom_tree_in_postorder(IN GRAPH & dom_tree, IN VERTEX * root,
										OUT LIST<VERTEX*> & lst)
{
	IS_TRUE0(root);
	BITSET is_visited;

	//Find the leaf node.
	VERTEX * v;
	SSTACK<VERTEX*> stk;
	stk.push(root);
	while ((v = stk.pop()) != NULL) {
		//Visit children first.
		EDGE_C * el = VERTEX_out_list(v);
		bool find = false; //find unvisited kid.
		VERTEX * succ;
		while (el != NULL) {
			succ = EDGE_to(EC_edge(el));
			if (!is_visited.is_contain(VERTEX_id(succ))) {
				stk.push(v);
				stk.push(succ);
				find = true;
				break;
			}
			el = EC_next(el);
		}
		if (!find) {
			is_visited.bunion(VERTEX_id(v));
			//The only place to process vertex.
			lst.append_tail(v);
		}
	}
}


void DGRAPH::_remove_unreach_node(UINT id, BITSET & visited)
{
	visited.bunion(id);
	VERTEX * vex = get_vertex(id);
	EDGE_C * el = VERTEX_out_list(vex);
	while (el != NULL) {
		UINT succ = VERTEX_id(EDGE_to(EC_edge(el)));
		if (!visited.is_contain(succ)) {
			_remove_unreach_node(succ, visited);
		}
		el = EC_next(el);
	}
}


/*
Perform DFS to seek for unreachable node.
Return true if some nodes removed.
*/
bool DGRAPH::remove_unreach_node(UINT entry_id)
{
	if (get_vertex_num() == 0) return false;
	bool removed = false;
	BITSET visited;
	_remove_unreach_node(entry_id, visited);
	INT c;
	for (VERTEX * v = get_first_vertex(c);
		 v != NULL; v = get_next_vertex(c)) {
		if (!visited.is_contain(VERTEX_id(v))) {
			remove_vertex(v);
			removed = true;
		}
	}
	return removed;
}
//END DGRAPH
