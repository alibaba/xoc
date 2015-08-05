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
// This file is distributed under the BSD License. See LICENSE.TXT for details.

#ifndef _IR_GCSE_H_
#define _IR_GCSE_H_

class TG : public DGRAPH {
	REGION * m_ru;
	virtual void * clone_edge_info(EDGE * e)
	{ return NULL; }
	virtual void * clone_vertex_info(VERTEX * v)
	{ return NULL; }
public:
	TG(REGION * ru) { m_ru = ru; }
	void pick_eh()
	{
		LIST<IR_BB*> * bbs = m_ru->get_bb_list();
		for (IR_BB * bb = bbs->get_head(); bb != NULL; bb = bbs->get_next()) {
			if (bb->is_exp_handling()) {
				remove_vertex(IR_BB_id(bb));
			}
		}
	}

	inline void compute_dom_idom()
	{
		if (!compute_dom()) { IS_TRUE0(0); }
		if (!compute_idom()) { IS_TRUE0(0); }
	}

	inline void compute_pdom_ipdom(VERTEX * root)
	{
		if (!compute_pdom_by_rpo(root, NULL)) { IS_TRUE0(0); }
		if (!compute_ipdom()) { IS_TRUE0(0); }
	}
};


class IR_GCSE : public IR_OPT {
protected:
	bool m_enable_filter; //filter determines which expression can be CSE.
	REGION * m_ru;
	IR_CFG * m_cfg;
	IR_DU_MGR * m_du;
	IR_AA * m_aa;
	IR_EXPR_TAB * m_expr_tab;
	DT_MGR * m_dm;
	IR_GVN * m_gvn;
	TG * m_tg;
	TMAP<IR*, IR*> m_exp2pr;
	TMAP<VN const*, IR*> m_vn2exp;
	LIST<IR*> m_newst_lst;

	bool do_prop(IR_BB * bb, LIST<IR*> & livexp);
	bool do_prop_vn(IR_BB * bb, UINT entry_id, MD_SET & tmp);
	bool elim(IR * use, IR * use_stmt, IR * gen, IR * gen_stmt);
	bool find_and_elim(IR * exp, VN const* vn, IR * gen);
	void handle_cand(IR * exp, IR_BB * bb, UINT entry_id, bool & change);
	bool is_cse_cand(IR * ir);
	void elim_cse_at_store(IR * use, IR * use_stmt, IR * gen);
	void elim_cse_at_call(IR * use, IR * use_stmt, IR * gen);
	void elim_cse_at_return(IR * use, IR * use_stmt, IR * gen);
	void elim_cse_at_br(IR * use, IR * use_stmt, IR * gen);
	void process_cse_gen(IR * cse, IR * cse_stmt, bool & change);
	bool process_cse(IR * ir, LIST<IR*> & livexp);
	bool should_be_cse(IR * det);
public:
	IR_GCSE(REGION * ru, IR_GVN * gvn)
	{
		IS_TRUE0(ru);
		m_ru = ru;
		m_cfg = ru->get_cfg();
		m_du = ru->get_du_mgr();
		m_aa = ru->get_aa();
		IS_TRUE0(m_du && m_aa);
		m_expr_tab = NULL;
		m_dm = ru->get_dm();
		m_gvn = gvn;
		m_tg = NULL;
	}
	virtual ~IR_GCSE() {}
	virtual CHAR const* get_opt_name() const
	{ return "Global Command Subscript Elimination"; }

	OPT_TYPE get_opt_type() const { return OPT_GCSE; }

	bool perform(OPT_CTX & oc);
};
#endif

