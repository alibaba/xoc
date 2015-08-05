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

//
//START CDG
//
void CDG::dump()
{
	dump_vcg("graph_cd_tree.vcg");
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP Control Dependence ----==");
	INT c;
	for (VERTEX * v = get_first_vertex(c);
		 v != NULL; v = get_next_vertex(c)) {
		EDGE_C * in = VERTEX_in_list(v);
		if (in == NULL) {
			fprintf(g_tfile, "\nBB%d has NO ctrl BB", VERTEX_id(v));
			continue;
		}
		fprintf(g_tfile, "\nBB%d ctrl BB is: ", VERTEX_id(v));
		while (in != NULL) {
			VERTEX * pred = EDGE_from(EC_edge(in));
			fprintf(g_tfile, "%d,", VERTEX_id(pred));
			in = EC_next(in);
		}
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


void CDG::get_cd_preds(UINT id, OUT LIST<VERTEX*> & lst)
{
	VERTEX * v = get_vertex(id);
	IS_TRUE0(v != NULL);
	EDGE_C * in = VERTEX_in_list(v);
	while (in != NULL) {
		VERTEX * pred = EDGE_from(EC_edge(in));
		lst.append_tail(pred);
		in = EC_next(in);
	}
}


//Return true if b is control dependent on a.
bool CDG::is_cd(UINT a, UINT b)
{
	IS_TRUE0(get_vertex(b));
	VERTEX * v = get_vertex(a);
	IS_TRUE0(v != NULL);
	EDGE_C * out = VERTEX_out_list(v);
	while (out != NULL) {
		if (VERTEX_id(EDGE_to(EC_edge(out))) == b) {
			return true;
		}
		out = EC_next(out);
	}
	return false;
}


bool CDG::is_only_cd_self(UINT id)
{
	VERTEX * v = get_vertex(id);
	IS_TRUE0(v != NULL);
	EDGE_C * out = VERTEX_out_list(v);
	while (out != NULL) {
		VERTEX * succ = EDGE_to(EC_edge(out));
		if (succ != v) return false;
		out = EC_next(out);
	}
	return true;
}


void CDG::get_cd_succs(UINT id, OUT LIST<VERTEX*> & lst)
{
	VERTEX * v = get_vertex(id);
	IS_TRUE0(v != NULL);
	EDGE_C * out = VERTEX_out_list(v);
	while (out != NULL) {
		VERTEX * succ = EDGE_to(EC_edge(out));
		lst.append_tail(succ);
		out = EC_next(out);
	}
}


void CDG::rebuild(IN OUT OPT_CTX & oc, DGRAPH & cfg)
{
	erase();
	build(oc, cfg);
}


void CDG::build(IN OUT OPT_CTX & oc, DGRAPH & cfg)
{
	if (cfg.get_vertex_num() == 0) { return; }
	START_TIMER("CDG");
	IS_TRUE0(OPTC_is_cfg_valid(oc));
	m_ru->check_valid_and_recompute(&oc, OPT_PDOM, OPT_UNDEF);

	GRAPH pdom_tree;
	cfg.get_pdom_tree(pdom_tree);
	if (pdom_tree.get_vertex_num() == 0) { return; }

	SVECTOR<UINT> top_order;
	pdom_tree.sort_in_toplog_order(top_order, false);
	//dump_vec(top_order);

	BITSET_MGR bs_mgr;
	SVECTOR<BITSET*> cd_set;
	for (INT j = 0; j <= top_order.get_last_idx(); j++) {
		UINT ii = top_order.get(j);
		VERTEX * v = cfg.get_vertex(ii);
		IS_TRUE0(v != NULL);
		add_vertex(VERTEX_id(v));
		BITSET * cd_of_v = cd_set.get(VERTEX_id(v));
		if (cd_of_v == NULL) {
			cd_of_v = bs_mgr.create();
			cd_set.set(VERTEX_id(v), cd_of_v);
		}

		EDGE_C * in = VERTEX_in_list(v);
		while (in != NULL) {
			VERTEX * pred = EDGE_from(EC_edge(in));
			if (VERTEX_id(v) != ((DGRAPH&)cfg).get_ipdom(VERTEX_id(pred))) {
				cd_of_v->bunion(VERTEX_id(pred));
				//if (pred != v)
				{
					add_edge(VERTEX_id(pred), VERTEX_id(v));
				}
			}
			in = EC_next(in);
		}
		INT c;
		for (VERTEX * z = cfg.get_first_vertex(c);
			 z != NULL; z = cfg.get_next_vertex(c)) {
			if (((DGRAPH&)cfg).get_ipdom(VERTEX_id(z)) == VERTEX_id(v)) {
				BITSET * cd = cd_set.get(VERTEX_id(z));
				if (cd == NULL) {
					cd = bs_mgr.create();
					cd_set.set(VERTEX_id(z), cd);
				}
				for (INT i = cd->get_first(); i != -1; i = cd->get_next(i)) {
					if (VERTEX_id(v) != ((DGRAPH&)cfg).get_ipdom(i)) {
						cd_of_v->bunion(i);
						//if (i != (INT)VERTEX_id(v))
						{
							add_edge(i, VERTEX_id(v));
						}
					}
				}
			}
		}
	} //end for

	OPTC_is_cdg_valid(oc) = true;
	END_TIMER();
}
//END CDG
