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
#ifndef __PASS_MGR_H__
#define __PASS_MGR_H__

//Time Info.
#define TI_pn(ti)		(ti)->pass_name
#define TI_pt(ti)		(ti)->pass_time
class TI {
public:
	CHAR const* pass_name;
	ULONGLONG pass_time;
};


class PASS_MGR {
protected:
	LIST<TI*> m_ti_list;
	SMEM_POOL * m_pool;
	REGION * m_ru;
	DT_MGR * m_dm;
	CDG * m_cdg;
	TMAP<OPT_TYPE, IR_OPT*> m_registered_opt;
	TMAP<OPT_TYPE, GRAPH*> m_registered_graph_based_opt;

	void * xmalloc(size_t size)
	{
		void * p = smpool_malloc_h(size, m_pool);
		if (p == NULL) return NULL;
		memset(p, 0, size);
		return p;
	}
	GRAPH * register_graph_based_opt(OPT_TYPE opty);
public:
	PASS_MGR(REGION * ru);
	virtual ~PASS_MGR()
	{
		destroy_opt();
		smpool_free_handle(m_pool);
	}

	void append_ti(CHAR const* pass_name, ULONGLONG t)
	{
		TI * ti = (TI*)xmalloc(sizeof(TI));
		TI_pn(ti) = pass_name;
		TI_pt(ti) = t;
		m_ti_list.append_tail(ti);
	}

	virtual GRAPH * alloc_cdg();
	virtual IR_OPT * alloc_cp();
	virtual IR_OPT * alloc_gcse();
	virtual IR_OPT * alloc_lcse();
	virtual IR_OPT * alloc_rp();
	virtual IR_OPT * alloc_pre();
	virtual IR_OPT * alloc_ivr();
	virtual IR_OPT * alloc_licm();
	virtual IR_OPT * alloc_dce();
	virtual IR_OPT * alloc_dse();
	virtual IR_OPT * alloc_rce();
	virtual IR_OPT * alloc_gvn();
	virtual IR_OPT * alloc_loop_cvt();
	virtual IR_OPT * alloc_ssa_opt_mgr();
	virtual IR_OPT * alloc_ccp();
	virtual IR_OPT * alloc_expr_tab();
	virtual IR_OPT * alloc_cfs_mgr();

	void destroy_opt();
	void dump_pass_time_info()
	{
		if (g_tfile == NULL) { return; }
		fprintf(g_tfile, "\n==---- PASS TIME INFO ----==");
		for (TI * ti = m_ti_list.get_head(); ti != NULL;
			 ti = m_ti_list.get_next()) {
			fprintf(g_tfile, "\n * %s --- use %llu ms ---",
					TI_pn(ti), TI_pt(ti));
		}
		fprintf(g_tfile, "\n===----------------------------------------===");
		fflush(g_tfile);
	}

	IR_OPT * register_opt(OPT_TYPE opty);

	IR_OPT * query_opt(OPT_TYPE opty)
	{
		if (opty == OPT_CDG) {
			return (IR_OPT*)m_registered_graph_based_opt.get(opty);
		}
		return m_registered_opt.get(opty);
	}

	virtual void perform_scalar_opt(OPT_CTX & oc);
};
#endif
