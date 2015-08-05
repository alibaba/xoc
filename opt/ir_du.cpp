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
#include "callg.h"
#include "prdf.h"
#include "prssainfo.h"
#include "ir_ssa.h"

//Set the interal data attribute to no sparse
//if you think the analysis objects are few enough,
//and no-sparse set may speed up compilation.
#define SOL_SET_IS_SPARSE	(true)

//Iterative methodology.
//#define WORK_LIST_DRIVE

#define CK_UNKNOWN		0 //Can not determine if memory is overlap.
#define CK_OVERLAP		1 //Can be confirmed memory is overlap.
#define CK_NOT_OVERLAP	2 //Can be confirmed memory is not overlap.

//
//START MDID2IRS
//
MDID2IRS::MDID2IRS(REGION * ru)
{
	m_ru = ru;
	m_md_sys = ru->get_md_sys();
	m_dm = ru->get_dm();
	m_du = ru->get_du_mgr();
	m_sbs_mgr = m_du->get_sbs_mgr();
	m_has_stmt_which_only_have_maydef = false;
}


MDID2IRS::~MDID2IRS()
{
	TMAP_ITER<UINT, SBITSETC*> c;
	SBITSETC * mapped = NULL;
	for (UINT mdid = get_first(c, &mapped);
		 mdid != MD_UNDEF; mdid = get_next(c, &mapped)) {
		IS_TRUE0(mapped);
		mapped->clean(*m_sbs_mgr);
		delete mapped;
	}

	m_global_md.clean(*m_sbs_mgr);
}


void MDID2IRS::clean()
{
	TMAP_ITER<UINT, SBITSETC*> c;
	SBITSETC * mapped = NULL;
	for (UINT mdid = get_first(c, &mapped);
		 mdid != MD_UNDEF; mdid = get_next(c, &mapped)) {
		IS_TRUE0(mapped);
		mapped->clean(*m_sbs_mgr);
	}

	m_global_md.clean(*m_sbs_mgr);
	m_has_stmt_which_only_have_maydef = false;

	//Do not clean SBITSET* here, it will incur memory leak.
	//TMAP<UINT, SBITSETC*>::clean();
}


//'md' corresponds to unique 'ir'.
void MDID2IRS::set(UINT mdid, IR * ir)
{
	IS_TRUE(mdid != MD_GLOBAL_MEM && mdid != MD_ALL_MEM,
			("there is not any md could kill Fake-May-MD."));
	IS_TRUE0(ir);
	SBITSETC * irtab = TMAP<UINT, SBITSETC*>::get(mdid);
	if (irtab == NULL) {
		irtab = new SBITSETC();
		TMAP<UINT, SBITSETC*>::set(mdid, irtab);
	} else {
		irtab->clean(*m_sbs_mgr);
	}
	irtab->bunion(IR_id(ir), *m_sbs_mgr);

	/* Enable folowed code to use append_to_global_md().
	MD const* md = m_md_sys->get_md(mdid);
	if (md->is_global()) {
		m_global_md.bunion(mdid, *m_sbs_mgr);
	}
	*/
}


void MDID2IRS::append_to_global_md(UINT irid)
{
	m_iter.clean();
	SC<SEG*> * sct;
	for (INT i = m_global_md.get_first(&sct);
		 i >= 0; i = m_global_md.get_next(i, &sct)) {
		IS_TRUE0(i != MD_GLOBAL_MEM && i != MD_ALL_MEM);
		SBITSETC * set = get(i);
		IS_TRUE0(set);
		set->bunion(irid, *m_sbs_mgr);
	}
}


void MDID2IRS::append_to_allmem_md(UINT irid)
{
	//NOTE: It will be disater if you use MD_ALL_MEM as mustdef or mustuse.
	//It will slow down significantly compiling speed.
	MDID2MD const* id2md = m_md_sys->get_id2md_map();
	for (INT i = 0; i <= id2md->get_last_idx(); i++) {
		if (i == MD_ALL_MEM) { continue; }

		MD * md = id2md->get(i);
		if (md == NULL) { continue; }

		SBITSETC * irlst = TMAP<UINT, SBITSETC*>::get(i);
		if (irlst == NULL) { continue; }

		irlst->bunion(irid, *m_sbs_mgr);
	}
}


//'md' corresponds to multiple 'ir'.
void MDID2IRS::append(UINT mdid, UINT irid)
{
	SBITSETC * irtab = TMAP<UINT, SBITSETC*>::get(mdid);
	if (irtab == NULL) {
		irtab = new SBITSETC();
		TMAP<UINT, SBITSETC*>::set(mdid, irtab);
	}
	irtab->bunion(irid, *m_sbs_mgr);

	/* Enable folowed code to use append_to_global_md().
	MD const* md = m_md_sys->get_md(mdid);
	if (md->is_global() && mdid != MD_GLOBAL_MEM && mdid != MD_ALL_MEM) {
		m_global_md.bunion(mdid, *m_sbs_mgr);
	}
	*/
}


void MDID2IRS::dump()
{
	if (g_tfile == NULL) { return; }
	m_md_sys->dump_all_mds();
	fprintf(g_tfile, "\n==-- DUMP MDID2IRLIST --==");
	TMAP_ITER<UINT, SBITSETC*> c;
	for (UINT mdid = get_first(c); mdid != MD_UNDEF; mdid = get_next(c)) {
		MD const * md = m_md_sys->get_md(mdid);
		md->dump();
		SBITSETC * irs = get(mdid);
		if (irs == NULL || irs->get_elem_count() == 0) { continue; }
		SC<SEG*> * sc;
		for (INT i = irs->get_first(&sc); i >= 0; i = irs->get_next(i, &sc)) {
			IR * d = m_ru->get_ir(i);
			g_indent = 4;
			dump_ir(d, m_dm, NULL, false, false, false);

			fprintf(g_tfile, "\n\t\tdef:");
			MD_SET const* ms = m_du->get_may_def(d);
			MD const* m = m_du->get_must_def(d);

			if (m != NULL) {
				fprintf(g_tfile, "MMD%d", MD_id(m));
			}

			if (ms != NULL) {
				for (INT j = ms->get_first(); j >= 0; j = ms->get_next(j)) {
					fprintf(g_tfile, " MD%d", j);
				}
			}
		}
	}
	fflush(g_tfile);
}
//END MDID2IRS


//
//START IR_DU_MGR
//
IR_DU_MGR::IR_DU_MGR(REGION * ru)
{
	m_ru = ru;
	m_dm = ru->get_dm();
	m_md_sys = ru->get_md_sys();
	m_aa = ru->get_aa();
	m_cfg = ru->get_cfg();
	m_mds_mgr = ru->get_mds_mgr();
	m_mds_hash = ru->get_mds_hash();
	m_sbs_mgr = ru->get_sbs_mgr();
	IS_TRUE0(m_aa && m_cfg && m_md_sys && m_dm);
	IR_BB_LIST * bbl = ru->get_bb_list();

	//zfor conservative purpose.
	m_is_pr_unique_for_same_no = RU_is_pr_unique_for_same_no(ru);

	//for conservative purpose.
	m_is_stmt_has_multi_result = false;

	m_is_compute_pr_du_chain = true;

	m_pool = smpool_create_handle(sizeof(DU_SET) * 2, MEM_COMM);

	m_is_init = NULL;
	m_md2irs = NULL;
}


IR_DU_MGR::~IR_DU_MGR()
{
	//Note you must ensure all ir DU_SET and MD_SET are freed back to
	//m_sbs_mgr before the destructor invoked.
	IS_TRUE0(m_is_init == NULL);
	IS_TRUE0(m_md2irs == NULL);

	IS_TRUE0(check_call_mds_in_hash());
	m_call2mayuse.clean();

	reset_local(false);
	reset_global(false);
	smpool_free_handle(m_pool);

	//Explicitly free SEG to SEG_MGR, or it
	//will complained during destruction.
	m_is_cached_mdset.clean(*m_sbs_mgr);
}


void IR_DU_MGR::destroy()
{
	IS_TRUE0(m_is_init == NULL);
	IS_TRUE0(m_md2irs == NULL);

	IS_TRUE0(check_call_mds_in_hash());
	m_call2mayuse.clean();

	free_duset_and_refmds();
	reset_local(false);
	reset_global(false);
	smpool_free_handle(m_pool);
	m_pool = NULL;
}


//Free DU_SET back to SEG_MGR, or it will complain and make an assertion.
void IR_DU_MGR::free_duset_and_refmds()
{
	SVECTOR<IR*> * vec = m_ru->get_ir_vec();
	INT l = vec->get_last_idx();
	for (INT i = 1; i <= l; i++) {
		IR * ir = vec->get(i);
		IS_TRUE0(ir);
		free_duset_and_refs(ir);
	}
}


//Free MD_SET related to IR_CALL.
bool IR_DU_MGR::check_call_mds_in_hash()
{
	TMAP_ITER<IR const*, MD_SET const*> iter;
	MD_SET const* mds = NULL;
	for (IR const* ir = m_call2mayuse.get_first(iter, &mds);
		 ir != NULL; ir = m_call2mayuse.get_next(iter, &mds)) {
		IS_TRUE0(m_mds_hash->find(*mds));
	}
	return true;
}


bool IR_DU_MGR::ir_is_stmt(UINT irid)
{
	return m_ru->get_ir(irid)->is_stmt();
}


bool IR_DU_MGR::ir_is_exp(UINT irid)
{
	return m_ru->get_ir(irid)->is_exp();
}


/* Compute the overlapping MD_SET that might overlap ones which 'ir' referred.
Then set the MD_SET to be ir's may-referred MD_SET.

e.g: int A[100], there are two referrence of array A: A[i], A[j]
	A[i] might overlap A[j].
recompute: true to compute overlapping MD_SET even if it has cached. */
void IR_DU_MGR::compute_overlap_use_mds(IR * ir, bool recompute)
{
	bool has_init = false;
	MD const* md = ir->get_ref_md();
	if (md != NULL) {
		if (MD_id(md) == MD_GLOBAL_MEM || MD_id(md) == MD_ALL_MEM) {
			return;
		}

		//Compute overlapped md set for must-ref.
		has_init = true;
		if (recompute || !m_is_cached_mdset.is_contain(MD_id(md))) {
			m_tmp_mds.clean();
			m_md_sys->compute_overlap(md, m_tmp_mds, m_tab_iter, true);

			MD_SET const* newmds = m_mds_hash->append(m_tmp_mds);
			if (newmds != NULL) {
				m_cached_overlap_mdset.set(md, newmds);
			}

			m_is_cached_mdset.bunion(MD_id(md), *m_sbs_mgr);
		} else {
			MD_SET const* hashed = m_cached_overlap_mdset.get(md);
			if (hashed != NULL) {
				m_tmp_mds.copy(*hashed);
			} else {
				m_tmp_mds.clean();
			}
		}
	}

	//Compute overlapped md set for may-ref, may-ref may contain several MDs.
	MD_SET const* mds = ir->get_ref_mds();
	if (mds != NULL) {
		if (!has_init) {
			has_init = true;
			m_tmp_mds.copy(*mds);
		} else {
			m_tmp_mds.bunion_pure(*mds);
		}

		m_md_sys->compute_overlap(*mds, m_tmp_mds, m_tab_iter, true);
	}

	if (!has_init || m_tmp_mds.is_empty()) {
		ir->clean_ref_mds();
		return;
	}

	MD_SET const* newmds = m_mds_hash->append(m_tmp_mds);
	IS_TRUE0(newmds);
	ir->set_ref_mds(newmds, m_ru);
}


IR * IR_DU_MGR::get_ir(UINT irid)
{
	return m_ru->get_ir(irid);
}


//Return IR stmt-id set.
DBITSETC * IR_DU_MGR::get_may_gen_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_may_gen_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_may_gen_def.set(bbid, set);
	}
	return set;
}


DBITSETC * IR_DU_MGR::get_must_gen_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_must_gen_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_must_gen_def.set(bbid, set);
	}
	return set;
}


DBITSETC * IR_DU_MGR::get_avail_in_reach_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_avail_in_reach_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_avail_in_reach_def.set(bbid, set);
	}
	return set;
}


DBITSETC * IR_DU_MGR::get_avail_out_reach_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_avail_out_reach_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_avail_out_reach_def.set(bbid, set);
	}
	return set;
}


DBITSETC * IR_DU_MGR::get_in_reach_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_in_reach_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_in_reach_def.set(bbid, set);
	}
	return set;
}


DBITSETC * IR_DU_MGR::get_out_reach_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_out_reach_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_out_reach_def.set(bbid, set);
	}
	return set;
}


DBITSETC * IR_DU_MGR::get_must_killed_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_must_killed_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_must_killed_def.set(bbid, set);
	}
	return set;
}


DBITSETC * IR_DU_MGR::get_may_killed_def(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_may_killed_def.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_may_killed_def.set(bbid, set);
	}
	return set;
}


//Return IR expression-id set.
DBITSETC * IR_DU_MGR::get_gen_ir_expr(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_gen_ir_expr.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_gen_ir_expr.set(bbid, set);
	}
	return set;
}


//Return IR expression-id set.
DBITSETC * IR_DU_MGR::get_killed_ir_expr(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_killed_ir_expr.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_killed_ir_expr.set(bbid, set);
	}
	return set;
}


//Return livein set for IR expression. Each element in the set is IR id.
DBITSETC * IR_DU_MGR::get_availin_expr(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_availin_ir_expr.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_availin_ir_expr.set(bbid, set);
	}
	return set;
}


//Return liveout set for IR expression. Each element in the set is IR id.
DBITSETC * IR_DU_MGR::get_availout_expr(UINT bbid)
{
	IS_TRUE0(m_ru->get_bb(bbid));
	DBITSETC * set = m_bb_availout_ir_expr.get(bbid);
	if (set == NULL) {
		set = m_sbs_mgr->create_dbitsetc();
		set->set_sparse(SOL_SET_IS_SPARSE);
		m_bb_availout_ir_expr.set(bbid, set);
	}
	return set;
}


//Allocate DU_SET for memory reference.
DU_SET * IR_DU_MGR::get_du_alloc(IR * ir)
{
	IS_TRUE0(ir->is_contain_mem_ref());
	DU * du = ir->get_du();
	if (du == NULL) {
		du = m_ru->alloc_du();
		ir->set_du(du);
	}

	DU_SET * dus = DU_duset(du);
	if (dus == NULL) {
		dus = (DU_SET*)m_sbs_mgr->create_sbitsetc();
		DU_duset(du) = dus;
	}
	return dus;
}


/* Return true if 'def_stmt' is the exact and unique reach-definition
to the operands of 'use_stmt', otherwise return false.

'def_stmt': should be stmt.
'use_stmt': should be stmt. */
bool IR_DU_MGR::is_exact_unique_def(IR const* def, IR const* exp)
{
	IS_TRUE0(def->is_stmt() && exp->is_exp());
	MD const* def_md = def->get_exact_ref();
	if (def_md == NULL) { return false; }
	IS_TRUE0(get_du_c(def)); //At least contains 'exp'.

	MD const* use_md = exp->get_exact_ref();
	DU_SET const* defset = get_du_c(exp);
	if (defset == NULL) { return false; }
	if (use_md == def_md && has_single_def_to_md(*defset, def_md)) {
		return true;
	}
	return false;
}


/* Find and return the exactly and unique stmt that defined 'exp',
otherwise return NULL.
'exp': should be exp, and it's use-md must be exact. */
IR const* IR_DU_MGR::get_exact_and_unique_def(IR const* exp)
{
	MD const* use_md = exp->get_exact_ref();
	if (use_md == NULL) { return NULL; }

	DU_SET const* defset = get_du_c(exp);
	if (defset == NULL) { return NULL; }

	DU_ITER di;
	INT d1 = defset->get_first(&di);
	INT d2 = defset->get_next(d1, &di);
	if (d1 < 0 || (d1 >=0 && d2 >= 0)) { return NULL; }

	IR const* d1ir = m_ru->get_ir(d1);
	if (d1ir->is_exact_def(use_md)) {
		return d1ir;
	}
	return NULL;
}


//Return true if there is only one stmt in 'defset' modify 'md'.
//Modification include both exact and inexact.
bool IR_DU_MGR::has_single_def_to_md(DU_SET const& defset, MD const* md) const
{
	UINT count = 0;
	IR_DU_MGR * pthis = const_cast<IR_DU_MGR*>(this);
	DU_ITER di;
	for (INT i = defset.get_first(&di); i >= 0; i = defset.get_next(i, &di)) {
		IR * def = m_ru->get_ir(i);
		if (pthis->get_must_def(def) == md) {
			count++;
		} else {
			MD_SET const* def_mds = pthis->get_may_def(def);
			if (def_mds == NULL) { continue; }
			if (def_mds->is_contain(md)) {
				count++;
			}
		}
	}
	return count == 1;
}


//Return true if 'def1' exactly modified md that 'def2' generated.
//'def1': should be stmt.
//'def2': should be stmt.
bool IR_DU_MGR::is_must_kill(IR const* def1, IR const* def2)
{
	IS_TRUE0(def1->is_stmt() && def2->is_stmt());
	if (def1->is_stpr() && def2->is_stpr()) {
		return STPR_no(def1) == STPR_no(def2);
	}

	MD const* md1 = def1->get_exact_ref();
	MD const* md2 = def2->get_exact_ref();
	if (md1 != NULL && md2 != NULL && md1 == md2)  {
		return true;
	}
	return false;
}


bool IR_DU_MGR::is_stpr_may_def(IR const* def, IR const* use, bool is_recur)
{
	IS_TRUE0(def->is_stpr());
	UINT prno = STPR_no(def);
	MD_SET const* maydef = get_may_def(def);
	if (is_recur) {
		m_citer.clean();
		for (IR const* x = ir_iter_init_c(use, m_citer);
			 x != NULL; x = ir_iter_next_c(m_citer)) {
			if (!x->is_pr()) { continue; }
			if (PR_no(x) == prno) { return true; }
			if (!is_pr_unique_for_same_no() && maydef != NULL) {
				MD_SET const* mayuse = get_may_use(x);
				if (mayuse != NULL &&
					(mayuse == maydef ||
					 mayuse->is_intersect(*maydef))) {
					return true;
				}
			}
		}
	} else {
		if (!use->is_pr()) { return false; }
		if (PR_no(use) == prno) { return true; }
		if (!is_pr_unique_for_same_no() && maydef != NULL) {
			MD_SET const* mayuse = get_may_use(use);
			if (mayuse != NULL &&
				(mayuse == maydef ||
				 mayuse->is_intersect(*maydef))) {
				IS_TRUE0(mayuse->is_pr_set(m_md_sys) &&
						 maydef->is_pr_set(m_md_sys));
				return true;
			}
		}
	}
	return false;
}


static bool is_call_may_def_core(IR const* call, IR const* use,
								MD_SET const* call_maydef)
{
	//MD of use may be exact or inexact.
	MD const* use_md = use->get_effect_ref();
	MD_SET const* use_mds = use->get_ref_mds();
	if (call->is_exact_def(use_md, use_mds)) {
		return true;
	}

	if (call_maydef != NULL) {
		if (use_md != NULL && call_maydef->is_contain(use_md)) {
			return true;
		}

		if (use_mds != NULL &&
			(use_mds == call_maydef || call_maydef->is_intersect(*use_mds))) {
			return true;
		}
	}
	return false;
}


bool IR_DU_MGR::is_call_may_def(IR const* call, IR const* use, bool is_recur)
{
	IS_TRUE0(call->is_call());

	//MayDef of stmt must involved the overlapped MD with Must Reference,
	//but except Must Reference itself.
	MD_SET const* call_maydef = get_may_def(call);
	if (is_recur) {
		m_citer.clean();
		for (IR const* x = ir_iter_init_c(use, m_citer);
			 x != NULL; x = ir_iter_next_c(m_citer)) {
			if (!x->is_memory_opnd()) { continue; }

			if (is_call_may_def_core(call, x, call_maydef)) {
				return true;
			}
		}
		return false;
	}

	IS_TRUE0(use->is_memory_opnd());
	return is_call_may_def_core(call, use, call_maydef);
}


/* Return true if 'def' may or must modify md-set that 'use' referenced.
'def': must be stmt.
'use': must be expression.
'is_recur': true if one intend to compute the mayuse MD_SET to walk
	through IR tree recusively. */
bool IR_DU_MGR::is_may_def(IR const* def, IR const* use, bool is_recur)
{
	IS_TRUE0(def->is_stmt() && use->is_exp());
	if (def->is_stpr()) {
		return is_stpr_may_def(def, use, is_recur);
	}
	if (def->is_call()) {
		return is_call_may_def(def, use, is_recur);
	}

	MD const* mustdef = get_must_def(def);
	MD_SET const* maydef = get_may_def(def);
	if (is_recur) {
		m_citer.clean();
		for (IR const* x = ir_iter_init_c(use, m_citer);
			 x != NULL; x = ir_iter_next_c(m_citer)) {
			if (!x->is_memory_opnd()) { continue; }

			MD const* mustuse = get_effect_use_md(x);
			if (mustuse != NULL) {
				if ((mustdef != NULL && mustdef == mustuse) ||
					(maydef != NULL && maydef->is_contain(mustuse))) {
					return true;
				}
			}

			MD_SET const* mayuse = get_may_use(x);
			if (mayuse != NULL) {
				if ((mustdef != NULL && mayuse->is_contain(mustdef)) ||
					(maydef != NULL &&
					 (maydef == mayuse || mayuse->is_intersect(*maydef)))) {
					return true;
				}
			}
		}
		return false;
	}

	MD const* mustuse = get_effect_use_md(use);
	if (mustuse != NULL) {
		if ((mustdef != NULL && mustdef == mustuse) ||
			(maydef != NULL && maydef->is_contain(mustuse))) {
			return true;
		}
	}

	MD_SET const* mayuse = get_may_use(use);
	if (mayuse != NULL) {
		if ((mustdef != NULL && mayuse->is_contain(mustdef)) ||
			(maydef != NULL && mayuse->is_intersect(*maydef))) {
			return true;
		}
	}
	return false;
}


//Return true if 'def1' may modify md-set that 'def2' generated.
//'def1': should be stmt.
//'def2': should be stmt.
bool IR_DU_MGR::is_may_kill(IR const* def1, IR const* def2)
{
	IS_TRUE0(def1->is_stmt() && def2->is_stmt());
	if (def1->is_stpr() && def2->is_stpr()) {
		if (STPR_no(def1) == STPR_no(def2)) {
			return true;
		}

		MD_SET const* maydef1;
		MD_SET const* maydef2;
		if (!is_pr_unique_for_same_no() &&
			(maydef1 = def1->get_ref_mds()) != NULL &&
			(maydef2 = def2->get_ref_mds()) != NULL) {
			return maydef1->is_intersect(*maydef2);
		}
	}

	MD const* md1 = get_must_def(def1);
	MD_SET const* mds1 = get_may_def(def1);
	MD const* md2 = get_must_def(def2);
	MD_SET const* mds2 = get_may_def(def2);

	if (md1 != NULL) {
		if (md2 != NULL && md1 == md2) {
			return true;
		}
		if (mds2 != NULL && (mds1 == mds2 || mds2->is_contain(md1))) {
			return true;
		}
		return false;
	}

	if (mds1 != NULL) {
		if (md2 != NULL && mds1->is_contain(md2)) {
			return true;
		}
		if (mds2 != NULL && (mds2 == mds1 || mds1->is_intersect(*mds2))) {
			return true;
		}
	}
	return false;
}


//Set/Clean call site mayuse MD_SET.
void IR_DU_MGR::set_map_call2mayuse(IR const* ir, MD_SET const* mds)
{
	IS_TRUE0(ir->is_call());
	IS_TRUE0(m_mds_hash->find(*mds));
	m_call2mayuse.aset(ir, mds);
}


void IR_DU_MGR::collect_must_use_for_lda(IR const* lda, OUT MD_SET * ret_mds)
{
	IR const* ldabase = LDA_base(lda);
	if (IR_type(ldabase) == IR_ID || ldabase->is_str(m_dm)) {
		return;
	}
	IS_TRUE0(IR_type(ldabase) == IR_ARRAY);
	IR const* arr = ldabase;
	for (IR const* s = ARR_sub_list(arr); s != NULL; s = IR_next(s)) {
		collect_must_use(s, *ret_mds);
	}

	//Process array base exp.
	if (IR_type(ARR_base(arr)) == IR_LDA) {
		collect_must_use_for_lda(ARR_base(arr), ret_mds);
		return;
	}
	collect_must_use(ARR_base(arr), *ret_mds);
}


void IR_DU_MGR::comp_array_lhs_ref(IR * ir)
{
	IS_TRUE0(IR_type(ir) == IR_ARRAY);
	MD const* t = ir->get_ref_md();
	MD_SET const* ts = ir->get_ref_mds();

	//For conservative, ist may reference any where.
	IS_TRUE0((ts != NULL && !ts->is_empty()) || t);
	compute_overlap_use_mds(ir, false);

	//Compute referred MDs to subdimension.
	for (IR * s = ARR_sub_list(ir); s != NULL; s = IR_next(s)) {
		compute_exp(s, NULL, COMP_EXP_RECOMPUTE);
	}
	compute_exp(ARR_base(ir), NULL, COMP_EXP_RECOMPUTE);
}


/* Walk through IR tree to collect referrenced MD.
'ret_mds': In COMP_EXP_RECOMPUTE mode, it is used as tmp;
	and in COMP_EXP_COLLECT_MUST_USE mode, it is
	used to collect MUST-USE MD. */
void IR_DU_MGR::compute_exp(IR * ir, OUT MD_SET * ret_mds, UINT flag)
{
	if (ir == NULL) { return; }
	IS_TRUE0(ir->is_exp());
	switch (IR_type(ir)) {
	case IR_CONST: break;
	case IR_ID:
		if (HAVE_FLAG(flag, COMP_EXP_RECOMPUTE)) {
			//The result of ref info should be avaiable.

			//We do not need MD or MDSET information of IR_ID.
			//IS_TRUE0(ir->get_ref_md());

			IS_TRUE0(ir->get_ref_mds() == NULL);
		} else if (HAVE_FLAG(flag, COMP_EXP_COLLECT_MUST_USE)) {
			IS_TRUE(0, ("should not be here."));
		}
		break;
	case IR_LD:
		if (HAVE_FLAG(flag, COMP_EXP_RECOMPUTE)) {
			//The result of ref info should be avaiable.
			IS_TRUE0(ir->get_ref_md());
			IS_TRUE0(ir->get_ref_mds() == NULL);

			/* e.g: struct {int a;} s;
			s = ...
			s.a = ...
			Where s and s.a is overlapped. */
			compute_overlap_use_mds(ir, false);
		} else if (HAVE_FLAG(flag, COMP_EXP_COLLECT_MUST_USE)) {
			MD const* t = ir->get_ref_md();
			IS_TRUE0(t && t->is_exact());
			ret_mds->bunion_pure(MD_id(t));
		}
		break;
	case IR_ILD:
		if (HAVE_FLAG(flag, COMP_EXP_RECOMPUTE)) {
			/* Sideeffect information should have been computed by AA.
			e.g: ... = ild(ld(p)) //p->a, p->b
			mayref of ild is: {a,b}, and mustref is NULL.
			mustref of ld is: {p}, and mayref is NULL */
			compute_exp(ILD_base(ir), ret_mds, flag);
			compute_overlap_use_mds(ir, false);
		} else if (HAVE_FLAG(flag, COMP_EXP_COLLECT_MUST_USE)) {
			MD * use;
			MD_SET const* use_mds = ir->get_ref_mds();
			if (use_mds != NULL &&
				(use = use_mds->get_exact_md(m_md_sys)) != NULL) {
				ret_mds->bunion(use);
			}
			compute_exp(ILD_base(ir), ret_mds, flag);
		}
		break;
	case IR_LDA:
		if (HAVE_FLAG(flag, COMP_EXP_RECOMPUTE)) {
			/* LDA do NOT reference MD itself.
			e.g: p=&a; the stmt do not reference MD 'a',
			just only reference a's address. */
			compute_exp(LDA_base(ir), ret_mds, flag);
		} else if (HAVE_FLAG(flag, COMP_EXP_COLLECT_MUST_USE)) {
			collect_must_use_for_lda(ir, ret_mds);
		}
		break;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		//Binary operation.
		compute_exp(BIN_opnd0(ir), ret_mds, flag);
		compute_exp(BIN_opnd1(ir), ret_mds, flag);
		IS_TRUE0(ir->get_du() == NULL);
		break;
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
		compute_exp(UNA_opnd0(ir), ret_mds, flag);
		IS_TRUE0(ir->get_du() == NULL);
		break;
	case IR_LABEL:
		break;
	case IR_ARRAY:
		if (HAVE_FLAG(flag, COMP_EXP_RECOMPUTE)) {
			compute_overlap_use_mds(ir, false);

			//Compute referred MDs to subdimension.
			for (IR * s = ARR_sub_list(ir); s != NULL; s = IR_next(s)) {
				compute_exp(s, ret_mds, flag);
			}
			compute_exp(ARR_base(ir), ret_mds, flag);
		} else if (HAVE_FLAG(flag, COMP_EXP_COLLECT_MUST_USE)) {
			for (IR * s = ARR_sub_list(ir); s != NULL; s = IR_next(s)) {
				compute_exp(s, ret_mds, flag);
			}
			compute_exp(ARR_base(ir), ret_mds, flag);

			//USE-MDs may be NULL, if array base is NOT an LDA(ID).
			//e.g: given (*p)[1], p is pointer that point to an array.
			MD const* use = ir->get_exact_ref();
			if (use != NULL) {
				ret_mds->bunion(use);
			}
		}
		break;
	case IR_CVT:
		compute_exp(CVT_exp(ir), ret_mds, flag);
		break;
	case IR_PR:
		if (HAVE_FLAG(flag, COMP_EXP_RECOMPUTE)) {
			/* e.g: PR1(U8) = ...
				     ... = PR(U32)
			PR1(U8) is correspond to MD7,
			PR1(U32) is correspond to MD9, through MD7 and MD9 are overlapped.
			*/
			if (!is_pr_unique_for_same_no()) {
				compute_overlap_use_mds(ir, false);
			}
		} else if (HAVE_FLAG(flag, COMP_EXP_COLLECT_MUST_USE)) {
			MD const* t = ir->get_ref_md();
			IS_TRUE0(t && t->is_exact());
			ret_mds->bunion(t);
		}
		break;
	case IR_SELECT:
		compute_exp(SELECT_det(ir), ret_mds, flag);
		compute_exp(SELECT_trueexp(ir), ret_mds, flag);
		compute_exp(SELECT_falseexp(ir), ret_mds, flag);
		break;
	default: IS_TRUE0(0);
	}
}


void IR_DU_MGR::dump_du_graph(CHAR const* name, bool detail)
{
	if (g_tfile == NULL) { return; }
	GRAPH dug;
	dug.set_unique(false);

	class _EDGEINFO {
	public:
		int id;
	};

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {

			dug.add_vertex(IR_id(ir));
			DU_SET const* useset = get_du_c(ir);
			if (useset == NULL) { continue; }

			DU_ITER di;
			for (INT i = useset->get_first(&di);
				 i >= 0; i = useset->get_next(i, &di)) {
				IR * u = m_ru->get_ir(i);
				IS_TRUE0(u->is_exp());
				IR const* ustmt = u->get_stmt();
				EDGE * e = dug.add_edge(IR_id(ir), IR_id(ustmt));
				_EDGEINFO * ei = (_EDGEINFO*)xmalloc(sizeof(_EDGEINFO));
				ei->id = i;
				EDGE_info(e) = ei;
			}
		}
	}

	if (name == NULL) {
		name = "graph_du.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h != NULL, ("%s create failed!!!",name));

	FILE * old;
	//Print comment
	//fprintf(h, "\n/*");
	//old = g_tfile;
	//g_tfile = h;
	//dump_bbs(m_ru->get_bb_list());
	//g_tfile = old;
	//fprintf(h, "\n*/\n");

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
	old = g_tfile;
	g_tfile = h;
	g_indent = 0;
	INT c;
	for (VERTEX * v = dug.get_first_vertex(c);
		 v != NULL; v = dug.get_next_vertex(c)) {
		INT id = VERTEX_id(v);
		if (detail) {
			fprintf(h, "\nnode: { title:\"%d\" shape:box color:gold "
						"fontname:\"courB\" label:\"", id);
			IR * ir = m_ru->get_ir(id);
			IS_TRUE0(ir != NULL);
			dump_ir(ir, m_dm);
			fprintf(h, "\"}");
		} else {
			fprintf(h, "\nnode: { title:\"%d\" shape:box color:gold "
						"fontname:\"courB\" label:\"%d\" }", id, id);
		}
	}
	g_tfile = old;

	//Print edge
	for (EDGE * e = dug.get_first_edge(c);
		 e != NULL; e = dug.get_next_edge(c)) {
		VERTEX * from = EDGE_from(e);
		VERTEX * to = EDGE_to(e);
		_EDGEINFO * ei = (_EDGEINFO*)EDGE_info(e);
		IS_TRUE0(ei);
		fprintf(h,
			"\nedge: { sourcename:\"%d\" targetname:\"%d\" label:\"id%d\"}",
			VERTEX_id(from), VERTEX_id(to),  ei->id);
	}
	fprintf(h, "\n}\n");
	fclose(h);
}


//Dump mem usage for each ir DU reference.
void IR_DU_MGR::dump_mem_usage_for_ir_du_ref()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile,
		"\n\n==---- DUMP '%s' IR_DU_MGR : Memory Usage for DU Reference ----==",
		m_ru->get_ru_name());

	fprintf(g_tfile, "\nMustD: must defined MD");
	fprintf(g_tfile, "\nMayDs: overlapped and may defined MD_SET");

	fprintf(g_tfile, "\nMustU: must used MD");

	fprintf(g_tfile, "\nMayUs: overlapped and may used MD_SET");
	IR_BB_LIST * bbs = m_ru->get_bb_list();
	UINT count = 0;
	CHAR const* str = NULL;
	g_indent = 0;
	for (IR_BB * bb = bbs->get_head(); bb != NULL; bb = bbs->get_next()) {
		fprintf(g_tfile, "\n--- BB%d ---", IR_BB_id(bb));
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			dump_ir(ir, m_dm);
			fprintf(g_tfile, "\n");
			m_citer.clean();
			for (IR const* x = ir_iter_init_c(ir, m_citer);
				 x != NULL; x = ir_iter_next_c(m_citer)) {
				fprintf(g_tfile, "\n\t%s(%d)::", IRNAME(x), IR_id(x));
				if (x->is_stmt()) {
					//MustDef
					MD const* md = get_must_def(x);
					if (md != NULL) {
						fprintf(g_tfile, "MustD%dB, ", (INT)sizeof(MD));
						count += sizeof(MD);
					}

					//MayDef
					MD_SET const* mds = get_may_def(x);
					if (mds != NULL) {
						UINT n = mds->count_mem();
						if (n < 1024) { str = "B"; }
						else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
						else  { n /= 1024*1024; str = "MB"; }
						fprintf(g_tfile, "MayDs(%d%s, %d elems, last %d), ",
										 n, str,
										 mds->get_elem_count(),
										 mds->get_last());
						count += n;
					}
				} else {
					//MustUse
					MD const* md = get_effect_use_md(x);
					if (md != NULL) {
						fprintf(g_tfile, "MustU%dB, ", (INT)sizeof(MD));
						count += sizeof(MD);
					}

					//MayUse
					MD_SET const* mds = get_may_use(x);
					if (mds != NULL) {
						UINT n = mds->count_mem();
						if (n < 1024) { str = "B"; }
						else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
						else { n /= 1024*1024; str = "MB"; }
						fprintf(g_tfile, "MayUs(%d%s, %d elems, last %d), ",
										n, str,
										mds->get_elem_count(),
										mds->get_last());
						count += n;
					}
				}
			}
	 	}
	}

	if (count < 1024) { str = "B"; }
	else if (count < 1024 * 1024) { count /= 1024; str = "KB"; }
	else  { count /= 1024*1024; str = "MB"; }
	fprintf(g_tfile, "\nTotal %d%s", count, str);
	fflush(g_tfile);
}


//Dump mem usage for each internal set of bb.
void IR_DU_MGR::dump_mem_usage_for_set()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile,
		"\n==---- DUMP '%s' IR_DU_MGR : Memory Usage for Value Set of BB ----==",
		m_ru->get_ru_name());

	IR_BB_LIST * bbs = m_ru->get_bb_list();
	UINT count = 0;
	CHAR const* str = NULL;
	for (IR_BB * bb = bbs->get_head(); bb != NULL; bb = bbs->get_next()) {
		fprintf(g_tfile, "\n--- BB%d ---", IR_BB_id(bb));
		UINT n;
		SC<SEG*> * st;
		DBITSETC * irs = m_bb_avail_in_reach_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tAvaInReachDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		irs = m_bb_avail_out_reach_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tAvaOutReachDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		irs = m_bb_in_reach_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tInReachDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		irs = m_bb_out_reach_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tOutReachDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		irs = m_bb_may_gen_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tMayGenDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		irs = m_bb_must_gen_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tMustGenDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		irs = m_bb_may_killed_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tMayKilledDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		irs = m_bb_must_killed_def.get(IR_BB_id(bb));
		if (irs != NULL) {
			n = irs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tMustKilledDef:%d%s, %d elems, last %d",
							 n, str, irs->get_elem_count(), irs->get_last(&st));
		}

		//
		DBITSETC * bs = m_bb_gen_ir_expr.get(IR_BB_id(bb));
		if (bs != NULL) {
			n = bs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tMayKilledDef:%d%s, %d elems, last %d",
							 n, str, bs->get_elem_count(), bs->get_last(&st));
		}

		bs = m_bb_killed_ir_expr.get(IR_BB_id(bb));
		if (bs != NULL) {
			n = bs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tKilledIrExp:%d%s, %d elems, last %d",
							 n, str, bs->get_elem_count(), bs->get_last(&st));
		}

		bs = m_bb_availin_ir_expr.get(IR_BB_id(bb));
		if (bs != NULL) {
			n = bs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tLiveInIrExp:%d%s, %d elems, last %d",
							 n, str, bs->get_elem_count(), bs->get_last(&st));
		}

		bs = m_bb_availout_ir_expr.get(IR_BB_id(bb));
		if (bs != NULL) {
			n = bs->count_mem();
			count += n;
			if (n < 1024) { str = "B"; }
			else if (n < 1024 * 1024) { n /= 1024; str = "KB"; }
			else  { n /= 1024*1024; str = "MB"; }
			fprintf(g_tfile, "\n\tLiveOutIrExp:%d%s, %d elems, last %d",
							 n, str, bs->get_elem_count(), bs->get_last(&st));
		}
	}

	if (count < 1024) { str = "B"; }
	else if (count < 1024 * 1024) { count /= 1024; str = "KB"; }
	else  { count /= 1024*1024; str = "MB"; }
	fprintf(g_tfile, "\nTotal %d%s", count, str);
	fflush(g_tfile);
}



//Dump REGION's IR BB list.
//DUMP ALL IR_BB_LIST DEF/USE/OVERLAP_DEF/OVERLAP_USE"
void IR_DU_MGR::dump_ref(UINT indent)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n\n==---- DUMP '%s' IR_DU_MGR : DU Reference ----==\n",
			m_ru->get_ru_name());
	IR_BB_LIST * bbs = m_ru->get_bb_list();
	IS_TRUE0(bbs);
	if (bbs->get_elem_count() != 0) {
		m_md_sys->dump_all_mds();
	}
	for (IR_BB * bb = bbs->get_head(); bb != NULL; bb = bbs->get_next()) {
		fprintf(g_tfile, "\n--- BB%d ---", IR_BB_id(bb));
		dump_bb_ref(bb, indent);
	}
}


void IR_DU_MGR::dump_bb_ref(IN IR_BB * bb, UINT indent)
{
	for (IR * ir = IR_BB_first_ir(bb); ir != NULL; ir = IR_BB_next_ir(bb)) {
		dump_ir_ref(ir, indent);
	}
}


void IR_DU_MGR::dump_ir_list_ref(IN IR * ir, UINT indent)
{
	for (; ir != NULL; ir = IR_next(ir)) {
		dump_ir_ref(ir, indent);
	}
}


//Dump IR tree's MD reference, where ir may be stmt or exp.
void IR_DU_MGR::dump_ir_ref(IN IR * ir, UINT indent)
{
	if (ir == NULL || g_tfile == NULL || IR_is_const(ir)) return;
	g_indent = indent;
	dump_ir(ir, m_dm, NULL, false);

	//Dump mustref MD.
	MD const* md = ir->get_ref_md();
	MD_SET const* mds = ir->get_ref_mds();

	//MustDef
	bool prt_mustdef = false;
	if (md != NULL) {
		fprintf(g_tfile, "\n");
		note("");
		fprintf(g_tfile, "MMD%d", MD_id(md));
		prt_mustdef = true;
	}

	if (mds != NULL) {
		//MayDef
		if (!prt_mustdef) {
			note("\n"); //dump indent blank.
		}
		fprintf(g_tfile, " : ");
		if (mds != NULL && !mds->is_empty()) {
			mds->dump(m_md_sys);
		}
	}

	if (ir->is_call()) {
		bool doit = false;
		CALLG * callg = m_ru->get_ru_mgr()->get_callg();
		if (callg != NULL) {
			REGION * ru = callg->map_ir2ru(ir);
			if (ru != NULL && RU_is_du_valid(ru)) {
				MD_SET const* muse = ru->get_may_use();
				//May use
				fprintf(g_tfile, " <-- ");
				if (muse != NULL && !muse->is_empty()) {
					muse->dump(m_md_sys);
					doit = true;
				}
			}
		}
		if (!doit) {
			//May use
			fprintf(g_tfile, " <-- ");
			MD_SET const* x = map_call2mayuse(ir);
			if (x != NULL && !x->is_empty()) {
				x->dump(m_md_sys);
			}
		}
	}

	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		if (ir->get_kid(i) != NULL) {
			dump_ir_list_ref(ir->get_kid(i), indent + 4);
		}
	}
	fflush(g_tfile);
}


void IR_DU_MGR::dump_bb_du_chain2(UINT bbid)
{
	dump_bb_du_chain2(m_ru->get_bb(bbid));
}


void IR_DU_MGR::dump_bb_du_chain2(IR_BB * bb)
{
	IS_TRUE0(bb);
	fprintf(g_tfile, "\n--- BB%d ---", IR_BB_id(bb));

	//Label Info list.
	LABEL_INFO * li = IR_BB_lab_list(bb).get_head();
	if (li != NULL) {
		fprintf(g_tfile, "\nLABEL:");
	}
	for (; li != NULL; li = IR_BB_lab_list(bb).get_next()) {
		switch (LABEL_INFO_type(li)) {
		case L_CLABEL:
			note(CLABEL_STR_FORMAT, CLABEL_CONT(li));
			break;
		case L_ILABEL:
			note(ILABEL_STR_FORMAT, ILABEL_CONT(li));
			break;
		case L_PRAGMA:
			note("%s", LABEL_INFO_pragma(li));
			break;
		default: IS_TRUE(0,("unsupport"));
		}
		if (LABEL_INFO_is_try_start(li) ||
			LABEL_INFO_is_try_end(li) ||
			LABEL_INFO_is_catch_start(li)) {
			fprintf(g_tfile, "(");
			if (LABEL_INFO_is_try_start(li)) {
				fprintf(g_tfile, "try_start,");
			}
			if (LABEL_INFO_is_try_end(li)) {
				fprintf(g_tfile, "try_end,");
			}
			if (LABEL_INFO_is_catch_start(li)) {
				fprintf(g_tfile, "catch_start");
			}
			fprintf(g_tfile, ")");
		}
		fprintf(g_tfile, " ");
	}

	for (IR * ir = IR_BB_ir_list(bb).get_head();
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next()) {
		dump_ir(ir, m_dm);
		fprintf(g_tfile, "\n");

		IR_ITER ii;
		for (IR * k = ir_iter_init(ir, ii);
			k != NULL; k = ir_iter_next(ii)) {
			if (!k->is_memory_ref() &&
				!k->has_result() &&
				IR_type(k) != IR_REGION) {
				continue;
			}

			fprintf(g_tfile, "\n\t>>%s", IRNAME(k));
			if (k->is_stpr() || k->is_pr()) {
				fprintf(g_tfile, "%d", k->get_prno());
			}
			fprintf(g_tfile, "(id:%d) ", IR_id(k));

			if (k->is_stmt()) {
				fprintf(g_tfile, "Dref:");
			} else {
				fprintf(g_tfile, "Uref:");
			}

			//Dump must ref.
			MD const* md = k->get_ref_md();
			if (md != NULL) {
				fprintf(g_tfile, "MMD%d", MD_id(md));
			}

			//Dump may ref.
			MD_SET const* mds = k->get_ref_mds();
			if (mds != NULL && !mds->is_empty()) {
				fprintf(g_tfile, ",");
				for (INT i = mds->get_first();
					 i >= 0; ) {
					fprintf(g_tfile, "MD%d", i);
					i = mds->get_next(i);
					if (i >= 0) { fprintf(g_tfile, ","); }
				}
			}

			//Dump def/use list.
			if (k->is_stmt() || ir->is_lhs(k)) {
				fprintf(g_tfile, "\n\t  USE LIST:");
			} else {
				fprintf(g_tfile, "\n\t  DEF LIST:");
			}

			DU_SET const* set = get_du_c(k);
			if (set != NULL) {
				DU_ITER di;
				for (INT i = set->get_first(&di);
					 i >= 0; ) {
					IR const* ref = m_ru->get_ir(i);
					fprintf(g_tfile, "%s", IRNAME(ref));
					if (ref->is_stpr() || ref->is_pr()) {
						fprintf(g_tfile, "%d", ref->get_prno());
					}
					fprintf(g_tfile, "(id:%d)", IR_id(ref));

					i = set->get_next(i, &di);

					if (i >= 0) { fprintf(g_tfile, ","); }
				}
			}
		}
		fprintf(g_tfile, "\n");
	} //end for each IR
}


//Dump du chain for each memory reference.
void IR_DU_MGR::dump_du_chain2()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n\n==---- DUMP '%s' DU_CHAIN2 ----==\n",
			m_ru->get_ru_name());
	g_indent = 0;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		dump_bb_du_chain2(bb);
	} //end for each BB
	fflush(g_tfile);
}


//Dump du chain for stmt.
//Collect must and may use MDs and regard stmt as a whole.
//So this function do not distingwish individual memory operand.
void IR_DU_MGR::dump_du_chain()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n\n==---- DUMP '%s' DU_CHAIN ----==\n",
			m_ru->get_ru_name());
	g_indent = 2;
	MD_SET mds;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		fprintf(g_tfile, "\n--- BB%d ---", IR_BB_id(bb));
		for (IR * ir = IR_BB_ir_list(bb).get_head();
			 ir != NULL; ir = IR_BB_ir_list(bb).get_next()) {
			dump_ir(ir, m_dm);
			fprintf(g_tfile, "\n>>");

			//MustDef
			fprintf(g_tfile, "Dref(");
			bool has_prt_something = false;
			MD const* md = get_must_def(ir);
			if (md != NULL) {
				fprintf(g_tfile, "MMD%d", MD_id(md));
				has_prt_something = true;
			}

			//MayDef
			if (get_may_def(ir) != NULL) {
				mds.clean();
				MD_SET const* x = get_may_def(ir);
				if (x != NULL) {
					if (has_prt_something) {
						fprintf(g_tfile, ",");
					}
					for (INT i = x->get_first(); i >= 0;) {
						fprintf(g_tfile, "MD%d", i);
						i = x->get_next(i);
						if (i >= 0) {
							fprintf(g_tfile, ",");
						}
					}
					has_prt_something = true;
				}
			}
			if (!has_prt_something) {
				fprintf(g_tfile, "--");
			}
			fprintf(g_tfile, ")");

			//MayUse
			fprintf(g_tfile, " <= ");
			mds.clean();

			fprintf(g_tfile, "Uref(");
			collect_may_use_recur(ir, mds, true);
			if (!mds.is_empty()) {
				mds.dump(m_md_sys);
			} else {
				fprintf(g_tfile, "--");
			}
			fprintf(g_tfile, ")");

			//Dump def chain.
			m_citer.clean();
			bool first = true;
			for (IR const* x = ir_iter_rhs_init_c(ir, m_citer);
				 x != NULL; x = ir_iter_rhs_next_c(m_citer)) {
				 if (!x->is_memory_opnd()) { continue; }

				DU_SET const* defset = get_du_c(x);
				if (defset == NULL || defset->get_elem_count() == 0) {
					continue;
				}

				if (first) {
					fprintf(g_tfile, "\n>>DEF LIST:");
					first = false;
				}

				DU_ITER di;
				for (INT i = defset->get_first(&di);
					 i >= 0; i = defset->get_next(i, &di)) {
					IR const* def = m_ru->get_ir(i);
					fprintf(g_tfile, "%s(id:%d), ", IRNAME(def), IR_id(def));
				}
			}

			//Dump use chain.
			DU_SET const* useset = get_du_c(ir);
			if (useset == NULL || useset->get_elem_count() == 0) { continue; }

			fprintf(g_tfile, "\n>>USE LIST:");
			DU_ITER di;
			for (INT i = useset->get_first(&di);
				 i >= 0; i = useset->get_next(i, &di)) {
				IR const* u = m_ru->get_ir(i);
				fprintf(g_tfile, "%s(id:%d), ", IRNAME(u), IR_id(u));
			}
		} //end for each IR
	} //end for each BB
	fflush(g_tfile);
}


//'is_bs': true to dump bitset info.
void IR_DU_MGR::dump_set(bool is_bs)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n\n==---- DUMP %s() IR_DU_MGR : SET ----==\n",
			m_ru->get_ru_name());
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	C<IR_BB*> * cb;
	g_indent = 2;
	for (IR_BB * bb = bbl->get_head(&cb);
		 bb != NULL; bb = bbl->get_next(&cb)) {

		UINT bbid = IR_BB_id(bb);
		fprintf(g_tfile, "\n---- BB%d ----", bbid);
		DBITSETC * def_in = get_in_reach_def(bbid);
		DBITSETC * def_out = get_out_reach_def(bbid);
		DBITSETC * avail_def_in = get_avail_in_reach_def(bbid);
		DBITSETC * avail_def_out = get_avail_out_reach_def(bbid);
		DBITSETC * may_def_gen = get_may_gen_def(bbid);
		DBITSETC * must_def_gen = get_must_gen_def(bbid);
		DBITSETC * must_def_kill = get_must_killed_def(bbid);
		DBITSETC * may_def_kill = get_may_killed_def(bbid);
		DBITSETC * gen_ir = get_gen_ir_expr(bbid);
		DBITSETC * kill_ir = get_killed_ir_expr(bbid);
		DBITSETC * livein_ir = get_availin_expr(bbid);
		DBITSETC * liveout_ir = get_availout_expr(bbid);

		INT i;

		fprintf(g_tfile, "\nDEF IN STMT: %dbyte ", def_in->count_mem());
		SC<SEG*> * st;
		for (i = def_in->get_first(&st);
			 i != -1; i = def_in->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			def_in->dump(g_tfile);
		}

		fprintf(g_tfile, "\nDEF OUT STMT: %dbyte ", def_out->count_mem());
		for (i = def_out->get_first(&st);
			 i != -1; i = def_out->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			def_out->dump(g_tfile);
		}

		fprintf(g_tfile, "\nDEF AVAIL_IN STMT: %dbyte ", avail_def_in->count_mem());
		for (i = avail_def_in->get_first(&st);
			 i != -1; i = avail_def_in->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			avail_def_in->dump(g_tfile);
		}

		fprintf(g_tfile, "\nDEF AVAIL_OUT STMT: %dbyte ",
				avail_def_out->count_mem());
		for (i = avail_def_out->get_first(&st);
			 i != -1; i = avail_def_out->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			avail_def_out->dump(g_tfile);
		}

		fprintf(g_tfile, "\nMAY GEN STMT: %dbyte ", may_def_gen->count_mem());
		for (i = may_def_gen->get_first(&st);
			 i != -1; i = may_def_gen->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			may_def_gen->dump(g_tfile);
		}

		fprintf(g_tfile, "\nMUST GEN STMT: %dbyte ", must_def_gen->count_mem());
		for (i = must_def_gen->get_first(&st);
			 i != -1; i = must_def_gen->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			must_def_gen->dump(g_tfile);
		}

		fprintf(g_tfile, "\nMUST KILLED STMT: %dbyte ", must_def_kill->count_mem());
		for (i = must_def_kill->get_first(&st);
			 i != -1; i = must_def_kill->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			must_def_kill->dump(g_tfile);
		}

		fprintf(g_tfile, "\nMAY KILLED STMT: %dbyte ", may_def_kill->count_mem());
		for (i = may_def_kill->get_first(&st);
			 i != -1; i = may_def_kill->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			may_def_kill->dump(g_tfile);
		}

		fprintf(g_tfile, "\nLIVEIN EXPR: %dbyte ", livein_ir->count_mem());
		for (i = livein_ir->get_first(&st);
			 i != -1; i = livein_ir->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			livein_ir->dump(g_tfile);
		}

		fprintf(g_tfile, "\nLIVEOUT EXPR: %dbyte ", liveout_ir->count_mem());
		for (i = liveout_ir->get_first(&st);
			 i != -1; i = liveout_ir->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			liveout_ir->dump(g_tfile);
		}

		fprintf(g_tfile, "\nGEN EXPR: %dbyte ", gen_ir->count_mem());
		for (i = gen_ir->get_first(&st);
			 i != -1; i = gen_ir->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			gen_ir->dump(g_tfile);
		}

		fprintf(g_tfile, "\nKILLED EXPR: %dbyte ", kill_ir->count_mem());
		for (i = kill_ir->get_first(&st);
			 i != -1; i = kill_ir->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir != NULL);
			fprintf(g_tfile, "%s(%d), ", IRNAME(ir), IR_id(ir));
		}
		if (is_bs) {
			fprintf(g_tfile, "\n             ");
			kill_ir->dump(g_tfile);
		}
	}
	fflush(g_tfile);
}


/* This function copy MustUse and MayUse mds from tree 'to' to tree 'from'
for memory reference.
copy_du_chain: if true to copy DU chain from tree 'to' to tree 'from'.
	this operation will establish DU chain between the DEF of 'to' and
	'from'.
'to': root node of target tree.
'from': root node of source tree.
NOTICE: IR tree 'to' and 'from' must be identical. to and from may be stmt. */
void IR_DU_MGR::copy_ir_tree_du_info(IR * to, IR const* from,
									 bool copy_du_chain)
{
	if (to == from) { return; }
	IS_TRUE0(to->is_ir_equal(from, true));
	m_citer.clean();
	m_iter2.clean();
	IR const* from_ir = ir_iter_init_c(from, m_citer);
	for (IR * to_ir = ir_iter_init(to, m_iter2);
		 to_ir != NULL;
		 to_ir = ir_iter_next(m_iter2),
		 from_ir = ir_iter_next_c(m_citer)) {
		IS_TRUE0(to_ir->is_ir_equal(from_ir, true));
		if (!to_ir->is_memory_ref() && IR_type(to_ir) != IR_ID) {
			//Copy MD for IR_ID also, some pass need it, e.g. GVN.
			continue;
		}

		to_ir->set_ref_md(from_ir->get_ref_md(), m_ru);
		to_ir->set_ref_mds(from_ir->get_ref_mds(), m_ru);

		if (!copy_du_chain) { continue; }

		DU_SET const* from_du = get_du_c(from_ir);
		if (from_du == NULL || from_du->is_empty()) { continue; }

		DU_SET * to_du = get_du_alloc(to_ir);
		to_du->copy(*from_du, *m_sbs_mgr);

		DU_ITER di;
		for (UINT i = from_du->get_first(&di);
			 di != NULL; i = from_du->get_next(i, &di)) {
			IR const* stmt = get_ir(i);
			DU_SET * dir_useset = stmt->get_duset();
			if (dir_useset == NULL) { continue; }
			dir_useset->add(IR_id(to_ir), *m_sbs_mgr);
		}
	}
	IS_TRUE0(m_iter2.get_elem_count() == 0 && m_iter.get_elem_count() == 0);
}


//Remove 'def' out of ir's DEF set. ir is exp.
void IR_DU_MGR::remove_def(IR const* ir, IR const* def)
{
	IS_TRUE0(ir->is_exp() && def->is_stmt());
	DU_SET * duset = ir->get_duset();
	if (duset == NULL) { return; }
	duset->remove_def(def, *m_sbs_mgr);
}


/* Check if the DEF of stmt's operands still modify the same memory object.
e.g: Revise DU chain if stmt's rhs has changed.
	x=10 //S1
	...
	a=x*0 //S2
  =>
	x=10 //S1
	...
	a=0 //S2
Given S1 is def, S2 is use, after ir refinement, x in S2
is removed, remove the data dependence between S1
and S2 operand. */
bool IR_DU_MGR::remove_expired_du_chain(IR * stmt)
{
	IS_TRUE0(stmt->is_stmt());

	DU_SET const* useset = get_du_c(stmt);
	if (useset == NULL) { return false; }

	bool change = false;
	m_citer.clean();
	for (IR const* k = ir_iter_rhs_init_c(stmt, m_citer);
		 k != NULL; k = ir_iter_rhs_next_c(m_citer)) {
		if (!k->is_memory_opnd()) { continue; }
		switch (IR_type(k)) {
		case IR_LD:
		case IR_PR:
		case IR_ILD:
		case IR_ARRAY:
			{
				DU_SET const* defset = get_du_c(k);
				if (defset == NULL) { continue; }

				DU_ITER di;
				UINT nd;
				for (UINT d = defset->get_first(&di); di != NULL; d = nd) {
					nd = defset->get_next(d, &di);

					IR const* stmt = m_ru->get_ir(d);
					IS_TRUE0(stmt->is_stmt());

					MD_SET const* maydef = get_may_def(stmt);
					if (maydef != NULL) {
						MD_SET const* mayuse = get_may_use(k);
						if (mayuse != NULL &&
							(mayuse == maydef ||
							 mayuse->is_intersect(*maydef))) {
							continue;
						}

						MD const* mustuse = get_effect_use_md(k);
						if (mustuse != NULL && maydef->is_contain(mustuse)) {
							continue;
						}
					}

					MD const* mustdef = get_must_def(stmt);
					if (mustdef != NULL) {
						MD_SET const* mayuse = get_may_use(k);
						if (mustdef != NULL) {
							if (mayuse != NULL && mayuse->is_contain(mustdef)) {
								continue;
							}

							MD const* mustuse = get_effect_use_md(k);
							if (mustuse != NULL &&
								(mustuse == mustdef ||
								 mustuse->is_overlap(mustdef))) {
								continue;
							}
						}
					}

					//There is no du-chain bewteen d and u. Cut the du chain.
					remove_du_chain(stmt, k);
					change = true;
				}
			}
			break;
		default:
			IS_TRUE0(!k->is_memory_ref());
			break;
		}
	}
	return change;
}


/* This function check all USE of memory references of ir tree and
cut its du-chain. 'ir' may be stmt or expression, if ir is stmt,
check its right-hand-side.

'ir': indicate the root of IR tree.

e.g: d1, d2 are def-stmt of stmt's operands.
this functin cut off du-chain between d1, d2 and their
use. */
void IR_DU_MGR::remove_use_out_from_defset(IR * ir)
{
	m_citer.clean();
	IR const* k;
	if (ir->is_stmt()) {
		k = ir_iter_rhs_init_c(ir, m_citer);
	} else {
		k = ir_iter_exp_init_c(ir, m_citer);
	}
	for (; k != NULL; k = ir_iter_rhs_next_c(m_citer)) {
		if (!k->is_memory_opnd()) { continue; }

		DU_SET * defset = k->get_duset();
		if (defset == NULL) { continue; }

		DU_ITER di;
		bool doclean = false;
		for (INT i = defset->get_first(&di);
			 i >= 0; i = defset->get_next(i, &di)) {
			doclean = true;
			IR const* stmt = m_ru->get_ir(i);
			IS_TRUE0(stmt->is_stmt());

			DU_SET * useset = stmt->get_duset();
			if (useset == NULL) { continue; }
			useset->remove_use(k, *m_sbs_mgr);
		}
		if (doclean) {
			defset->clean(*m_sbs_mgr);
		}
	}
}


/* Cut off the DU chain between def and its use expression.
Remove 'def' out from its use's def-list.
e.g:u1, u2 are its use expressions.
cut off the du chain between def and u1, u2. */
void IR_DU_MGR::remove_def_out_from_useset(IR * def)
{
	IS_TRUE0(def);
	if (def->is_pr()) {
		//def must be result of CALL/ICALL.
		//Call may have multiple result PRs.
		IR * call = IR_parent(def);
		IS_TRUE0(call && call->is_call() && call->is_lhs(def));

		DU_SET * useset = call->get_duset();
		if (useset == NULL) { return; }

		UINT defprno = PR_no(def);
		DU_ITER di;

		//Remove the du chain bewteen DEF and its USE.
		INT ni = 0; //next ith element.
		for (INT i = useset->get_first(&di); i >= 0; i = ni) {
			ni = useset->get_next(i, &di);
			IR const* exp = m_ru->get_ir(i);
			IS_TRUE0(exp->is_exp());

			//Only cut the du chain if exp is equivalent to def.
			if (!exp->is_pr() || PR_no(exp) != defprno) { continue; }

			//Handle DU_SET of use.
			DU_SET * du = exp->get_duset();
			if (du != NULL) { du->remove_def(call, *m_sbs_mgr); }

			//Handle DU_SET of def.
			useset->remove_use(exp, *m_sbs_mgr);
		}
		return;
	}

	IS_TRUE0(def->is_stmt());
	DU_SET * useset = def->get_duset();
	if (useset == NULL) { return; }

	DU_ITER di;
	bool doclean = false;
	//Remove the du chain bewteen DEF and its USE.
	for (INT i = useset->get_first(&di);
		 i >= 0; i = useset->get_next(i, &di)) {
		doclean = true;
		IR const* exp = m_ru->get_ir(i);
		IS_TRUE0(exp->is_exp());

		DU_SET * du = exp->get_duset();
		if (du != NULL) { du->remove_def(def, *m_sbs_mgr); }
	}

	//Clean USE set.
	if (doclean) {
		useset->clean(*m_sbs_mgr);
	}
}


//Remove all du info of ir out from DU mgr.
void IR_DU_MGR::remove_ir_out_from_du_mgr(IR * ir)
{
	IS_TRUE0(ir->is_stmt());
	remove_use_out_from_defset(ir);
	remove_def_out_from_useset(ir);
}


//Count up the memory has been allocated.
UINT IR_DU_MGR::count_mem()
{
	SVECTOR<DBITSETC*> * ptr;

	UINT count = sizeof(m_mds_mgr);
	count += smpool_get_pool_size_handle(m_pool);

	count += m_bb_avail_in_reach_def.count_mem();
	ptr = &m_bb_avail_in_reach_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_avail_out_reach_def.count_mem();
	ptr = &m_bb_avail_out_reach_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_in_reach_def.count_mem();
	ptr = &m_bb_in_reach_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_out_reach_def.count_mem();
	ptr = &m_bb_out_reach_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_may_gen_def.count_mem();
	ptr = &m_bb_may_gen_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_must_gen_def.count_mem();
	ptr = &m_bb_must_gen_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_must_killed_def.count_mem();
	ptr = &m_bb_must_killed_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_may_killed_def.count_mem();
	ptr = &m_bb_may_killed_def;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_gen_ir_expr.count_mem();
	ptr = &m_bb_gen_ir_expr;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_killed_ir_expr.count_mem();
	ptr = &m_bb_killed_ir_expr;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_availin_ir_expr.count_mem();
	ptr = &m_bb_availin_ir_expr;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	count += m_bb_availout_ir_expr.count_mem();
	ptr = &m_bb_availout_ir_expr;
	for (INT i = 0; i <= ptr->get_last_idx(); i++) {
		DBITSETC * dset = ptr->get(i);
		if (dset != NULL) {
			count += dset->count_mem();
		}
	}

	return count;
}


//Count up memory of DU_SET for all irs.
UINT IR_DU_MGR::count_mem_duset()
{
	UINT count = 0;
	SVECTOR<IR*> * vec = m_ru->get_ir_vec();
	INT l = vec->get_last_idx();
	for (INT i = 1; i <= l; i++) {
		IR const* ir = vec->get(i);
		DU_SET const* duset = ir->get_duset_c();
		if (duset != NULL) {
			count += duset->count_mem();
		}
	}
	return count;
}


/* Collect MustUse MD.
e.g: a = b + *p;
	if p->w, the MustUse is {a,b,p,w}
	if p->w,u, the MustUse is {a,b,p} */
void IR_DU_MGR::collect_must_use(IN IR const* ir, OUT MD_SET & mustuse)
{
	switch (IR_type(ir)) {
	case IR_ST:
		compute_exp(ST_rhs(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		return;
	case IR_STPR:
		compute_exp(STPR_rhs(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		return;
	case IR_IST:
		compute_exp(IST_rhs(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		compute_exp(IST_base(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		return;
	case IR_ICALL:
		compute_exp(ICALL_callee(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
	case IR_CALL:
		{
			IR * param = CALL_param_list(ir);
			while (param != NULL) {
				compute_exp(param, &mustuse, COMP_EXP_COLLECT_MUST_USE);
				param = IR_next(param);
			}
			return;
		}
	case IR_GOTO:
	case IR_REGION:
		break;
	case IR_IGOTO:
		compute_exp(IGOTO_vexp(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		return;
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_CASE:
		IS_TRUE(0, ("TODO"));
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		compute_exp(BR_det(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		return;
	case IR_SWITCH:
		compute_exp(SWITCH_vexp(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		return;
	case IR_RETURN:
		compute_exp(RET_exp(ir), &mustuse, COMP_EXP_COLLECT_MUST_USE);
		return;
	default: IS_TRUE0(0);
	} //end switch
}


void IR_DU_MGR::infer_st(IR * ir)
{
	IS_TRUE0(ir->is_stid() && ir->get_ref_md());

	/* Find ovelapped MD.
	e.g: struct {int a;} s;
	s = ...
	s.a = ...
	Where s and s.a is overlapped. */
	compute_overlap_def_mds(ir, false);

	compute_exp(ST_rhs(ir), NULL, COMP_EXP_RECOMPUTE);
}


void IR_DU_MGR::infer_stpr(IR * ir)
{
	IS_TRUE0(ir->is_stpr() && ir->get_ref_md() && ir->get_ref_mds() == NULL);
	if (!is_pr_unique_for_same_no()) {
		compute_overlap_use_mds(ir, false);
	}
	compute_exp(STPR_rhs(ir), NULL, COMP_EXP_RECOMPUTE);
}


void IR_DU_MGR::infer_phi(IR * ir)
{
	IS_TRUE0(ir->is_phi() && ir->get_ref_md() && ir->get_ref_mds() == NULL);
	if (!is_pr_unique_for_same_no()) {
		compute_overlap_use_mds(ir, false);
	}

	//Set call result list MD.
	IR * r = PHI_opnd_list(ir);
	while (r != NULL) {
		IS_TRUE0(r->get_ref_md() && r->get_ref_md()->is_pr());
		IS_TRUE0(r->get_ref_mds() == NULL);
		compute_exp(r, NULL, COMP_EXP_RECOMPUTE);
		r = IR_next(r);
	}
}


void IR_DU_MGR::infer_ist(IR * ir)
{
	IS_TRUE0(IR_type(ir) == IR_IST);
	if (IST_base(ir)->is_direct_array_ref()) {
		comp_array_lhs_ref(IST_base(ir));
	} else {
		compute_exp(IST_base(ir), NULL, COMP_EXP_RECOMPUTE);
	}

	//Compute DEF mdset. AA should guarantee either mustdef is not NULL or
	//maydef not NULL.
	MD_SET const* maydef = ir->get_ref_mds();
	MD const* mustdef = ir->get_ref_md();
	IS_TRUE0((maydef && !maydef->is_empty()) ^ (mustdef != NULL));

	compute_overlap_def_mds(ir, false);
	compute_exp(IST_rhs(ir), NULL, COMP_EXP_RECOMPUTE);
}


/* Inference call clobbering. Function calls may modify addressable
local variables and globals in indefinite ways.
Variables that may be use and clobbered are global auxiliary var. */
void IR_DU_MGR::infer_call(IR * ir, MD_SET * tmp, IN MD2MDS * mx)
{
	IS_TRUE0(ir->is_call());
	if (!is_pr_unique_for_same_no()) {
		compute_overlap_use_mds(ir, false);
	}

	if (IR_type(ir) == IR_ICALL) {
		compute_exp(ICALL_callee(ir), NULL, COMP_EXP_RECOMPUTE);
	}

	MD_SET * maydefuse = m_mds_mgr->create();

	//Set MD which parameters pointed to.
	IR * param = CALL_param_list(ir);
	while (param != NULL) {
		//Compute USE mdset.
		if (param->is_ptr(m_dm)) {
			/* e.g: foo(p); where p->{a, b} then foo may use p, a, b.
			Note that point-to information is only avaiable for the
			last stmt of BB. The call is just in the situation. */
			m_aa->compute_may_point_to(param, mx, *maydefuse);
		}
		compute_exp(param, NULL, COMP_EXP_RECOMPUTE);
		param = IR_next(param);
	}

	m_tmp_mds.clean();
	m_md_sys->compute_overlap(*maydefuse, m_tmp_mds, m_tab_iter, true);
	maydefuse->bunion_pure(m_tmp_mds);

	//Set global memory MD.
	maydefuse->bunion(m_md_sys->get_md(MD_GLOBAL_MEM));

	set_map_call2mayuse(ir, m_mds_hash->append(*maydefuse));
	m_mds_mgr->free(maydefuse);

	if (ir->is_readonly_call()) {
		ir->clean_ref_mds();
	}
}


//Collect MD which ir may use, include overlapped MD.
void IR_DU_MGR::collect_may_use(IR const* ir, MD_SET & may_use, bool comp_pr)
{
	m_citer.clean();
	IR const* x = NULL;
	bool const is_stmt = ir->is_stmt();
	if (is_stmt) {
		x = ir_iter_rhs_init_c(ir, m_citer);
	} else {
		x = ir_iter_exp_init_c(ir, m_citer);
	}

	for (; x != NULL; x = ir_iter_rhs_next_c(m_citer)) {
		if (!x->is_memory_opnd()) { continue; }

		IS_TRUE0(IR_parent(x));

		if ((IR_type(x) == IR_ID || IR_type(x) == IR_LD) &&
			IR_type(IR_parent(x)) == IR_LDA) {
			continue;
		}

		if (x->is_pr() && comp_pr) {
			IS_TRUE0(get_must_use(x));

			may_use.bunion_pure(MD_id(get_must_use(x)));

			if (!is_pr_unique_for_same_no()) {
				MD_SET const* ts = get_may_use(x);
				if (ts != NULL) {
					may_use.bunion_pure(*ts);
				}
			}
			continue;
		}

		MD const* mustref = get_must_use(x);
		MD_SET const* mayref = get_may_use(x);

		if (mustref != NULL) {
			may_use.bunion(mustref);
		}

		if (mayref != NULL) {
			may_use.bunion(*mayref);
		}
	}

	if (is_stmt) {
		if (ir->is_call()) {
			bool done = false;
			CALLG * callg = m_ru->get_ru_mgr()->get_callg();
			if (callg != NULL) {
				REGION * ru = callg->map_ir2ru(ir);
				if (ru != NULL && RU_is_du_valid(ru)) {
					MD_SET const* muse = ru->get_may_use();
					if (muse != NULL) {
						may_use.bunion(*muse);
						done = true;
					}
				}
			}
			if (!done) {
				MD_SET const* muse = map_call2mayuse(ir);
				if (muse != NULL) {
					may_use.bunion(*muse);
				}
			}
		} else if (IR_type(ir) == IR_REGION) {
			MD_SET const* x = REGION_ru(ir)->get_may_use();
			if (x != NULL) {
				may_use.bunion(*x);
			}
		}
	}
}


//Collect MD which ir may use, include overlapped MD.
void IR_DU_MGR::collect_may_use_recur(IR const* ir,
									MD_SET & may_use,
									bool comp_pr)
{
	if (ir == NULL) { return; }
	switch (IR_type(ir)) {
	case IR_CONST:  return;
	case IR_ID:
	case IR_LD:
		IS_TRUE0(IR_parent(ir) != NULL);
		if (IR_type(IR_parent(ir)) != IR_LDA) {
			IS_TRUE0(get_must_use(ir));
			may_use.bunion(get_must_use(ir));

			MD_SET const* ts = get_may_use(ir);
			if (ts != NULL) {
				may_use.bunion(*ts);
			}
		}
		return;
	case IR_ST:
		collect_may_use_recur(ST_rhs(ir), may_use, comp_pr);
		return;
	case IR_STPR:
		collect_may_use_recur(STPR_rhs(ir), may_use, comp_pr);
		return;
	case IR_ILD:
		collect_may_use_recur(ILD_base(ir), may_use, comp_pr);
		IS_TRUE0(IR_parent(ir) != NULL);
		if (IR_type(IR_parent(ir)) != IR_LDA) {
			MD const* t = get_must_use(ir);
			if (t != NULL) {
				may_use.bunion(t);
			}

			MD_SET const* ts = get_may_use(ir);
			if (ts != NULL) {
				may_use.bunion(*ts);
			}
		}
		return;
	case IR_IST:
		collect_may_use_recur(IST_rhs(ir), may_use, comp_pr);
		collect_may_use_recur(IST_base(ir), may_use, comp_pr);
		break;
	case IR_LDA:
		IS_TRUE0(ir->verify(m_dm));
		//case: &x, the must_use should not be 'x'.
		//collect_may_use_recur(LDA_base(ir), may_use, comp_pr);
		return;
	case IR_ICALL:
		collect_may_use_recur(ICALL_callee(ir), may_use, comp_pr);
		//Fall through.
	case IR_CALL:
		{
			IR * p = CALL_param_list(ir);
			while (p != NULL) {
				collect_may_use_recur(p, may_use, comp_pr);
				p = IR_next(p);
			}

			bool done = false;
			CALLG * callg = m_ru->get_ru_mgr()->get_callg();
			if (callg != NULL) {
				REGION * ru = callg->map_ir2ru(ir);
				if (ru != NULL && RU_is_du_valid(ru)) {
					MD_SET const* muse = ru->get_may_use();
					if (muse != NULL) {
						may_use.bunion(*muse);
						done = true;
					}
				}
			}
			if (!done) {
				MD_SET const* muse = map_call2mayuse(ir);
				if (muse != NULL) {
					may_use.bunion(*muse);
				}
			}
		}
		return;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		//Binary operation.
		collect_may_use_recur(BIN_opnd0(ir), may_use, comp_pr);
		collect_may_use_recur(BIN_opnd1(ir), may_use, comp_pr);
		return;
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
		collect_may_use_recur(UNA_opnd0(ir), may_use, comp_pr);
		return;
	case IR_GOTO:
	case IR_LABEL:
		return;
	case IR_IGOTO:
		collect_may_use_recur(IGOTO_vexp(ir), may_use, comp_pr);
		return;
	case IR_SWITCH:
		collect_may_use_recur(SWITCH_vexp(ir), may_use, comp_pr);
		return;
	case IR_ARRAY:
		{
			IS_TRUE0(IR_parent(ir) != NULL);
			if (IR_type(IR_parent(ir)) != IR_LDA) {
 				MD const* t = get_must_use(ir);
				if (t != NULL) {
					may_use.bunion(t);
				}


				MD_SET const* ts = get_may_use(ir);
				if (ts != NULL) {
					may_use.bunion(*ts);
				}
			}

			for (IR * s = ARR_sub_list(ir); s != NULL; s = IR_next(s)) {
				collect_may_use_recur(s, may_use, comp_pr);
			}
			collect_may_use_recur(ARR_base(ir), may_use, comp_pr);
		}
		return;
	case IR_CVT:
		//CVT should not has any use-mds. Even if the operation
		//will genrerate different type.
		collect_may_use_recur(CVT_exp(ir), may_use, comp_pr);
		return;
	case IR_PR:
		if (!comp_pr) { return; }

		IS_TRUE0(get_must_use(ir));
		may_use.bunion_pure(MD_id(get_must_use(ir)));

		if (!is_pr_unique_for_same_no()) {
			MD_SET const* ts = get_may_use(ir);
			if (ts != NULL) {
				may_use.bunion_pure(*ts);
			}
		}
		return;
	case IR_TRUEBR:
	case IR_FALSEBR:
		IS_TRUE0(BR_lab(ir));
		collect_may_use_recur(BR_det(ir), may_use, comp_pr);
		return;
	case IR_RETURN:
		collect_may_use_recur(RET_exp(ir), may_use, comp_pr);
		return;
	case IR_SELECT:
		collect_may_use_recur(SELECT_det(ir), may_use, comp_pr);
		collect_may_use_recur(SELECT_trueexp(ir), may_use, comp_pr);
		collect_may_use_recur(SELECT_falseexp(ir), may_use, comp_pr);
		return;
	case IR_PHI:
		for (IR * p = PHI_opnd_list(ir); p != NULL; p = IR_next(p)) {
			collect_may_use_recur(p, may_use, comp_pr);
		}
		return;
	case IR_REGION:
		{
			MD_SET const* x = REGION_ru(ir)->get_may_use();
			if (x != NULL) {
				may_use.bunion(*x);
			}
		}
		return;
	default: IS_TRUE0(0);
	}
}


void IR_DU_MGR::add_overlapped_exact_md(OUT MD_SET * mds, MD const* x,
										MD_ITER & mditer)
{
	IS_TRUE0(x->is_exact());
	MD_TAB * mdt = m_md_sys->get_md_tab(MD_base(x));
	IS_TRUE0(mdt);

	OFST_TAB * ofstab = mdt->get_ofst_tab();
	IS_TRUE0(ofstab);
	if (ofstab->get_elem_count() > 0) {
		mditer.clean();
		for (MD * md = ofstab->get_first(mditer, NULL);
			 md != NULL; md = ofstab->get_next(mditer, NULL)) {
			if (md == x || !md->is_exact()) { continue; }
			if (md->is_overlap(x)) {
				mds->bunion(md);
			}
		}
	}
}


void IR_DU_MGR::comp_mustdef_for_region(IR const* ir, MD_SET * bb_mustdefmds,
										MD_ITER & mditer)
{
	MD_SET const* mustdef = REGION_ru(ir)->get_must_def();
	if (mustdef != NULL) {
		bb_mustdefmds->bunion(*mustdef);
		m_tmp_mds.clean();
		m_md_sys->compute_overlap(*bb_mustdefmds, m_tmp_mds,
								  m_tab_iter, true);
		bb_mustdefmds->bunion_pure(m_tmp_mds);
	}
}


void IR_DU_MGR::comp_bb_maydef(IR const* ir, MD_SET * bb_maydefmds,
								DBITSETC * maygen_stmt)
{
	if (!ir->is_stpr() || !is_pr_unique_for_same_no()) {
		//Collect maydef mds.
		MD_SET const* xs = get_may_def(ir);
		if (xs != NULL && !xs->is_empty()) {
			bb_maydefmds->bunion(*xs);
		}
	}

	/* Computing May GEN set of reach-definition.
	The computation of reach-definition problem
	is conservative. If we can not say
	whether a DEF is killed, regard it as lived STMT. */
	SC<SEG*> * st;
	INT ni;
	for (INT i = maygen_stmt->get_first(&st); i != -1; i = ni) {
		ni = maygen_stmt->get_next(i, &st);
		IR * gened_ir = m_ru->get_ir(i);
		IS_TRUE0(gened_ir != NULL && gened_ir->is_stmt());
		if (is_must_kill(ir, gened_ir)) {
			maygen_stmt->diff(i, *m_sbs_mgr);
		}
	}
	maygen_stmt->bunion(IR_id(ir), *m_sbs_mgr);
}


void IR_DU_MGR::comp_bb_mustdef(IR const* ir, MD_SET * bb_mustdefmds,
								DBITSETC * mustgen_stmt, MD_ITER & mditer)
{
	switch (IR_type(ir)) {
	case IR_CALL:
	case IR_ICALL:
	case IR_PHI:
		{
			MD const* x = ir->get_exact_ref();
			if (x != NULL) {
				//call may not have return value.

				bb_mustdefmds->bunion(x);

				//Add MD which is exact and overlapped with x.
				add_overlapped_exact_md(bb_mustdefmds, x, mditer);
			}
		}
		break;
	case IR_REGION:
		comp_mustdef_for_region(ir, bb_mustdefmds, mditer);
		break;
	default: //Handle general stmt.
		{
			MD const* x = ir->get_exact_ref();
			if (x != NULL) {
				bb_mustdefmds->bunion(x);

				//Add MD which is exact and overlapped with x.
				add_overlapped_exact_md(bb_mustdefmds, x, mditer);
			}
		}
	}

	//Computing Must GEN set of reach-definition.
	SC<SEG*> * st;
	INT ni;
	for (INT i = mustgen_stmt->get_first(&st); i != -1; i = ni) {
		ni = mustgen_stmt->get_next(i, &st);
		IR * gened_ir = m_ru->get_ir(i);
		IS_TRUE0(gened_ir != NULL && gened_ir->is_stmt());
		if (is_may_kill(ir, gened_ir)) {
			mustgen_stmt->diff(i, *m_sbs_mgr);
		}
	}
	mustgen_stmt->bunion(IR_id(ir), *m_sbs_mgr);
}


/* NOTE: MD referrence must be available.
mustdefs: record must modified MD for each bb.
maydefs: record may modified MD for each bb.
mayuse: record may used MD for each bb. */
void IR_DU_MGR::comp_mustdef_maydef_mayuse(OUT SVECTOR<MD_SET*> * mustdefmds,
										   OUT SVECTOR<MD_SET*> * maydefmds,
										   OUT MD_SET * mayusemds,
										   UINT flag)
{
	if (HAVE_FLAG(flag, SOL_REACH_DEF) ||
		HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF)) {
		IS_TRUE0(mustdefmds);
	}

	if (HAVE_FLAG(flag, SOL_REACH_DEF) ||
		HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF) ||
		HAVE_FLAG(flag, SOL_AVAIL_EXPR)) {
		IS_TRUE0(maydefmds);
	}

	if (HAVE_FLAG(flag, SOL_RU_REF)) {
		IS_TRUE0(mayusemds);
	}

	MD_ITER mditer;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		DBITSETC * maygen_stmt = NULL;
		DBITSETC * mustgen_stmt = NULL;
		MD_SET * bb_mustdefmds = NULL;
		MD_SET * bb_maydefmds = NULL;
		UINT bbid = IR_BB_id(bb);
		if (mustdefmds != NULL) {
			bb_mustdefmds = mustdefmds->get(bbid);
			mustgen_stmt = get_must_gen_def(bbid);
			mustgen_stmt->clean(*m_sbs_mgr);
		}

		if (maydefmds != NULL) {
			bb_maydefmds = maydefmds->get(bbid);
			maygen_stmt = get_may_gen_def(bbid);
			maygen_stmt->clean(*m_sbs_mgr);
		}

		//may_def_mds, must_def_mds should be already clean.
		for (IR const* ir = IR_BB_ir_list(bb).get_head();
			 ir!= NULL; ir = IR_BB_ir_list(bb).get_next()) {
			if (mayusemds != NULL) {
				collect_may_use_recur(ir, *mayusemds, is_compute_pr_du_chain());
				//collect_may_use(ir, *mayusemds, is_compute_pr_du_chain());
			}

			if (!ir->has_result()) { continue; }

			//Do not compute MustDef/MayDef for PR.
			if ((ir->is_stpr() || ir->is_phi()) &&
				!is_compute_pr_du_chain()) {
				continue;
			}

			//Collect mustdef mds.
			if (bb_mustdefmds != NULL) {
				comp_bb_mustdef(ir, bb_mustdefmds, mustgen_stmt, mditer);
			}

			if (bb_maydefmds != NULL) {
				comp_bb_maydef(ir, bb_maydefmds, maygen_stmt);
			}
		} //for each ir.
	} //for each bb.
}


/* Compute Defined, Used md-set, Generated ir-stmt-set, and
MayDefined md-set for each IR. */
void IR_DU_MGR::comp_ref()
{
	m_cached_overlap_mdset.clean();
	m_is_cached_mdset.clean(*m_sbs_mgr);

	IS_TRUE0(m_aa != NULL);
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	C<IR_BB*> * bbct;
	for (IR_BB * bb = bbl->get_head(&bbct);
		 bb != NULL; bb = bbl->get_next(&bbct)) {
		C<IR*> * irct;
		for (IR * ir = IR_BB_ir_list(bb).get_head(&irct);
			 ir!= NULL; ir = IR_BB_ir_list(bb).get_next(&irct)) {
			switch (IR_type(ir)) {
			case IR_ST:
				infer_st(ir);
				break;
			case IR_STPR:
				infer_stpr(ir);
				break;
			case IR_IST:
				infer_ist(ir);
				break;
			case IR_CALL:
			case IR_ICALL:
				{
					//Because CALL is always the last ir in BB, the
					//querying process is only executed once per BB.
					MD2MDS * mx = NULL;
					if (m_aa->is_flow_sensitive()) {
						mx = m_aa->map_bb_to_md2mds(bb);
					} else {
						mx = m_aa->get_unique_md2mds();
					}
					IS_TRUE0(mx);

					MD_SET * tmp = m_mds_mgr->create();
					infer_call(ir, tmp, mx);
					m_mds_mgr->free(tmp);
				}
				break;
			case IR_RETURN:
				compute_exp(RET_exp(ir), NULL, COMP_EXP_RECOMPUTE);
				break;
			case IR_TRUEBR:
			case IR_FALSEBR:
				//Compute USE mdset.
				IS_TRUE0(BR_lab(ir));
				compute_exp(BR_det(ir), NULL, COMP_EXP_RECOMPUTE);
				break;
			case IR_SWITCH:
				//Compute USE mdset.
				compute_exp(SWITCH_vexp(ir), NULL, COMP_EXP_RECOMPUTE);
				break;
			case IR_IGOTO:
				//Compute USE mdset.
				compute_exp(IGOTO_vexp(ir), NULL, COMP_EXP_RECOMPUTE);
				break;
			case IR_GOTO:
				break;
			case IR_PHI:
				if (is_compute_pr_du_chain()) {
					infer_phi(ir);
				}
				break;
			case IR_REGION:
				//The memory reference information should already be avaiable.
				break;
			default: IS_TRUE0(0);
			}
		}
	}
}


/* Compute must and may killed stmt.
mustdefs: record must modified MD for each bb.
maydefs: record may modified MD for each bb.
flag: switches.
NOTE: compute maykill and mustkill both need may-gen-def. */
void IR_DU_MGR::comp_kill(SVECTOR<MD_SET*> const* mustdefmds,
						  SVECTOR<MD_SET*> const* maydefmds,
						  UINT flag)
{
	IS_TRUE0(mustdefmds || maydefmds);

	IR_BB_LIST * ir_bb_list = m_ru->get_bb_list();
	DBITSETC univers;
	univers.set_sparse(SOL_SET_IS_SPARSE);
	for (IR_BB * bb = ir_bb_list->get_head();
		 bb != NULL; bb = ir_bb_list->get_next()) {
		univers.bunion(*get_may_gen_def(IR_BB_id(bb)), *m_sbs_mgr);
	}

	DBITSETC xtmp;
	xtmp.set_sparse(SOL_SET_IS_SPARSE);
	for (IR_BB * bb = ir_bb_list->get_head();
		 bb != NULL; bb = ir_bb_list->get_next()) {

		UINT bbid = IR_BB_id(bb);
		DBITSETC const* maygendef = get_may_gen_def(bbid);
		DBITSETC * must_killed_set = NULL;
		DBITSETC * may_killed_set = NULL;

		bool comp_must = false;
		MD_SET const* bb_mustdef_mds = NULL;
		if (mustdefmds != NULL) {
			bb_mustdef_mds = mustdefmds->get(bbid);
			if (bb_mustdef_mds != NULL && !bb_mustdef_mds->is_empty()) {
				comp_must = true;
				must_killed_set = get_must_killed_def(bbid);
				must_killed_set->clean(*m_sbs_mgr);
			}
		}

		bool comp_may = false;
		MD_SET const* bb_maydef_mds = NULL;
		if (maydefmds != NULL) {
			//Compute may killed stmts.
			bb_maydef_mds = maydefmds->get(bbid);
			if (bb_maydef_mds != NULL && !bb_maydef_mds->is_empty()) {
				comp_may = true;
				may_killed_set = get_may_killed_def(bbid);
				may_killed_set->clean(*m_sbs_mgr);
			}
		}

		if (comp_must || comp_may) {
			SC<SEG*> * st;
			xtmp.copy(univers, *m_sbs_mgr);
			xtmp.diff(*maygendef, *m_sbs_mgr);
		 	for (INT i = xtmp.get_first(&st);
				 i != -1; i = xtmp.get_next(i, &st)) {

				IR const* stmt = m_ru->get_ir(i);
				IS_TRUE0(stmt->is_stmt());

				if (comp_must) {
					//Get the IR set that except current bb's stmts.
					//if (maygendef->is_contain(i)) { continue; }

					MD const* stmt_mustdef_md = stmt->get_exact_ref();
					if (stmt_mustdef_md == NULL) { continue; }
					if (bb_mustdef_mds->is_contain(stmt_mustdef_md)) {
						must_killed_set->bunion(i, *m_sbs_mgr);
					}
				}

				if (comp_may) {
					//Compute may killed stmts.
					//Get the IR set that except current bb's IR stmts.
					//if (maygendef->is_contain(i)) { continue; }
					MD_SET const* maydef = get_may_def(stmt);
					if (maydef == NULL) { continue; }

					if (bb_maydef_mds->is_intersect(*maydef)) {
						may_killed_set->bunion(i, *m_sbs_mgr);
					}
				}
			}
		}
	}
	univers.clean(*m_sbs_mgr);
	xtmp.clean(*m_sbs_mgr);
}


//Return true if ir can be candidate of live-expr.
bool IR_DU_MGR::can_be_live_expr_cand(IR const* ir) const
{
	IS_TRUE0(ir);
	switch (IR_type(ir)) {
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
	case IR_SELECT:
	case IR_PR:
		return true;
	default: break;
	}
	return false;
}


//Compute generated IR-EXPR for BB.
void IR_DU_MGR::compute_gen_for_bb(IR_BB * bb, IN OUT DBITSETC & expr_univers,
								   MD_SET & tmp)
{
	DBITSETC * gen_ir_exprs = get_gen_ir_expr(IR_BB_id(bb));
	gen_ir_exprs->clean(*m_sbs_mgr);
	C<IR*> * ct;
	for (IR const* ir = IR_BB_ir_list(bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&ct)) {
		IS_TRUE0(ir->is_stmt());
		switch (IR_type(ir)) {
		case IR_ST:
			if (can_be_live_expr_cand(ST_rhs(ir))) {
				//Compute the generated expressions set.
				gen_ir_exprs->bunion(IR_id(ST_rhs(ir)), *m_sbs_mgr);
				expr_univers.bunion(IR_id(ST_rhs(ir)), *m_sbs_mgr);
			}
			//Fall through.
		case IR_STPR:
			if (ir->is_stpr() && can_be_live_expr_cand(STPR_rhs(ir))) {
				//Compute the generated expressions set.
				gen_ir_exprs->bunion(IR_id(STPR_rhs(ir)), *m_sbs_mgr);
				expr_univers.bunion(IR_id(STPR_rhs(ir)), *m_sbs_mgr);
			}
			//Fall through.
		case IR_IST:
			if (IR_type(ir) == IR_IST) {
				//Compute the generated expressions set.
				if (can_be_live_expr_cand(IST_rhs(ir))) {
					gen_ir_exprs->bunion(IR_id(IST_rhs(ir)), *m_sbs_mgr);
					expr_univers.bunion(IR_id(IST_rhs(ir)), *m_sbs_mgr);
				}
				if (IR_type(IST_base(ir)) != IR_ARRAY) {
					if (can_be_live_expr_cand(IST_base(ir))) {
						//e.g: *(int*)0x1000 = 10, IST_base(ir) is NULL.
						gen_ir_exprs->bunion(IR_id(IST_base(ir)), *m_sbs_mgr);
						expr_univers.bunion(IR_id(IST_base(ir)), *m_sbs_mgr);
					}
				}
			}

			{
				/* Compute lived IR expression after current statement executed.
				e.g:
					i = i + 1 //S1

					lhs 'i' killed the rhs expression: 'i + 1', that means
					'i + 1' is dead after S1 statement. */
				MD_SET const* maydef = get_may_def(ir);
				MD const* mustdef = ir->get_effect_ref();
				if (maydef != NULL || mustdef != NULL) {
					SC<SEG*> * st;
					for (INT j = gen_ir_exprs->get_first(&st), nj;
						 j != -1; j = nj) {
						nj = gen_ir_exprs->get_next(j, &st);

						IR * tir = m_ru->get_ir(j);
						IS_TRUE0(tir != NULL);

						if (IR_type(tir) == IR_LDA || IR_is_const(tir)) {
							continue;
						}

						tmp.clean();

						collect_may_use_recur(tir, tmp, true);
						//collect_may_use(tir, tmp, true);

						if ((maydef != NULL &&
							 maydef->is_intersect(tmp)) ||
							(mustdef != NULL &&
							 tmp.is_contain(mustdef))) {
							//'ir' killed 'tir'.
							gen_ir_exprs->diff(j, *m_sbs_mgr);
						}
					}
				}
			}
			break;
		case IR_CALL:
		case IR_ICALL:
			{
				//Compute the generated expressions set.
				if (IR_type(ir) == IR_ICALL) {
					IS_TRUE0(IR_type(ICALL_callee(ir)) == IR_LD);
					if (can_be_live_expr_cand(ICALL_callee(ir))) {
						gen_ir_exprs->bunion(IR_id(ICALL_callee(ir)), *m_sbs_mgr);
						expr_univers.bunion(IR_id(ICALL_callee(ir)), *m_sbs_mgr);
					}
				}

				IR * parm = CALL_param_list(ir);
				while (parm != NULL) {
					if (can_be_live_expr_cand(parm)) {
						gen_ir_exprs->bunion(IR_id(parm), *m_sbs_mgr);
						expr_univers.bunion(IR_id(parm), *m_sbs_mgr);
					}
					parm = IR_next(parm);
				}

				/* Compute lived IR expression after current statement executed.
				e.g:
					i = i + 1 //S1

					lhs 'i' killed the rhs expression: 'i + 1', that means
					'i + 1' is dead after S1 statement.	*/
				MD_SET const* maydef = get_may_def(ir);
				MD const* mustdef = ir->get_effect_ref();
				if (maydef != NULL || mustdef != NULL) {
					SC<SEG*> * st;
					for (INT j = gen_ir_exprs->get_first(&st), nj;
						 j != -1; j = nj) {
						nj = gen_ir_exprs->get_next(j, &st);
						IR * tir = m_ru->get_ir(j);
						IS_TRUE0(tir != NULL);
						if (IR_type(tir) == IR_LDA || IR_is_const(tir)) {
							continue;
						}

						tmp.clean();

						collect_may_use_recur(tir, tmp, true);
						//collect_may_use(tir, tmp, true);

						if ((maydef != NULL &&
							 maydef->is_intersect(tmp)) ||
							(mustdef != NULL &&
							 tmp.is_contain(mustdef))) {
							//'ir' killed 'tir'.
							gen_ir_exprs->diff(j, *m_sbs_mgr);
						}
					}
				}
			}
			break;
		case IR_REGION:
			{
				MD_SET const* maydef = REGION_ru(ir)->get_may_def();
				/* Compute lived IR expression after current statement executed.
				e.g:
					i = i + 1 //S1

					lhs 'i' killed the rhs expression: 'i + 1', that means
					'i + 1' is dead after S1 statement.
				*/
				if (maydef == NULL) { break; }

				MD_SET * tmp = m_mds_mgr->create();
				SC<SEG*> * st;
				bool first = true;
				for (INT j = gen_ir_exprs->get_first(&st), nj;
					 j != -1; j = nj) {
					nj = gen_ir_exprs->get_next(j, &st);
					IR * tir = m_ru->get_ir(j);
					IS_TRUE0(tir != NULL);
					if (IR_type(tir) == IR_LDA || IR_is_const(tir)) {
						continue;
					}
					if (first) {
						first = false;
					} else {
						tmp->clean();
					}

					collect_may_use_recur(tir, *tmp, true);
					//collect_may_use(tir, *tmp, true);

					if (maydef->is_intersect(*tmp)) {
						//'ir' killed 'tir'.
						gen_ir_exprs->diff(j, *m_sbs_mgr);
					}
				}
				m_mds_mgr->free(tmp);
			}
			break;
		case IR_GOTO:
			break;
		case IR_IGOTO:
			//Compute the generated expressions.
			if (can_be_live_expr_cand(IGOTO_vexp(ir))) {
				gen_ir_exprs->bunion(IR_id(IGOTO_vexp(ir)), *m_sbs_mgr);
				expr_univers.bunion(IR_id(IGOTO_vexp(ir)), *m_sbs_mgr);
			}
			break;
		case IR_DO_WHILE:
		case IR_WHILE_DO:
		case IR_DO_LOOP:
		case IR_IF:
		case IR_LABEL:
		case IR_CASE:
			IS_TRUE(0, ("TODO"));
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			//Compute the generated expressions.
			if (can_be_live_expr_cand(BR_det(ir))) {
				gen_ir_exprs->bunion(IR_id(BR_det(ir)), *m_sbs_mgr);
				expr_univers.bunion(IR_id(BR_det(ir)), *m_sbs_mgr);
			}
			break;
		case IR_SWITCH:
			//Compute the generated expressions.
			if (can_be_live_expr_cand(SWITCH_vexp(ir))) {
				gen_ir_exprs->bunion(IR_id(SWITCH_vexp(ir)), *m_sbs_mgr);
				expr_univers.bunion(IR_id(SWITCH_vexp(ir)), *m_sbs_mgr);
			}
			break;
		case IR_RETURN:
			if (RET_exp(ir) != NULL) {
				if (can_be_live_expr_cand(RET_exp(ir))) {
					gen_ir_exprs->bunion(IR_id(RET_exp(ir)), *m_sbs_mgr);
					expr_univers.bunion(IR_id(RET_exp(ir)), *m_sbs_mgr);
				}
			}
			break;
		case IR_PHI:
			for (IR * p = PHI_opnd_list(ir); p != NULL; p = IR_next(p)) {
				if (can_be_live_expr_cand(p)) {
					gen_ir_exprs->bunion(IR_id(p), *m_sbs_mgr);
					expr_univers.bunion(IR_id(p), *m_sbs_mgr);
				}
			}
			break;
		default: IS_TRUE0(0);
		} //end switch
	} //end for each IR
}


//Compute local-gen IR-EXPR set and killed IR-EXPR set.
//'expr_universe': record the universal of all ir-expr of region.
void IR_DU_MGR::comp_ir_expr(OUT DBITSETC * expr_universe,
							 SVECTOR<MD_SET*> const* maydefmds)
{
	IS_TRUE0(expr_universe && maydefmds);
	MD_SET * tmp = m_mds_mgr->create();
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	C<IR_BB*> * cb;
	for (IR_BB * bb = bbl->get_head(&cb); bb != NULL; bb = bbl->get_next(&cb)) {
		compute_gen_for_bb(bb, *expr_universe, *tmp);
	}

	//Compute kill-set. The DEF MD_SET of current ir, killed all
	//other exprs which use the MD_SET modified by 'ir'.
	for (IR_BB * bb = bbl->get_head(&cb); bb != NULL; bb = bbl->get_next(&cb)) {
		MD_SET const* bb_maydef = maydefmds->get(IR_BB_id(bb));
		IS_TRUE0(bb_maydef != NULL);

		//Get killed IR EXPR set.
		DBITSETC * killed_set = get_killed_ir_expr(IR_BB_id(bb));
		killed_set->clean(*m_sbs_mgr);

		SC<SEG*> * st;
		for (INT i = expr_universe->get_first(&st);
			 i != -1; i = expr_universe->get_next(i, &st)) {
			IR * ir = m_ru->get_ir(i);
			IS_TRUE0(ir->is_exp());
			if (IR_type(ir) == IR_LDA || IR_is_const(ir)) {
				continue;
			}

			tmp->clean();

			collect_may_use_recur(ir, *tmp, true);
			//collect_may_use(ir, *tmp, true);

			if (bb_maydef->is_intersect(*tmp)) {
				killed_set->bunion(i, *m_sbs_mgr);
			}
		}
	}
	m_mds_mgr->free(tmp);
}


//This equation needs May Kill Def and Must Gen Def.
bool IR_DU_MGR::for_avail_reach_def(UINT bbid, LIST<IR_BB*> & preds,
									LIST<IR_BB*> * lst)
{
	bool change = false;
	DBITSETC * news = m_sbs_mgr->create_dbitsetc();
	news->set_sparse(SOL_SET_IS_SPARSE);
	DBITSETC * in = get_avail_in_reach_def(bbid);
	bool first = true;
	for (IR_BB * p = preds.get_head(); p != NULL; p = preds.get_next()) {
		//Intersect
		if (first) {
			first = false;
			in->copy(*get_avail_out_reach_def(IR_BB_id(p)), *m_sbs_mgr);
		} else {
			in->intersect(*get_avail_out_reach_def(IR_BB_id(p)), *m_sbs_mgr);
			//in->bunion(*get_avail_out_reach_def(IR_BB_id(p)), *m_sbs_mgr);
		}
	}

	news->copy(*in, *m_sbs_mgr);
	news->diff(*get_may_killed_def(bbid), *m_sbs_mgr);
	news->bunion(*get_must_gen_def(bbid), *m_sbs_mgr);

	DBITSETC * out = get_avail_out_reach_def(bbid);
	if (!out->is_equal(*news)) {
		out->copy(*news, *m_sbs_mgr);
		change = true;

		#ifdef WORK_LIST_DRIVE
		IS_TRUE0(lst);
		VERTEX * bbv = m_cfg->get_vertex(bbid);
		EDGE_C const* ecs = VERTEX_out_list(bbv);
		while (ecs != NULL) {
			INT succ = VERTEX_id(EDGE_to(EC_edge(ecs)));
			IS_TRUE0(succ >= 0 && m_cfg->get_bb(succ));
			lst->append_tail(m_cfg->get_bb(succ));
			ecs = EC_next(ecs);
		}
		#endif
	}
	m_sbs_mgr->free_dbitsetc(news);
	return change;
}


bool IR_DU_MGR::for_reach_def(UINT bbid, LIST<IR_BB*> & preds,
							  LIST<IR_BB*> * lst)
{
	bool change = false;
	DBITSETC * in_reach_def = get_in_reach_def(bbid);
	DBITSETC * news = m_sbs_mgr->create_dbitsetc();
	news->set_sparse(SOL_SET_IS_SPARSE);
	bool first = true;
	for (IR_BB * p = preds.get_head(); p != NULL; p = preds.get_next()) {
		if (first) {
			in_reach_def->copy(*get_out_reach_def(IR_BB_id(p)), *m_sbs_mgr);
			first = false;
		} else {
			in_reach_def->bunion(*get_out_reach_def(IR_BB_id(p)), *m_sbs_mgr);
		}
	}
	if (first) {
		//bb does not have predecessor.
		IS_TRUE0(in_reach_def->is_empty());
	}

	news->copy(*in_reach_def, *m_sbs_mgr);
	news->diff(*get_must_killed_def(bbid), *m_sbs_mgr);
	news->bunion(*get_may_gen_def(bbid), *m_sbs_mgr);

	DBITSETC * out_reach_def = get_out_reach_def(bbid);
	if (!out_reach_def->is_equal(*news)) {
		out_reach_def->copy(*news, *m_sbs_mgr);
		change = true;

		#ifdef WORK_LIST_DRIVE
		IS_TRUE0(lst);
		VERTEX * bbv = m_cfg->get_vertex(bbid);
		EDGE_C const* ecs = VERTEX_out_list(bbv);
		while (ecs != NULL) {
			INT succ = VERTEX_id(EDGE_to(EC_edge(ecs)));
			IS_TRUE0(succ >= 0 && m_cfg->get_bb(succ));
			lst->append_tail(m_cfg->get_bb(succ));
			ecs = EC_next(ecs);
		}
		#endif
	}

	m_sbs_mgr->free_dbitsetc(news);
	return change;
}


bool IR_DU_MGR::for_avail_expr(UINT bbid, LIST<IR_BB*> & preds,
								LIST<IR_BB*> * lst)
{
	bool change = false;
	DBITSETC * news = m_sbs_mgr->create_dbitsetc();
	news->set_sparse(SOL_SET_IS_SPARSE);

	bool first = true;
	DBITSETC * in = get_availin_expr(bbid);
	for (IR_BB * p = preds.get_head(); p != NULL; p = preds.get_next()) {
		DBITSETC * liveout = get_availout_expr(IR_BB_id(p));
		if (first) {
			first = false;
			in->copy(*liveout, *m_sbs_mgr);
		} else {
			in->intersect(*liveout, *m_sbs_mgr);
		}
	}

	news->copy(*in, *m_sbs_mgr);
	news->diff(*get_killed_ir_expr(bbid), *m_sbs_mgr);
	news->bunion(*get_gen_ir_expr(bbid), *m_sbs_mgr);
	DBITSETC * out = get_availout_expr(bbid);
	if (!out->is_equal(*news)) {
		out->copy(*news, *m_sbs_mgr);
		change = true;

		#ifdef WORK_LIST_DRIVE
		IS_TRUE0(lst);
		VERTEX * bbv = m_cfg->get_vertex(bbid);
		EDGE_C const* ecs = VERTEX_out_list(bbv);
		while (ecs != NULL) {
			INT succ = VERTEX_id(EDGE_to(EC_edge(ecs)));
			IS_TRUE0(succ >= 0 && m_cfg->get_bb(succ));
			lst->append_tail(m_cfg->get_bb(succ));
			ecs = EC_next(ecs);
		}
		#endif
	}
	m_sbs_mgr->free_dbitsetc(news);
	return change;
}


/* Solve reaching definitions problem for IR STMT and
computing LIVE IN and LIVE OUT IR expressions.

'ir_expr_universal_set': that is to be the Universal SET for IR_EXPR. */
void IR_DU_MGR::solve(DBITSETC const* expr_univers, UINT const flag)
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_tail(); bb != NULL; bb = bbl->get_prev()) {
		UINT bbid = IR_BB_id(bb);
		if (HAVE_FLAG(flag, SOL_REACH_DEF)) {
			//Initialize reach-def IN, reach-def OUT.
			get_out_reach_def(bbid)->clean(*m_sbs_mgr);
			get_in_reach_def(bbid)->clean(*m_sbs_mgr);
		}
		if (HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF)) {
			get_avail_out_reach_def(bbid)->clean(*m_sbs_mgr);
			get_avail_in_reach_def(bbid)->clean(*m_sbs_mgr);
		}
		if (HAVE_FLAG(flag, SOL_AVAIL_EXPR)) {
			//Initialize available in, available out expression.
			//IN-SET of BB must be universal of all IR-expressions.
			DBITSETC * availin = get_availin_expr(bbid);
			availin->copy(*expr_univers, *m_sbs_mgr);
			DBITSETC * liveout = get_availout_expr(bbid);
			liveout->copy(*availin, *m_sbs_mgr);
			liveout->diff(*get_killed_ir_expr(bbid), *m_sbs_mgr);
			liveout->bunion(*get_gen_ir_expr(bbid), *m_sbs_mgr);
		}
	}

	//Rpo already checked to be available. Here double check again.
	LIST<IR_BB*> * tbbl = m_cfg->get_bblist_in_rpo();
	IS_TRUE0(tbbl->get_elem_count() == bbl->get_elem_count());
	LIST<IR_BB*> preds;
	LIST<IR_BB*> lst;
#ifdef WORK_LIST_DRIVE
	for (IR_BB * bb = tbbl->get_head(); bb != NULL; bb = tbbl->get_next()) {
		lst.append_tail(bb);
	}
	UINT count = tbbl->get_elem_count() * 20;
	UINT i = 0; //time of bb accessed.
	do {
		IR_BB * bb = lst.remove_head();
		UINT bbid = IR_BB_id(bb);
		preds.clean();
		m_cfg->get_preds(preds, bb);
		if (HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF)) {
			for_avail_reach_def(bbid, preds, &lst);
		}
		if (HAVE_FLAG(flag, SOL_REACH_DEF)) {
			for_reach_def(bbid, preds, &lst);
		}
		if (HAVE_FLAG(flag, SOL_AVAIL_EXPR)) {
			for_avail_expr(bbid, preds, &lst);
		}
		i++;
	} while (lst.get_elem_count() != 0);
	IS_TRUE0(i < count);
#else
	bool change;
	UINT count = 0;
	do {
		change = false;
		for (IR_BB * bb = tbbl->get_head(); bb != NULL; bb = tbbl->get_next()) {
			UINT bbid = IR_BB_id(bb);
			preds.clean();
			m_cfg->get_preds(preds, bb);
			if (HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF)) {
				change |= for_avail_reach_def(bbid, preds, NULL);
			}
			if (HAVE_FLAG(flag, SOL_REACH_DEF)) {
				change |= for_reach_def(bbid, preds, NULL);
			}
			if (HAVE_FLAG(flag, SOL_AVAIL_EXPR)) {
				change |= for_avail_expr(bbid, preds, NULL);
			}
		}
		count++;
	} while (change && count < 20);
	UINT i = count * tbbl->get_elem_count(); //time of bb accessed.
	IS_TRUE0(!change);
#endif
}


/* Check if stmt is killing-define to exp.
This function matchs the pattern, where dt means data-type and
they must be identical.
	ist(dt, ofst:n), x = ...
	... = ild(dt, ofst:n), x
Stmt and exp must be in same bb. */
UINT IR_DU_MGR::check_is_local_killing_def(IR const* stmt, IR const* exp,
											C<IR*> * expct)
{
	IS_TRUE0(stmt->get_bb() == exp->get_stmt()->get_bb());

	if (IR_type(exp) != IR_ILD || IR_type(stmt) != IR_IST) { return CK_UNKNOWN; }

	IR const* t = ILD_base(exp);

	while (IR_type(t) == IR_CVT) { t = CVT_exp(t); }

	if (!t->is_pr() && IR_type(t) != IR_LD) { return CK_UNKNOWN; }

	IR const* t2 = IST_base(stmt);

	while (IR_type(t2) == IR_CVT) { t2 = CVT_exp(t2); }

	if (!t2->is_pr() && IR_type(t2) != IR_LD) { return CK_UNKNOWN; }

	if (IR_type(t) != IR_type(t2)) { return CK_UNKNOWN; }

	IR_BB * curbb = stmt->get_bb();

	/* Note, the defset of t must be avaiable here.
	And t could not be defined between stmt and exp.
	e.g:
		*base = ...
		... //base can not be modified in between ist and ild.
		= *base
	*/
	DU_SET const* defset_of_t = t->get_duset_c();
	if (defset_of_t != NULL) {
		DU_ITER di;
		for (UINT d = defset_of_t->get_first(&di);
			 di != NULL; d = defset_of_t->get_next(d, &di)) {
			IR const* def_of_t = get_ir(d);
			IS_TRUE0(def_of_t->is_stmt());
			if (def_of_t->get_bb() != curbb) { continue; }

			//Find the def that clobber exp after the stmt.
			C<IR*> * localct(expct);
			for (IR * ir = IR_BB_ir_list(curbb).get_prev(&localct);
				 ir != NULL; ir = IR_BB_ir_list(curbb).get_prev(&localct)) {
				if (ir == stmt) { break; }
				else if (ir == def_of_t) { return CK_UNKNOWN; }
			}
		}
	}

	if (IR_dt(exp) == VOID_TY || IR_dt(stmt) == VOID_TY) {
		return CK_OVERLAP;
	}

	UINT exptysz = exp->get_dt_size(m_dm);
	UINT stmttysz = stmt->get_dt_size(m_dm);
	if ((((ILD_ofst(exp) + exptysz) <= IST_ofst(stmt)) ||
		((IST_ofst(stmt) + stmttysz) <= ILD_ofst(exp)))) {
		return CK_NOT_OVERLAP;
	}
	return CK_OVERLAP;
}


/* Find nearest killing def to md in bb.
Here we search exactly killing DEF from current stmt to previous
for expmd even if it is exact,
e.g: g is global variable, it is exact.
x is a pointer that we do not know where it pointed to.
	1. *x += 1;
	2. g = 0;
	3. *x += 2; # *x may overlapped with global variable g.
	4. return g;
In the case, the last reference of g in stmt 4 may be defined by
stmt 2 or 3. */
IR const* IR_DU_MGR::find_killing_local_def(
					IR const* stmt, C<IR*> * ct,
					IR const* exp, MD const* expmd)
{
	IS_TRUE0(!exp->is_pr() || is_compute_pr_du_chain());

	IR_BB * bb = stmt->get_bb();

	C<IR*> * localct(ct);
	for (IR * ir = IR_BB_ir_list(bb).get_prev(&localct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_prev(&localct)) {
		if (!ir->is_memory_ref()) { continue; }

		MD const* mustdef = ir->get_ref_md();
		if (mustdef != NULL) {
			if (mustdef == expmd || mustdef->is_overlap(expmd)) {
				if (mustdef->is_exact()) {
					return ir;
				}

				UINT result = check_is_local_killing_def(ir, exp, ct);
				if (result == CK_OVERLAP) {
					return ir;
				} else if (result == CK_NOT_OVERLAP) {
					continue;
				}

				//ir is neither inexact, nor the nonkilling def of exp.
				return NULL;
			}

			//If both def and exp has must reference, we do not need to
			//check maydef.
			continue;
		}

		//We need to check maydef.
		MD_SET const* maydefs = ir->get_ref_mds();
		if (maydefs != NULL && maydefs->is_contain(expmd)) {
			//There is a nonkilling DEF, ir may modified expmd.
			//The subsequent searching is meaningless.
			//We can not find a local killing DEF.
			return NULL;
		}
	}

	//We can not find a local killing DEF.
	return NULL;
}


//Build DU chain for exp and local killing def stmt.
//Return true if find local killing def, otherwise means
//there are not local killing def.
bool IR_DU_MGR::build_local_du_chain(IR const* stmt, IR const* exp,
									 MD const* expmd, DU_SET * expdu,
									 C<IR*> * ct)
{
	IR const* nearest_def = find_killing_local_def(stmt, ct, exp, expmd);
	if (nearest_def == NULL) { return false; }

	IS_TRUE0(expdu);
	expdu->add_def(nearest_def, *m_sbs_mgr);

	DU_SET * xdu = nearest_def->get_duset();
	IS_TRUE0(xdu);
	if (!m_is_init->is_contain(IR_id(nearest_def))) {
		m_is_init->bunion(IR_id(nearest_def));
		xdu->clean(*m_sbs_mgr);
	}
	xdu->add_use(exp, *m_sbs_mgr);
	return true;
}


//Check memory operand and build DU chain for them.
//Note we always find the nearest exact def, and build
//the DU between the def and its use.
void IR_DU_MGR::check_and_build_chain_recur(IR * stmt, IR * exp, C<IR*> * ct)
{
	IS_TRUE0(exp && exp->is_exp());
	switch (IR_type(exp)) {
	case IR_LD:
		break;
	case IR_PR:
		if (!is_compute_pr_du_chain()) { return; }
		break;
	case IR_ILD:
		check_and_build_chain_recur(stmt, ILD_base(exp), ct);
		break;
	case IR_ARRAY:
		for (IR * sub = ARR_sub_list(exp);
			 sub != NULL; sub = IR_next(sub)) {
			check_and_build_chain_recur(stmt, sub, ct);
		}
	 	check_and_build_chain_recur(stmt, ARR_base(exp), ct);
		break;
	case IR_CONST:
	case IR_LDA:
		return;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_LAND:
	case IR_LOR:
		check_and_build_chain_recur(stmt, BIN_opnd0(exp), ct);
		check_and_build_chain_recur(stmt, BIN_opnd1(exp), ct);
		return;
	case IR_SELECT:
		check_and_build_chain_recur(stmt, SELECT_det(exp), ct);
		check_and_build_chain_recur(stmt, SELECT_trueexp(exp), ct);
		check_and_build_chain_recur(stmt, SELECT_falseexp(exp), ct);
		return;
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
		check_and_build_chain_recur(stmt, UNA_opnd0(exp), ct);
		return;
	case IR_CVT:
		check_and_build_chain_recur(stmt, CVT_exp(exp), ct);
		return;
	case IR_CASE:
		check_and_build_chain_recur(stmt, CASE_vexp(exp), ct);
		return;
	default: IS_TRUE0(0);
	}

	DU_SET * xdu = get_du_alloc(exp);
	if (!m_is_init->is_contain(IR_id(exp))) {
		m_is_init->bunion(IR_id(exp));
		xdu->clean(*m_sbs_mgr);
	}

	MD const* xmd = get_must_use(exp);
	bool has_local_killing_def = false;
	if (xmd != NULL) {
		IS_TRUE(xmd->is_effect(), ("MustUse should be effect"));
		has_local_killing_def =
			build_local_du_chain(stmt, exp, xmd, xdu, ct);
	}

	if (has_local_killing_def) { return; }

	if (m_is_stmt_has_multi_result) {
		MD_SET const* xmds = get_may_use(exp);
		if (xmds != NULL) {
			build_chain_for_mds(exp, xmd, *xmds, xdu);
		}
		if (xmd != NULL && (xmds == NULL || !xmds->is_contain(xmd))) {
			//If x do neither have local def nor be computed already,
			//try to find def for mustref of x.
			build_chain_for_md(exp, xmd, xdu);
		}
	} else {
		if (xmd != NULL) {
			//Find nonkilling def for must-use of x.
			build_chain_for_must(exp, xmd, xdu);
		}

		if (xmd == NULL || m_md2irs->has_ineffect_def_stmt()) {
			MD_SET const* xmds = get_may_use(exp);
			if (xmds != NULL) {
				build_chain_for_mds(exp, xmd, *xmds, xdu);
			}
		}
	}
}


//Check memory operand and build DU chain for them.
//Note we always find the nearest exact def, and build
//the DU between the def and its use.
void IR_DU_MGR::check_and_build_chain(IR * stmt, C<IR*> * ct)
{
	IS_TRUE0(stmt->is_stmt());
	switch (IR_type(stmt)) {
	case IR_ST:
		{
			DU_SET * du = get_du_alloc(stmt);
			if (!m_is_init->is_contain(IR_id(stmt))) {
				m_is_init->bunion(IR_id(stmt));
				du->clean(*m_sbs_mgr);
			}

			check_and_build_chain_recur(stmt, stmt->get_rhs(), ct);
			return;
		}
	case IR_STPR:
		{
			if (is_compute_pr_du_chain()) {
				DU_SET * du = get_du_alloc(stmt);
				if (!m_is_init->is_contain(IR_id(stmt))) {
					m_is_init->bunion(IR_id(stmt));
					du->clean(*m_sbs_mgr);
				}
			}
			check_and_build_chain_recur(stmt, stmt->get_rhs(), ct);
			return;
		}
	case IR_IST:
		{
			DU_SET * du = get_du_alloc(stmt);
			if (!m_is_init->is_contain(IR_id(stmt))) {
				m_is_init->bunion(IR_id(stmt));
				du->clean(*m_sbs_mgr);
			}

			if (stmt->is_st_array()) {
				IR * arr = IST_base(stmt);
				for (IR * sub = ARR_sub_list(arr);
					 sub != NULL; sub = IR_next(sub)) {
					check_and_build_chain_recur(stmt, sub, ct);
				}
			 	check_and_build_chain_recur(stmt, ARR_base(arr), ct);
			} else {
				check_and_build_chain_recur(stmt, IST_base(stmt), ct);
			}
			check_and_build_chain_recur(stmt, IST_rhs(stmt), ct);
			return;
		}
	case IR_CALL:
		for (IR * p = CALL_param_list(stmt); p != NULL; p = IR_next(p)) {
			check_and_build_chain_recur(stmt, p, ct);
		}
		return;
	case IR_ICALL:
		for (IR * p = CALL_param_list(stmt); p != NULL; p = IR_next(p)) {
			check_and_build_chain_recur(stmt, p, ct);
		}
		check_and_build_chain_recur(stmt, ICALL_callee(stmt), ct);
		return;
	case IR_PHI:
		{
			if (is_compute_pr_du_chain()) {
				DU_SET * du = get_du_alloc(stmt);
				if (!m_is_init->is_contain(IR_id(stmt))) {
					m_is_init->bunion(IR_id(stmt));
					du->clean(*m_sbs_mgr);
				}
				for (IR * opnd = PHI_opnd_list(stmt);
					 opnd != NULL; opnd = IR_next(opnd)) {
					check_and_build_chain_recur(stmt, opnd, ct);
				}
			}
			return;
		}
	case IR_REGION:
	case IR_GOTO:
		return;
	case IR_IGOTO:
	case IR_SWITCH:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_RETURN:
		for (UINT i = 0; i < IR_MAX_KID_NUM(stmt); i++) {
			IR * kid = stmt->get_kid(i);
			if (kid == NULL) { continue; }
			check_and_build_chain_recur(stmt, kid, ct);
		}
		return;
	default: IS_TRUE(0, ("unsupport"));
	}
}


/* Check if stmt killing define exp, stmt and exp may be at same bb.
e.g: BB is loop body,
	start:
	= *t
	...
	*t = ...
	goto start

*/
UINT IR_DU_MGR::check_is_nonlocal_killing_def(IR const* stmt, IR const* exp)
{
	IS_TRUE0(m_oc);
	if (!OPTC_is_live_expr_valid(*m_oc)) { return CK_UNKNOWN; }

	if (IR_type(exp) != IR_ILD || IR_type(stmt) != IR_IST) { return CK_UNKNOWN; }

	IR const* t = ILD_base(exp);
	while (IR_type(t) == IR_CVT) { t = CVT_exp(t); }
	if (IR_type(t) != IR_PR && IR_type(t) != IR_LD) { return CK_UNKNOWN; }

	IR const* t2 = IST_base(stmt);
	while (IR_type(t2) == IR_CVT) { t2 = CVT_exp(t2); }
	if (IR_type(t2) != IR_PR && IR_type(t2) != IR_LD) { return CK_UNKNOWN; }

	if (IR_type(t) != IR_type(t2)) { return CK_UNKNOWN; }

	/* Note, t could not be modified in the path between stmt and exp.
	e.g:
		*t = ...
		... //t can not be defined.
		= *t
	*/
	DBITSETC const* lived_in_expr =
				get_availin_expr(IR_BB_id(exp->get_stmt()->get_bb()));
	if (!lived_in_expr->is_contain(IR_id(t2))) { return CK_UNKNOWN; }

	UINT exptysz = exp->get_dt_size(m_dm);
	UINT stmttysz = stmt->get_dt_size(m_dm);
	if ((((ILD_ofst(exp) + exptysz) <= IST_ofst(stmt)) ||
		((IST_ofst(stmt) + stmttysz) <= ILD_ofst(exp)))) {
		return CK_NOT_OVERLAP;
	}
	return CK_OVERLAP;
}


//Check and build DU chain for operand accroding to md.
void IR_DU_MGR::build_chain_for_must(IR const* exp, MD const* expmd,
									DU_SET * expdu)
{
	IS_TRUE0(exp && expmd && expdu);
	SC<SEG*> * sc;
	UINT id = IR_id(exp);
	INT mdid = MD_id(expmd);
	SBITSETC const* defset = m_md2irs->get_c(mdid);
	if (defset == NULL) { return; }

	IR_BB * curbb = exp->get_stmt()->get_bb();
	for (INT d = defset->get_first(&sc);
		 d >= 0; d = defset->get_next(d, &sc)) {
		IR * stmt = m_ru->get_ir(d);
		IS_TRUE0(stmt->is_stmt());
		bool build_du = false;

		MD const* m;
		MD_SET const* ms;
		if ((m = get_must_def(stmt)) != NULL) {
			//If def has must reference, we do not consider
			//maydef set any more.
			IS_TRUE(!MD_is_may(m), ("m can not be mustdef."));
			if (expmd == m || expmd->is_overlap(m)) {
				if (m->is_exact()) {
					build_du = true;
				} else if (stmt->get_bb() == curbb) {
					/* If stmt is at same bb with exp, then
					we can not determine whether they are independent,
					because if they are, the situation should be processed
					in build_local_du_chain().
					Build du chain for conservative purpose. */
					build_du = true;
				} else {
					UINT result = check_is_nonlocal_killing_def(stmt, exp);
					if (result == CK_OVERLAP || result == CK_UNKNOWN) {
						build_du = true;
					}
				}
			}
		} else if ((ms = get_may_def(stmt)) != NULL && ms->is_contain(expmd)) {
			//Check to find Nonkilling Def if stmt's maydef set contain expmd.
			build_du = true;
		}

		if (build_du) {
			//Build DU chain between exp and stmt.
			expdu->add(d, *m_sbs_mgr);

			DU_SET * def_useset = get_du_alloc(stmt);
			if (!m_is_init->is_contain(d)) {
				m_is_init->bunion(d);
				def_useset->clean(*m_sbs_mgr);
			}
			def_useset->add(id, *m_sbs_mgr);
		}
	}
}


//Check and build DU chain for operand accroding to md.
void IR_DU_MGR::build_chain_for_md(IR const* exp, MD const* expmd,
								   DU_SET * expdu)
{
	IS_TRUE0(exp && expmd && expdu);
	SC<SEG*> * sc;
	UINT id = IR_id(exp);
	INT mdid = MD_id(expmd);
	SBITSETC const* defset = m_md2irs->get_c(mdid);
	if (defset == NULL) { return; }

	for (INT d = defset->get_first(&sc);
		 d >= 0; d = defset->get_next(d, &sc)) {
		IR * stmt = m_ru->get_ir(d);
		IS_TRUE0(stmt->is_stmt());

		bool build_du = false;
		MD const* m;
		MD_SET const* ms;
		if ((m = get_must_def(stmt)) == expmd) {
			//If mustdef is exact, killing def, otherwise nonkilling def.
			build_du = true;
		} else if ((ms = get_may_def(stmt)) != NULL && ms->is_contain(expmd)) {
			//Nonkilling Def.
			build_du = true;
		}
		if (build_du) {
			expdu->add(d, *m_sbs_mgr);

			DU_SET * def_useset = get_du_alloc(stmt);
			if (!m_is_init->is_contain(d)) {
				m_is_init->bunion(d);
				def_useset->clean(*m_sbs_mgr);
			}
			def_useset->add(id, *m_sbs_mgr);
		}
	}
}


//Check and build DU chain for operand accroding to mds.
void IR_DU_MGR::build_chain_for_mds(IR const* exp, MD const* expmd,
									MD_SET const& expmds, DU_SET * expdu)
{
	IS_TRUE0(expdu);
	SC<SEG*> * sc;
	UINT id = IR_id(exp);
	IR_BB * curbb = exp->get_stmt()->get_bb();

	for (INT u = expmds.get_first(); u >= 0; u = expmds.get_next(u)) {
		SBITSETC const* defset = m_md2irs->get_c(u);
		if (defset == NULL) { continue; }

		MD const* umd = m_md_sys->get_md(u);
		for (INT d = defset->get_first(&sc);
			 d >= 0; d = defset->get_next(d, &sc)) {
			IR * stmt = m_ru->get_ir(d);
			IS_TRUE0(stmt->is_stmt());

			bool build_du = false;
			MD const* m;
			MD_SET const* ms;
			if (expmd != NULL && (m = get_must_def(stmt)) != NULL) {
				//If def has must reference, we do not consider
				//maydef set any more.
				IS_TRUE(!MD_is_may(m), ("m can not be mustdef."));
				if (expmd == m || expmd->is_overlap(m)) {
					if (m->is_exact()) {
						build_du = true;
					} else if (stmt->get_bb() == curbb) {
						/* If stmt is at same bb with exp, then
						we can not determine whether they are independent,
						because if they are, the situation should be processed
						in build_local_du_chain().
						Build du chain for conservative purpose. */
						build_du = true;
					} else {
						UINT result = check_is_nonlocal_killing_def(stmt, exp);
						if (result == CK_OVERLAP || result == CK_UNKNOWN) {
							build_du = true;
						}
					}
				}
			} else if ((ms = get_may_def(stmt)) != NULL &&
					   (ms == &expmds || ms->is_intersect(expmds))) {
				//Nonkilling Def.
				build_du = true;
			}

			if (build_du) {
				expdu->add(d, *m_sbs_mgr);

				DU_SET * def_useset = get_du_alloc(stmt);
				if (!m_is_init->is_contain(d)) {
					m_is_init->bunion(d);
					def_useset->clean(*m_sbs_mgr);
				}
				def_useset->add(id, *m_sbs_mgr);
			}
		}
	}
}


void IR_DU_MGR::update_region(IR * ir)
{
	MD_SET const* mustdef = REGION_ru(ir)->get_must_def();
	if (mustdef != NULL && !mustdef->is_empty()) {
		for (INT i = mustdef->get_first();
			 i >= 0; i = mustdef->get_next(i)) {
			m_md2irs->set(i, ir);
		}
	} else {
		m_md2irs->set_mark_stmt_only_has_maydef();
	}

	MD_SET const* maydef = REGION_ru(ir)->get_may_def();
	if (maydef != NULL) {
		//May kill DEF stmt of overlap-set of md.
		for (INT i = maydef->get_first();
			 i >= 0; i = maydef->get_next(i)) {
			IS_TRUE0(m_md_sys->get_md(i));
			m_md2irs->append(i, ir);
		}
	}
}


//Return true if all element in mds are not PR.
static bool not_be_pr(MD_SET const* mds, MD_SYS * mdsys)
{
	for (INT i = mds->get_first();
		 i >= 0; i = mds->get_next(i)) {
		MD const* md = mdsys->get_md(i);
		IS_TRUE0(!MD_is_pr(md));
	}
	return true;
}


void IR_DU_MGR::update_def(IR * ir)
{
	switch (IR_type(ir)) {
	case IR_REGION:
		update_region(ir);
		return;
	case IR_STPR:
	case IR_CALL:
	case IR_ICALL:
	case IR_PHI:
		if (!is_compute_pr_du_chain()) { return; }
		break;
	default: if (!ir->has_result()) { return; }
	}

	IS_TRUE0(ir->is_stid() || IR_type(ir) == IR_IST ||
			 ir->is_stpr() || ir->is_phi() || ir->is_call());

	//Handle general stmt.

	MD const* must = ir->get_ref_md();
	if (must != NULL && must->is_exact()) {
		//Handle exactly killing def. This def kills
		//prior DEF stmt to exact md.
		m_md2irs->set(MD_id(must), ir);

		/* Pick off stmt from md's definition-list.
		e.g: Assume the md of struct {int a;} s is MD13, and s.a is MD4,
		then MD4 and MD13 are overlapped.
			MD13 has overlap-def in list:
				s.a = 10;
		When we meet exact definition to MD4, it kill all lived
		stmts that exact-def MD4, also include the
		stmt in MD13's def-list. */
		MD_SET const* maydef = get_may_def(ir);
		if (maydef != NULL) {
			for (INT i = maydef->get_first(); i >= 0; i = maydef->get_next(i)) {
				if (MD_id(must) == (UINT)i) { continue; }

				SBITSETC * dlst = m_md2irs->get(i);
				if (dlst == NULL) { continue; }

				SC<SEG*> * sc;
				INT nk;
				for (INT k = dlst->get_first(&sc); k >= 0; k = nk) {
					nk = dlst->get_next(k, &sc);
					IR * stmt = m_ru->get_ir(k);
					IS_TRUE0(stmt && stmt->is_stmt());
					MD const* w = stmt->get_exact_ref();
					if (must == w || (w != NULL && must->is_cover(w))) {
						//Current ir kills stmt k.
						dlst->diff(k, *m_sbs_mgr);
					}
				}
			}
		}
	} else {
		//Handle inexactly nonkilling DEF.
		//And alloc overlapped MD_SET for DEF as maydef.
		if (must != NULL) {
			IS_TRUE0(must->is_effect());
			m_md2irs->append(must, ir);
		} else {
			m_md2irs->set_mark_stmt_only_has_maydef();
		}

		MD_SET const* maydef = get_may_def(ir);
		if (maydef != NULL) {
			//May kill DEF stmt of overlapped set of md.
			if (must != NULL) {
				UINT mustid = MD_id(must);
				for (INT i = maydef->get_first();
					 i >= 0; i = maydef->get_next(i)) {
					if (mustid == (UINT)i) {
						//Already add to md2irs.
						continue;
					}

					IS_TRUE0(m_md_sys->get_md(i));
					m_md2irs->append(i, ir);
				}
			} else {
				for (INT i = maydef->get_first();
					 i >= 0; i = maydef->get_next(i)) {
					IS_TRUE0(m_md_sys->get_md(i));
					m_md2irs->append(i, ir);
				}
			}
		}
	}
}


//Initialize md2maydef_irs for each bb.
void IR_DU_MGR::init_md2irs(IR_BB * bb)
{
	DBITSETC * reach_def_irs = get_in_reach_def(IR_BB_id(bb));
	m_md2irs->clean();

	//Record DEF IR STMT for each MD.
	SC<SEG*> * st;
	for (INT i = reach_def_irs->get_first(&st);
		 i != -1; i = reach_def_irs->get_next(i, &st)) {
		IR const* stmt = m_ru->get_ir(i);
		/* stmt may be PHI, REGION, CALL.
		If stmt is IR_PHI, its maydef is NULL.
		If stmt is IR_REGION, its mustdef is NULL, but the maydef
		may be not empty. */
		MD const* mustdef = get_must_def(stmt);
		if (mustdef != NULL) {
			//mustdef may be fake object.
			//IS_TRUE0(mustdef->is_effect());
			m_md2irs->append(mustdef, i);
		}

		if (mustdef == NULL) {
			m_md2irs->set_mark_stmt_only_has_maydef();
		}

		MD_SET const* maydef = get_may_def(stmt);
		if (maydef != NULL) {
			for (INT j = maydef->get_first();
				 j != -1; j = maydef->get_next(j)) {
				if (mustdef != NULL && MD_id(mustdef) == (UINT)j) {
					continue;
				}
				IS_TRUE0(m_md_sys->get_md(j) != NULL);
				m_md2irs->append(j, i);
			}
		}
	}
}


/* Compute inexactly DU chain.
'is_init': record initialized DU.

NOTICE: The Reach-Definition and Must-Def, May-Def,
May Use must be avaliable. */
void IR_DU_MGR::compute_bb_du(IN IR_BB * bb, MD_SET & tmp1, MD_SET & tmp2)
{
	init_md2irs(bb);
	C<IR*> * ct;
	for (IR * ir = IR_BB_ir_list(bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&ct)) {
		if (!ir->is_contain_mem_ref()) { continue; }

		//Process USE
		check_and_build_chain(ir, ct);

		//Process DEF.
		update_def(ir);
	} //end for
}


bool IR_DU_MGR::verify_livein_exp()
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	SC<SEG*> * st;
	for (IR_BB * bb = bbl->get_head();
		 bb != NULL; bb = bbl->get_next()) {
		DBITSETC * x = get_availin_expr(IR_BB_id(bb));
		for (INT i = x->get_first(&st); i != -1; i = x->get_next(i, &st)) {
			IR * y = m_ru->get_ir(i);
			IS_TRUE0(y && y->is_exp());
		}
	}
	return true;
}


bool IR_DU_MGR::check_is_truely_dep(MD const* mustdef, MD_SET const* maydef,
									IR const* def, IR const* use)
{
	MD const* mustuse = use->get_ref_md();
	MD_SET const* mayuse = use->get_ref_mds();
	if (mustdef != NULL) {
		if (mustuse != NULL) {
			IS_TRUE0(mustdef == mustuse || mustdef->is_overlap(mustuse));
		} else if (mayuse != NULL) {
			IS_TRUE0(mayuse->is_overlap(mustdef));
		} else {
			IS_TRUE(0, ("Not a truely dependence"));
		}
	} else if (maydef != NULL) {
		if (mustuse != NULL) {
			IS_TRUE0(maydef->is_overlap(mustuse));
		} else if (mayuse != NULL) {
			IS_TRUE0(mayuse->is_intersect(*maydef));
		} else {
			IS_TRUE(0, ("Not a truely dependence"));
		}
	} else {
		IS_TRUE(0, ("Not a truely dependence"));
	}
	return true;
}


//Verify if DU chain is correct between each Def and Use of MD.
bool IR_DU_MGR::verify_du_info(IR const* ir)
{
	bool precision_check = true;
	IS_TRUE0(ir->is_stmt());
	MD const* mustdef = ir->get_ref_md();
	MD_SET const* maydef = ir->get_ref_mds();

	DU_SET const* useset = get_du_c(ir);
	if (useset != NULL) {
		DU_ITER di;
		for (INT i = useset->get_first(&di);
			 i >= 0; i = useset->get_next(i, &di)) {
			IR const* use = m_ru->get_ir(i);
			IS_TRUE0(use->is_exp());

			//Check the existence to 'use'.
			IR * tmp = use->get_stmt();
			IS_TRUE0(tmp && tmp->get_bb());
			IS_TRUE0(IR_BB_ir_list(tmp->get_bb()).find(tmp));

			//use must be a memory operation.
			IS_TRUE0(use->is_memory_opnd());

			DU_SET const* use_defset = get_du_c(use);

			//ir must be def of 'use'.
			IS_TRUE0(use_defset);

			//Check consistence between ir and use du info.
			IS_TRUE0(use_defset->is_contain(IR_id(ir)));

			if (precision_check) {
				IS_TRUE0(check_is_truely_dep(mustdef, maydef, ir, use));
			}
		}
	}

	m_citer.clean();
	for (IR const* u = ir_iter_rhs_init_c(ir, m_citer);
		 u != NULL; u = ir_iter_rhs_next_c(m_citer)) {
		IS_TRUE0(!ir->is_lhs(u) && u->is_exp());

		DU_SET const* defset = get_du_c(u);
		if (defset == NULL) { continue; }

		IS_TRUE(u->is_memory_opnd(), ("only memory operand has DU_SET"));
		DU_ITER di;
		for (INT i = defset->get_first(&di);
			 i >= 0; i = defset->get_next(i, &di)) {
			IR const* def = m_ru->get_ir(i);
			IS_TRUE0(def->is_stmt());

			//Check the existence to 'def'.
			IS_TRUE0(def->get_bb());
			IS_TRUE0(IR_BB_ir_list(def->get_bb()).find(const_cast<IR*>(def)));

			DU_SET const* def_useset = get_du_c(def);

			//u must be use of 'def'.
			IS_TRUE0(def_useset);

			//Check consistence between DEF and USE.
			IS_TRUE0(def_useset->is_contain(IR_id(u)));

			if (precision_check) {
				IS_TRUE0(check_is_truely_dep(def->get_ref_md(),
						 def->get_ref_mds(), def, u));
			}
		}
	}
	return true;
}


//Verify DU chain's sanity.
bool IR_DU_MGR::verify_du_chain()
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head();
		 bb != NULL; bb = bbl->get_next()) {
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			verify_du_info(ir);
	 	}
	}
	return true;
}


//Verify MD reference to stmts and expressions.
bool IR_DU_MGR::verify_du_ref()
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head();
		 bb != NULL; bb = bbl->get_next()) {
		for (IR * ir = IR_BB_first_ir(bb); ir != NULL; ir = IR_BB_next_ir(bb)) {
			m_citer.clean();
			for (IR const* t = ir_iter_init_c(ir, m_citer);
				 t != NULL; t = ir_iter_next_c(m_citer)) {
				switch (IR_type(t)) {
				case IR_ID:
					//We do not need MD or MDSET information of IR_ID.
					//IS_TRUE0(get_exact_ref(t));

					IS_TRUE0(get_may_use(t) == NULL);
					break;
				case IR_LD:
					IS_TRUE0(t->get_exact_ref());
					IS_TRUE0(!MD_is_pr(t->get_exact_ref()));
					/* MayUse of ld may not empty.
					e.g: cvt(ld(id(x,i8)), i32) x has exact md4(size=1), and
					an overlapped md5(size=4). */

					if (t->get_ref_mds() != NULL) {
						IS_TRUE0(m_mds_hash->find(*t->get_ref_mds()));
					}
					break;
				case IR_PR:
					IS_TRUE0(t->get_exact_ref());
					IS_TRUE0(MD_is_pr(t->get_exact_ref()));
					if (is_pr_unique_for_same_no()) {
						IS_TRUE0(get_may_use(t) == NULL);
					} else {
						/* If the mapping between pr and md is not unique,
						maydef is not NULL.
						Same PR may have different referrence type.
						e.g: PR1(U8)=...
							...=PR(U32)
						*/
						if (t->get_ref_mds() != NULL) {
							IS_TRUE0(m_mds_hash->find(*t->get_ref_mds()));
						}
					}
					break;
				case IR_ARRAY:
				case IR_ILD:
					{
						MD const* mustuse = get_effect_use_md(t);
						MD_SET const* mayuse = get_may_use(t);
						IS_TRUE0(mustuse ||
								 (mayuse && !mayuse->is_empty()));
						if (mustuse != NULL) {
							//PR can not be accessed by indirect operation.
							IS_TRUE0(!MD_is_pr(mustuse));
						}
						if (mayuse != NULL) {
							//PR can not be accessed by indirect operation.
							for (INT i = mayuse->get_first();
								 i >= 0; i = mayuse->get_next(i)) {
								MD const* x = m_md_sys->get_md(i);
								IS_TRUE0(x && !MD_is_pr(x));
							}
							IS_TRUE0(m_mds_hash->find(*mayuse));
						}
					}
					break;
				case IR_ST:
					IS_TRUE0(t->get_exact_ref() &&
							 !MD_is_pr(t->get_exact_ref()));
					//ST may modify overlapped memory object.
					if (t->get_ref_mds() != NULL) {
						IS_TRUE0(m_mds_hash->find(*t->get_ref_mds()));
					}
					break;
				case IR_STPR:
					IS_TRUE0(t->get_exact_ref() &&
							 MD_is_pr(t->get_exact_ref()));

					if (is_pr_unique_for_same_no()) {
						IS_TRUE0(get_may_def(t) == NULL);
					} else {
						/* If the mapping between pr and md is not unique,
						maydef is not NULL.
						Same PR may have different referrence type.
						e.g: PR1(U8)=...
							...=PR(U32)
						*/
						if (t->get_ref_mds() != NULL) {
							IS_TRUE0(m_mds_hash->find(*t->get_ref_mds()));
						}
					}
					break;
				case IR_IST:
					{
						MD const* mustdef = get_must_def(t);
						if (mustdef != NULL) {
							//mustdef may be fake object, e.g: global memory.
							//IS_TRUE0(mustdef->is_effect());

							//PR can not be accessed by indirect operation.
							IS_TRUE0(!MD_is_pr(mustdef));
						}
						MD_SET const* maydef = get_may_def(t);
						IS_TRUE0(mustdef != NULL ||
								(maydef != NULL && !maydef->is_empty()));
						if (maydef != NULL) {
							//PR can not be accessed by indirect operation.
							for (INT i = maydef->get_first();
								 i >= 0; i = maydef->get_next(i)) {
								MD const* x = m_md_sys->get_md(i);
								IS_TRUE0(x);
								IS_TRUE0(!MD_is_pr(x));
							}
							IS_TRUE0(m_mds_hash->find(*maydef));
						}
					}
					break;
				case IR_CALL:
				case IR_ICALL:
					if (t->get_ref_mds() != NULL) {
						IS_TRUE0(m_mds_hash->find(*t->get_ref_mds()));
					}
					break;
				case IR_PHI:
					IS_TRUE0(t->get_exact_ref() &&
							 MD_is_pr(t->get_exact_ref()));

					if (is_pr_unique_for_same_no()) {
						IS_TRUE0(get_may_def(t) == NULL);
					} else {
						/* If the mapping between pr and md is not unique,
						maydef is not NULL.
						Same PR may have different referrence type.
						e.g: PR1(U8)=...
							...=PR(U32)
						*/
						if (t->get_ref_mds() != NULL) {
							IS_TRUE0(m_mds_hash->find(*t->get_ref_mds()));
						}
					}
					break;
				case IR_CVT:
					//CVT should not have any reference. Even if the
					//operation will genrerate different type memory
					//accessing.
				case IR_CONST:
				case IR_LDA:
				case IR_ADD:
				case IR_SUB:
				case IR_MUL:
				case IR_DIV:
				case IR_REM:
				case IR_MOD:
				case IR_LAND:
				case IR_LOR:
				case IR_BAND:
				case IR_BOR:
				case IR_XOR:
				case IR_BNOT:
				case IR_LNOT:
				case IR_NEG:
				case IR_LT:
				case IR_LE:
				case IR_GT:
				case IR_GE:
				case IR_EQ:
				case IR_NE:
				case IR_ASR:
				case IR_LSR:
				case IR_LSL:
				case IR_SELECT:
				case IR_CASE:
				case IR_BREAK:
				case IR_CONTINUE:
				case IR_TRUEBR:
				case IR_FALSEBR:
				case IR_GOTO:
				case IR_IGOTO:
				case IR_SWITCH:
				case IR_RETURN:
				case IR_REGION:
					IS_TRUE0(t->get_ref_md() == NULL &&
							 t->get_ref_mds() == NULL);
					break;
				default: IS_TRUE(0, ("unsupport ir type"));
				}
			}
		}
	}
	return true;
}


void IR_DU_MGR::reset_reach_out(bool clean_member)
{
	for (INT i = 0; i <= m_bb_out_reach_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_out_reach_def.get(i);
		if (bs == NULL) { continue; }
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_out_reach_def.clean();
	}
}


void IR_DU_MGR::reset_reach_in(bool clean_member)
{
	for (INT i = 0; i <= m_bb_in_reach_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_in_reach_def.get(i);
		if (bs == NULL) { continue; }
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_in_reach_def.clean();
	}
}


void IR_DU_MGR::reset_global(bool clean_member)
{
	for (INT i = 0; i <= m_bb_avail_in_reach_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_avail_in_reach_def.get(i);
		if (bs == NULL) { continue; }
		//m_sbs_mgr->freedc(bs);
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_avail_in_reach_def.clean();
	}

	for (INT i = 0; i <= m_bb_avail_out_reach_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_avail_out_reach_def.get(i);
		if (bs == NULL) { continue; }
		//m_sbs_mgr->freedc(bs);
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_avail_out_reach_def.clean();
	}

	reset_reach_in(clean_member);
	reset_reach_out(clean_member);

	for (INT i = m_bb_availin_ir_expr.get_first();
		 i >= 0; i = m_bb_availin_ir_expr.get_next(i)) {
		DBITSETC * bs = m_bb_availin_ir_expr.get(i);
		IS_TRUE0(bs);
		//m_sbs_mgr->freedc(bs);
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_availin_ir_expr.clean();
	}

	for (INT i = m_bb_availout_ir_expr.get_first();
		 i >= 0; i = m_bb_availout_ir_expr.get_next(i)) {
		DBITSETC * bs = m_bb_availout_ir_expr.get(i);
		IS_TRUE0(bs);
		//m_sbs_mgr->freedc(bs);
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_availout_ir_expr.clean();
	}
}


/* Sort stmt in 'src' in breadth first order.
'input': input stmt list.
'output': output stmt list.
'bfs_order': record each BB's bfs order.
	Sort bb list by invoking sort_in_bfs_order() of CFG. */
void IR_DU_MGR::sort_stmt_in_bfs(IN SLIST<IR const*> & input,
								 OUT SLIST<IR const*> & output,
								 SVECTOR<UINT> & bfs_order)
{
	SC<IR const*> * sct;
	for (IR const* d = input.get_head(&sct);
		 d != NULL; d = input.get_next(&sct)) {
		bool inserted = false;
		IR_BB * dbb = d->get_bb();
		UINT dbbid = IR_BB_id(d->get_bb());
		SC<IR const*> * tgt;
		SC<IR const*> * prev_tgt = NULL;
		for (IR const* x = output.get_head(&tgt);
			 x != NULL; prev_tgt = tgt, x = output.get_next(&tgt)) {
			IS_TRUE0(x != d);
			IR_BB * xbb = x->get_bb();
			if (bfs_order.get(dbbid) < bfs_order.get(IR_BB_id(xbb))) {
				output.insert_after(d, prev_tgt);
				inserted = true;
				break;
			}
			if (dbb == xbb && dbb->is_dom(d, x, true)) {
				output.insert_after(d, prev_tgt);
				inserted = true;
				break;
			}
		}
		if (!inserted) {
			output.append_tail(d);
		}
	}
}


//Free auxiliary data structure used in solving.
void IR_DU_MGR::reset_local(bool clean_member)
{
	for (INT i = 0; i <= m_bb_may_gen_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_may_gen_def.get(i);
		if (bs == NULL) { continue; }
		//m_sbs_mgr->freedc(bs);
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_may_gen_def.clean();
	}

	for (INT i = 0; i <= m_bb_must_gen_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_must_gen_def.get(i);
		if (bs == NULL) { continue; }
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_must_gen_def.clean();
	}

	for (INT i = 0; i <= m_bb_must_killed_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_must_killed_def.get(i);
		if (bs == NULL) { continue; }
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_must_killed_def.clean();
	}

	for (INT i = 0; i <= m_bb_may_killed_def.get_last_idx(); i++) {
		DBITSETC * bs = m_bb_may_killed_def.get(i);
		if (bs == NULL) { continue; }
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_may_killed_def.clean();
	}

	for (INT i = m_bb_gen_ir_expr.get_first();
		 i >= 0; i = m_bb_gen_ir_expr.get_next(i)) {
		DBITSETC * bs = m_bb_gen_ir_expr.get(i);
		IS_TRUE0(bs);
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_gen_ir_expr.clean();
	}

	for (INT i = m_bb_killed_ir_expr.get_first();
		 i >= 0; i = m_bb_killed_ir_expr.get_next(i)) {
		DBITSETC * bs = m_bb_killed_ir_expr.get(i);
		IS_TRUE0(bs);
		m_sbs_mgr->destroy_seg_and_freedc(bs);
	}
	if (clean_member) {
		m_bb_killed_ir_expr.clean();
	}
}


/* Find the nearest dominated DEF stmt of 'exp'.
NOTE: RPO of bb of stmt must be available.

'exp': expression
'exp_stmt': stmt that exp is belong to.
'expdu': def set of exp.
'omit_self': true if we do not consider the 'exp_stmt' itself. */
IR * IR_DU_MGR::find_dom_def(IR const* exp, IR const* exp_stmt,
							 DU_SET const* expdu, bool omit_self)
{
	IR const* domdef = NULL;
	IR_DU_MGR * pthis = const_cast<IR_DU_MGR*>(this);
	IS_TRUE0(pthis->get_may_use(exp) != NULL ||
			 pthis->get_must_use(exp) != NULL);
	IR * last = NULL;
	INT lastrpo = -1;
	DU_ITER di;
	for (INT i = expdu->get_first(&di);
		 i >= 0; i = expdu->get_next(i, &di)) {
		IR * d = m_ru->get_ir(i);
		IS_TRUE0(d->is_stmt());
		if (!is_may_def(d, exp, false)) {
			continue;
		}
		if (omit_self && d == exp_stmt) {
			continue;
		}
		if (last == NULL) {
			last = d;
			lastrpo = IR_BB_rpo(d->get_bb());
			IS_TRUE0(lastrpo >= 0);
			continue;
		}
		IR_BB * dbb = d->get_bb();
		IS_TRUE0(dbb);
		IS_TRUE0(IR_BB_rpo(dbb) >= 0);
		if (IR_BB_rpo(dbb) > lastrpo) {
			last = d;
			lastrpo = IR_BB_rpo(dbb);
		} else if (dbb == last->get_bb() && dbb->is_dom(last, d, true)) {
			last = d;
			lastrpo = IR_BB_rpo(dbb);
		}
	}

	if (last == NULL) { return NULL; }
	IR_BB * last_bb = last->get_bb();
	IR_BB * exp_bb = exp_stmt->get_bb();
	if (!m_cfg->is_dom(IR_BB_id(last_bb), IR_BB_id(exp_bb))) {
		return NULL;
	}

	/* e.g: *p = *p + 1
	Def and Use in same stmt, in this situation,
	the stmt can not regard as dom-def. */
	if (exp_bb == last_bb && !exp_bb->is_dom(last, exp_stmt, true)) {
		return NULL;
	}
	IS_TRUE0(last != exp_stmt);
	return last;
}


//Compute maydef, mustdef, mayuse information for current region.
void IR_DU_MGR::comp_ru_du(SVECTOR<MD_SET*> const* mustdef_mds,
						   SVECTOR<MD_SET*> const* maydef_mds,
						   MD_SET const* mayuse_mds)
{
	IS_TRUE0(mustdef_mds && maydef_mds && mayuse_mds);
	m_ru->init_ref_info();

	MD_SET * ru_maydef = m_ru->get_may_def();
	IS_TRUE0(ru_maydef);
	ru_maydef->clean();

	MD_SET * ru_mustdef = m_ru->get_must_def();
	IS_TRUE0(ru_mustdef);
	ru_mustdef->clean();

	MD_SET * ru_mayuse = m_ru->get_may_use();
	IS_TRUE0(ru_mayuse);
	ru_mayuse->clean();

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head();
		 bb != NULL; bb = bbl->get_next()) {
		MD_SET const* mds = mustdef_mds->get(IR_BB_id(bb));
		IS_TRUE0(mds != NULL);
		for (INT i = mds->get_first(); i >= 0; i = mds->get_next(i)) {
			MD const* md = m_md_sys->get_md(i);
			if (!MD_is_pr(md)) {
				ru_mustdef->bunion(md);
			}
		}

		mds = maydef_mds->get(IR_BB_id(bb));
		IS_TRUE0(mds != NULL);
		for (INT i = mds->get_first(); i >= 0; i = mds->get_next(i)) {
			MD const* md = m_md_sys->get_md(i);
			if (!MD_is_pr(md)) {
				ru_maydef->bunion(md);
			}
		}
	}
	for (INT i = mayuse_mds->get_first();
		 i >= 0; i = mayuse_mds->get_next(i)) {
		MD const* md = m_md_sys->get_md(i);
		if (!MD_is_pr(md)) {
			ru_mayuse->bunion(md);
		}
	}
}


//Compute all DEF,USE MD, MD-set, bb related IR-set info.
bool IR_DU_MGR::perform(IN OUT OPT_CTX & oc, UINT flag)
{
	if (flag == 0) { return true; }

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	if (bbl->get_elem_count() == 0) { return true; }

	IS_TRUE0(OPTC_is_cfg_valid(oc)); //First, only cfg is needed.

	if (OPTC_pass_mgr(oc) != NULL) {
		IR_SSA_MGR * ssamgr =
			(IR_SSA_MGR*)OPTC_pass_mgr(oc)->query_opt(OPT_SSA_MGR);
		if (ssamgr != NULL) {
			set_compute_pr_du_chain(!ssamgr->is_ssa_construct());
		} else {
			set_compute_pr_du_chain(true);
		}
	} else {
		set_compute_pr_du_chain(true);
	}

	START_TIMERS("Build DU ref", t1);
	if (HAVE_FLAG(flag, SOL_REF)) {
		IS_TRUE0(OPTC_is_aa_valid(oc));
		comp_ref();
		OPTC_is_ref_valid(oc) = true;
	}
	END_TIMERS(t1);

	SVECTOR<MD_SET*> * maydef_mds = NULL;
	SVECTOR<MD_SET*> * mustdef_mds = NULL;
	MD_SET * mayuse_mds = NULL;

	if (HAVE_FLAG(flag, SOL_RU_REF)) {
		mayuse_mds = new MD_SET();
	}

	MD_SET * mds_arr_for_must = NULL;
	MD_SET * mds_arr_for_may = NULL;

	//Some system need these set.
	DBITSETC * expr_univers = NULL;
	if (HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF) ||
		HAVE_FLAG(flag, SOL_REACH_DEF) ||
		HAVE_FLAG(flag, SOL_RU_REF) ||
		HAVE_FLAG(flag, SOL_AVAIL_EXPR)) {
		IS_TRUE0(OPTC_is_ref_valid(oc));

		if (HAVE_FLAG(flag, SOL_REACH_DEF) ||
			HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF) ||
			HAVE_FLAG(flag, SOL_RU_REF)) {
			mustdef_mds = new SVECTOR<MD_SET*>();
		}

		if (HAVE_FLAG(flag, SOL_REACH_DEF) ||
			HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF) ||
			HAVE_FLAG(flag, SOL_AVAIL_EXPR) ||
			HAVE_FLAG(flag, SOL_RU_REF)) {
			maydef_mds = new SVECTOR<MD_SET*>();
		}

		START_TIMERS("Allocate May/Must MDS table", t2);

		if (mustdef_mds != NULL) {
			mds_arr_for_must = new MD_SET[bbl->get_elem_count()]();
		}

		if (maydef_mds != NULL) {
			mds_arr_for_may = new MD_SET[bbl->get_elem_count()]();
		}

		UINT i = 0;
		for (IR_BB * bb = bbl->get_tail();
			 bb != NULL; bb = bbl->get_prev(), i++) {
			if (mustdef_mds != NULL) {
				mustdef_mds->set(IR_BB_id(bb), &mds_arr_for_must[i]);
			}
			if (maydef_mds != NULL) {
				maydef_mds->set(IR_BB_id(bb), &mds_arr_for_may[i]);
			}
		}

		END_TIMERS(t2);

		START_TIMERS("Build MustDef, MayDef, MayUse", t3);
		comp_mustdef_maydef_mayuse(mustdef_mds, maydef_mds,
								   mayuse_mds, flag);
		END_TIMERS(t3);

		if (HAVE_FLAG(flag, SOL_REACH_DEF) ||
			HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF)) {
			START_TIMERS("Build KillSet", t);
			comp_kill(mustdef_mds, maydef_mds, flag);
			END_TIMERS(t);
		}

		if (HAVE_FLAG(flag, SOL_AVAIL_EXPR)) {
			//Compute GEN, KILL IR-EXPR.
			START_TIMERS("Build AvailableExp", t);
			expr_univers = new DBITSETC();
			expr_univers->set_sparse(SOL_SET_IS_SPARSE);
			comp_ir_expr(expr_univers, maydef_mds);
			END_TIMERS(t);
		}

		if (HAVE_FLAG(flag, SOL_RU_REF)) {
			//Compute DEF,USE mds for REGION.
			START_TIMERS("Build RU DefUse MDS", t);
			comp_ru_du(mustdef_mds, maydef_mds, mayuse_mds);
			END_TIMERS(t);
		}

		m_ru->check_valid_and_recompute(&oc, OPT_RPO, OPT_UNDEF);

		START_TIMERS("Solve DU set", t4);

		solve(expr_univers, flag);

		END_TIMERS(t4);

		if (HAVE_FLAG(flag, SOL_AVAIL_REACH_DEF)) {
			OPTC_is_avail_reach_def_valid(oc) = true;
		}

		if (HAVE_FLAG(flag, SOL_REACH_DEF)) {
			OPTC_is_reach_def_valid(oc) = true;
		}

		if (HAVE_FLAG(flag, SOL_AVAIL_EXPR)) {
			OPTC_is_live_expr_valid(oc) = true;
		}
	}

#if 0
	int cnt += count_mem_local_data(expr_univers, maydef_mds, mustdef_mds,
							mayuse_mds, mds_arr_for_must, mds_arr_for_may,
							bbl->get_elem_count());
#endif

	if (expr_univers != NULL) {
		expr_univers->clean(m_sbs_mgr->get_seg_mgr(),
							&SDBITSET_MGR_sc_free_list(m_sbs_mgr));
		delete expr_univers;
	}

	if (mustdef_mds != NULL) {
		IS_TRUE0(mds_arr_for_must);
		delete [] mds_arr_for_must;
		delete mustdef_mds;
		mustdef_mds = NULL;
	}

	if (maydef_mds != NULL) {
		IS_TRUE0(mds_arr_for_may);
		delete [] mds_arr_for_may;
		delete maydef_mds;
		maydef_mds = NULL;
	}

	if (mayuse_mds != NULL) {
		delete mayuse_mds;
	}

#if	0
	dump_ref(2);
	dump_set(true);
	//dump_mem_usage_for_ir_du_ref();
	//dump_mem_usage_for_set();
#endif

	reset_local(true);
	if (HAVE_FLAG(flag, SOL_REACH_DEF)) {
		reset_reach_out(true);
	}
	IS_TRUE0(verify_du_ref());
	return true;
}


UINT IR_DU_MGR::count_mem_local_data(DBITSETC * expr_univers,
									SVECTOR<MD_SET*> * maydef_mds,
									SVECTOR<MD_SET*> * mustdef_mds,
									MD_SET * mayuse_mds,
									MD_SET mds_arr_for_must[],
									MD_SET mds_arr_for_may[],
									UINT elemnum)
{
	UINT count = 0;
	if (expr_univers != NULL) {
		count += expr_univers->count_mem();
	}

	if (mustdef_mds != NULL) {
		IS_TRUE0(mds_arr_for_must);
		count += mustdef_mds->count_mem();
		for (UINT i = 0; i < elemnum; i++) {
			count += mds_arr_for_must[i].count_mem();
		}
	}

	if (maydef_mds != NULL) {
		IS_TRUE0(mds_arr_for_may);
		count += maydef_mds->count_mem();
		for (UINT i = 0; i < elemnum; i++) {
			count += mds_arr_for_may[i].count_mem();
		}
	}

	if (mayuse_mds != NULL) {
		count += mayuse_mds->count_mem();
	}

	return count;
}


/* Construct inexactly Du, Ud chain.
NOTICE: Reach-Definition and Must-Def, May-Def,
May-Use must be avaliable. */
void IR_DU_MGR::compute_du_chain(IN OUT OPT_CTX & oc)
{
	START_TIMER("Build DU-CHAIN");

	//If PRs have already been in SSA form, then computing DU chain for them
	//doesn't make any sense.
	if (OPTC_pass_mgr(oc) != NULL) {
		IR_SSA_MGR * ssamgr =
			(IR_SSA_MGR*)OPTC_pass_mgr(oc)->query_opt(OPT_SSA_MGR);
		if (ssamgr != NULL) {
			set_compute_pr_du_chain(!ssamgr->is_ssa_construct());
		} else {
			set_compute_pr_du_chain(true);
		}
	} else {
		set_compute_pr_du_chain(true);
	}

	IS_TRUE0(OPTC_is_ref_valid(oc) && OPTC_is_reach_def_valid(oc));

	m_oc = &oc;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	if (bbl->get_elem_count() > g_thres_opt_bb_num) {
		//There are too many BB. Leave it here.
		END_TIMER();
		reset_reach_in(true);
		OPTC_is_reach_def_valid(oc) = false;
		return;
	}

	IS_TRUE0(OPTC_is_cfg_valid(oc));

	//Record IRs which may defined these referred MDs.
	IS_TRUE0(m_md2irs == NULL && m_is_init == NULL);
	m_md2irs = new MDID2IRS(m_ru);
	m_is_init = new BITSET(MAX(1, (m_ru->get_ir_vec()->
							get_last_idx()/BITS_PER_BYTE)));

	//Compute the DU chain linearly.
	MD_SET tmp1, tmp2;
	C<IR_BB*> * ct;
	MD_ITER mditer;

	for (IR_BB * bb = bbl->get_tail(&ct);
		 bb != NULL; bb = bbl->get_prev(&ct)) {
		compute_bb_du(bb, tmp1, tmp2);
	}

	delete m_md2irs;
	m_md2irs = NULL;

	delete m_is_init;
	m_is_init = NULL;

	OPTC_is_du_chain_valid(oc) = true;

	//Reach def info will be cleaned.
	reset_reach_in(true);
	OPTC_is_reach_def_valid(oc) = false;

	//dump_bbs(bbl);
	//m_md_sys->dump_all_mds();
	//dump_du_chain();
	//dump_du_chain2();
	IS_TRUE0(verify_du_chain());
	END_TIMER();
}
//END IR_DU_MGR
