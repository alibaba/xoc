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
#ifndef __A_GRAPH_H_
#define __A_GRAPH_H_

//Alogrithmic Graph.
class AGRAPH : public GRAPH {
protected:
	MATRIX<UINT> * m_spath_mat; //record shortest-path.
	void build_adj_matrix(MATRIX<UINT> & adj_mat);
public:
	AGRAPH(UINT edge_hash_size = 47, UINT vex_hash_size = 47) :
		GRAPH(edge_hash_size, vex_hash_size)
	{
		m_edge_hash_size = edge_hash_size;
		m_vex_hash_size = vex_hash_size;
		m_spath_mat = NULL;
	}

	AGRAPH(AGRAPH & g) : GRAPH(g)
	{
		m_edge_hash_size = g.m_edge_hash_size;
		m_vex_hash_size = g.m_vex_hash_size;
		clone(g);
	}

	virtual ~AGRAPH()
	{
		if (m_spath_mat != NULL) {
			delete m_spath_mat;//Delete shortest path matrix
			m_spath_mat = NULL;
		}
	}

	bool clone(AGRAPH & src)
	{
		if (src.m_spath_mat != NULL) {
			if (m_spath_mat == NULL) {
				m_spath_mat = new MATRIX<UINT>(*src.m_spath_mat);
			} else {
				m_spath_mat->copy(*src.m_spath_mat);
			}
		} else {
			delete m_spath_mat;
			m_spath_mat = NULL;
		}
	}

	UINT count_mem() const
	{
		UINT count = GRAPH::count_mem();
		if (m_spath_mat != NULL) {
			count += m_spath_mat->count_mem();
		}
	}

	void shortest_path(UINT infinite);

	//Erasing graph, include all nodes and edges,
	//except for mempool, freelist.
	void erasure()
	{
		if (m_spath_mat != NULL) {
			delete m_spath_mat; //Delete shortest path matrix
			m_spath_mat = NULL;
		}
		GRAPH::erasure();
	}
};
#endif

