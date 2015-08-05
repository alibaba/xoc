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
#ifndef __GRAPH_H_
#define __GRAPH_H_

#define MAGIC_METHOD

class VERTEX;
class EDGE;
class GRAPH;

#define EDGE_next(e)		(e)->next
#define EDGE_prev(e)		(e)->prev
#define EDGE_from(e)		(e)->from
#define EDGE_to(e)			(e)->to
#define EDGE_info(e)		(e)->info
class EDGE {
public:
	EDGE()
	{
		prev = next = NULL;
		from = to = NULL;
		info = NULL;
	}
	EDGE * prev; //used by FREE_LIST and EDGE_HASH
	EDGE * next; //used by FREE_LIST and EDGE_HASH
	VERTEX * from;
	VERTEX * to;
	void * info;
};


//The container of EDEG.
#define EC_next(el)		(el)->next
#define EC_prev(el)		(el)->prev
#define EC_edge(el)		(el)->edge
class EDGE_C {
public:
	EDGE_C()
	{
		next = prev = NULL;
		edge = NULL;
	}
	EDGE_C * next;
	EDGE_C * prev;
	EDGE * edge;
};



#define VERTEX_next(v)			(v)->next
#define VERTEX_prev(v)			(v)->prev
#define VERTEX_id(v)			(v)->id
#define VERTEX_rpo(v)			(v)->rpo
#define VERTEX_in_list(v)		(v)->in_list
#define VERTEX_out_list(v)		(v)->out_list
#define VERTEX_info(v)			(v)->info
class VERTEX {
public:
	VERTEX()
	{
		prev = next = NULL;
		in_list = out_list = NULL;
		info = NULL;
		id = 0;
	}

	VERTEX * prev; //used by FREE_LIST and HASH
	VERTEX * next; //used by FREE_LIST and HASH
	EDGE_C * in_list; //incoming edge list
	EDGE_C * out_list;//outgoing edge list
	UINT id;
	UINT rpo;
	void * info;
};


#define MAKE_VALUE(from, to) (((from)<<16)|(to))
class EDGE_HF {
public:
	UINT get_hash_value(EDGE * e, UINT bs) const
	{
		IS_TRUE0(is_power_of_2(bs));
		return hash32bit(MAKE_VALUE(VERTEX_id(EDGE_from(e)),
									VERTEX_id(EDGE_to(e)))) & (bs - 1);
	}

	UINT get_hash_value(OBJTY val, UINT bs) const
	{ return get_hash_value((EDGE*)val, bs); }

	bool compare(EDGE * e1, EDGE * e2) const
	{
		return (VERTEX_id(EDGE_from(e1)) == VERTEX_id(EDGE_from(e2))) &&
			   (VERTEX_id(EDGE_to(e1)) == VERTEX_id(EDGE_to(e2)));
	}

	bool compare(EDGE * t1, OBJTY val) const
	{
		EDGE * t2 = (EDGE*)val;
		return VERTEX_id(EDGE_from(t1)) == VERTEX_id(EDGE_from(t2)) &&
			   VERTEX_id(EDGE_to(t1)) == VERTEX_id(EDGE_to(t2));
	}
};


class EDGE_HASH : public SHASH<EDGE*, EDGE_HF> {
	GRAPH * m_g;
public:
	EDGE_HASH(UINT bsize = 64) : SHASH<EDGE*, EDGE_HF>(bsize) {}
	virtual ~EDGE_HASH() {}

	void init_g(GRAPH * g) { m_g = g; }
	void init(GRAPH * g, UINT bsize)
	{
		m_g = g;
		SHASH<EDGE*, EDGE_HF>::init(bsize);
	}

	void destroy()
	{
		m_g = NULL;
		SHASH<EDGE*, EDGE_HF>::destroy();
	}

	virtual EDGE * create(OBJTY v);
};


class VERTEX_HF {
public:
	UINT get_hash_value(OBJTY val, UINT bs) const
	{
		IS_TRUE0(is_power_of_2(bs));
		return hash32bit((UINT)(size_t)val) & (bs - 1);
	}

	UINT get_hash_value(VERTEX const* vex, UINT bs) const
	{
		IS_TRUE0(is_power_of_2(bs));
		return hash32bit(VERTEX_id(vex)) & (bs - 1);
	}

	bool compare(VERTEX * v1, VERTEX * v2) const
	{ return (VERTEX_id(v1) == VERTEX_id(v2)); }

	bool compare(VERTEX * v1, OBJTY val) const
	{ return (VERTEX_id(v1) == (UINT)(size_t)val); }
};


class VERTEX_HASH : public SHASH<VERTEX*, VERTEX_HF> {
protected:
	SMEM_POOL * m_ec_pool;
public:
	VERTEX_HASH(UINT bsize = 64) : SHASH<VERTEX*, VERTEX_HF>(bsize)
	{
		m_ec_pool = smpool_create_handle(sizeof(VERTEX) * 4, MEM_CONST_SIZE);
	}

	virtual ~VERTEX_HASH()
	{ smpool_free_handle(m_ec_pool); }

	virtual VERTEX * create(OBJTY v)
	{
		VERTEX * vex = (VERTEX*)smpool_malloc_h_const_size(sizeof(VERTEX),
														   m_ec_pool);
		IS_TRUE0(vex);
		memset(vex, 0, sizeof(VERTEX));
		VERTEX_id(vex) = (UINT)(size_t)v;
		return vex;
	}
};


/* A graph G = (V, E), consists of a set of vertices, V, and a set of edges, E.
Each edge is a pair (v,w), where v,w belong to V. Edges are sometimes
referred to as arcs. If the pair is ordered, then the graph is directed.
Directed graphs are sometimes referred to as digraphs. Vertex w is adjacent
to v if and only if (v,w) belong to E. In an undirected graph with edge (v,w),
and hence (w,v), w is adjacent to v and v is adjacent to w.
Sometimes an edge has a third component, known as either a weight or a cost.

NOTICE: for accelerating perform operation of each vertex, e.g
compute dominator, please try best to add vertex with
topological order. */
class GRAPH {
	friend class EDGE_HASH;
	friend class VERTEX_HASH;
protected:
	//it is true if the number of edges between any two
	//vertices are not more than one.
	BYTE m_is_unique:1;
	BYTE m_is_direction:1; //true if graph is direction.
	UINT m_edge_hash_size;
	UINT m_vex_hash_size;
	EDGE_HASH m_edges; //record all edges.
	VERTEX_HASH m_vertices; //record all vertices.
	FREE_LIST<EDGE> m_e_free_list; //record freed EDGE for reuse.
	FREE_LIST<EDGE_C> m_el_free_list; //record freed EDGE_C for reuse.
	FREE_LIST<VERTEX> m_v_free_list; //record freed VERTEX for reuse.
	SMEM_POOL * m_vertex_pool;
	SMEM_POOL * m_edge_pool;
	SMEM_POOL * m_ec_pool;

	//record vertex if vertex id is densen distribution.
	//map vertex id to vertex.
	SVECTOR<VERTEX*> * m_dense_vertex;

	//Add 'e' into out-edges of 'vex'
	inline void add_out_list(VERTEX * vex, EDGE * e)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		if (vex == NULL || e == NULL)return;

		EDGE_C * el = VERTEX_out_list(vex);
		while (el != NULL) {
			if (EC_edge(el) == e) return;
			el = EC_next(el);
		}
		el = new_ec(e);
		add_next(&VERTEX_out_list(vex), el);
	}

	//Add 'e' into in-edges of 'vex'
	inline void add_in_list(VERTEX * vex, EDGE * e)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		if (vex == NULL || e == NULL) return;

		EDGE_C * el = VERTEX_in_list(vex);
		while (el != NULL) {
			if (EC_edge(el) == e) return;
			el = EC_next(el);
		}
		el = new_ec(e);
		add_next(&VERTEX_in_list(vex), el);
	}

	virtual void * clone_edge_info(EDGE * e)
	{ IS_TRUE(0, ("should be overloaded")); return NULL; }

	virtual void * clone_vertex_info(VERTEX * v)
	{ IS_TRUE(0, ("should be overloaded")); return NULL; }

	inline VERTEX * new_vertex()
	{
		VERTEX * vex = (VERTEX*)smpool_malloc_h_const_size(sizeof(VERTEX),
													  m_vertex_pool);
		IS_TRUE0(vex);
		memset(vex, 0, sizeof(VERTEX));
		return vex;
	}

	inline EDGE * new_edge(UINT from, UINT to)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		VERTEX * fp = add_vertex(from);
		VERTEX * tp = add_vertex(to);
		return new_edge(fp, tp);
	}
	EDGE * new_edge(VERTEX * from, VERTEX * to);
	VERTEX * new_vertex(UINT vid);

	inline EDGE * new_edge_impl(VERTEX * from, VERTEX * to)
	{
		EDGE * e = m_e_free_list.get_free_elem();
		if (e == NULL) {
			e = (EDGE*)smpool_malloc_h_const_size(sizeof(EDGE), m_edge_pool);
			memset(e, 0, sizeof(EDGE));
		}
		EDGE_from(e) = from;
		EDGE_to(e) = to;
		add_in_list(to, e);
		add_out_list(from, e);
		return e;
	}

	inline EDGE_C * new_ec(EDGE * e)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		if (e == NULL) { return NULL; }

		EDGE_C * el = m_el_free_list.get_free_elem();
		if (el == NULL) {
			el = (EDGE_C*)smpool_malloc_h_const_size(sizeof(EDGE_C),
													 m_ec_pool);
			memset(el, 0, sizeof(EDGE_C));
		}
		EC_edge(el) = e;
		return el;
	}
public:
	GRAPH(UINT edge_hash_size = 64, UINT vex_hash_size = 64);
	GRAPH(GRAPH & g);
	virtual ~GRAPH() { destroy(); }
	void init();
	void destroy();
	inline EDGE * add_edge(UINT from, UINT to)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return new_edge(from, to);
	}
	inline EDGE * add_edge(VERTEX * from, VERTEX * to)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return new_edge(from, to);
	}
	inline VERTEX * add_vertex(UINT vid)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return m_vertices.append(new_vertex(vid));
	}

	void compute_rpo_norec(IN OUT VERTEX * root, OUT LIST<VERTEX const*> & vlst);
	bool clone(GRAPH & src);
	UINT count_mem() const;

	void dump_dot(CHAR const* name = NULL);
	void dump_vcg(CHAR const* name = NULL);

	bool is_succ(VERTEX * v, VERTEX * succ);
	bool is_pred(VERTEX * v, VERTEX * pred);
	bool is_equal(GRAPH & g);
	bool is_unique() const { return m_is_unique; }
	bool is_direction() const { return m_is_direction; }

	//Is there exist a path connect 'from' and 'to'.
	inline bool is_reachable(UINT from, UINT to)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return is_reachable(get_vertex(from), get_vertex(to));
	}
	bool is_reachable(VERTEX * from, VERTEX * to);
	void insert_vertex_between(IN VERTEX * v1, IN VERTEX * v2,
							   IN VERTEX * newv, OUT EDGE ** e1 = NULL,
							   OUT EDGE ** e2 = NULL);
	void insert_vertex_between(UINT v1, UINT v2, UINT newv,
							   OUT EDGE ** e1 = NULL, OUT EDGE ** e2 = NULL);
	bool is_graph_entry(VERTEX const* v) const
	{ return VERTEX_in_list(v) == NULL;	}

	//Return true if vertex is exit node of graph.
	bool is_graph_exit(VERTEX const* v) const
	{ return VERTEX_out_list(v) == NULL; }

	void erase();

	bool get_neighbor_list(IN OUT LIST<UINT> & ni_list, UINT vid) const;
	bool get_neighbor_set(OUT SBITSET & niset, UINT vid) const;
	inline UINT get_degree(UINT vid)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return get_degree(get_vertex(vid));
	}
	UINT get_degree(VERTEX const* vex) const;
	UINT get_in_degree(VERTEX const* vex) const;
	UINT get_out_degree(VERTEX const* vex) const;
	inline UINT get_vertex_num()
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return m_vertices.get_elem_count();
	}
	inline UINT get_edge_num()
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return m_edges.get_elem_count();
	}
	inline VERTEX * get_vertex(UINT vid)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		if (m_dense_vertex != NULL) {
			return m_dense_vertex->get(vid);
		}
		return (VERTEX*)m_vertices.find((OBJTY)(size_t)vid);
	}
	EDGE * get_edge(UINT from, UINT to);
	EDGE * get_edge(VERTEX const* from, VERTEX const* to);
	inline EDGE * get_first_edge(INT & cur)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return m_edges.get_first(cur);
	}
	inline EDGE * get_next_edge(INT & cur)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return m_edges.get_next(cur);
	}
	inline VERTEX * get_first_vertex(INT & cur)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return m_vertices.get_first(cur);
	}
	inline VERTEX * get_next_vertex(INT & cur)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return m_vertices.get_next(cur);
	}

	void resize(UINT vertex_hash_sz, UINT edge_hash_sz);
	EDGE * rev_edge(EDGE * e); //Reverse edge direction.(e.g: a->b => b->a)
	void rev_edges(); //Reverse all edges.
	EDGE * remove_edge(EDGE * e);
	void remove_edges_between(VERTEX * v1, VERTEX * v2);
	VERTEX * remove_vertex(VERTEX * vex);
	inline VERTEX * remove_vertex(UINT vid)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		return remove_vertex(get_vertex(vid));
	}
	void remove_transitive_edge();

	bool sort_in_toplog_order(OUT SVECTOR<UINT> & vex_vec, bool is_topdown);
	void set_unique(bool is_unique)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		m_is_unique = is_unique;
	}
	void set_direction(bool has_direction)
	{
		IS_TRUE(m_ec_pool != NULL, ("not yet initialized."));
		m_is_direction = has_direction;
	}
	void set_dense(bool is_dense)
	{
		if (is_dense) {
			if (m_dense_vertex == NULL) {
				m_dense_vertex = new SVECTOR<VERTEX*>();
			}
			return;
		}
		if (m_dense_vertex == NULL) {
			delete m_dense_vertex;
			m_dense_vertex = NULL;
		}
	}
};


class ITER_DOM_TREE {
public:
	BITSET is_visited;
	SSTACK<VERTEX*> stk;
	UINT is_preorder:1;
	UINT is_postorder:1;

	ITER_DOM_TREE()
	{
		is_preorder = 0;
		is_postorder = 0;
	}
	void clean() { is_visited.clean(); stk.clean(); }
	void set_preorder() { is_preorder = 1; }
	void set_postorder() { is_postorder = 1; }
};


//This class indicate a Dominator Tree.
class DOM_TREE : public GRAPH {
public:
	DOM_TREE()
	{ set_dense(true); }
};


//
//GRAPH with Dominator info.
//
class DGRAPH : public GRAPH {
protected:
	SVECTOR<BITSET*> m_dom_set; //record dominator-set of each vertex.
	SVECTOR<BITSET*> m_pdom_set; //record post-dominator-set of each vertex.
	SVECTOR<INT> m_idom_set; //immediate dominator.
	SVECTOR<INT> m_ipdom_set; //immediate post dominator.
	BITSET_MGR * m_bs_mgr;
	void _remove_unreach_node(UINT id, BITSET & visited);
public:
	DGRAPH(UINT edge_hash_size = 64, UINT vex_hash_size = 64);
	DGRAPH(DGRAPH & g);

	inline bool clone(DGRAPH & g)
	{
		m_bs_mgr = g.m_bs_mgr;
		return GRAPH::clone(g);
	}
	bool clone_bs(DGRAPH & src);
	bool compute_dom3(LIST<VERTEX const*> const* vlst, BITSET const* uni);
	bool compute_dom2(LIST<VERTEX const*> const& vlst);
	bool compute_dom(LIST<VERTEX const*> const* vlst = NULL,
					 BITSET const* uni = NULL);
	bool compute_pdom_by_rpo(VERTEX * root, BITSET const* uni);
	bool compute_pdom(LIST<VERTEX const*> const* vlst);
	bool compute_pdom(LIST<VERTEX const*> const* vlst, BITSET const* uni);
	bool compute_idom();
	bool compute_idom2(LIST<VERTEX const*> const& vlst);
	bool compute_ipdom();
	UINT count_mem() const;

	void dump_dom(FILE * h, bool dump_dom_tree = true);

	//idom must be positive
	//NOTE: Entry does not have idom.
	//'id': vertex id.
	inline UINT get_idom(UINT id) { return m_idom_set.get(id); }

	//ipdom must be positive
	//NOTE: Exit does not have idom.
	//'id': vertex id.
	inline UINT get_ipdom(UINT id) { return m_ipdom_set.get(id); }

	void get_dom_tree(OUT GRAPH & dom);
	void get_pdom_tree(OUT GRAPH & pdom);

	//Get vertices who dominate vertex 'id'.
	//NOTE: set does NOT include 'v' itself.
	inline BITSET * get_dom_set(UINT id)
	{
		IS_TRUE0(m_bs_mgr != NULL);
		BITSET * set = m_dom_set.get(id);
		if (set == NULL) {
			set = m_bs_mgr->create();
			m_dom_set.set(id, set);
		}
		return set;
	}

	//Get vertices who dominate vertex 'v'.
	//NOTE: set does NOT include 'v' itself.
	BITSET * get_dom_set(VERTEX const* v)
	{
		IS_TRUE0(v != NULL);
		return get_dom_set(VERTEX_id(v));
	}

	//Get vertices who post dominated by vertex 'id'.
	//NOTE: set does NOT include 'v' itself.
	inline BITSET * get_pdom_set(UINT id)
	{
		IS_TRUE0(m_bs_mgr != NULL);
		BITSET * set = m_pdom_set.get(id);
		if (set == NULL) {
			set = m_bs_mgr->create();
			m_pdom_set.set(id, set);
		}
		return set;
	}

	//Get vertices who post dominated by vertex 'v'.
	//NOTE: set does NOT include 'v' itself.
	BITSET * get_pdom_set(VERTEX const* v)
	{
		IS_TRUE0(v != NULL);
		return get_pdom_set(VERTEX_id(v));
	}

	//Return true if 'v1' dominate 'v2'.
	bool is_dom(UINT v1, UINT v2)
	{ return get_dom_set(v2)->is_contain(v1); }

	//Return true if 'v1' post dominate 'v2'.
	bool is_pdom(UINT v1, UINT v2)
	{ return get_pdom_set(v2)->is_contain(v1); }

	void sort_in_bfs_order(SVECTOR<UINT> & order_buf, VERTEX * root,
						   BITSET & visit);
	void sort_dom_tree_in_preorder(IN GRAPH & dom_tree, IN VERTEX * root,
								   OUT LIST<VERTEX*> & lst);
	void sort_dom_tree_in_postorder(IN GRAPH & dom_tree, IN VERTEX * root,
									OUT LIST<VERTEX*> & lst);
	void set_bs_mgr(BITSET_MGR * bs_mgr) { m_bs_mgr = bs_mgr; }
	bool remove_unreach_node(UINT entry_id);
};
#endif
