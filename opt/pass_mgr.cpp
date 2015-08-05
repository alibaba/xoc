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

PASS_MGR::PASS_MGR(REGION * ru)
{
	IS_TRUE0(ru);
	m_pool = smpool_create_handle(sizeof(TI) * 4, MEM_COMM);
	m_ru = ru;
	m_dm = ru->get_dm();
	IS_TRUE0(m_dm);
}


void PASS_MGR::destroy_opt()
{
	TMAP_ITER<OPT_TYPE, IR_OPT*> tabiter;
	IR_OPT * opt;
	for (m_registered_opt.get_first(tabiter, &opt);
		 opt != NULL; m_registered_opt.get_next(tabiter, &opt)) {
		delete opt;
	}

	GRAPH * opt2;
	TMAP_ITER<OPT_TYPE, GRAPH*> tabiter2;
	for (m_registered_graph_based_opt.get_first(tabiter2, &opt2);
		 opt2 != NULL; m_registered_graph_based_opt.get_next(tabiter2, &opt2)) {
		delete opt2;
	}
}


IR_OPT * PASS_MGR::alloc_cp()
{
	IR_OPT * opt = new IR_CP(m_ru);
	SIMP_CTX * simp = (SIMP_CTX*)xmalloc(sizeof(SIMP_CTX));
	simp->init();
	opt->set_simp_cont(simp);
	return opt;
}


IR_OPT * PASS_MGR::alloc_gcse()
{
	return new IR_GCSE(m_ru, (IR_GVN*)register_opt(OPT_GVN));
}


IR_OPT * PASS_MGR::alloc_lcse()
{
	return new IR_LCSE(m_ru);
}


IR_OPT * PASS_MGR::alloc_rp()
{
	return new IR_RP(m_ru, (IR_GVN*)register_opt(OPT_GVN));
}


IR_OPT * PASS_MGR::alloc_pre()
{
	//return new IR_PRE(m_ru);
	return NULL;
}


IR_OPT * PASS_MGR::alloc_ivr()
{
	return new IR_IVR(m_ru);
}


IR_OPT * PASS_MGR::alloc_licm()
{
	return new IR_LICM(m_ru);
}


IR_OPT * PASS_MGR::alloc_dce()
{
	return new IR_DCE(m_ru);
}


IR_OPT * PASS_MGR::alloc_dse()
{
	//return new IR_DSE(m_ru);
	return NULL;
}


IR_OPT * PASS_MGR::alloc_rce()
{
	return new IR_RCE(m_ru, (IR_GVN*)register_opt(OPT_GVN));
}


IR_OPT * PASS_MGR::alloc_gvn()
{
	return new IR_GVN(m_ru);
}


IR_OPT * PASS_MGR::alloc_loop_cvt()
{
	return new IR_LOOP_CVT(m_ru);
}


IR_OPT * PASS_MGR::alloc_ssa_opt_mgr()
{
	return new IR_SSA_MGR(m_ru, this);
}


GRAPH * PASS_MGR::alloc_cdg()
{
	return new CDG(m_ru);
}


IR_OPT * PASS_MGR::alloc_ccp()
{
	//return new IR_CCP(m_ru, (IR_SSA_MGR*)register_opt(OPT_SSA_MGR));
	return NULL;
}


IR_OPT * PASS_MGR::alloc_expr_tab()
{
	return new IR_EXPR_TAB(m_ru);
}


IR_OPT * PASS_MGR::alloc_cfs_mgr()
{
	return new CFS_MGR(m_ru);
}


GRAPH * PASS_MGR::register_graph_based_opt(OPT_TYPE opty)
{
	GRAPH * opt = NULL;
	switch (opty) {
	case OPT_CDG:
		opt = alloc_cdg();
		break;
    default: IS_TRUE(0, ("Unsupport Optimization."));
    }

	IS_TRUE0(opty != OPT_UNDEF && opt);
	m_registered_graph_based_opt.set(opty, opt);
    return opt;
}


IR_OPT * PASS_MGR::register_opt(OPT_TYPE opty)
{
	IR_OPT * opt = query_opt(opty);
	if (opt != NULL) { return opt; }

    switch (opty) {
    case OPT_CP:
		opt = alloc_cp();
		break;
	case OPT_GCSE:
		opt = alloc_gcse();
		break;
	case OPT_LCSE:
		opt = alloc_lcse();
		break;
	case OPT_RP:
		opt = alloc_rp();
		break;
	case OPT_PRE:
		opt = alloc_pre();
		break;
	case OPT_IVR:
		opt = alloc_ivr();
		break;
	case OPT_LICM:
		opt = alloc_licm();
		break;
	case OPT_DCE:
		opt = alloc_dce();
		break;
	case OPT_DSE:
		opt = alloc_dse();
		break;
	case OPT_RCE:
		opt = alloc_rce();
		break;
	case OPT_GVN:
		opt = alloc_gvn();
		break;
	case OPT_LOOP_CVT:
		opt = alloc_loop_cvt();
		break;
	case OPT_SSA_MGR:
		opt = alloc_ssa_opt_mgr();
		break;
	case OPT_CCP:
		opt = alloc_ccp();
		break;
	case OPT_CDG:
		return (IR_OPT*)register_graph_based_opt(opty);
	case OPT_EXPR_TAB:
		opt = alloc_expr_tab();
		break;
	case OPT_CFS_MGR:
		opt = alloc_cfs_mgr();
		break;
    default: IS_TRUE(0, ("Unsupport Optimization."));
    }

	IS_TRUE0(opty != OPT_UNDEF && opt);
	m_registered_opt.set(opty, opt);
    return opt;
}


void PASS_MGR::perform_scalar_opt(OPT_CTX & oc)
{
	TTAB<IR_OPT*> opt_tab;
	LIST<IR_OPT*> opt_list;
	SIMP_CTX simp;
	IR_GVN * gvn = NULL;
	if (g_do_gvn) { register_opt(OPT_GVN); }

	if (g_do_pre) {
		//Do PRE individually.
		//Since it will incur the opposite effect with Copy-Propagation.
		IR_OPT * pre = register_opt(OPT_PRE);
		pre->perform(oc);
		IS_TRUE0(verify_ir_and_bb(m_ru->get_bb_list(), m_dm));
	}

	if (g_do_dce) {
		IR_DCE * dce = (IR_DCE*)register_opt(OPT_DCE);
		opt_list.append_tail(dce);
		if (g_do_dce_aggressive) {
			dce->set_elim_cfs(true);
		}
	}

	opt_list.append_tail(register_opt(OPT_RP));
	opt_list.append_tail(register_opt(OPT_CP));
	opt_list.append_tail(register_opt(OPT_LICM));
	opt_list.append_tail(register_opt(OPT_DCE));
	opt_list.append_tail(register_opt(OPT_LOOP_CVT));

	if (g_do_cp) {
		IR_CP * opt = (IR_CP*)register_opt(OPT_CP);
		opt->set_prop_kind(CP_PROP_SIMPLEX);
		opt_list.append_tail(opt);
	}

	if (g_do_rp) {
		opt_list.append_tail(register_opt(OPT_RP));
	}

	if (g_do_gcse) {
		opt_list.append_tail(register_opt(OPT_GCSE));
	}

	if (g_do_lcse) {
		opt_list.append_tail(register_opt(OPT_LCSE));
	}

	if (g_do_pre) {
		opt_list.append_tail(register_opt(OPT_PRE));
	}

	if (g_do_rce) {
		opt_list.append_tail(register_opt(OPT_RCE));
	}

	if (g_do_dse) {
		opt_list.append_tail(register_opt(OPT_DSE));
	}

	if (g_do_licm) {
		opt_list.append_tail(register_opt(OPT_LICM));
	}

	if (g_do_ivr) {
		opt_list.append_tail(register_opt(OPT_IVR));
	}

	bool change;
	UINT count = 0;
	IR_DU_MGR * dumgr = m_ru->get_du_mgr();
	IR_AA * aa = m_ru->get_aa();
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	IR_CFG * cfg = m_ru->get_cfg();
	do {
		change = false;
		for (IR_OPT * opt = opt_list.get_head();
			 opt != NULL; opt = opt_list.get_next()) {
			CHAR const* optn = opt->get_opt_name();
			IS_TRUE0(verify_ir_and_bb(bbl, m_dm));
			ULONGLONG t = getusec();
			bool doit = opt->perform(oc);
			append_ti(optn, getusec() - t);
			if (doit) {
				change = true;
				IS_TRUE0(verify_ir_and_bb(bbl, m_dm));
				IS_TRUE0(cfg->verify());
			}
			RF_CTX rc;
			m_ru->refine_ir_bb_list(bbl, rc);
		}
		count++;
	} while (change && count < 20);
	IS_TRUE0(!change);

	if (g_do_lcse) {
		IR_LCSE * lcse = (IR_LCSE*)register_opt(OPT_LCSE);
		lcse->set_enable_filter(false);
		ULONGLONG t = getusec();
		lcse->perform(oc);
		t = getusec() - t;
		append_ti(lcse->get_opt_name(), t);
	}

	if (g_do_rp) {
		IR_RP * r = (IR_RP*)register_opt(OPT_RP);
		ULONGLONG t = getusec();
		r->perform(oc);
		append_ti(r->get_opt_name(), getusec() - t);
	}
}
