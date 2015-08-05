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
#ifndef _IR_LICM_H_
#define _IR_LICM_H_

//Loop Invariant code Motion.
class IR_LICM : public IR_OPT {
protected:
	REGION * m_ru;
	IR_AA * m_aa;
	IR_DU_MGR * m_du;
	IR_CFG * m_cfg;
	CIR_ITER m_iriter;
	MD_ITER m_mditer;
	DT_MGR * m_dm;
	MD_SYS * m_md_sys;
	SMEM_POOL * m_pool;
	LIST<IR*> m_analysable_stmt_list;
	TMAP<MD const*, UINT*> m_md2num;

	bool do_loop_tree(LI<IR_BB> * li,
					OUT bool & du_set_info_changed,
					OUT bool & insert_bb,
					TTAB<IR*> & invariant_stmt,
					TTAB<IR*> & invariant_exp);

	bool is_dom_all_use_in_loop(IR const* ir, LI<IR_BB> * li);

	bool hoist_cand(TTAB<IR*> & invariant_exp,
					TTAB<IR*> & invariant_stmt,
					IN IR_BB * prehead,
					IN LI<IR_BB> * li);

	bool mark_exp_and_stmt(IR * ir, TTAB<IR*> & invariant_exp);

	bool scan_opnd(IN LI<IR_BB> * li,
					OUT TTAB<IR*> & invariant_exp,
					TTAB<IR*> & invariant_stmt,
					bool * is_legal, bool first_scan);
	bool scan_result(OUT TTAB<IR*> & invariant_stmt);
	bool stmt_can_be_hoisted(IR * stmt, TTAB<IR*> & invariant_stmt,
							LI<IR_BB> * li, IR_BB * backedge_bb);

	bool unique_def(MD const* md);
	void update_md2num(IR * ir);

	void * xmalloc(UINT size)
	{
		IS_TRUE0(m_pool != NULL);
		void * p = smpool_malloc_h_const_size(sizeof(UINT), m_pool);
		IS_TRUE0(p != NULL);
		memset(p, 0, size);
		return p;
	}
public:
	IR_LICM(REGION * ru)
	{
		IS_TRUE0(ru != NULL);
		m_ru = ru;
		m_aa = ru->get_aa();
		m_du = ru->get_du_mgr();
		m_cfg = ru->get_cfg();
		m_dm = ru->get_dm();
		m_md_sys = ru->get_md_sys();
		m_pool = smpool_create_handle(4 * sizeof(UINT), MEM_CONST_SIZE);
	}
	virtual ~IR_LICM() { smpool_free_handle(m_pool); }

	bool analysis(IN LI<IR_BB> * li,
				OUT TTAB<IR*> & invariant_stmt,
				OUT TTAB<IR*> & invariant_exp);

	//Given loop info li, dump the invariant stmt and invariant expression.
	void dump(LI<IR_BB> const* li,
			TTAB<IR*> const& invariant_stmt,
			TTAB<IR*> const& invariant_exp);

	//Consider whether exp is worth hoisting.
	bool is_worth_hoist(IR * exp)
	{
		IS_TRUE0(exp->is_exp());
		return true;
	}

	virtual CHAR const* get_opt_name() const
	{ return "Loop Invariant Code Motion"; }

	OPT_TYPE get_opt_type() const { return OPT_LICM; }

	virtual bool perform(OPT_CTX & oc);
};
#endif
