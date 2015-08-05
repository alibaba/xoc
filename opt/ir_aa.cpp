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
#include "prdf.h"
#include "prssainfo.h"
#include "ir_ssa.h"

//
//START PP_SET_MGR
//
UINT PP_SET_MGR::count_mem()
{
	UINT count = 0;
	SC<PT_PAIR_SET*> * ct;
	for (PT_PAIR_SET * pps = m_pp_set_list.get_head(&ct);
		 pps != NULL; pps = m_pp_set_list.get_next(&ct)) {
		count += pps->count_mem();
	}
	count += m_pp_set_list.count_mem();
	count += m_free_pp_set.count_mem();
	return count;
}
//END PP_SET_MGR


//
//START PT_PAIR_MGR
//
UINT PT_PAIR_MGR::count_mem() const
{
	UINT count = 0;
	TMAP_ITER<UINT, TMAP<UINT, PT_PAIR*>*> ti;
	TMAP<UINT, PT_PAIR*> * v = NULL;
	count += m_from_tmap.count_mem();
	for (m_from_tmap.get_first(ti, &v);
		 v != NULL; m_from_tmap.get_next(ti, &v)) {
		count += v->count_mem();
	}

	count += m_id2pt_pair.count_mem();
	count += smpool_get_pool_size_handle(m_pool_pt_pair);
	count += smpool_get_pool_size_handle(m_pool_tmap);
	count += sizeof(m_pp_count);
	return count;
}


//Add POINT-TO pair: from -> to.
PT_PAIR * PT_PAIR_MGR::add(MD const* from, MD const* to)
{
	TMAP<UINT, PT_PAIR*> * to_tmap = m_from_tmap.get(MD_id(from));
	if (to_tmap == NULL) {
		to_tmap = xmalloc_tmap();
		to_tmap->init();
		m_from_tmap.set(MD_id(from), to_tmap);
	}

	PT_PAIR * pp = to_tmap->get(MD_id(to));
	if (pp == NULL) {
		pp = (PT_PAIR*)xmalloc_pt_pair();
		PP_id(pp) = m_pp_count++;
		PP_from(pp) = from;
		PP_to(pp) = to;
		to_tmap->set(MD_id(to), pp);
		m_id2pt_pair.set(PP_id(pp), pp);
	}
	return pp;
}
//END PT_PAIR_MGR


//
//START IR_AA
//
IR_AA::IR_AA(REGION * ru)
{
	IS_TRUE0(ru);
	m_cfg = ru->get_cfg();
	m_var_mgr = ru->get_var_mgr();
	m_ru = ru;
	m_dm = ru->get_dm();
	m_md_sys = ru->get_md_sys();
	m_mds_mgr = ru->get_mds_mgr();
	m_mds_hash = ru->get_mds_hash();
	m_flow_sensitive = true;
	m_is_pr_unique_for_same_no = RU_is_pr_unique_for_same_no(ru);
	m_is_ssa_available = false;
	m_pool = smpool_create_handle(128, MEM_COMM);
	m_dummy_global = NULL;
	m_hashed_maypts = NULL;
}


IR_AA::~IR_AA()
{
	OPT_CTX oc;
	destroy_context(oc);
	smpool_free_handle(m_pool);
}


UINT IR_AA::count_mem()
{
	UINT count = 0;
	count += sizeof(m_cfg);
	count += sizeof(m_var_mgr);
	count += sizeof(m_ru);
	count += m_in_pp_set.count_mem();
	count += m_out_pp_set.count_mem();
	count += smpool_get_pool_size_handle(m_pool);
	count += sizeof(m_mds_mgr);

	count += m_pt_pair_mgr.count_mem();

	if (m_hashed_maypts != NULL) {
		count += m_hashed_maypts->count_mem();
	}

	count += count_md2mds_mem();
	count += m_id2heap_md_map.count_mem();
	count += m_unique_md2mds.count_mem();
	count += sizeof(BYTE);
	return count;
}


UINT IR_AA::count_md2mds_mem()
{
	UINT count1 = 0;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		MD2MDS * mx = m_md2mds_vec.get(IR_BB_id(bb));
		if (mx == NULL) { continue; }
		for (INT i = mx->get_first(); i >= 0; i = mx->get_next(i)) {
			MD_SET const* mds = mx->get(i);
			if (mds != NULL) {
				count1 += mds->count_mem();
			}
		}
		count1 += mx->count_mem();
	}
	count1 += m_md2mds_vec.count_mem();
	return count1;
}


/* Destroy all context data structure.
DU and another info do not need these info.
If you query sideeffect md or mdset, these data structure must be recomputed. */
void IR_AA::destroy_context(OPT_CTX & oc)
{
	for (INT i = 0; i <= m_md2mds_vec.get_last_idx(); i++) {
		MD2MDS * mx = m_md2mds_vec.get(i);
		if (mx == NULL) { continue; }
		mx->destroy();
	}
	OPTC_is_aa_valid(oc) = false;
}


//Clean but not destory context data structure.
//If you query sideeffect md or mdset, these data structure must be recomputed.
void IR_AA::clean_context(OPT_CTX & oc)
{
	for (INT i = 0; i <= m_md2mds_vec.get_last_idx(); i++) {
		MD2MDS * mx = m_md2mds_vec.get(i);
		if (mx == NULL) { continue; }
		mx->clean();
	}
	OPTC_is_aa_valid(oc) = false;
}


/* This function do clean and free operation, but do not
destroy any data structure. Clean PT_SET for each BB.
Free MD_SET of MDA back to MD_SET_MGR. */
void IR_AA::clean()
{
	m_in_pp_set.clean();
	m_out_pp_set.clean();

	if (!m_flow_sensitive) {
		m_unique_md2mds.clean();
	}

	OPT_CTX oc;
	clean_context(oc);
	m_id2heap_md_map.clean();
}


/* MD size should be determined by 'pointer base type' of a pointer.
e.g:
	char v[]
	int * p = &v
	*p = ...

	The memory size that '*p' modified is 4,
	the size of pointer-base of *p.

'size': pointer base size.

NOTICE: MD should be taken address. */
void IR_AA::revise_md_size(IN OUT MD_SET & mds, UINT size)
{
	IS_TRUE0(size > 0);
	SVECTOR<MD*> res;
	INT num = 0;
	bool change = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md && md->is_effect());

		if (VAR_is_addr_taken(MD_base(md))) {
			IS_TRUE0(MD_is_addr_taken(md));
		}

		/* We need to register a new MD which size is equal to pointer's base.
		e.g: int a[100], char * p = &a;
		Even if size of element of a is 4 bytes, the size of p pointed to
		is only 1 byte. */
		if (MD_size(md) != size) {
			MD tmd(*md);
			MD_size(&tmd) = size;
			MD * entry = m_md_sys->register_md(tmd);
			IS_TRUE0(MD_id(entry) > 0);
			IS_TRUE0(entry->is_effect());
			res.set(num, entry);
			change = true;
		} else {
			res.set(num, md);
		}
		num++;
	}
	if (change) {
		mds.clean();
		for (INT i = num - 1; i >= 0; i--) {
			mds.bunion(res.get(i));
		}
	}
}


/* Return true if IR is a valid statement that could be handled
by AA.
NOTICE:
	High level control flow or similar statements are unacceptable here. */
bool IR_AA::is_valid_stmt_to_aa(IR * ir)
{
	switch(IR_type(ir)) {
	case IR_ST: //store
	case IR_STPR:
	case IR_IST: //indirective store
	case IR_CALL:
	case IR_ICALL: //indirective call
	case IR_GOTO:
	case IR_IGOTO:
	case IR_LABEL: //Permit it in high level IR analysis.
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_SWITCH:
	case IR_RETURN:
	case IR_PHI:
	case IR_REGION:
		return true;
	default:
		return false;
	} //end switch
}


//Process LDA operator, and generate MD.
//Note this function does not handle array's lda base.
//e.g: y = &x.
//'mds' : record memory descriptor of 'ir'
void IR_AA::process_lda(IR * ir, IN OUT MD_SET & mds,
						IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_LDA && ir->is_ptr(m_dm));
	IS_TRUE0(IR_type(IR_parent(ir)) != IR_ARRAY);
	IS_TRUE0(ic && mx);

	IR * lda_base = LDA_base(ir); //symbol

	//Fulfill ic.
	infer_exp(lda_base, mds, ic, mx);
	IS_TRUE0(mds.get_elem_count() == 1);

	//Inform the caller that there is MD has been taken address.
	AC_has_comp_lda(ic) = true;
	if (AC_is_mds_mod(ic) || LDA_ofst(ir) != 0) {
		MD * x = m_md_sys->get_md(mds.get_first());
		if (VAR_is_addr_taken(MD_base(x))) { MD_is_addr_taken(x) = true; }

		if ((LDA_ofst(ir) != 0 && x->is_exact()) ||
			IR_type(IR_parent(ir)) == IR_ARRAY) {
			/*
			If LDA is array base, its ofst may not be 0.
			e.g: struct S { int a; int b[..]; }
				access s.b[..]
			*/
			MD md(*x);
			MD_ofst(&md) += LDA_ofst(ir);
			if (IR_type(IR_parent(ir)) == IR_ARRAY) {
				//'ir' is lda base of array operation.
				//Adjust MD size to be array's element size.
				IR const* arr = IR_parent(ir);
				while (IR_parent(arr) != NULL &&
					   IR_type(IR_parent(arr)) == IR_ARRAY) {
					arr = IR_parent(arr);
				}
				IS_TRUE0(arr && IR_type(arr) == IR_ARRAY);
				UINT elem_sz = arr->get_dt_size(m_dm);
				IS_TRUE0(elem_sz > 0);
				MD_size(&md) = elem_sz;
			}
			MD * entry = m_md_sys->register_md(md);
			IS_TRUE0(MD_id(entry) > 0);
			IS_TRUE0(x->is_effect() && entry->is_effect());
			mds.diff(x);
			mds.bunion_pure(MD_id(entry));
		}
	}
}


//Process array LDA base, and generate MD.
//e.g: p = &A[i]
//'mds' : record memory descriptor of 'ir'.
void IR_AA::process_array_lda_base(IR * ir, IN OUT MD_SET & mds,
								   IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_LDA && ir->is_ptr(m_dm));
	IS_TRUE0(IR_type(IR_parent(ir)) == IR_ARRAY); //lda must be array base.
	IR * lda_base = LDA_base(ir); //symbol

	//Fulfill ic.
	infer_exp(lda_base, mds, ic, mx);
	IS_TRUE0(mds.get_elem_count() == 1);

	//Inform the caller that there is MD has been taken address.
	AC_has_comp_lda(ic) = true;
	if (AC_is_mds_mod(ic)) {
		MD * x = m_md_sys->get_md(mds.get_first());
		if (VAR_is_addr_taken(MD_base(x))) {
			MD_is_addr_taken(x) = true;
		}
		if (x->is_exact()) {
			/* The result data type of LDA should change to array element tyid if
			array is the field of a D_MC type data structure.
			e.g: struct S { int a; int b[..]; }
				access s.b[..] generate array(lda(s, ofst(4))
			*/
			MD md(*x);
			MD_ofst(&md) += LDA_ofst(ir);

			//'ir' is lda base of array operation.
			//Adjust MD size to be array's element size.
			UINT elem_sz = IR_parent(ir)->get_dt_size(m_dm);
			IS_TRUE0(elem_sz > 0);
			MD_size(&md) = elem_sz;
			if (!x->is_equ(md)) {
				MD * entry = m_md_sys->register_md(md);
				IS_TRUE0(MD_id(entry) > 0);
				IS_TRUE0(x->is_effect() && entry->is_effect());
				mds.diff(x);
				mds.bunion_pure(MD_id(entry));
			}
		}
	}
}


/* Convert type-size.
e.g: int a; char b;
	a = (int)b
'mds' : record memory descriptor of 'ir'. */
void IR_AA::process_cvt(IR const* ir, IN OUT MD_SET & mds,
						OUT AA_CTX * ic, OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_CVT);
	infer_exp(CVT_exp(ir), mds, ic, mx);
	if (AC_is_mds_mod(ic)) {
		for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
			MD const* l = m_md_sys->get_md(i);
			UINT size = ir->get_dt_size(m_dm); //cvt's tgt byte size.
			if (l->is_exact() && MD_size(l) != size) {
				MD md(*l);
				//Convert type-size to 'ir' claimed.
				MD_size(&md) = size;
				MD * entry = m_md_sys->register_md(md);
				IS_TRUE0(MD_id(entry) > 0);
				IS_TRUE0(l->is_effect() && entry->is_effect());
				mds.diff(i);
				mds.bunion_pure(MD_id(entry));
			}
		}
	}
}


//Infer the unbounded set.
void IR_AA::infer_array_inf(INT ofst, bool is_ofst_pred, UINT md_size,
							MD_SET const& in, OUT MD_SET & out)
{
	for (INT i = in.get_first(); i >= 0; i = in.get_next(i)) {
		MD const* org = m_md_sys->get_md(i);
		MD tmd(*org);
		if (is_ofst_pred && tmd.is_exact()) {
			MD_ofst(&tmd) += ofst;
			MD_size(&tmd) = md_size;
		} else {
			MD_ty(&tmd) = MD_UNBOUND;
		}

		if (tmd == *org) {
			out.bunion(org);
			continue;
		}

		MD * entry = m_md_sys->register_md(tmd);
		IS_TRUE0(MD_id(entry) > 0);
		out.bunion(entry);
	}
}


//Get to know where the pointer pointed to.
//This function will not clean 'mds' since user may be perform union operation.
void IR_AA::compute_may_point_to(IR * pointer, OUT MD_SET & mds)
{
	IS_TRUE0(pointer && pointer->is_ptr(m_dm));

	//Get context.
	MD2MDS * mx = NULL;
	if (m_flow_sensitive) {
		IS_TRUE0(pointer->get_stmt() && pointer->get_stmt()->get_bb());
		mx = map_bb_to_md2mds(pointer->get_stmt()->get_bb());
	} else  {
		mx = &m_unique_md2mds;
	}
	compute_may_point_to(pointer, mx, mds);
}


//Get to know where the pointer pointed to.
//This function will not clean 'mds' since caller may
//perform union operation to 'mds'.
void IR_AA::compute_may_point_to(IR * pointer, IN MD2MDS * mx, OUT MD_SET & mds)
{
	IS_TRUE0(pointer && pointer->is_ptr(m_dm) && mx);

	if (IR_type(pointer) == IR_LDA) {
		AA_CTX ic;
		AC_comp_pt(&ic) = true;
		MD_SET tmp;
		process_lda(pointer, tmp, &ic, mx);
		IS_TRUE0(!tmp.is_empty());
		mds.bunion(tmp);
		return;
	}

	MD const* md = pointer->get_ref_md();
	IS_TRUE0(md);

	MD_SET const* ptset = get_point_to(md, *mx);
	MD * typed_md = NULL;
	if (ptset != NULL && !ptset->is_empty()) {
		if (IR_ai(pointer) != NULL &&
			ptset->is_contain_global() &&
			(typed_md = comp_point_to_via_type(pointer)) != NULL) {
			mds.bunion(typed_md);
		} else {
			mds.bunion(*ptset);
		}
	} else if (IR_ai(pointer) != NULL &&
			   (typed_md = comp_point_to_via_type(pointer)) != NULL) {
		mds.bunion(typed_md);
	} else {
		//We do NOT known where p pointed to.
		mds.bunion(*m_hashed_maypts);
	}
}


/* The function will compute point-to for ir.
'ir': array operator.
'array_base': base of array, must be pointer.
'is_ofst_predicable': true if array ofst is constant. */
void IR_AA::predict_array_ptrbase(IR * ir,
								  IR * array_base,
								  bool is_ofst_predicable,
								  UINT ofst,
								  OUT MD_SET & mds,
								  OUT bool mds_is_may_pt,
								  IN OUT AA_CTX * ic,
								  IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_ARRAY && array_base->is_ptr(m_dm));
	MD_SET * tmp = m_mds_mgr->create();
	AA_CTX tic;
	tic.copy_top_down_flag(*ic);

	//Comp point-to at current func scope since we want to do more evaluation.
	AC_comp_pt(&tic) = false;

	infer_exp(array_base, *tmp, &tic, mx);

	if (tmp->is_empty()) {
		//We could not determine which MD the array base refer to,
		//so we could not infer which MD the array accessed.
		MD * typed_md = NULL;
		if (IR_ai(array_base) != NULL &&
			(typed_md = comp_point_to_via_type(array_base)) != NULL) {
			mds.clean();
			mds.bunion(typed_md);
		} else {
			mds.copy(*m_hashed_maypts);
			mds_is_may_pt = true;
		}
	} else {
		/* Intent to look for original array base that the base-pointer
		pointed to.
		e.g: (ld(p))[i], looking for where p pointed to.
		Each MD in 'tmp' should be pointer. */
		mds.clean();
		UINT mdsz = ir->get_dt_size(m_dm);
		for (INT i = tmp->get_first(); i >= 0; i = tmp->get_next(i)) {
			MD * array_base_md = m_md_sys->get_md(i);
			IS_TRUE0(array_base_md != NULL);

			//Get to know where the base pointed to.
			MD_SET const* pt_mds = get_point_to(array_base_md, *mx);
			MD * typed_md = NULL;
			if (pt_mds != NULL && !pt_mds->is_empty()) {
				if (IR_ai(array_base) != NULL &&
					pt_mds->is_contain_global() &&
					(typed_md = comp_point_to_via_type(array_base)) != NULL) {
					set_point_to_unique_md(array_base_md, *mx, typed_md);
					mds.clean();
					mds.bunion(typed_md);
				} else {
					infer_array_inf(ofst, is_ofst_predicable,
									mdsz, *pt_mds, mds);
				}

			} else if (IR_ai(array_base) != NULL &&
					   (typed_md = comp_point_to_via_type(array_base)) != NULL) {
				mds.bunion(typed_md);
			} else {
				infer_array_inf(ofst, false, 0, *m_hashed_maypts, mds);
				mds_is_may_pt = true;
				break;
			}
		}
	}

	AC_is_mds_mod(ic) = true;
	MD const* x = NULL;
	if (mds.get_elem_count() == 1 &&
		!MD_is_may(x = m_md_sys->get_md(mds.get_first()))) {
		set_must_addr(ir, m_md_sys->get_md(mds.get_first()));
		ir->clean_ref_mds();
	} else {
		ir->clean_ref_md();
		set_may_addr(ir, m_mds_hash->append(mds));
	}

	m_mds_mgr->free(tmp);
}


void IR_AA::infer_array_ldabase(IR * ir, IR * array_base, bool is_ofst_pred,
								UINT ofst, OUT MD_SET & mds,
								IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_ARRAY && IR_type(array_base) == IR_LDA);
	AA_CTX tic(*ic);
	process_array_lda_base(array_base, mds, &tic, mx);
	//infer_exp(array_base, mds, &tic, mx);

	if (AC_is_mds_mod(&tic)) {
		//Compute the MD size and offset if 'ofst' is constant.
		IS_TRUE0(mds.get_elem_count() == 1);
		MD const* org = m_md_sys->get_md(mds.get_first());
		IS_TRUE0(org->is_exact());
		MD tmd(*org);
		if (is_ofst_pred) {
			MD_ofst(&tmd) += ofst;
			//MD_ty(&tmd) = MD_EXACT;
		} else {
			UINT basesz = m_dm->get_pointer_base_sz(IR_dt(array_base));
			MD_ofst(&tmd) = LDA_ofst(array_base);

			//The approximate and conservative size of array.
			MD_size(&tmd) = basesz - LDA_ofst(array_base);
			MD_ty(&tmd) = MD_RANGE;
		}

		if (tmd == *org) {
			//mds is unchanged.
			set_must_addr(ir, org);
		} else {
			MD * entry = m_md_sys->register_md(tmd);
			IS_TRUE0(MD_id(entry) > 0);
			set_must_addr(ir, entry);
			mds.clean();
			IS_TRUE0(entry->is_effect());
			mds.bunion_pure(MD_id(entry));
		}
		ir->clean_ref_mds();
	} else {
		IS_TRUE(get_may_addr(ir) == NULL, ("have no mayaddr"));
		MD const* x = get_must_addr(ir);
		IS_TRUE0(x && x->is_effect());
		mds.clean();
		mds.bunion_pure(MD_id(x));
	}
}


//Compute the memory address and ONLY record the top level
//ARRAY node's memory address.
//'mds' : record memory descriptor of 'ir'.
void IR_AA::process_array(IR * ir, IN OUT MD_SET & mds,
						  IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_ARRAY);
	//Record ofst-expr's mds.
	//Visit array offset and generate the related MD.
	for (IR * s = ARR_sub_list(ir); s != NULL; s = IR_next(s)) {
		AA_CTX tic;
		tic.copy_top_down_flag(*ic);
		AC_comp_pt(&tic) = false;
		infer_exp(s, mds, &tic, mx);
	}
	IR * array_base = ARR_base(ir);

	//Visit the array base, it may be LDA or pointer.
	UINT ofst_val = 0;
	bool is_ofst_predicable = ir->calc_array_ofst(&ofst_val, m_dm);
	bool mds_is_may_pt = false;
	AA_CTX tic;
	tic.copy_top_down_flag(*ic);
	AC_comp_pt(&tic) = false;

	if (IR_type(array_base) == IR_LDA) {
		infer_array_ldabase(ir, array_base, is_ofst_predicable,
							ofst_val, mds, &tic, mx);
	} else {
		//DTD const* dt = m_dm->get_dtd(IR_dt(array_base));
		//Array base is a pointer.
		predict_array_ptrbase(ir, array_base, is_ofst_predicable,
							  ofst_val, mds, mds_is_may_pt,
							  &tic, mx);
	}
	ic->copy_bottom_up_flag(tic);

	IS_TRUE0(!mds.is_empty());
	if (AC_comp_pt(ic)) {
		//Caller need array's point-to.
		if (mds_is_may_pt) {
			//Do not recompute again.
			return;
		}

		//Compute the POINT-TO of array.
		MD_SET * res = m_mds_mgr->create();
		bool has_unified_may_pt = false;
		for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
			MD_SET const* pts = get_point_to(m_md_sys->get_md(i), *mx);
			if (pts != NULL && !pts->is_empty()) {
				res->bunion(*pts);
			} else if (!has_unified_may_pt) {
				has_unified_may_pt = true;
				//We do NOT known where p[...] pointed to, use the
				//conservative solution.
				res->bunion(*m_hashed_maypts);
			}
		}
		mds.copy(*res);
		m_mds_mgr->free(res);
	}
}


//The function generates new MD for given pr.
//It should be called if new PR generated in optimzations.
MD const* IR_AA::alloc_pr_md(IR * pr)
{
	IS_TRUE0(pr->is_pr());
	MD const* md = m_ru->gen_md_for_pr(pr);
	set_must_addr(pr, md);
	pr->clean_ref_mds();
	return md;
}


//The function generates new MD for given pr.
//It should be called if new PR generated in optimzations.
MD const* IR_AA::alloc_phi_md(IR * phi)
{
	IS_TRUE0(phi->is_phi());
	MD const* md = m_ru->gen_md_for_pr(phi);
	set_must_addr(phi, md);
	phi->clean_ref_mds();
	return md;
}


MD const* IR_AA::alloc_id_md(IR * ir)
{
	IS_TRUE0(IR_type(ir) == IR_ID);
	MD const* t = m_ru->gen_md_for_var(ir);
	set_must_addr(ir, t);
	ir->clean_ref_mds();
	return t;
}


MD const* IR_AA::alloc_ld_md(IR * ir)
{
	MD const* t = m_ru->gen_md_for_ld(ir);
	IS_TRUE0(t);
	ir->clean_ref_mds();
	if (LD_ofst(ir) != 0) {
		MD t2(*t);
		IS_TRUE0(t2.is_exact());
		MD_ofst(&t2) += LD_ofst(ir);
		MD_size(&t2) = ir->get_dt_size(m_dm);
		MD * entry = m_md_sys->register_md(t2);
		IS_TRUE(MD_id(entry) > 0, ("Not yet registered"));
		t = entry; //regard MD with offset as return result.
	}
	set_must_addr(ir, t);
	return t;
}


MD const* IR_AA::alloc_stpr_md(IR * ir)
{
	IS_TRUE0(ir->is_stpr());
	MD const* md = m_ru->gen_md_for_pr(ir);
	set_must_addr(ir, md);
	ir->clean_ref_mds();
	return md;
}


MD const* IR_AA::alloc_call_pr_md(IR * ir)
{
	IS_TRUE0(ir->is_call());
	MD const* md = m_ru->gen_md_for_pr(ir);
	set_must_addr(ir, md);
	ir->clean_ref_mds();
	return md;
}


MD const* IR_AA::alloc_setepr_md(IR * ir)
{
	IS_TRUE0(ir->is_setepr());
	MD const* md = m_ru->gen_md_for_pr(ir);

	IR const* ofst = SETEPR_ofst(ir);
	IS_TRUE0(ofst);

	if (ofst->is_const()) {
		IS_TRUE(ofst->is_int(m_dm), ("offset of SETE must be integer."));

		//Accumulating offset of identifier.
		//e.g: struct {int a,b; } s; s.a = 10
		//generate: st('s', ofst:4) = 10
		MD t(*md);
		IS_TRUE0(t.is_exact());
		IS_TRUE0(ir->get_dt_size(m_dm) > 0);
		MD_ofst(&t) += (UINT)CONST_int_val(ofst);
		MD_size(&t) = ir->get_dt_size(m_dm);
		MD * entry = m_md_sys->register_md(t);
		IS_TRUE(MD_id(entry) > 0, ("Not yet registered"));
		md = entry; //regard MD with offset as return result.
	} else {
		/* Offset is variable.
		e.g: vector<4xi32> v; v[i] = 0;
		will generate:
			st pr1(v)
			sete pr1(ofst:i), 10)
			st(v, pr1)
		*/

		MD t(*md);
		IS_TRUE0(t.is_exact());
		IS_TRUE0(ir->get_dt_size(m_dm) > 0);
		MD_ty(&t) = MD_RANGE;
		MD_ofst(&t) = 0;
		MD_size(&t) = ir->get_dt_size(m_dm);
		MD * entry = m_md_sys->register_md(t);
		IS_TRUE(MD_id(entry) > 0, ("Not yet registered"));
		md = entry; //regard MD with range as return result.
	}

	set_must_addr(ir, md);
	ir->clean_ref_mds();
	return md;
}


MD const* IR_AA::alloc_getepr_md(IR * ir)
{
	IS_TRUE0(ir->is_getepr());
	MD const* md = m_ru->gen_md_for_pr(ir);
	set_must_addr(ir, md);
	ir->clean_ref_mds();
	return md;
}


MD const* IR_AA::alloc_st_md(IR * ir)
{
	IS_TRUE0(ir->is_stid());
	MD const* md = m_ru->gen_md_for_st(ir);
	IS_TRUE0(md);
	ir->clean_ref_mds();
	if (ST_ofst(ir) != 0) {
		//Accumulating offset of identifier.
		//e.g: struct {int a,b; } s; s.a = 10
		//generate: st('s', ofst:4) = 10
		MD t(*md);
		IS_TRUE0(t.is_exact());
		IS_TRUE0(ir->get_dt_size(m_dm) > 0);
		MD_ofst(&t) += ST_ofst(ir);
		MD_size(&t) = ir->get_dt_size(m_dm);
		MD * entry = m_md_sys->register_md(t);
		IS_TRUE(MD_id(entry) > 0, ("Not yet registered"));
		md = entry; //regard MD with offset as return result.
	}
	set_must_addr(ir, md);
	return md;
}


bool IR_AA::evaluate_from_lda(IR const* ir)
{
	if (!m_is_ssa_available) { return false; }

	//Attempt to infer more presicion point-to if ssa info is avaiable.

	if (IR_type(ir) == IR_CVT) { return evaluate_from_lda(CVT_exp(ir)); }

	if (!ir->is_pr()) { return false; }

	SSAINFO const* ssainfo = PR_ssainfo(ir);
	if (ssainfo == NULL) { return false; }

	IR * defstmt = SSA_def(ssainfo);
	if (defstmt == NULL) { return false; }

	IS_TRUE0(defstmt->is_stpr());

	IR const* rhs = STPR_rhs(defstmt);
	switch (IR_type(rhs)) {
	case IR_LDA: return true;
	case IR_PR: return evaluate_from_lda(rhs);
	case IR_CVT: return evaluate_from_lda(CVT_exp(rhs));
	default:;
	}

	IR const* r = rhs;
	for (;;) {
		switch (IR_type(r)) {
		case IR_ADD:
			{
				//Check the opnd0 if current expresion is : op0 + imm(0)
				IR const* op1 = BIN_opnd1(r);
				if (op1->is_const() && op1->is_int(m_dm)) {
					r = BIN_opnd0(r);
				} else {
					return false;
				}
			}
			break;
		case IR_LDA: return true;
		case IR_PR: return evaluate_from_lda(r);
		default:; return false;
		}
	}
	return false;
}


/* Perform pointer arith to compute where ir might point to.
If we compute the point-to set of p+1, that always equivilate to
compute the point-to of p, and each element in the set will be
registered to be unbound. Since if p+1 is placed in a loop,
we could not determine the exact MD where p pointed to.
'mds' : record memory descriptor of 'ir' */
void IR_AA::infer_pt_arith(IR const* ir,
						   IN OUT MD_SET & mds,
						   IN OUT MD_SET & opnd0_mds,
						   IN OUT AA_CTX * opnd0_ic,
						   IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_ADD || IR_type(ir) == IR_SUB);
	IR * opnd1 = BIN_opnd1(ir);
	if (IR_is_const(opnd1) && opnd1->is_int(m_dm)) {
		//Compute the offset for pointer arithmetic.
		if (CONST_int_val(opnd1) == 0) {
			mds.copy(opnd0_mds);
			return;
		} else {
			mds.clean();
			if (IR_type(BIN_opnd0(ir)) == IR_LDA ||
				evaluate_from_lda(BIN_opnd0(ir))) {
				//In the case: lda(x) + ofst, we can determine
				//the value of lda(x) is constant.
				//Keep offset validation unchanged.
				for (INT i = opnd0_mds.get_first();
					 i >= 0; i = opnd0_mds.get_next(i)) {
					MD * imd = m_md_sys->get_md(i);
					if (imd->is_exact()) {
						MD * entry = NULL;
						MD x(*imd);
						if (IR_type(ir) == IR_ADD) {
							//In the case: lda(x) + ofst, we can determine
							//the value of lda(x) is constant.
							; //Keep offset validation unchanged.
							MD_ofst(&x) += (UINT)CONST_int_val(opnd1);
							entry = m_md_sys->register_md(x);
							IS_TRUE0(MD_id(entry) > 0);
						} else {
							//case: &x - ofst.
							//Keep offset validation unchanged.
							INT s = MD_ofst(&x);
							s -= (INT)CONST_int_val(opnd1);
							if (s < 0) {
								MD_ty(&x) = MD_UNBOUND;
								MD_size(&x) = 0;
								MD_ofst(&x) = 0;
							}
							entry = m_md_sys->register_md(x);
							IS_TRUE0(MD_id(entry) > 0);
						}
						mds.bunion(entry);
					} else {
						mds.bunion(imd);
					}
				}
				return;
			}
		}
	} else {
		//Generate MD expression for opnd1.
		AA_CTX opnd1_tic(*opnd0_ic);
		opnd1_tic.clean_bottom_up_flag();
		infer_exp(opnd1, mds, &opnd1_tic, mx);
		//Do not copy bottom-up flag of opnd1.
		mds.clean();
		if (AC_has_comp_lda(&opnd1_tic) &&
			AC_has_comp_lda(opnd0_ic)) {
			//In the situation like this: &a - &b.
			IS_TRUE(IR_type(ir) == IR_SUB, ("only support pointer sub pointer"));
			AC_has_comp_lda(opnd0_ic) = false;
			return;
		}
	}

	/* Pointer arithmetic cause ambiguous memory access.
	Pointer arithmetic cause ambiguous memory access.
	e.g: while (...) { p = p+1 }
	Where is p pointing to at all?
	Set ofst is invalid to keep the conservation. */
	IS_TRUE(mds.is_empty(), ("output buffer not yet initialized"));
	for (INT i = opnd0_mds.get_first(); i >= 0; i = opnd0_mds.get_next(i)) {
		MD * imd = m_md_sys->get_md(i);
		if (imd->is_exact()) {
			MD x(*imd);
			MD_ty(&x) = MD_UNBOUND;
			MD * entry = m_md_sys->register_md(x);
			IS_TRUE0(MD_id(entry) > 0);
			mds.bunion_pure(MD_id(entry));
		} else {
			mds.bunion(imd);
		}
	}

}


//Compute the point-to set of expression.
//ir may be pointer arithmetic, even if opnd0 is not pointer type.
//'mds' : record memory-descriptor set or
//	point-to set of 'ir' if AC_comp_pt(ic) is true.
void IR_AA::process_pointer_arith(IR * ir, IN OUT MD_SET & mds,
								  IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_ADD || IR_type(ir) == IR_SUB);
	IR * opnd0 = BIN_opnd0(ir);
	IR * opnd1 = BIN_opnd1(ir);
	ir->clean_ref();

	//ic may have been set comp_pt to true.
	AA_CTX tic(*ic);
	tic.clean_bottom_up_flag();
	MD_SET * tmp = m_mds_mgr->create();
	infer_exp(opnd0, *tmp, &tic, mx); //Generate MDS of opnd0.

	//For given expression: a + b, we
	//can not tell which memory it descripted.
	if (opnd0->is_ptr(m_dm)) {
		/* This is pointer arithmetic.
		If p is a pointer, the followed expr is analyzable:
			(p +/- n), where n is constant.
			(p +/- n + ...), where n is constant.
		p may be literal, e.g: ((int*)0x1000) + 1. */
		if (IR_type(ir) == IR_ADD) {
			IS_TRUE(!opnd1->is_ptr(m_dm), ("pointer can not plus pointer"));
		}
		if (!opnd1->is_ptr(m_dm)) {
			//pointer +/- n still be pointer.
			IS_TRUE0(ir->is_ptr(m_dm));
		}

		infer_pt_arith(ir, mds, *tmp, &tic, mx);
		ic->copy_bottom_up_flag(tic);
	} else {
		IS_TRUE0(!ir->is_ptr(m_dm));
		if (IR_type(ir) == IR_ADD) {
			//Opnd1 might be poniter. e.g: &p-&q
			IS_TRUE0(!opnd1->is_ptr(m_dm));
		}

		if (AC_comp_pt(&tic)) {
			//tmp already record the point-to set of opnd0.
			//We need to infer the final point-to set according to op0 +/- op1.

			//interwarn("operand of %s(line:%d) is not pointer, "
			//		  "and you should not query its point-to set",
			//		  IRNAME(ir), get_lineno(ir));
			infer_pt_arith(ir, mds, *tmp, &tic, mx);
		} else {
			//Scan and generate MDS of opnd1.
			AA_CTX tic(*ic);
			tic.clean_bottom_up_flag();
			infer_exp(opnd1, mds, &tic, mx);
			mds.clean(); //Do not remove this code.
		}
		ic->copy_bottom_up_flag(tic);
	}
	m_mds_mgr->free(tmp);
}


//Assign unique MD to pr.
//'mds' : record memory descriptor of 'ir'.
MD const* IR_AA::assign_pr_md(IR * ir, IN OUT MD_SET * mds,
							  IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(ir->is_pr());
	IS_TRUE0(mds && ic);
	MD const* t;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		t = alloc_pr_md(ir);
		AC_is_mds_mod(ic) = true;
	} else {
		t = get_must_addr(ir);
		AC_is_mds_mod(ic) = false;
	}
	IS_TRUE0(t);

	if (AC_comp_pt(ic)) {
		IS_TRUE0(mx);
		MD_SET const* pts = get_point_to(t, *mx);
		MD * typed_md = NULL;
		if (pts != NULL && !pts->is_empty()) {
			if (pts->is_contain_global() &&
				IR_ai(ir) != NULL &&
				(typed_md = comp_point_to_via_type(ir)) != NULL) {
				set_point_to_unique_md(t, *mx, typed_md);
				mds->clean();
				mds->bunion(typed_md);
			} else {
				mds->copy(*pts);
			}
		} else if (IR_ai(ir) != NULL &&
				   (typed_md = comp_point_to_via_type(ir)) != NULL) {
			set_point_to_set_add_md(t, *mx, typed_md);
			mds->clean();
			mds->bunion(typed_md);
		} else {
			//We do NOT known where p pointed to, and compute
			//the offset as well.
			mds->copy(*m_hashed_maypts);
		}
	} else {
		mds->clean();
		mds->bunion(t);
	}
	return t;
}


//'mds' : record memory descriptor of 'ir'
MD const* IR_AA::assign_ld_md(IR * ir, IN OUT MD_SET * mds,
							  IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_LD);
	IS_TRUE0(mds && ic);
	MD const* t;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		t = alloc_ld_md(ir);
		AC_is_mds_mod(ic) = true;
	} else {
		t = get_must_addr(ir);
		AC_is_mds_mod(ic) = false;
	}
	IS_TRUE0(t);

	if (AC_comp_pt(ic)) {
		IS_TRUE0(mx);
		AC_is_mds_mod(ic) = true;
		MD_SET const* pts = get_point_to(t, *mx);
		MD * typed_md = NULL;
		if (pts != NULL && !pts->is_empty()) {
			if (pts->is_contain_global() &&
				IR_ai(ir) != NULL &&
				(typed_md = comp_point_to_via_type(ir)) != NULL) {
				set_point_to_unique_md(t, *mx, typed_md);
				mds->clean();
				mds->bunion(typed_md);
			} else {
				mds->copy(*pts);
			}
		} else if (IR_ai(ir) != NULL &&
				   (typed_md = comp_point_to_via_type(ir)) != NULL) {
			set_point_to_set_add_md(t, *mx, typed_md);
			mds->clean();
			mds->bunion(typed_md);
		} else {
			//We do NOT known where p pointed to,
			//e.g: If p->? (p+2)->??
			mds->copy(*m_hashed_maypts);
		}
	} else {
		mds->clean();
		mds->bunion(t);
	}
	return t;
}


//Assign unique MD to 'id'.
//'mds': record memory descriptor of 'ir'.
MD const* IR_AA::assign_id_md(IR * ir, IN OUT MD_SET * mds,
					 		  IN OUT AA_CTX * ic)
{
	IS_TRUE0(IR_type(ir) == IR_ID);
	IS_TRUE0(ic && mds);
	if (VAR_is_str(ID_info(ir))) {
		return assign_string_identifier(ir, mds, ic);
	}

	MD const* t;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		t = alloc_id_md(ir);
		AC_is_mds_mod(ic) = true;
	} else {
		t = get_must_addr(ir);
		AC_is_mds_mod(ic) = false;
	}
	IS_TRUE0(t);

	mds->clean();
	mds->bunion(t);
	return t;
}


MD const* IR_AA::alloc_string_md(IR * ir)
{
	IS_TRUE0(ir->is_str(m_dm) || (IR_type(ir) == IR_ID &&
			 VAR_is_str(ID_info(ir))));
	MD const* strmd;
	REGION_MGR * rm = m_ru->get_ru_mgr();
	if ((strmd = rm->get_dedicated_str_md()) == NULL) {
		SYM * name = NULL;
		if (ir->is_str(m_dm)) { name = CONST_str_val(ir); }
		else { name = VAR_name(ID_info(ir)); }

		VAR * v = m_ru->get_var_mgr()->register_str(NULL, name, 1);
		VAR_allocable(v) = true;
		MD md;
		MD_base(&md) = v;
		MD_size(&md) = strlen(SYM_name(name)) + 1;
		MD_ofst(&md) = 0;
		MD_ty(&md) = MD_EXACT;
		IS_TRUE0(MD_is_str(&md));

		//Set string address to be taken only if it is base of LDA.
		//MD_is_addr_taken(&md) = true;
		MD * e = m_md_sys->register_md(md);
		IS_TRUE0(MD_id(e) > 0);
		strmd = e;
	}
	set_must_addr(ir, strmd);
	ir->clean_ref_mds();
	return strmd;
}


//'mds' : record memory descriptor of 'ir'.
void IR_AA::process_ild(IR * ir, IN OUT MD_SET & mds,
						IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_ILD);
	IS_TRUE0(ILD_base(ir)->is_ptr(m_dm));

	//... = *q, if q->x, set ir's MD to be 'x'.
	AA_CTX tic(*ic);

	//Compute the address that ILD described.
	AC_comp_pt(&tic) = true;
	infer_exp(ILD_base(ir), mds, &tic, mx);

	IS_TRUE0(!mds.is_empty());
	AC_is_mds_mod(ic) |= AC_is_mds_mod(&tic);
	if (ILD_ofst(ir) != 0) {
		BITSET t;
		bool change = false;
		for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
			MD * l = m_md_sys->get_md(i);
			IS_TRUE0(l);
			UINT size = ir->get_dt_size(m_dm);
			if (l->is_exact() && (ILD_ofst(ir) != 0 || MD_size(l) != size)) {
				MD md(*l);
				MD_ofst(&md) += ILD_ofst(ir);
				MD_size(&md) = size;
				MD * entry = m_md_sys->register_md(md);
				IS_TRUE(MD_id(entry) > 0, ("Not yet registered"));
				t.bunion(MD_id(entry));
				change = true;
			} else {
				t.bunion(i);
			}
		}
		if (change) { mds.copy(t); }
		IS_TRUE0(!mds.is_empty());
	}

	//Set this set as the address MD_SET of ILD.
	MD const* mustaddr = NULL;
	if (mds.get_elem_count() == 1 &&
		!MD_is_may(mustaddr = m_md_sys->get_md(mds.get_first()))) {
		mustaddr = m_md_sys->get_md(mds.get_first());
		set_must_addr(ir, mustaddr);
		ir->clean_ref_mds();
	} else {
		ir->clean_ref_md();
		set_may_addr(ir, m_mds_hash->append(mds));
	}

	if (AC_comp_pt(ic)) {
		//Compute the ILD pointed to.
		if (mustaddr != NULL) {
			MD_SET const* pts = get_point_to(mustaddr, *mx);
			if (pts != NULL && !pts->is_empty()) {
				mds.copy(*pts);
			} else {
				/* We do NOT known where p pointed to, and compute
				the offset as well.
				e.g: If p->? (p+2)->?? */
				mds.copy(*m_hashed_maypts);
			}
		} else {
			MD_SET * res = m_mds_mgr->create();
			bool has_unified_may_pt = false;
			for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
				MD_SET const* pts = get_point_to(m_md_sys->get_md(i), *mx);
				if (pts != NULL && !pts->is_empty()) {
					res->bunion(*pts);
				} else if (!has_unified_may_pt) {
					has_unified_may_pt = true;
					/* We do NOT known where p pointed to, and compute
					the offset as well.
					e.g: If p->? (p+2)->?? */
					res->bunion(*m_hashed_maypts);
				}
			}
			mds.copy(*res);
			m_mds_mgr->free(res);
		}
	}
}


/* 'ir' describes memory address of string const.
Add a new VAR to describe the string.
p = q, for any x : if q -> x, add p -> x.

'mds' : record memory descriptor of 'ir'. */
MD const* IR_AA::assign_string_const(IR * ir, IN OUT MD_SET * mds,
									 IN OUT AA_CTX * ic)
{
	IS_TRUE0(ir->is_str(m_dm));
	IS_TRUE0(mds && ic);
	MD const* t = NULL;
	SYM * name = CONST_str_val(ir);
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		if ((t = m_ru->get_ru_mgr()->get_dedicated_str_md()) == NULL) {
			//Each string const corresponding to an unique md.
			VAR * v = m_ru->get_var_mgr()->register_str(NULL, name, 1);
			VAR_allocable(v) = true;
			MD md;
			MD_base(&md) = v;
			MD_size(&md) = strlen(SYM_name(name)) + 1;
			MD_ofst(&md) = 0;
			MD_ty(&md) = MD_EXACT;
			IS_TRUE0(MD_is_str(&md));

			//Set string address to be taken only if it is base of LDA.
			//MD_is_addr_taken(&md) = true;
			MD * e = m_md_sys->register_md(md);
			IS_TRUE0(MD_id(e) > 0);
			t = e;
		}
		AC_is_mds_mod(ic) = true;
	} else {
		if ((t = m_ru->get_ru_mgr()->get_dedicated_str_md()) == NULL) {
			VAR * v = m_ru->get_var_mgr()->find_str_var(name);
			IS_TRUE0(v);
			MD md;
			MD_base(&md) = v;
			MD_size(&md) = strlen(SYM_name(name)) + 1;
			MD_ofst(&md) = 0;
			MD_ty(&md) = MD_EXACT;
			IS_TRUE0(MD_is_str(&md));

			//Set string address to be taken only if it is base of LDA.
			//MD_is_addr_taken(&md) = true;
			t = m_md_sys->register_md(md);
			IS_TRUE0(MD_id(t) > 0);
		}
		AC_is_mds_mod(ic) = false;
	}

	IS_TRUE0(t);
	mds->clean();
	mds->bunion(t);
	return t;
}


/* 'ir' describes memory address of identifer that indicate a string.
Add a new VAR to describe the string.
p = q, for any x : if q -> x, add p -> x.

'mds' : record memory descriptor of 'ir'. */
MD const* IR_AA::assign_string_identifier(IR * ir, IN OUT MD_SET * mds,
										  IN OUT AA_CTX * ic)
{
	IS_TRUE0(IR_type(ir) == IR_ID && VAR_is_str(ID_info(ir)));
	IS_TRUE0(mds && ic);
	MD const* t = NULL;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		if ((t = m_ru->get_ru_mgr()->get_dedicated_str_md()) == NULL) {
			SYM * name = VAR_name(ID_info(ir));
			VAR * v = m_ru->get_var_mgr()->register_str(NULL, name, 1);
			VAR_allocable(v) = true;
			MD md;
			MD_base(&md) = v;
			MD_size(&md) = strlen(SYM_name(name)) + 1;
			MD_ofst(&md) = 0;
			MD_ty(&md) = MD_EXACT;
			IS_TRUE0(MD_is_str(&md));

			//Set string address to be taken only if it is base of LDA.
			//MD_is_addr_taken(&md) = true;
			MD * e = m_md_sys->register_md(md);
			IS_TRUE0(MD_id(e) > 0);
			t = e;
		}
		set_must_addr(ir, t);
		ir->clean_ref_mds();
		AC_is_mds_mod(ic) = true;
	} else {
		t = get_must_addr(ir);
		AC_is_mds_mod(ic) = false;
	}
	IS_TRUE0(t);
	mds->clean();
	mds->bunion(t);
	return t;
}


//'mds' : record memory descriptor of 'ir'.
void IR_AA::process_cst(IR * ir, IN OUT MD_SET & mds,
						IN OUT AA_CTX * ic)
{
	if (ir->is_str(m_dm)) {
		assign_string_const(ir, &mds, ic);
	} else {
		mds.clean();
		AC_is_mds_mod(ic) = false;
	}
}


void IR_AA::infer_stv(IN IR * ir, IR * rhs, MD const* lhs_md,
					  IN AA_CTX * ic, IN MD2MDS * mx)
{
	IS_TRUE0(ir->is_stid() || ir->is_stpr() || ir->is_setepr());

	/* Considering the STORE operation, there
	are three situations should be processed:
		1. The RHS's address was taken.
		2. The RHS is ILD, that cause indirect POINT-TO generated.
			e.g: p=ILD(q), if q->a->w, then p->w.
		3. Propagate the MD_SET that RHS pointed to.
			e.g: p=q, if q->a, then p->a. */
	IS_TRUE0(lhs_md && lhs_md->is_exact());

	/* 1. p = q, q is pointer, if q->x, add p->x.
	2. p = q, q is array base (e.g:q[100]), add p->q.
	3. p = &q, add p->q.
	4. p = (&q)+n+m, add p->q. */
	MD_SET * tmp = m_mds_mgr->create();
	MD_SET * pts = tmp;
	AA_CTX rhsic(*ic);
	if (ir->is_ptr(m_dm)) {
		AC_comp_pt(&rhsic) = true;
	}

	infer_exp(rhs, *pts, &rhsic, mx);

	if (AC_has_comp_lda(&rhsic)) {
		IS_TRUE0(ir->is_ptr(m_dm));
		IS_TRUE0(pts->get_effect_md(m_md_sys) != NULL);
		IS_TRUE0(ir->is_ptr(m_dm));
		UINT size = DTD_ptr_base_sz(ir->get_dtd(m_dm));
		revise_md_size(*pts, size);
	}

	//Update POINT-TO of LHS.
	IS_TRUE0(pts);
	if (AC_has_comp_lda(&rhsic) || AC_comp_pt(&rhsic)) {
		/* 2. p = q, q is array base (e.g:q[100]), add p->q.
		3. p = &q, add p->q.
		4. p = (&q)+n+m, add p->q. */
		if (AC_has_comp_lda(&rhsic)) {
			IS_TRUE0(pts->get_elem_count() == 1);

			//CASE: =&g[i] violate this constrain.
			//IS_TRUE0(ptset->is_contain_only_exact_and_str(m_md_sys));
		}

		/* We need to determine where is the rhs expression pointed to.
		Note that the point-to set can not be empty, rhs may be const/cvt.
		So lhs may point to anywhere. */
		MD * typed_md = NULL;
		if (pts != NULL && !pts->is_empty()) {
			if (pts->is_contain_global() &&
				IR_ai(ir) != NULL &&
				(typed_md = comp_point_to_via_type(rhs)) != NULL) {
				//Make use of typed pointer info to improve the precsion.
				pts->clean();
				pts->bunion(typed_md);
			}
		} else if (IR_ai(ir) != NULL &&
				   (typed_md = comp_point_to_via_type(rhs)) != NULL) {
			pts->bunion(typed_md);
		} else {
			//We do NOT known where p pointed to, and compute
			//the offset as well.

			//Do NOT modify pts any more.
			pts = const_cast<MD_SET*>(m_hashed_maypts);
		}

		IS_TRUE0(!pts->is_empty());
		if (m_flow_sensitive) {
			if (pts == m_hashed_maypts) {
				set_point_to(lhs_md, *mx, pts);
			} else {
				set_point_to_set(lhs_md, *mx, *pts);
			}
		} else {
			set_point_to_set_add_set(lhs_md, *mx, *pts);
		}
	} else {
		IS_TRUE0(!ir->is_ptr(m_dm));
		/* 1. p = q, q is pointer, if q->x, add p->x.
		Given a pointer, if its point-to is empty, the pointer
		points to MAY_POINT_TO_SET.

		May be unify MAY_PT_SET is correct in comprehension,
		but it  will occupy more memory.
		e.g: For pr1->MAY_PT_SET, and if we set
		pr1->NULL here, that might cause convert_md2mds_to_ptpair()
		can not recog pr1's POINT-TO set, and its pt-pair info
		is missing. That will cause a dead cycle at global
		iterative solver. */
		if (m_flow_sensitive) {
			clean_point_to(lhs_md, *mx);
		}
	}
	m_mds_mgr->free(tmp);
}


/* Caculate pointer info accroding to rules for individiual ir, and
constructing the mapping table that maps MD to an unique VAR.
e.g For given four point-to pairs {p->a,p->b,q->c,q->d}.
	store can be shown as
		p = q;
	this make the point-to set of 'p' to be {p->c, p->d}.
	and the formally formular form is:
	MD_SET(p) = MD_SET(q) */
void IR_AA::process_st(IN IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(ir->is_stid());
	MD const* t;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		t = alloc_st_md(ir);
	} else {
		t = get_must_addr(ir);
	}

	AA_CTX ic;
	infer_stv(ir, ST_rhs(ir), t, &ic, mx);
}


/* Caculate pointer info accroding to rules for individiual ir, and
constructing the mapping table that maps MD to an unique VAR.
e.g For given four point-to pairs {p->a,p->b,q->c,q->d}.
	store can be shown as
		p = q;
	this make the point-to set of 'p' to be {p->c, p->d}.
	and the formally formular form is:
	MD_SET(p) = MD_SET(q) */
void IR_AA::process_stpr(IN IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(ir->is_stpr());
	MD const* t;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		t = alloc_stpr_md(ir);
	} else {
		t = get_must_addr(ir);
	}

	AA_CTX ic;
	infer_stv(ir, STPR_rhs(ir), t, &ic, mx);
}


//Compute the point to info for IR_SETEPR.
void IR_AA::process_setepr(IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(ir->is_setepr());
	MD const* t;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		t = alloc_setepr_md(ir);
	} else {
		t = get_must_addr(ir);
	}

	AA_CTX ic;
	infer_stv(ir, SETEPR_rhs(ir), t, &ic, mx);

	if (!SETEPR_ofst(ir)->is_const()) {
		ic.clean();
		MD_SET * tmp = m_mds_mgr->create();
		MD_SET * pts = tmp;
		infer_exp(SETEPR_ofst(ir), *tmp, &ic, mx);
		m_mds_mgr->free(tmp);
	}
}


//Compute the point to info for IR_GETEPR.
void IR_AA::process_getepr(IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(ir->is_getepr() && GETEPR_ofst(ir));
	MD const* t;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		t = alloc_getepr_md(ir);
	} else {
		t = get_must_addr(ir);
	}

	//Process base field, it must refer to memory object.
	AA_CTX ic;
	MD_SET * mds = m_mds_mgr->create();
	infer_exp(GETEPR_base(ir), *mds, &ic, mx);

	//Process byte offset to base field.
	ic.clean();
	infer_exp(GETEPR_ofst(ir), *mds, &ic, mx);
	m_mds_mgr->free(mds);
}


void IR_AA::process_phi(IN IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_PHI);
	MD const* phi_res_md;
	if (!m_is_visit.is_contain(IR_id(ir))) {
		m_is_visit.bunion(IR_id(ir));
		phi_res_md = alloc_phi_md(ir);
	} else {
		phi_res_md = get_must_addr(ir);
	}

	AA_CTX ic;
	if (ir->is_ptr(m_dm)) {
		AC_comp_pt(&ic) = true;
	}

	bool const comp_pt = AC_comp_pt(&ic);
	MD_SET * tmp_res_pts = NULL;
	if (comp_pt) {
		tmp_res_pts = m_mds_mgr->create();
		MD_SET const* res_pts = get_point_to(phi_res_md, *mx);
		if (res_pts != NULL) {
			tmp_res_pts->copy(*res_pts);
		}
	} else {
		clean_point_to(phi_res_md, *mx);
	}

	MD_SET * tmp = m_mds_mgr->create();
	for (IR * opnd = PHI_opnd_list(ir); opnd != NULL; opnd = IR_next(opnd)) {
		infer_exp(opnd, *tmp, &ic, mx);

		ic.clean_bottom_up_flag();
		if (comp_pt) {
			//res may point to the union set of each operand.
			IS_TRUE0(tmp);
			tmp_res_pts->bunion(*tmp);
		}
	}

	m_mds_mgr->free(tmp);
	if (tmp_res_pts != NULL) {
		MD_SET const* hashed = m_mds_hash->append(*tmp_res_pts);
		set_point_to(phi_res_md, *mx, hashed);
		m_mds_mgr->free(tmp_res_pts);
	}
}


void IR_AA::infer_istv(IN IR * ir, IN AA_CTX * ic, IN MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_IST);
	MD_SET const* ist_mayaddr = get_may_addr(ir);
	MD const* ist_mustaddr = get_must_addr(ir);
	IS_TRUE0(ist_mustaddr != NULL ||
			(ist_mayaddr != NULL && !ist_mayaddr->is_empty()));

	/* Considering the ISTORE operation, there are three
	situations should be handled:
		1. The RHS is LDA.
		2. The RHS is ILD, that caused indirect POINT-TO generated.
			e.g: *p=ILD(q), and p->x,q->a,a->w, then x->w,
		3. Propagate the MD_SET that RHS pointed to the LHS.
			e.g: *p=q, and p->x,q->a, then x->a. */
	AA_CTX tic(*ic);
	if (ir->is_ptr(m_dm)) {
		AC_comp_pt(&tic) = 1;
	}

	MD_SET * tmp = m_mds_mgr->create();
	MD_SET * pts = tmp;

	infer_exp(IST_rhs(ir), *pts, &tic, mx);

	if (AC_has_comp_lda(&tic)) {
		IS_TRUE0(ir->is_ptr(m_dm));
		IS_TRUE0(pts->get_effect_md(m_md_sys) != NULL);

		//ptset may include element which also be in m_hashed_maypts.
		//IS_TRUE0(!ptset->is_intersect(*m_hashed_maypts));

		IS_TRUE(ir->is_ptr(m_dm), ("lda's base must be pointer."));
		UINT size = DTD_ptr_base_sz(ir->get_dtd(m_dm));
		revise_md_size(*pts, size);
	}

	IS_TRUE0(pts);
	if (AC_has_comp_lda(&tic) || AC_comp_pt(&tic)) {
		/* If result type of IST is pointer, and the ptset is empty, then
		it might point to anywhere.
		e.g: Given p=&q and *p=(int*)0x1000;
		=> q=0x1000, and q is pointer, so q may point to anywhere.

		*p = q, if p->{x}, q->{a}, add {x}->{a}
		*p = q, if p->{x}, q->{¦µ}, add {x}->{¦µ}
		*p = q, if p->{¦µ}, q->{x}, add {all mem}->{x}

		Update the POINT-TO of elems in p's point-to set.
		Aware of whether if the result of IST is pointer. */
		if (AC_has_comp_lda(&tic)) {
			IS_TRUE0(!pts->is_empty());

			//ptset may include element which also be in m_hashed_maypts.
			//IS_TRUE0(!ptset->is_intersect(m_hashed_maypts));
		}

		/* We need to determine where is the rhs expression pointed to.
		Note that the point-to set can not be empty, rhs may be const/cvt.
		So lhs may point to anywhere. */
		MD * typed_md = NULL;
		if (pts != NULL && !pts->is_empty()) {
			if (pts->is_contain_global() &&
				IR_ai(ir) != NULL &&
				(typed_md = comp_point_to_via_type(IST_rhs(ir))) != NULL) {
				//Make use of typed pointer info to improve the precsion.
				pts->clean();
				pts->bunion(typed_md);
			}
		} else if (IR_ai(ir) != NULL &&
				   (typed_md = comp_point_to_via_type(IST_rhs(ir))) != NULL) {
			pts->bunion(typed_md);
		} else {
			//We do NOT known where p pointed to, and compute
			//the offset as well.

			//Do NOT modify pts any more.
			pts = const_cast<MD_SET*>(m_hashed_maypts);
		}

		if (m_flow_sensitive) {
			if (ist_mustaddr != NULL) {
				if (ist_mustaddr->is_exact()) {
					if (pts == m_hashed_maypts) {
						set_point_to(ist_mustaddr, *mx, pts);
					} else {
						set_point_to_set(ist_mustaddr, *mx, *pts);
					}
				} else {
					set_point_to_set_add_set(ist_mustaddr, *mx, *pts);
				}
			} else {
				//mayaddr may contain inexact MD.
				elem_copy_union_pt(*ist_mayaddr, *pts, mx);
			}
		} else {
			//flow insensitive.
			if (ist_mustaddr != NULL) {
				set_point_to_set_add_set(ist_mustaddr, *mx, *pts);
			} else {
				elem_bunion_pt(*ist_mayaddr, *pts, mx);
			}
		}
	} else if (m_flow_sensitive) {
		IS_TRUE0(!ir->is_ptr(m_dm));
		if (ist_mustaddr != NULL) {
			if (ist_mustaddr->is_exact()) {
				clean_point_to(ist_mustaddr, *mx);
			}
		} else {
			//mayaddr may contain inexact MD.
			elem_clean_exact_pt(*ist_mayaddr, mx);
		}
	}
	m_mds_mgr->free(tmp);
}


/* Indirect store.
Analyse pointers according to rules for individiual ir to
constructe the map-table that maps MD to an unique VAR. */
void IR_AA::process_ist(IN IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_IST);
	IS_TRUE0(IST_base(ir)->is_ptr(m_dm) ||
			 IR_type(IST_base(ir)) == IR_ARRAY);

	//mem location may pointed to set.
	MD_SET * ml_may_pt = m_mds_mgr->create();
	AA_CTX ic;
	if (IR_type(IST_base(ir)) == IR_ARRAY) {
		//We do not need to known where array's elem point-to.
		//Array is special case of ist.
		AC_comp_pt(&ic) = false;
	} else {
		AC_comp_pt(&ic) = true;
	}

	//Compute where IST_base may point to.
	infer_exp(IST_base(ir), *ml_may_pt, &ic, mx);
	if (ml_may_pt->is_empty()) {
		m_mds_mgr->free(ml_may_pt);

		//If we can not determine where IST_base points to, it may
		//point to any variables which have been taken address.

		//Set ir has no exact mem-addr for convervative.
		ir->clean_ref_md();
		set_may_addr(ir, m_hashed_maypts);

		AA_CTX ic2;
		infer_istv(ir, &ic2, mx);
		return;
	}

	INT sz = -1;
	if (IST_ofst(ir) != 0 ||
		((sz = ir->get_dt_size(m_dm)) !=
		 (INT)IST_base(ir)->get_dt_size(m_dm))) {
		//Compute where IST_base may point to.
		MD_SET * t = m_mds_mgr->create();
		bool change = false;
		for (INT i = ml_may_pt->get_first();
			 i >= 0; i = ml_may_pt->get_next(i)) {
			MD * l = m_md_sys->get_md(i);
			IS_TRUE0(l);
			if (l->is_exact()) {
				MD md(*l);
				MD_ofst(&md) += IST_ofst(ir);
				IS_TRUE0(ir->get_dt_size(m_dm) > 0);
				MD_size(&md) = sz == -1 ? ir->get_dt_size(m_dm) : sz;
				MD * entry = m_md_sys->register_md(md);
				IS_TRUE(MD_id(entry) > 0, ("Not yet registered"));
				t->bunion(entry);
				change = true;
			} else {
				t->bunion(l);
			}
		}

		if (change) { ml_may_pt->copy(*t); }
		m_mds_mgr->free(t);
	}

	MD const* x;
	if (ml_may_pt->get_elem_count() == 1 &&
		!MD_is_may(x = m_md_sys->get_md(ml_may_pt->get_first()))) {
		set_must_addr(ir, x);
		ir->clean_ref_mds();
	} else {
		//Set ir has no exact mem-addr for convervative.
		ir->clean_ref_md();
		set_may_addr(ir, m_mds_hash->append(*ml_may_pt));
	}

	AA_CTX ic2;
	infer_istv(ir, &ic2, mx);
	m_mds_mgr->free(ml_may_pt);
}


//NOTE: The def and use info should be available for region, otherwise
//this function will be costly.
void IR_AA::process_region(IR const* ir, IN MD2MDS * mx)
{
	REGION * ru = REGION_ru(ir);
	IS_TRUE0(ru);
	IS_TRUE0(RU_type(ru) == RU_BLX || RU_type(ru) == RU_SUB);

	bool comp = false;

	//Check if region def and use md set.
	MD_SET const* defmds = ru->get_may_def();
	if (defmds != NULL && !defmds->is_empty()) {
		elem_copy_pt_maypts(*defmds, mx);
		comp = true;
	}

	defmds = ru->get_must_def();
	if (defmds != NULL && !defmds->is_empty()) {
		elem_copy_pt_maypts(*defmds, mx);
		comp = true;
	}

	if (!comp) {
		/* For conservative purpose, region may change
		global variable's point-to and local variable's
		point-to which are forseeable. */
		MDID2MD const* id2md = m_md_sys->get_id2md_map();

		for (INT j = 0; j <= id2md->get_last_idx(); j++) {
			MD * t = id2md->get(j);
			if (t == NULL) { continue; }

			if (VAR_is_pointer(MD_base(t))) {
				set_point_to_set_add_set(t, *mx, *m_hashed_maypts);
			}
		}
	}
}


void IR_AA::process_return(IN IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_RETURN);
	if (RET_exp(ir) != NULL) {
		MD_SET * tmp = m_mds_mgr->create();
		AA_CTX tic;
		infer_exp(RET_exp(ir), *tmp, &tic, mx);
		m_mds_mgr->free(tmp);
	}
}


void IR_AA::process_call_sideeffect(IN OUT MD2MDS & mx, bool by_addr,
									MD_SET const& by_addr_mds)
{
	//Set all mds which are global pointers or parameters which taken
	//address point to maypts.
	MDID2MD const* id2md = m_md_sys->get_id2md_map();
	for (INT j = 0; j <= id2md->get_last_idx(); j++) {
		MD * t = id2md->get(j);
		if (t == NULL) { continue; }
		VAR const* v = MD_base(t);
		if (VAR_is_global(v) && VAR_is_pointer(v)) {
			set_point_to(t, mx, m_hashed_maypts);

			//Set the point-to set of 't' to be empty in order
			//to enforce its point-to set will be recomputed if
			//necessary.
			//clean_point_to(t, mx);
		}
	}

	if (by_addr) {
		for (INT j = by_addr_mds.get_first();
			 j >= 0; j = by_addr_mds.get_next(j)) {
			MD const* t = m_md_sys->get_md(j);
			IS_TRUE0(t != NULL);
			if (VAR_is_pointer(MD_base(t)) && MD_is_addr_taken(t)) {
				set_point_to(t, mx, m_hashed_maypts);

				//Set the point-to set of 't' to be empty in order
				//to enforce its point-to set will be recomputed if
				//necessary.
				//clean_point_to(t, mx);
			}
		}
	}
}


MD * IR_AA::alloc_heap_obj(IR * ir)
{
	MD * heap_obj = m_ir2heapobj.get(ir);
	if (heap_obj != NULL) {
		return heap_obj;
	}

	CHAR name[64];
	sprintf(name, "heap_obj%d", m_ir2heapobj.get_elem_count());
	IS_TRUE0(strlen(name) < 64);
	VAR * tv = m_ru->get_var_mgr()->register_var(name, D_MC,
												 D_UNDEF, 0, 0, 0,
												 VAR_GLOBAL);
	/* Set the var to be unallocable, means do NOT add
	var immediately as a memory-variable.
	For now, it is only be regarded as a placeholder.
	And set it to allocable if the var is in essence need to be
	allocated in memory. */
	VAR_allocable(tv) = false;

	//Will be freed region destruction.
	m_ru->add_to_var_tab(tv);

	MD md;
	MD_base(&md) = tv;
	MD_ty(&md) = MD_UNBOUND;
	MD * entry = m_md_sys->register_md(md);
	IS_TRUE0(MD_id(entry) > 0);
	m_ir2heapobj.set(ir, entry);
	return entry;
}


//Compute the point-to set modification when we meet call.
void IR_AA::process_call(IN IR * ir, IN MD2MDS * mx)
{
	IS_TRUE0(IR_type(ir) == IR_CALL || IR_type(ir) == IR_ICALL);

	MD_SET * tmp = m_mds_mgr->create();
	if (IR_type(ir) == IR_ICALL) {
		AA_CTX tic;
		infer_exp(ICALL_callee(ir), *tmp, &tic, mx);
	}

	//Analyz parameters.
	IR * param = CALL_param_list(ir);
	bool by_addr = false;
	MD_SET * by_addr_mds = m_mds_mgr->create();
	while (param != NULL) {
		AA_CTX tic;
		infer_exp(param, *tmp, &tic, mx);
		if (AC_has_comp_lda(&tic)) {
			by_addr = true;
			by_addr_mds->bunion(*tmp);
		}
		param = IR_next(param);
	}

	if (CALL_is_alloc_heap(ir)) {
		if (ir->has_return_val()) {
			MD const* t;
			if (!m_is_visit.is_contain(IR_id(ir))) {
				m_is_visit.bunion(IR_id(ir));
				t = alloc_call_pr_md(ir);
			} else {
				t = get_must_addr(ir);
			}
			set_point_to_unique_md(t, *mx, alloc_heap_obj(ir));
		}

		//The function such as malloc or new function should not modify
		//the memory in XOC scope.
		m_mds_mgr->free(tmp);
		m_mds_mgr->free(by_addr_mds);
		return;
	}

	//Analyz return-values.
	if (ir->has_return_val()) {
		MD const* t = NULL;
		if (!m_is_visit.is_contain(IR_id(ir))) {
			m_is_visit.bunion(IR_id(ir));
			t = alloc_call_pr_md(ir);
		} else {
			t = get_must_addr(ir);
		}

		IS_TRUE(t, ("result of call miss exact MD."));

		if (ir->is_ptr(m_dm)) {
			//Set the pointer points to the May-Point-To set.

			//Try to improve the precsion via typed alias info.
			MD * typed_md;
			if (IR_ai(ir) != NULL &&
				(typed_md = comp_point_to_via_type(ir)) != NULL) {
				//Make use of typed pointer info to improve the precsion.
				set_point_to_unique_md(t, *mx, typed_md);
			} else {
				//Finally, set result PR points to May-Point-To set.
				set_point_to(t, *mx, m_hashed_maypts);
			}
		} else {
			clean_point_to(t, *mx);
		}
	}

	if (ir->is_readonly_call()) {
		//Readonly call does not modify any point-to informations.
		m_mds_mgr->free(tmp);
		m_mds_mgr->free(by_addr_mds);
		return;
	}

	process_call_sideeffect(*mx, by_addr, *by_addr_mds);
	m_mds_mgr->free(tmp);
	m_mds_mgr->free(by_addr_mds);
}


/* Analyze the TREE style memory-address-expression,
and compute the MD_SET for 'expr'.

'expr': IR expressions that describing memory address.
'mds': record memory descriptors which 'expr' might express.
'ic': context of analysis. */
void IR_AA::infer_exp(IR * expr, IN OUT MD_SET & mds,
					  IN OUT AA_CTX * ic, IN OUT MD2MDS * mx)
{
	switch (IR_type(expr)) {
	case IR_ID:
		assign_id_md(expr, &mds, ic);
		return;
	case IR_ILD:
		process_ild(expr, mds, ic, mx);
		return;
	case IR_LD:
		assign_ld_md(expr, &mds, ic, mx);
		return;
	case IR_LDA:
		process_lda(expr, mds, ic, mx);
		return;
	case IR_ARRAY:
		process_array(expr, mds, ic, mx);
		return;
	case IR_CONST:
		process_cst(expr, mds, ic);
		return;
	case IR_ADD:
	case IR_SUB:
		process_pointer_arith(expr, mds, ic, mx);
		return;
	case IR_PR:
		assign_pr_md(expr, &mds, ic, mx);
		return;
	case IR_CVT:
		process_cvt(expr, mds, ic, mx);
		return;
	case IR_ASR:
	case IR_LSR: //Logical shift right
	case IR_LSL: //Logical shift left
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
		{
			IS_TRUE(!BIN_opnd0(expr)->is_ptr(m_dm),
					("illegal, left operand can not be pointer type"));
			AA_CTX tic(*ic);
			AC_comp_pt(&tic) = false;
			infer_exp(BIN_opnd1(expr), mds, &tic, mx);

			tic.clean_bottom_up_flag();
			infer_exp(BIN_opnd0(expr), mds, &tic, mx);
			/* These expressions does not descripte
			an accurate memory-address. So, for the
			conservative purpose, we claim that can
			not find any MD. */
			if (AC_comp_pt(ic)) {
				mds.copy(*m_hashed_maypts);
			} else {
				mds.clean();
			}
		}
		return;
	case IR_BNOT:
	case IR_NEG:
	case IR_LNOT:
		{
			IS_TRUE(!UNA_opnd0(expr)->is_ptr(m_dm),
					("Illegal, left operand can not be pointer type"));
			AA_CTX tic(*ic);
			AC_comp_pt(&tic) = false;
			infer_exp(UNA_opnd0(expr), mds, &tic, mx);

			/* These expressions does not descripte
			an accurate memory-address. So, for the
			conservative purpose, we claim that can
			not find any MD. */
			if (AC_comp_pt(ic)) {
				mds.copy(*m_hashed_maypts);
			} else {
				mds.clean();
			}
		}
		return;
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
		{
			AA_CTX tic(*ic);
			AC_comp_pt(&tic) = false;
			infer_exp(BIN_opnd0(expr), mds, &tic, mx);

			tic.clean_bottom_up_flag();
			infer_exp(BIN_opnd1(expr), mds, &tic, mx);
			if (AC_comp_pt(ic)) {
				mds.copy(*m_hashed_maypts);
			} else {
				mds.clean();
			}
		}
		return;
	case IR_LABEL: return;
	case IR_SELECT:
		{
			AA_CTX tic(*ic);
			AC_comp_pt(&tic) = false;

			infer_exp(SELECT_det(expr), mds, &tic, mx);

			tic.clean_bottom_up_flag();
			infer_exp(SELECT_trueexp(expr), mds, &tic, mx);

			tic.clean_bottom_up_flag();
			infer_exp(SELECT_falseexp(expr), mds, &tic, mx);
			if (AC_comp_pt(ic)) {
				//We do not know if condition is true or false.
				mds.copy(*m_hashed_maypts);
			} else {
				mds.clean();
			}
		}
		return;
	default: IS_TRUE0(0);
	}
}


//Set POINT TO info.
//Set each md in 'mds' add set 'pt_set'.
void IR_AA::elem_bunion_pt(MD_SET const& mds, IN MD_SET & pt_set,
						   IN MD2MDS * mx)
{
	bool set_all = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md);
		if (is_all_mem(md)) {
			set_all = true;
			MDID2MD const* id2md = m_md_sys->get_id2md_map();
			for (INT j = 0; j <= id2md->get_last_idx(); j++) {
				MD * t = id2md->get(j);
				if (t == NULL) { continue; }
				set_point_to_set_add_set(t, *mx, pt_set);
			}
		} else if (is_heap_mem(md)) {
			if (set_all) { continue; }
			for (INT j = m_id2heap_md_map.get_first();
				 j >= 0; j = m_id2heap_md_map.get_next(j)) {
				MD * t = m_md_sys->get_md(j);
				IS_TRUE0(t != NULL);
				set_point_to_set_add_set(t, *mx, pt_set);
			}
		} else {
			set_point_to_set_add_set(md, *mx, pt_set);
		}
	}
}


//Set POINT TO info.
//Set each md in 'mds' add 'pt_elem'.
void IR_AA::elem_bunion_pt(IN MD_SET const& mds, IN MD * pt_elem,
						   IN MD2MDS * mx)
{
	bool set_all = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md);
		if (is_all_mem(md)) {
			set_all = true;

			MDID2MD const* id2md = m_md_sys->get_id2md_map();
			for (INT j = 0; j <= id2md->get_last_idx(); j++) {
				MD * t = id2md->get(j);
				if (t == NULL) { continue; }
				set_point_to_set_add_md(t, *mx, pt_elem);
			}
		} else if (is_heap_mem(md)) {
			if (set_all) { continue; }
			for (INT j = m_id2heap_md_map.get_first();
				 j >= 0; j = m_id2heap_md_map.get_next(j)) {
				MD * t = m_md_sys->get_md(j);
				IS_TRUE0(t != NULL);
				set_point_to_set_add_md(t, *mx, pt_elem);
			}
		} else {
			set_point_to_set_add_md(md, *mx, pt_elem);
		}
	}
}


/* Set POINT TO info.
Set each md in 'mds' points to 'pt_set' if it is exact, or
else unify the set. */
void IR_AA::elem_copy_union_pt(MD_SET const& mds, IN MD_SET & pt_set,
							   IN MD2MDS * mx)
{
	bool set_all = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD const* md = m_md_sys->get_md(i);
		IS_TRUE0(md != NULL);
		if (is_all_mem(md)) {
			set_all = true;

			MDID2MD const* id2md = m_md_sys->get_id2md_map();
			for (INT j = 0; j <= id2md->get_last_idx(); j++) {
				MD * t = id2md->get(j);
				if (t == NULL) { continue; }

				if (t->is_exact()) {
					set_point_to_set(t, *mx, pt_set);
				} else {
					set_point_to_set_add_set(t, *mx, pt_set);
				}
			}
		} else if (is_heap_mem(md)) {
			if (set_all) { continue; }
			for (INT j = m_id2heap_md_map.get_first();
				 j >= 0; j = m_id2heap_md_map.get_next(j)) {
				MD * t = m_md_sys->get_md(j);
				IS_TRUE0(t != NULL);
				if (t->is_exact()) {
					set_point_to_set(t, *mx, pt_set);
				} else {
					set_point_to_set_add_set(t, *mx, pt_set);
				}
			}
		} else {
			if (md->is_exact()) {
				set_point_to_set(md, *mx, pt_set);
			} else {
				set_point_to_set_add_set(md, *mx, pt_set);
			}
		}
	}
}


//Set POINT TO info.
//Set each md in 'mds' points to 'pt_set'.
void IR_AA::elem_copy_pt(MD_SET const& mds, IN MD_SET & pt_set,
						 IN MD2MDS * mx)
{
	bool set_all = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md != NULL);
		if (is_all_mem(md)) {
			set_all = true;
			MDID2MD const* id2md = m_md_sys->get_id2md_map();
			for (INT j = 0; j <= id2md->get_last_idx(); j++) {
				MD * t = id2md->get(j);
				if (t == NULL) { continue; }
				set_point_to_set(t, *mx, pt_set);
			}
		} else if (is_heap_mem(md)) {
			if (set_all) { continue; }
			for (INT j = m_id2heap_md_map.get_first();
				 j >= 0; j = m_id2heap_md_map.get_next(j)) {
				MD * t = m_md_sys->get_md(j);
				IS_TRUE0(t != NULL);
				set_point_to_set(t, *mx, pt_set);
			}
		} else {
			set_point_to_set(md, *mx, pt_set);
		}
	}
}


//Set POINT TO info.
//Set each md in 'mds' points to May-Point-To set.
void IR_AA::elem_copy_pt_maypts(MD_SET const& mds, IN MD2MDS * mx)
{
	bool set_all = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md != NULL);
		if (is_all_mem(md)) {
			set_all = true;
			MDID2MD const* id2md = m_md_sys->get_id2md_map();
			for (INT j = 0; j <= id2md->get_last_idx(); j++) {
				MD * t = id2md->get(j);
				if (t == NULL) { continue; }
				set_point_to(t, *mx, m_hashed_maypts);
			}
		} else if (is_heap_mem(md)) {
			if (set_all) { continue; }
			for (INT j = m_id2heap_md_map.get_first();
				 j >= 0; j = m_id2heap_md_map.get_next(j)) {
				MD * t = m_md_sys->get_md(j);
				IS_TRUE0(t != NULL);
				set_point_to(t, *mx, m_hashed_maypts);
			}
		} else {
			set_point_to(md, *mx, m_hashed_maypts);
		}
	}
}


//Set POINT TO info.
//Set md in 'mds' points to NULL if it is exact.
void IR_AA::elem_clean_exact_pt(MD_SET const& mds, IN MD2MDS * mx)
{
	bool set_all = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md != NULL);
		if (is_all_mem(md)) {
			set_all = true;
			MDID2MD const* id2md = m_md_sys->get_id2md_map();
			for (INT j = 0; j <= id2md->get_last_idx(); j++) {
				MD * t = id2md->get(j);
				if (t == NULL) { continue; }
				clean_point_to(t, *mx);
			}
		} else if (is_heap_mem(md)) {
			if (set_all) { continue; }
			for (INT j = m_id2heap_md_map.get_first();
				 j >= 0; j = m_id2heap_md_map.get_next(j)) {
				MD * t = m_md_sys->get_md(j);
				IS_TRUE0(t != NULL);
				clean_point_to(t, *mx);
			}
		} else if (md->is_exact()) {
			clean_point_to(md, *mx);
		}
	}
}


//Set POINT TO info.
//Set md in 'mds' points to NULL.
void IR_AA::elem_clean_pt(MD_SET const& mds, IN MD2MDS * mx)
{
	bool set_all = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = m_md_sys->get_md(i);
		IS_TRUE0(md != NULL);
		if (is_all_mem(md)) {
			set_all = true;

			MDID2MD const* id2md = m_md_sys->get_id2md_map();
			for (INT j = 0; j <= id2md->get_last_idx(); j++) {
				MD * t = id2md->get(j);
				if (t == NULL) { continue; }
				clean_point_to(t, *mx);
			}
		} else if (is_heap_mem(md)) {
			if (set_all) { continue; }
			for (INT j = m_id2heap_md_map.get_first();
				 j >= 0; j = m_id2heap_md_map.get_next(j)) {
				MD * t = m_md_sys->get_md(j);
				IS_TRUE0(t != NULL);
				clean_point_to(t, *mx);
			}
		} else {
			clean_point_to(md, *mx);
		}
	}
}


//Dump IR's point-to of each BB.
void IR_AA::dump_bb_in_out_pt_set()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP POINT TO INFO ----==");
	IR_BB_LIST * bbl = m_cfg->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		PT_PAIR_SET * in_set = get_in_pp_set(bb);
		PT_PAIR_SET * out_set = get_out_pp_set(bb);
		fprintf(g_tfile, "\n--- BB%d ---", IR_BB_id(bb));
		fprintf(g_tfile, "\nIN-SET::");
		dump_pps(*in_set);
		fprintf(g_tfile, "\n\nOUT-SET::");
		dump_pps(*out_set);
	}
	fflush(g_tfile);
}


//Dump POINT-TO pair record in 'pps'.
void IR_AA::dump_pps(PT_PAIR_SET & pps)
{
	if (g_tfile == NULL) return;
	CHAR buf[1000];
	UINT k = 0;
	bool detail = true;
	for (INT i = pps.get_first(); i >= 0; i = pps.get_next(i), k++) {
		PT_PAIR * pp = m_pt_pair_mgr.get(i);
		IS_TRUE0(pp);
		fprintf(g_tfile, "\nMD%d->MD%d,  ",
						MD_id(PP_from(pp)), MD_id(PP_to(pp)));

		if (detail) {
			fprintf(g_tfile, "%s", MD_base(PP_from(pp))->dump(buf));
			if (PP_from(pp)->is_exact()) {
				fprintf(g_tfile, ":ofst(%d):size(%d)",
						MD_ofst(PP_from(pp)), MD_size(PP_from(pp)));
			} else {
				fprintf(g_tfile, ":ofst(--):size(%d)", MD_size(PP_from(pp)));
			}

			fprintf(g_tfile, " ------> ");

			fprintf(g_tfile, "%s", MD_base(PP_to(pp))->dump(buf));
			if (PP_to(pp)->is_exact()) {
				fprintf(g_tfile, ":ofst(%d):size(%d)",
						MD_ofst(PP_to(pp)), MD_size(PP_to(pp)));
			} else {
				fprintf(g_tfile, ":ofst(--):size(%d)", MD_size(PP_to(pp)));
			}
		}
	}
	fflush(g_tfile);
}


//Dump 'ir' point-to according to 'mx'.
//'dump_kid': dump kid's memory object if it exist.
void IR_AA::dump_ir_point_to(IN IR * ir, bool dump_kid, IN MD2MDS * mx)
{
	if (ir == NULL || g_tfile == NULL) return;
	MD const* must = get_must_addr(ir);
	MD_SET const* may = get_may_addr(ir);
	if (must != NULL ||
		(may != NULL && may->get_elem_count() > 0)) {
		dump_ir(ir, m_dm, NULL, false, false);
	}

	switch (IR_type(ir)) {
	case IR_ID:
	case IR_LD:
	case IR_PR:
	case IR_ST:
		if (must != NULL) {
			dump_md2mds(must, mx);
		}
		break;
	default:
		if (may != NULL) {
			for (INT i = may->get_first(); i >= 0; i = may->get_next(i)) {
				MD * md = m_md_sys->get_md(i);
				IS_TRUE0(md);
				dump_md2mds(md, mx);
			}
		} else if (must != NULL) {
			dump_md2mds(must, mx);
		}
	}
	if (dump_kid) {
		for (INT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
			IR * kid = ir->get_kid(i);
			if (kid != NULL) {
				dump_ir_point_to(kid, dump_kid, mx);
			}
		}
	}
	fflush(g_tfile);
}


//Dump all relations between IR, MD, and MD_SET.
//'md2mds': mapping from 'md' to an md-set it pointed to.
void IR_AA::dump_bb_ir_point_to(IR_BB * bb, bool dump_kid)
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n\n--- BB%d ---", IR_BB_id(bb));
	PT_PAIR_SET * in_set = get_in_pp_set(bb);
	C<IR*> * ct;
	MD2MDS * mx;
	if (m_flow_sensitive) {
		mx = map_bb_to_md2mds(bb);
	} else {
		mx = &m_unique_md2mds;
	}
	if (mx == NULL) {
		//e.g: If one has performed PRE and generated new BB, but
		//not invoked the IRAA::perform(), then the mx of
		//the new BB is not constructed.

		//interwarn("In IRAA, MD2MDS of BB%d is NULL, may be new "
		//		  "bb was generated. One should recall IRAA::perform()",
		//		  IR_BB_id(bb));
		fprintf(g_tfile, "\n--- BB%d's MD2MDS is NULL ---",
				IR_BB_id(bb));
		return;
	}

	dump_md2mds(mx, true);
	for (IR * ir = IR_BB_ir_list(bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&ct)) {
		fprintf(g_tfile, "\n---------");
		g_indent = 4;
		dump_irs(ir, m_dm);
		fprintf(g_tfile, "\n");
		switch (IR_type(ir)) {
		case IR_ST:
			fprintf(g_tfile, "LHS:");
			dump_ir_point_to(ir, false, mx);
			fprintf(g_tfile, "\nRHS:");
			dump_ir_point_to(ST_rhs(ir), false, mx);

			if (dump_kid) {
				fprintf(g_tfile, "\n>> MD_SET DETAIL:");
				dump_ir_point_to(ST_rhs(ir), true, mx);
			}
			break;
		case IR_STPR:
			fprintf(g_tfile, "LHS:");
			dump_ir_point_to(ir, false, mx);
			fprintf(g_tfile, "\nRHS:");
			dump_ir_point_to(STPR_rhs(ir), false, mx);
			if (dump_kid) {
				fprintf(g_tfile, "\n>> MD_SET DETAIL:");
				dump_ir_point_to(STPR_rhs(ir), true, mx);
			}
			break;
		case IR_IST: //indirective store
			fprintf(g_tfile, "LHS:");
			dump_ir_point_to(ir, false, mx);
			fprintf(g_tfile, "\nRHS:");
			dump_ir_point_to(IST_rhs(ir), false, mx);
			if (dump_kid) {
				fprintf(g_tfile, "\n>> MD_SET DETAIL:");
				dump_ir_point_to(IST_base(ir), true, mx);
				dump_ir_point_to(IST_rhs(ir), true, mx);
			}
			break;
		case IR_CALL:
			{
				if (ir->has_return_val()) {
					fprintf(g_tfile, "LHS:");
					dump_ir_point_to(ir, false, mx);
				}

				UINT i = 0;
				IR * param = CALL_param_list(ir);
				while (param != NULL) {
					fprintf(g_tfile, "\nPARAM%d:", i++);
					dump_ir_point_to(param, false, mx);
					param = IR_next(param);
				}

				if (dump_kid && CALL_param_list(ir) != NULL) {
					fprintf(g_tfile, "\n>> MD_SET DETAIL:\n");
				}

				if (dump_kid && CALL_param_list(ir) != NULL) {
					IR * param = CALL_param_list(ir);
					while (param != NULL) {
						dump_ir_point_to(param, true, mx);
						param = IR_next(param);
					}
				}
			}
			break;
		case IR_ICALL: //indirective call
			{
				if (ir->has_return_val()) {
					fprintf(g_tfile, "LHS:");
					dump_ir_point_to(ir, false, mx);
				}

				IS_TRUE0(ICALL_callee(ir) != NULL);
				fprintf(g_tfile, "CALLEE:");
				dump_ir_point_to(ICALL_callee(ir), false, mx);

				if (dump_kid && CALL_param_list(ir) != NULL) {
					fprintf(g_tfile, "\n>> MD_SET DETAIL:\n");
				}

				if (dump_kid && CALL_param_list(ir) != NULL) {
					IR * param = CALL_param_list(ir);
					while (param != NULL) {
						dump_ir_point_to(param, true, mx);
						param = IR_next(param);
					}
				}
			}
			break;
		case IR_GOTO:
			break;
		case IR_IGOTO:
			IS_TRUE0(IGOTO_vexp(ir) != NULL);
			fprintf(g_tfile, "VEXP:");
			dump_ir_point_to(IGOTO_vexp(ir), false, mx);
			if (dump_kid) {
				fprintf(g_tfile, "\n>> MD_SET DETAIL:");
				dump_ir_point_to(IGOTO_vexp(ir), true, mx);
			}
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			IS_TRUE0(BR_det(ir) != NULL);
			fprintf(g_tfile, "DET:");
			dump_ir_point_to(BR_det(ir), false, mx);
			if (dump_kid) {
				fprintf(g_tfile, "\n>> MD_SET DETAIL:");
				dump_ir_point_to(BR_det(ir), true, mx);
			}
			break;
		case IR_SELECT:
			IS_TRUE0(SELECT_det(ir) != NULL);
			fprintf(g_tfile, "DET:");
			dump_ir_point_to(SELECT_det(ir), false, mx);
			if (dump_kid) {
				fprintf(g_tfile, "\n>> MD_SET DETAIL:");
				dump_ir_point_to(SELECT_det(ir), true, mx);
			}
			break;
		case IR_SWITCH:
			IS_TRUE0(SWITCH_vexp(ir) != NULL);
			fprintf(g_tfile, "VEXP:");
			dump_ir_point_to(SWITCH_vexp(ir), false, mx);
			if (dump_kid) {
				fprintf(g_tfile, "\n>> MD_SET DETAIL:");
				dump_ir_point_to(SWITCH_vexp(ir), true, mx);
			}
			break;
		case IR_RETURN:
			{
				IR * p = RET_exp(ir);
				if (RET_exp(ir) != NULL) {
					dump_ir_point_to(RET_exp(ir), false, mx);
				}

				if (dump_kid && RET_exp(ir) != NULL) {
					fprintf(g_tfile, "\n>> MD_SET DETAIL:");
					dump_ir_point_to(RET_exp(ir), true, mx);
				}
			}
			break;
		default:;
		} //end switch
	}
}


//Dump all relations between IR, MD, and MD_SET.
//'md2mds': mapping from 'md' to an md-set it pointed to.
void IR_AA::dump_all_ir_point_to(bool dump_kid)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==--- DUMP '%s' IR_AA : IR ADDRESS and POINT-TO ----==",
			m_ru->get_ru_name());
	m_md_sys->dump_all_mds();
	fprintf(g_tfile, "\n\n---- All MD in MAY-POINT-TO SET : ");

	IS_TRUE0(m_hashed_maypts);
	m_hashed_maypts->dump(m_md_sys, true);

	IR_BB_LIST * bbl = m_cfg->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		dump_bb_ir_point_to(bb, dump_kid);
	}
	fflush(g_tfile);
}


void IR_AA::dump_may_point_to()
{
	if (g_tfile == NULL || m_hashed_maypts == NULL) return;
	for (INT j = m_hashed_maypts->get_first();
		 j >= 0; j = m_hashed_maypts->get_next(j)) {
		MD * mmd = m_md_sys->get_md(j);
		IS_TRUE0(mmd != NULL);
		fprintf(g_tfile, "MD%d,", MD_id(mmd));
	}
	fflush(g_tfile);
}


//Dump MD's point-to for each BB.
void IR_AA::dump_all_md2mds(bool dump_pt_graph)
{
	if (g_tfile == NULL) return;
	if (m_flow_sensitive) {
		fprintf(g_tfile, "\n\n==---- DUMP ALL MD POINT-TO "
						 "OUT-SET (FLOW SENSITIVE) ----==");
		IR_BB_LIST * bbl = m_cfg->get_bb_list();
		for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
			fprintf(g_tfile, "\n\n--- BB%d ---", IR_BB_id(bb));
			dump_md2mds(map_bb_to_md2mds(bb), dump_pt_graph);
		}
	} else {
		fprintf(g_tfile, "\n\n==---- DUMP ALL MD POINT-TO "
						 "OUT-SET (FLOW-INSENSITIVE) ----==");
		dump_md2mds(&m_unique_md2mds, dump_pt_graph);
	}
	fflush(g_tfile);
}


//Dump MD's point-to according to individual 'mx'.
//'dump_ptg': dump POINT-TO graph.
void IR_AA::dump_md2mds(IN MD2MDS * mx, bool dump_ptg)
{
	if (g_tfile == NULL || mx == NULL) return;
	GRAPH g;
	MDID2MD const* id2md = m_md_sys->get_id2md_map();
	for (INT i = 0; i <= id2md->get_last_idx(); i++) {
		MD * md = id2md->get(i);
		if (md == NULL) { continue; }

		MD_SET const* mds = get_point_to(md, *mx);
		if (mds != NULL) {
			fprintf(g_tfile, "\nMD%d -- PT_SET: ", MD_id(md));
			for (INT j = mds->get_first(); j >= 0; j = mds->get_next(j)) {
				MD const* mmd = m_md_sys->get_md(j);
				IS_TRUE0(mmd != NULL);
				fprintf(g_tfile, "MD%d,", MD_id(mmd));
				if (dump_ptg) {
					g.add_edge(i, j);
				}
			}
		} else {
			fprintf(g_tfile, "\nMD%d -- NO PT", MD_id(md));
		}
	}

	if (dump_ptg) {
		g.dump_vcg("graph_point_to.vcg");
	}
	fflush(g_tfile);
}


/* Dump relations between MD, MD_SET.
'md': candidate to dump.
'md2mds': mapping from 'md' to an md-set it pointed to. */
void IR_AA::dump_md2mds(MD const* md, IN MD2MDS * mx)
{
	if (g_tfile == NULL) return;
	CHAR buf[MAX_BUF_LEN];
	buf[0] = 0;
	fprintf(g_tfile, "\n\t  %s", md->dump(buf, MAX_BUF_LEN));

	//Dump MD_SET of 'md'.
	MD_SET const* pts = get_point_to(md, *mx);
	fprintf(g_tfile, "\n\t\tPOINT TO:");
	if (pts != NULL && !pts->is_empty()) {
		fprintf(g_tfile, "\n");
		for (INT j = pts->get_first(); j >= 0; j = pts->get_next(j)) {
			MD const* mmd = m_md_sys->get_md(j);
			IS_TRUE0(mmd != NULL);
			buf[0] = 0;
			fprintf(g_tfile, "\t\t\t%s\n", mmd->dump(buf, MAX_BUF_LEN));
		}
	} else {

		fprintf(g_tfile, "----");
	}
	fflush(g_tfile);
}


void IR_AA::md2mds2pt(OUT PT_PAIR_SET & pps, IN PT_PAIR_MGR & pt_pair_mgr,
					  IN MD2MDS * mx)
{
	MD * allm = m_md_sys->get_md(MD_ALL_MEM);
	for (INT i = mx->get_first(); i >= 0; i = mx->get_next(i)) {
		MD const* from_md = m_md_sys->get_md(i);
		MD_SET const* from_md_pts = get_point_to(i, *mx);
		if (from_md_pts == NULL) {
			continue;
		}
		if (from_md_pts->is_contain_all()) {
			PT_PAIR const* pp = pt_pair_mgr.add(from_md, allm);
			IS_TRUE0(pp);
			pps.bunion(PP_id(pp));
		} else {
			for (INT j = from_md_pts->get_first();
				 j >= 0; j = from_md_pts->get_next(j)) {
				MD const* to_md = m_md_sys->get_md(j);
				PT_PAIR const* pp = pt_pair_mgr.add(from_md, to_md);
				IS_TRUE0(pp != NULL);
				pps.bunion(PP_id(pp));
			}
		}
	}
}


void IR_AA::pt2md2mds(PT_PAIR_SET const& pps,
					  IN PT_PAIR_MGR & pt_pair_mgr,
					  IN OUT MD2MDS * ctx)
{
	for (INT i = pps.get_first(); i >= 0; i = pps.get_next(i)) {
		PT_PAIR * pp = pt_pair_mgr.get(i);
		IS_TRUE0(pp != NULL);
		set_point_to_set_add_md(PP_from(pp), *ctx, PP_to(pp));
	}
}


/* Solving POINT-TO out set.
While the function terminiate, OUT info has been recorded
in related 'mx'. */
void IR_AA::compute_stmt_pt(IR_BB const* bb, IN OUT MD2MDS * mx)
{
	IS_TRUE0(mx != NULL);
	C<IR*> * ct;
	IR_BB * readonly_bb = const_cast<IR_BB*>(bb); //ensure we do not moidy it.
	for (IR * ir = IR_BB_ir_list(readonly_bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(readonly_bb).get_next(&ct)) {
		IS_TRUE0(is_valid_stmt_to_aa(ir));
		switch (IR_type(ir)) {
		case IR_ST:
			process_st(ir, mx);
			break;
		case IR_STPR:
			process_stpr(ir, mx);
			break;
		case IR_SETEPR:
			process_setepr(ir, mx);
			break;
		case IR_GETEPR:
			process_getepr(ir, mx);
			break;
		case IR_IST:
			process_ist(ir, mx);
			break;
		case IR_CALL:
		case IR_ICALL:
			process_call(ir, mx);
			break;
		case IR_GOTO:
			IS_TRUE0(ir == IR_BB_last_ir(readonly_bb));
			break;
		case IR_IGOTO:
			{
				IS_TRUE0(ir == IR_BB_last_ir(readonly_bb));
				MD_SET * t = m_mds_mgr->create();
				AA_CTX ic;
				infer_exp(IGOTO_vexp(ir), *t, &ic, mx);
				m_mds_mgr->free(t);
			}
			break;
		case IR_PHI:
			process_phi(ir, mx);
			break;
		case IR_REGION:
			process_region(ir, mx);
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			{
				IS_TRUE0(ir == IR_BB_last_ir(readonly_bb));
				MD_SET * t = m_mds_mgr->create();
				AA_CTX ic;
				infer_exp(BR_det(ir), *t, &ic, mx);
				m_mds_mgr->free(t);
			}
			break;
		case IR_RETURN:
			IS_TRUE0(ir == IR_BB_last_ir(readonly_bb));
			process_return(ir, mx);
			break;
		case IR_SWITCH:
			{
				IS_TRUE0(ir == IR_BB_last_ir(readonly_bb));
				MD_SET * t = m_mds_mgr->create();
				AA_CTX ic;
				infer_exp(SWITCH_vexp(ir), *t, &ic, mx);
				m_mds_mgr->free(t);
			}
			break;
		default: IS_TRUE(0, ("unsupported IR type"));
		} //end switch
	} //end for
}


bool IR_AA::verify_ir(IR * ir)
{
	switch (IR_type(ir)) {
	case IR_ID:
	case IR_LD:
	case IR_PR:
		IS_TRUE0(get_must_addr(ir));
		IS_TRUE0(get_may_addr(ir) == NULL);
		break;
	case IR_ST:
		IS_TRUE0(get_must_addr(ir));
		IS_TRUE0(get_may_addr(ir) == NULL);
		break;
	case IR_STPR:
		IS_TRUE0(get_must_addr(ir));
		IS_TRUE0(get_may_addr(ir) == NULL);
		break;
	case IR_ARRAY:
		IS_TRUE0(IR_parent(ir));
		if (IR_type(IR_parent(ir)) == IR_ARRAY) {
			//Compute the memory address and ONLY
			//record the top level ARRAY node's memory address.
			break;
		}
		//fallthrough
	case IR_ILD:
	case IR_IST:
		{
			MD const* mustaddr = get_must_addr(ir);
			MD_SET const* mayaddr = get_may_addr(ir);
			IS_TRUE0(mustaddr ||
					 (mayaddr && !mayaddr->is_empty()));
			IS_TRUE0((mustaddr != NULL) ^
					 (mayaddr && !mayaddr->is_empty()));
			if (mustaddr != NULL) {
				//PR's address can not be taken.
				IS_TRUE0(!MD_is_pr(mustaddr));
			}
			if (mayaddr != NULL && !mayaddr->is_empty()) {
				//PR's address can not be taken.
				for (INT i = mayaddr->get_first();
					 i >= 0; i = mayaddr->get_next(i)) {
					MD const* x = m_md_sys->get_md(i);
					IS_TRUE0(x);
					IS_TRUE0(!MD_is_pr(x));
				}
			}
		}
		break;
	case IR_CALL:
	case IR_ICALL:
		if (ir->has_return_val()) {
			IS_TRUE0(get_must_addr(ir));
		}

		for (IR * p = CALL_param_list(ir); p != NULL; p = IR_next(p)) {
			verify_ir(p);
		}
		break;
	case IR_PHI:
		IS_TRUE0(get_must_addr(ir));
		IS_TRUE0(get_may_addr(ir) == NULL);
		break;
	default:
		IS_TRUE0(get_must_addr(ir) == NULL);
		IS_TRUE0(get_may_addr(ir) == NULL);
	}
	for (INT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid != NULL) {
			verify_ir(kid);
		}
	}
	return true;
}


bool IR_AA::verify()
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head();
		 bb != NULL; bb = bbl->get_next()) {
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			verify_ir(ir);
		}
	}
	return true;
}


/* This method is accurate than Andersen's algo.
NOTICE: Do NOT clean 'md2mds' of BB at the last iter,
it supplied the POINT TO information for subsequently
optimizations. */
void IR_AA::compute_flowsenpt(LIST<IR_BB*> const& bbl)
{
	bool change = true;
	UINT count = 0;
	PT_PAIR_SET tmp;
	while (change && count < 20) {
		count++;
		change = false;
		C<IR_BB*> * ct = NULL;
		for (IR_BB const* bb = bbl.get_head(&ct);
			 bb != NULL; bb = bbl.get_next(&ct)) {
			PT_PAIR_SET * pps = get_in_pp_set(bb);

			MD2MDS * md2mds = alloc_md2mds_for_bb(bb);
			tmp.clean();
			VERTEX * vex = m_cfg->get_vertex(IR_BB_id(bb));
			EDGE_C * el = VERTEX_in_list(vex);
			if (el != NULL) {
				while (el != NULL) {
					IR_BB * p = m_cfg->get_bb(VERTEX_id(EDGE_from(EC_edge(el))));
					IS_TRUE0(p);
					tmp.bunion(*get_out_pp_set(p));
					el = EC_next(el);
				}

				if (!pps->is_equal(tmp)) {
					pps->copy(tmp);
					change = true;
				}

				/* Regard 'mx' as a reservation table to hold
				the tmp info for MD->MD_SET during each iteration.
				Note that we must preserve MD->MD_SET at the last
				iteration. And it will be used during computing POINT-TO
				info and DU analysis. */
				md2mds->clean();
				pt2md2mds(*pps, m_pt_pair_mgr, md2mds);
			}
			compute_stmt_pt(bb, md2mds);

			tmp.clean();
			md2mds2pt(tmp, m_pt_pair_mgr, md2mds);

			#ifdef _DEBUG_
			//MD2MDS x;
			//pt2md2mds(*out_set, m_pt_pair_mgr, &x);
			//dump_md2mds(&x, false);
			#endif
			pps = get_out_pp_set(bb);
			if (!pps->is_equal(tmp)) {
				pps->copy(tmp);
				change = true;
			} //end if
		} //end for
	} //end while
	IS_TRUE0(!change);
}


/* Initialize the POINT-TO set of global pointer and formal parameter pointer.
'param': formal parameter.

Note may point to set must be available before call this function.*/
void IR_AA::init_vpts(VAR * param, MD2MDS * mx, MD_ITER & iter)
{
	MD * dmd = NULL; //dedicated MD which param pointed to.
	if (VAR_is_restrict(param)) {
		dmd = m_varid2md.get(VAR_id(param));
		if (dmd == NULL) {
			CHAR name[64];
			sprintf(name, "pt_var_by_var%d", VAR_id(param));
			VAR * tv = m_ru->get_var_mgr()->register_var(name, D_MC,
										D_UNDEF, 0, 0, 0,
										VAR_GLOBAL|VAR_ADDR_TAKEN);

			/* Set the var to be unallocable, means do NOT add
			var immediately as a memory-variable.
			For now, it is only be regarded as a pseduo-register.
			And set it to allocable if the PR is in essence need to be
			allocated in memory. */
			VAR_allocable(tv) = false;
			m_ru->add_to_var_tab(tv);

			MD md;
			MD_base(&md) = tv;
			MD_ofst(&md) = 0;
			MD_size(&md) = 16; //anysize you want.
			MD_ty(&md) = MD_EXACT;
			dmd = m_md_sys->register_md(md);
			IS_TRUE0(MD_id(dmd) > 0);
			m_varid2md.set(VAR_id(param), dmd);
		}
	}

	MD_TAB * mdt = m_md_sys->get_md_tab(param);
	if (mdt != NULL) {
		MD * x = mdt->get_effect_md();
		if (x != NULL) {
			if (dmd != NULL) {
				IS_TRUE(get_point_to(x, *mx) == NULL ||
						get_point_to(x, *mx)->is_empty(),
						("should already be clean"));
				set_point_to_set_add_md(x, *mx, dmd);
			} else {
				set_point_to(x, *mx, m_hashed_maypts);
			}
		}

		OFST_TAB * ofstab = mdt->get_ofst_tab();
		IS_TRUE0(ofstab);
		if (ofstab->get_elem_count() > 0) {
			iter.clean();
			for (MD * md = ofstab->get_first(iter, NULL);
				 md != NULL; md = ofstab->get_next(iter, NULL)) {
				if (dmd != NULL) {
					IS_TRUE(get_point_to(md, *mx) == NULL ||
							get_point_to(md, *mx)->is_empty(),
							("should already be clean"));
					set_point_to_set_add_md(md, *mx, dmd);
				} else {
					set_point_to(md, *mx, m_hashed_maypts);
				}
			}
		}
		return;
	}

	MD md;
	MD_base(&md) = param;
	MD_ofst(&md) = 0;
	MD_size(&md) = VAR_data_size(param);
	MD_ty(&md) = MD_EXACT;
	MD * entry = m_md_sys->register_md(md);
	if (dmd != NULL) {
		IS_TRUE(get_point_to(entry, *mx) == NULL ||
				get_point_to(entry, *mx)->is_empty(),
				("should already be clean"));

		set_point_to_set_add_md(entry, *mx, dmd);
	} else {
		set_point_to(entry, *mx, m_hashed_maypts);
	}
}


/* Initialize POINT_TO set for input MD at the entry of REGION.
e.g:char * q;
	void f(int * p)
	{
		*q='c';
		*p=0;
	}
	where p and q are entry MD.
ptset_arr: used to record all the PT_PAIR set. */
void IR_AA::init_entry_ptset(PT_PAIR_SET ** ptset_arr)
{
	MD_ITER iter;
	VAR_TAB * vt = m_ru->get_var_tab();
	if (m_flow_sensitive) {
		IS_TRUE0(m_cfg->verify());

		IS_TRUE0(m_ru->get_bb_list()->get_elem_count() != 0);

		IR_BB_LIST * rubblst = m_ru->get_bb_list();
		PT_PAIR_SET * loc_ptset_arr =
			new PT_PAIR_SET[rubblst->get_elem_count() * 2]();

		IS_TRUE0(ptset_arr);

		*ptset_arr = loc_ptset_arr;

		//Make vector to accommodate the max bb id.
		m_in_pp_set.set(rubblst->get_elem_count(), NULL);
		m_out_pp_set.set(rubblst->get_elem_count(), NULL);
		m_md2mds_vec.set(rubblst->get_elem_count(), NULL);

		UINT i = 0;
		C<IR_BB*> * ct;
		for (IR_BB * bb = rubblst->get_head(&ct);
			 bb != NULL; bb = rubblst->get_next(&ct)) {
			m_in_pp_set.set(IR_BB_id(bb), &loc_ptset_arr[i]);
			i++;
			m_out_pp_set.set(IR_BB_id(bb), &loc_ptset_arr[i]);
			i++;
		}

		LIST<IR_BB*> * bbl = m_cfg->get_entry_list();
		for (IR_BB * bb = bbl->get_head(&ct);
			 bb != NULL; bb = bbl->get_next(&ct)) {
			MD2MDS * mx = alloc_md2mds_for_bb(bb);
			set_pt_all_mem(m_md_sys->get_md(MD_ALL_MEM), *mx);
			set_pt_global_mem(m_md_sys->get_md(MD_GLOBAL_MEM), *mx);
			VTAB_ITER c;
			for (VAR * v = vt->get_first(c); v != NULL; v = vt->get_next(c)) {
				if (!VAR_is_global(v) && !VAR_is_formal_param(v)) { continue; }
				if (!VAR_is_pointer(v)) { continue; }
				init_vpts(v, mx, iter);
			} //end for each VAR

			md2mds2pt(*get_in_pp_set(bb), m_pt_pair_mgr, mx);
		} //end for each BB
	} else {
		set_pt_all_mem(m_md_sys->get_md(MD_ALL_MEM), m_unique_md2mds);
		set_pt_global_mem(m_md_sys->get_md(MD_GLOBAL_MEM), m_unique_md2mds);
		VTAB_ITER c;
		for (VAR * v = vt->get_first(c); v != NULL; v = vt->get_next(c)) {
			if (!VAR_is_global(v) && !VAR_is_formal_param(v)) { continue; }
			if (!VAR_is_pointer(v)) { continue; }
			init_vpts(v, &m_unique_md2mds, iter);
		} //end for each VAR
	}
}


//This function initialize May Point-To set.
//Note that this function should only be invoked once.
void IR_AA::init_may_ptset()
{
	//Record MDs whose address have been takens or it is global variable.
	REGION * ru = m_ru;
	REGION_MGR * rm = m_ru->get_ru_mgr();
	VTAB_ITER c;
	MD_ITER iter;
	MD_SET tmp;
	for (;;) {
		VAR_TAB * vtab = ru->get_var_tab();
		c.clean();
		for (VAR * v = vtab->get_first(c); v != NULL; v = vtab->get_next(c)) {
			if (!VAR_is_addr_taken(v)) { continue; }

			//Handle dedicated string md which has been taken address.
			MD const* strmd;
			if (VAR_is_str(v) &&
				(strmd = rm->get_dedicated_str_md()) != NULL) {
				tmp.bunion_pure(MD_id(strmd));
				continue;
			}

			//General md.
			MD_TAB * mdt = m_md_sys->get_md_tab(v);
			IS_TRUE0(mdt != NULL);

			MD * x = mdt->get_effect_md();
			if (x != NULL) {
				IS_TRUE0(MD_ty(x) == MD_UNBOUND);
				tmp.bunion(x);
			}

			/* OFST_TAB * ofstab = mdt->get_ofst_tab();
			IS_TRUE0(ofstab);
			if (ofstab->get_elem_count() > 0) {
				iter.clean();
				for (MD * md = ofstab->get_first(iter, NULL);
					 md != NULL; md = ofstab->get_next(iter, NULL)) {
					IS_TRUE0(md->is_effect());
					m_hashed_maypts->bunion(md);
				}
			} */
		}

		if (RU_type(ru) == RU_FUNC || RU_type(ru) == RU_PROGRAM) {
			break;
		} else {
			IS_TRUE0(RU_type(ru) == RU_SUB || RU_type(ru) == RU_EH);
		}

		ru = RU_parent(ru);
		if (ru == NULL) {
			break;
		}
	}
	tmp.bunion(MD_GLOBAL_MEM);
	m_hashed_maypts = m_mds_hash->append(tmp);
}


void IR_AA::compute_flowinsenpt()
{
	IR_BB_LIST * bbl = m_cfg->get_bb_list();
	C<IR_BB*> * ct = NULL;
	UINT c = 0;
	while (++c < 3) {
		for (IR_BB const* bb = bbl->get_head(&ct);
			 bb != NULL; bb = bbl->get_next(&ct)) {
			compute_stmt_pt(bb, &m_unique_md2mds);
		}
	}
}


//Calculate point-to set.
bool IR_AA::perform(IN OUT OPT_CTX & oc)
{
	IS_TRUE(m_hashed_maypts, ("Not initialize may point-to set."));

	START_TIMER_AFTER();

	if (m_ru->get_bb_list()->get_elem_count() == 0) { return true; }

	if (OPTC_pass_mgr(oc) != NULL) {
		IR_SSA_MGR * ssamgr =
			(IR_SSA_MGR*)OPTC_pass_mgr(oc)->query_opt(OPT_SSA_MGR);
		if (ssamgr != NULL) {
			set_ssa_available(ssamgr->is_ssa_construct());
		} else {
			set_ssa_available(false);
		}
	} else {
		set_ssa_available(false);
	}

	//Initialization.
	m_pt_pair_mgr.init();
	clean();
	m_is_visit.clean();

	PT_PAIR_SET * ptset_arr = NULL;
	if (m_flow_sensitive) {
		init_entry_ptset(&ptset_arr);

		m_ru->check_valid_and_recompute(&oc, OPT_RPO, OPT_UNDEF);

		LIST<IR_BB*> * tbbl = m_cfg->get_bblist_in_rpo();
		IS_TRUE0(tbbl->get_elem_count() ==
				 m_ru->get_bb_list()->get_elem_count());

		compute_flowsenpt(*tbbl);
	} else {
		init_entry_ptset(NULL);
		compute_flowinsenpt();
	}

	OPTC_is_aa_valid(oc) = true;

	#ifdef _DEBUG_
	UINT mds_count = m_mds_mgr->get_mdset_count();
	UINT free_mds_count = m_mds_mgr->get_free_mdset_count();
	UINT mds_in_hash = m_mds_hash->get_elem_count();
	IS_TRUE(mds_count == free_mds_count + mds_in_hash,
			("there are MD_SETs leaked."));
	#endif

	if (ptset_arr != NULL) {
		//PT_PAIR information will be unavailable.
		delete [] ptset_arr;
	}

#if 0
	//m_md_sys->dump_all_mds();
	//dump_all_md2mds();
	//dump_bb_in_out_pt_set();
	dump_all_ir_point_to(true);
#endif

	IS_TRUE0(verify());

	//DU info does not depend on these data structure.
	m_pt_pair_mgr.clobber();
	END_TIMER_AFTER(get_opt_name());
	return true;
}
