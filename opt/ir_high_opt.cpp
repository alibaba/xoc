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

/* Perform high-level optimizaitions.
Basis step to do:
	1. Build control flow graph.
	2. Compute POINT-TO info.
	3. Compute DEF-USE info.
	4. Compute Lived Expression info.

Optimizations to be performed:
	1. Auto Parallel
	2. Loop interchange
	3. Loop reverese(may be a little helpful)
	4. Loop tiling
	5. Loop fusion
	6. Loop unrolling */
bool REGION::high_process(OPT_CTX & oc)
{
	CHAR const* ru_name = get_ru_name();
	g_indent = 0;
	note("\n\n==== REGION:%s HIGHEST LEVEL FARMAT ====\n\n", get_ru_name());

	SIMP_CTX simp;
	if (g_do_cfs_opt) {
		IR_CFS_OPT co(this);
		co.perform(simp);
		IS_TRUE0(verify_irs(get_ir_list(), NULL, get_dm()));
	}

	if (g_build_cfs) {
		IS_TRUE0(OPTC_pass_mgr(oc));
		SIMP_is_record_cfs(&simp) = true;
		CFS_MGR * cfsmgr =
			(CFS_MGR*)OPTC_pass_mgr(oc)->register_opt(OPT_CFS_MGR);
		IS_TRUE0(cfsmgr);
		SIMP_cfs_mgr(&simp) = cfsmgr;
	}

	simp.set_simp_cf();
	set_ir_list(simplify_stmt_list(get_ir_list(), &simp));
	IS_TRUE0(verify_simp(get_ir_list(), simp));
	IS_TRUE0(verify_irs(get_ir_list(), NULL, get_dm()));

	if (g_cst_bb_list) {
		construct_ir_bb_list();
		IS_TRUE0(verify_ir_and_bb(get_bb_list(), get_dm()));
		set_ir_list(NULL); //All IRs have been moved to each IR_BB.
	}

	if (g_do_cfg) {
		IS_TRUE0(g_cst_bb_list);
		IR_CFG * cfg = init_cfg(oc);
		if (g_do_loop_ana) {
			cfg->loop_analysis(oc);
		}
	}

	if (g_do_aa) {
		IS_TRUE0(g_cst_bb_list && OPTC_is_cfg_valid(oc));
		init_aa(oc);
	}

	if (g_do_du_ana) {
		IS_TRUE0(g_cst_bb_list && OPTC_is_cfg_valid(oc) && OPTC_is_aa_valid(oc));
		init_du(oc);
	}

	if (g_do_expr_tab) {
		IS_TRUE0(g_cst_bb_list);
		IR_EXPR_TAB * exprtab =
			(IR_EXPR_TAB*)OPTC_pass_mgr(oc)->register_opt(OPT_EXPR_TAB);
		IS_TRUE0(exprtab);
		exprtab->perform(oc);
	}

	if (g_do_cdg && OPTC_pass_mgr(oc) != NULL) {
		IS_TRUE0(g_cst_bb_list && OPTC_is_cfg_valid(oc));
		CDG * cdg = (CDG*)OPTC_pass_mgr(oc)->register_opt(OPT_CDG);
		IS_TRUE0(cdg);
		cdg->build(oc, *get_cfg());
	}

	if (g_opt_level == NO_OPT) {
		return false;
	}

	/* Regenerate high level IR, and do high level optimizations.
	Now, I get one thing: We cannot or not very easy
	construct High Level Control IR,
	(IF,DO_LOOP,...) via analysing CFG.
		e.g:

			if (i > j) { //BB1
				...
			} else {
				return 2; //S1
			}
		BB1 does not have a ipdom, so we can not find the indispensible 3 parts:
			True body, False body, and the Sibling node.

	Solution: We can scan IF stmt first, in order to mark
	start stmt and end stmt of IF.

	//ABS_NODE * an = RU_ana(this)->m_cfs_mgr->construct_abstract_cfs();
	//Polyhedra optimization.
	//IR_POLY * poly = new_poly();
	//if (poly->construct_poly(an)) {
	//	poly->perform_poly_trans();
	//}
	//delete poly;
	*/
	return true;
}
