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
#include "ir_gvn.h"

//
//START IR_GVN
//
typedef SVECTOR<SVECTOR<SVECTOR<SVECTOR<VN*>*>*>*> VEC4;
typedef SVECTOR<SVECTOR<SVECTOR<VN*>*>*> VEC3;
typedef SVECTOR<SVECTOR<VN*>*> VEC2;
typedef SVECTOR<VN*> VEC1;

IR_GVN::IR_GVN(REGION * ru)
{
	IS_TRUE0(ru != NULL);
	m_ru = ru;
	m_md_sys = m_ru->get_md_sys();
	m_du = m_ru->get_du_mgr();
	m_dm = m_ru->get_dm();
	m_cfg = m_ru->get_cfg();
	m_vn_count = 1;
	m_is_vn_fp = false;
	m_is_valid = false;
	m_is_comp_ild_vn_by_du = true;
	m_is_comp_lda_string = false;
	m_zero_vn = NULL;

	LIST<IR_BB*> * bbl = ru->get_bb_list();
	UINT n = 0;
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		n += IR_BB_ir_num(bb);
	}
	m_stmt2domdef.init(MAX(4, get_nearest_power_of_2(n/2)));
	m_pool = smpool_create_handle(sizeof(VN) * 4, MEM_COMM);
}


IR_GVN::~IR_GVN()
{
	for (SVECTOR<VN*> * v = m_vec_lst.get_head();
		 v != NULL; v = m_vec_lst.get_next()) {
		delete v;
	}
	smpool_free_handle(m_pool);
}


void IR_GVN::clean()
{
	m_vn_count = 1;
	if (m_md2vn.get_bucket_size() != 0) {
		m_md2vn.clean();
	}
	if (m_ll2vn.get_bucket_size() != 0) {
		m_ll2vn.clean();
	}
	if (m_fp2vn.get_bucket_size() != 0) {
		m_fp2vn.clean();
	}
	if (m_str2vn.get_bucket_size() != 0) {
		m_str2vn.clean();
	}
	BITSET inlst;
	for (INT k = 0; k <= m_ir2vn.get_last_idx(); k++) {
		VN * x = m_ir2vn.get(k);
		if (x != NULL && !inlst.is_contain(VN_id(x))) {
			inlst.bunion(VN_id(x));
			m_free_lst.append_tail(x);
		}
	}
	m_ir2vn.clean();
	for (SVECTOR<VN*> * v = m_vnvec_lst.get_head();
		 v != NULL; v = m_vnvec_lst.get_next()) {
		v->clean();
	}

	m_def2ildtab.clean();
	m_def2arrtab.clean();
	m_def2sctab.clean();
	m_stmt2domdef.clean();
	m_zero_vn = NULL;
}


bool IR_GVN::verify()
{
	for (INT i = 0; i <= m_irt_vec.get_last_idx(); i++) {
		if (is_bin_irt((IR_TYPE)i) || i == IR_LDA) {
			SVECTOR<SVECTOR<VN*>*> * v0_vec = m_irt_vec.get(i);
			if (v0_vec == NULL) { continue; }
			for (INT j = 0; j <= v0_vec->get_last_idx(); j++) {
				SVECTOR<VN*> * v1_vec = v0_vec->get(j);
				if (v1_vec == NULL) { continue; }
				//v1_vec->clean();
			}
		} else if (i == IR_BNOT || i == IR_LNOT ||
				   i == IR_NEG || i == IR_CVT) {
			SVECTOR<VN*> * v0_vec = (SVECTOR<VN*>*)m_irt_vec.get(i);
			if (v0_vec == NULL) { continue; }
			//v0_vec->clean();
		} else if (is_triple((IR_TYPE)i)) {
			VEC3 * v0_vec = (VEC3*)m_irt_vec.get(i);
			if (v0_vec == NULL) { continue; }
			for (INT j = 0; j <= v0_vec->get_last_idx(); j++) {
				VEC2 * v1_vec = v0_vec->get(j);
				if (v1_vec == NULL) { continue; }
				for (INT k = 0; k <= v1_vec->get_last_idx(); k++) {
					VEC1 * v2_vec = v1_vec->get(k);
					if (v1_vec == NULL) { continue; }
					//v2_vec->clean();
				}
			}
		} else if (is_quad((IR_TYPE)i)) {
			//
		} else {
			IS_TRUE0(m_irt_vec.get(i) == NULL);
		}
	}
	return true;
}


VN * IR_GVN::register_md_vn(MD const* md)
{
	if (m_md2vn.get_bucket_size() == 0) {
		m_md2vn.init(10/*TO reevaluate*/);
	}
	VN * x = m_md2vn.get(md);
	if (x == NULL) {
		x = new_vn();
		VN_type(x) = VN_VAR;
		m_md2vn.set(md, x);
	}
	return x;
}


VN * IR_GVN::register_int_vn(LONGLONG v)
{
	if (v == 0) {
		if (m_zero_vn == NULL) {
			m_zero_vn = new_vn();
			VN_type(m_zero_vn) = VN_INT;
			VN_int_val(m_zero_vn) = 0;
		}
		return m_zero_vn;
	}
	if (m_ll2vn.get_bucket_size() == 0) {
		m_ll2vn.init(10/*TO reevaluate*/);
	}
	VN * vn = m_ll2vn.get(v);
	if (vn != NULL) {
		return vn;
	}
	vn = new_vn();
	VN_type(vn) = VN_INT;
	VN_int_val(vn) = v;
	m_ll2vn.set(v, vn);
	return vn;
}


VN * IR_GVN::register_str_vn(SYM * v)
{
	if (m_str2vn.get_bucket_size() == 0) {
		m_str2vn.init(16/*TO reevaluate*/);
	}
	VN * vn = m_str2vn.get(v);
	if (vn != NULL) {
		return vn;
	}
	vn = new_vn();
	VN_type(vn) = VN_STR;
	VN_str_val(vn) = v;
	m_str2vn.set(v, vn);
	return vn;
}


VN * IR_GVN::register_fp_vn(double v)
{
	if (m_fp2vn.get_bucket_size() == 0) {
		m_fp2vn.init(10/*TO reevaluate*/);
	}
	VN * vn = m_fp2vn.get(v);
	if (vn != NULL) {
		return vn;
	}
	vn = new_vn();
	VN_type(vn) = VN_FP;
	VN_fp_val(vn) = v;
	m_fp2vn.set(v, vn);
	return vn;
}


VN * IR_GVN::register_uni_vn(IR_TYPE irt, VN const* v0)
{
	SVECTOR<VN*> * v1_vec = (SVECTOR<VN*>*)m_irt_vec.get(irt);
	if (v1_vec == NULL) {
		v1_vec = new SVECTOR<VN*>();
		m_vec_lst.append_tail(v1_vec);
		m_vnvec_lst.append_tail(v1_vec);
		m_irt_vec.set(irt, (SVECTOR<SVECTOR<VN*>*>*)v1_vec);
	}
	VN * res = v1_vec->get(VN_id(v0));
	if (res == NULL) {
		res = new_vn();
		VN_type(res) = VN_OP;
		VN_op(res) = irt;
		v1_vec->set(VN_id(v0), res);
	}
	return res;
}


VN * IR_GVN::register_bin_vn(IR_TYPE irt, VN const* v0, VN const* v1)
{
	IS_TRUE0(v0 && v1);
	if (is_commutative(irt) && (VN_id(v0) > VN_id(v1))) {
		return register_bin_vn(irt, v1, v0);
	} else if (irt == IR_GT) {
		return register_bin_vn(IR_LT, v1, v0);
	} else if (irt == IR_GE) {
		return register_bin_vn(IR_LE, v1, v0);
	}
	VEC2 * v0_vec = m_irt_vec.get(irt);
	if (v0_vec == NULL) {
		v0_vec = new VEC2();
		m_vec_lst.append_tail((SVECTOR<VN*>*)v0_vec);
		m_irt_vec.set(irt, v0_vec);
	}

	VEC1 * v1_vec = v0_vec->get(VN_id(v0));
	if (v1_vec == NULL) {
		v1_vec = new VEC1();
		m_vec_lst.append_tail((SVECTOR<VN*>*)v1_vec);
		m_vnvec_lst.append_tail(v1_vec);
		v0_vec->set(VN_id(v0), v1_vec);
	}

	VN * res = v1_vec->get(VN_id(v1));
	if (res == NULL) {
		res = new_vn();
		VN_type(res) = VN_OP;
		VN_op(res) = irt;
		v1_vec->set(VN_id(v1), res);
	}
	return res;
}


VN * IR_GVN::register_tri_vn(IR_TYPE irt, VN const* v0, VN const* v1,
							 VN const* v2)
{
	IS_TRUE0(v0 && v1 && v2);
	IS_TRUE0(is_triple(irt));
	VEC3 * v0_vec = (VEC3*)m_irt_vec.get(irt);
	if (v0_vec == NULL) {
		v0_vec = new VEC3();
		m_vec_lst.append_tail((VEC1*)v0_vec);
		m_irt_vec.set(irt, (VEC2*)v0_vec);
	}

	VEC2 * v1_vec = v0_vec->get(VN_id(v0));
	if (v1_vec == NULL) {
		v1_vec = new VEC2();
		m_vec_lst.append_tail((VEC1*)v1_vec);
		v0_vec->set(VN_id(v0), v1_vec);
	}

	VEC1 * v2_vec = v1_vec->get(VN_id(v1));
	if (v2_vec == NULL) {
		v2_vec = new VEC1();
		m_vec_lst.append_tail((VEC1*)v2_vec);
		m_vnvec_lst.append_tail(v2_vec);
		v1_vec->set(VN_id(v1), v2_vec);
	}

	VN * res = v2_vec->get(VN_id(v2));
	if (res == NULL) {
		res = new_vn();
		VN_type(res) = VN_OP;
		VN_op(res) = irt;
		v2_vec->set(VN_id(v2), res);
	}
	return res;
}


VN * IR_GVN::register_qua_vn(IR_TYPE irt, VN const* v0, VN const* v1,
							 VN const* v2, VN const* v3)
{
	IS_TRUE0(v0 && v1 && v2 && v3);
	IS_TRUE0(is_quad(irt));
	VEC4 * v0_vec = (VEC4*)m_irt_vec.get(irt);
	if (v0_vec == NULL) {
		v0_vec = new VEC4();
		m_vec_lst.append_tail((VEC1*)v0_vec);
		m_irt_vec.set(irt, (VEC2*)v0_vec);
	}

	VEC3 * v1_vec = v0_vec->get(VN_id(v0));
	if (v1_vec == NULL) {
		v1_vec = new VEC3();
		m_vec_lst.append_tail((VEC1*)v1_vec);
		v0_vec->set(VN_id(v0), v1_vec);
	}

	VEC2 * v2_vec = v1_vec->get(VN_id(v1));
	if (v2_vec == NULL) {
		v2_vec = new VEC2();
		m_vec_lst.append_tail((VEC1*)v2_vec);
		v1_vec->set(VN_id(v1), v2_vec);
	}

	VEC1 * v3_vec = v2_vec->get(VN_id(v2));
	if (v3_vec == NULL) {
		v3_vec = new VEC1();
		m_vec_lst.append_tail(v3_vec);
		m_vnvec_lst.append_tail(v3_vec);
		v2_vec->set(VN_id(v2), v3_vec);
	}

	VN * res = v3_vec->get(VN_id(v3));
	if (res == NULL) {
		res = new_vn();
		VN_type(res) = VN_OP;
		VN_op(res) = irt;
		v3_vec->set(VN_id(v3), res);
	}
	return res;
}


/*
Memory location may be parameter or global variable.
'emd': exact md
*/
VN * IR_GVN::alloc_livein_vn(IR const* exp, MD const* emd, bool & change)
{
	VN * x = m_ir2vn.get(IR_id(exp));
	if (x == NULL) {
		x = register_md_vn(emd);
		change = true;
		m_ir2vn.set(IR_id(exp), x);
	}
	return x;
}


//Only compute memory operation's vn.
VN * IR_GVN::comp_mem(IR const* exp, bool & change)
{
	IS_TRUE0(exp->is_memory_opnd());
	MD const* emd = exp->get_exact_ref();
	if (emd == NULL) { return NULL; }

	IR const* ed = m_du->get_exact_and_unique_def(exp);
	if (ed == NULL) {
		DU_SET const* defset = m_du->get_du_c(exp);
		if (defset == NULL) {
			return alloc_livein_vn(exp, emd, change);
		} else {
			//Check if some may-def or overlapped-def disrupts the emd.
			//Omit the DEF which has effect md and it does not overlapped
			//with emd.
			DU_ITER di;
			UINT defcount = 0;
			for (INT i = defset->get_first(&di);
				 i >= 0; i = defset->get_next(i, &di), defcount++) {
				IR const* dir = m_ru->get_ir(i);
				IS_TRUE0(dir->is_stmt());
				MD const* xd = m_du->get_must_def(dir);
				if (xd == NULL) {
					MD_SET const* xds = m_du->get_may_def(dir);
					if (xds != NULL && xds->is_contain(emd)) {
						IS_TRUE0(m_ir2vn.get(IR_id(exp)) == NULL);
						//exp's value is may defined, here we can not
						//determine if exp have an individual VN.
						return NULL;
					}
				} else {
					if (xd == emd || xd->is_overlap(emd)) {
						IS_TRUE0(m_ir2vn.get(IR_id(exp)) == NULL);
						//exp's value is defined by nonkilling define,
						//here we can not determine if exp have an
						//individual VN.
						return NULL;
					}
				}
			}

			if (defcount == 0) {
				return alloc_livein_vn(exp, emd, change);
			}
		}
		return alloc_livein_vn(exp, emd, change);
	}

	VN * defvn = NULL;
	IR const* exp_stmt = exp->get_stmt();
	IS_TRUE0(exp_stmt->is_stmt());
	IR_BB * b1 = ed->get_bb();
	IR_BB * b2 = exp_stmt->get_bb();
	IS_TRUE0(b1 && b2);
	if ((b1 != b2 && m_cfg->is_dom(IR_BB_id(b1), IR_BB_id(b2))) ||
		(b1 == b2 && b1->is_dom(ed, exp_stmt, true))) {
		defvn = m_ir2vn.get(IR_id(ed));
	}

	VN * ux = m_ir2vn.get(IR_id(exp));
	if (defvn != ux) {
		m_ir2vn.set(IR_id(exp), defvn);
		change = true;
	}
	return defvn;
}


//Compute VN for ild according to anonymous domdef.
VN * IR_GVN::comp_ild_by_anon_domdef(IR const* ild, VN const* mlvn,
									 IR const* domdef, bool & change)
{
	IS_TRUE0(IR_type(ild) == IR_ILD && m_du->is_may_def(domdef, ild, false));
	ILD_VNE2VN * vnexp_map = m_def2ildtab.get(domdef);
	UINT dtsz = ild->get_dt_size(m_dm);
	VNE_ILD vexp(VN_id(mlvn), ILD_ofst(ild), dtsz);
	/*
	NOTE:
		foo();
		ild(v1); //s1
		goo();
		ild(v1); //s2
		vn of s1 should not same with s2.
	*/
	if (vnexp_map == NULL) {
		vnexp_map = new ILD_VNE2VN(m_pool, 16); //bsize to be evaluate.
		m_def2ildtab.set(domdef, vnexp_map);
	}
	VN * ildvn = vnexp_map->get(&vexp);
	if (ildvn == NULL) {
		ildvn = new_vn();
		VN_type(ildvn) = VN_OP;
		VN_op(ildvn) = IR_ILD;
		vnexp_map->setv((OBJTY)&vexp, ildvn);
	}
	m_ir2vn.set(IR_id(ild), ildvn);
	change = true;
	return ildvn;
}


VN * IR_GVN::comp_ild(IR const* exp, bool & change)
{
	IS_TRUE0(IR_type(exp) == IR_ILD);
	VN * mlvn = comp_vn(ILD_base(exp), change);
	if (mlvn == NULL) {
		IS_TRUE0(m_ir2vn.get(IR_id(exp)) == NULL);
		IS_TRUE0(m_ir2vn.get(IR_id(ILD_base(exp))) == NULL);
		return NULL;
	}

	VN * evn = m_ir2vn.get(IR_id(exp));
	if (evn != NULL) { return evn; }

	evn = comp_mem(exp, change);
	if (evn != NULL) { return evn; }

	DU_SET const* defset = m_du->get_du_c(exp);
	if (defset == NULL || defset->get_elem_count() == 0) {
		VN * v = register_tri_vn(IR_ILD, mlvn,
								 register_int_vn(ILD_ofst(exp)),
								 register_int_vn(exp->get_dt_size(m_dm)));
		m_ir2vn.set(IR_id(exp), v);
		return v;
	}

	IR const* exp_stmt = const_cast<IR*>(exp)->get_stmt();
	IR const* domdef = m_stmt2domdef.get(exp_stmt);
	if (domdef == NULL) {
		domdef = m_du->find_dom_def(exp, exp_stmt, defset, false);
		if (domdef != NULL) {
			m_stmt2domdef.set(exp_stmt, domdef);
		}
	}
	if (domdef == NULL) { return NULL; }

	/*
	//ofst will be distinguished in comp_ild_by_anon_domdef(), so
	//we do not need to differentiate the various offset of ild and ist.
	if (IR_type(domdef) == IR_IST && !domdef->is_st_array() &&
		IST_ofst(domdef) != ILD_ofst(exp)) {
		return NULL;
	}
	*/

	if (IR_type(domdef) != IR_IST || domdef->is_st_array() ||
		IST_ofst(domdef) != ILD_ofst(exp)) {
		return comp_ild_by_anon_domdef(exp, mlvn, domdef, change);
	}

	//domdef is ist and the offset is matched.
	//Check if IR expression is match.
	VN const* mcvn = m_ir2vn.get(IR_id(IST_base(domdef)));
	if (mcvn == NULL || mcvn != mlvn) {
		return NULL;
	}
	VN * uni_vn = m_ir2vn.get(IR_id(domdef));
	if (uni_vn == NULL) {
		uni_vn = register_tri_vn(IR_ILD, mlvn,
						register_int_vn(ILD_ofst(exp)),
						register_int_vn(exp->get_dt_size(m_dm)));
		m_ir2vn.set(IR_id(domdef), uni_vn);
	}
	m_ir2vn.set(IR_id(exp), uni_vn);
	change = true;
	return uni_vn;
}


void IR_GVN::comp_array_addr_ref(IR const* ir, bool & change)
{
	IS_TRUE0(ir->is_st_array());
	IR const* arr = IST_base(ir);
	comp_vn(ARR_base(arr), change);

	for (IR const* s = ARR_sub_list(arr); s != NULL; s = IR_next(s)) {
		comp_vn(s, change);
	}
}


//Compute VN for array according to anonymous domdef.
VN * IR_GVN::comp_array_by_anon_domdef(IR const* arr, VN const* basevn,
									   VN const* ofstvn, IR const* domdef,
									   bool & change)
{
	IS_TRUE0(IR_type(arr) == IR_ARRAY && m_du->is_may_def(domdef, arr, false));
	ARR_VNE2VN * vnexp_map = m_def2arrtab.get(domdef);
	UINT dtsz = arr->get_dt_size(m_dm);
	VNE_ARR vexp(VN_id(basevn), VN_id(ofstvn), ARR_ofst(arr), dtsz);
	/* NOTE:
		foo();
		array(v1); //s1
		goo();
		array(v1); //s2
		vn of s1 should not same with s2. */
	if (vnexp_map == NULL) {
		vnexp_map = new ARR_VNE2VN(m_pool, 16); //bsize to be evaluate.
		m_def2arrtab.set(domdef, vnexp_map);
	}
	VN * vn = vnexp_map->get(&vexp);
	if (vn == NULL) {
		vn = new_vn();
		VN_type(vn) = VN_OP;
		VN_op(vn) = IR_ARRAY;
		vnexp_map->setv((OBJTY)&vexp, vn);
	}
	m_ir2vn.set(IR_id(arr), vn);
	change = true;
	return vn;
}


VN * IR_GVN::comp_array(IR const* exp, bool & change)
{
	IS_TRUE0(IR_type(exp) == IR_ARRAY);
	for (IR const* s = ARR_sub_list(exp); s != NULL; s = IR_next(s)) {
		comp_vn(s, change);
	}
	VN * evn = m_ir2vn.get(IR_id(exp));
	if (evn != NULL) { return evn; }

	evn = comp_mem(exp, change);
	if (evn != NULL) {
		return evn;
	}

	VN const* abase_vn = NULL;
	VN const* aofst_vn = NULL;
	if (((CARRAY*)exp)->get_dimn() == 1) {
		//only handle one dim array.
		abase_vn = comp_vn(ARR_base(exp), change);
		if (abase_vn == NULL) {
			return NULL;
		}
		aofst_vn = m_ir2vn.get(IR_id(ARR_sub_list(exp)));
		if (aofst_vn == NULL) {
			return NULL;
		}
	} else {
		return NULL;
	}

	DU_SET const* du = m_du->get_du_c(exp);
	if (du == NULL || du->get_elem_count() == 0) {
		//Array does not have any DEF.
		VN * x = register_qua_vn(IR_ARRAY, abase_vn, aofst_vn,
								 register_int_vn(ARR_ofst(exp)),
								 register_int_vn(exp->get_dt_size(m_dm)));
		if (m_ir2vn.get(IR_id(exp)) != x) {
			m_ir2vn.set(IR_id(exp), x);
			change = true;
		}
		return x;
	}

	IR const* exp_stmt = const_cast<IR*>(exp)->get_stmt();
	IS_TRUE0(exp_stmt->is_stmt());
	IR const* domdef = m_du->find_dom_def(exp, exp_stmt, du, false);
	if (domdef == NULL) {
		return NULL;
	}
	if (domdef->is_st_array() && IST_ofst(domdef) != ARR_ofst(exp)) {
		return NULL;
	}
	if (!domdef->is_st_array()) {
		return comp_array_by_anon_domdef(exp, abase_vn,
										 aofst_vn, domdef, change);
	}

	IS_TRUE0(IR_type(domdef) == IR_IST);
	//Check if VN expression is match.
	IR const* narr = IST_base(domdef);
	IS_TRUE(((CARRAY*)narr)->get_dimn() == 1, ("only handle one dim array."));

	VN const* b = m_ir2vn.get(IR_id(ARR_base(narr)));
	if (b == NULL || b != abase_vn) {
		return NULL;
	}
	VN const* o = m_ir2vn.get(IR_id(ARR_sub_list(narr)));
	if (o == NULL || o != aofst_vn) {
		return NULL;
	}

	VN * uni_vn = m_ir2vn.get(IR_id(domdef));
	if (uni_vn == NULL) {
		uni_vn = register_qua_vn(IR_ARRAY, abase_vn, aofst_vn,
								 register_int_vn(ARR_ofst(exp)),
								 register_int_vn(exp->get_dt_size(m_dm)));
		m_ir2vn.set(IR_id(domdef), uni_vn);
		m_ir2vn.set(IR_id(narr), uni_vn);
	}
	m_ir2vn.set(IR_id(exp), uni_vn);
	change = true;
	return uni_vn;
}


VN * IR_GVN::comp_sc_by_anon_domdef(IR const* exp, IR const* domdef,
									bool & change)
{
	IS_TRUE0((IR_type(exp) == IR_LD || IR_type(exp) == IR_PR) &&
			 m_du->is_may_def(domdef, exp, false));
	SCVNE2VN * vnexp_map = m_def2sctab.get(domdef);
	UINT dtsz = exp->get_dt_size(m_dm);
	MD const* md = exp->get_exact_ref();
	IS_TRUE0(md);
	VNE_SC vexp(MD_id(md), exp->get_ofst(), dtsz);
	/*
	NOTE:
		foo();
		v1; //s1
		goo();
		v1; //s2
		vn of s1 should not same with s2.
	*/
	if (vnexp_map == NULL) {
		vnexp_map = new SCVNE2VN(m_pool, 16); //bsize to be evaluate.
		m_def2sctab.set(domdef, vnexp_map);
	}
	VN * vn = vnexp_map->get(&vexp);
	if (vn == NULL) {
		vn = new_vn();
		VN_type(vn) = VN_VAR;
		vnexp_map->setv((OBJTY)&vexp, vn);
	}
	m_ir2vn.set(IR_id(exp), vn);
	change = true;
	return vn;
}


VN * IR_GVN::comp_sc(IR const* exp, bool & change)
{
	VN * evn = m_ir2vn.get(IR_id(exp));
	if (evn != NULL) { return evn; }

	evn = comp_mem(exp, change);
	if (evn != NULL) { return evn; }

	DU_SET const* du = m_du->get_du_c(exp);
	IS_TRUE(du, ("This situation should be catched in comp_mem()"));
	IS_TRUE0(du->get_elem_count() > 0);

	IR const* exp_stmt = const_cast<IR*>(exp)->get_stmt();
	IR const* domdef = m_du->find_dom_def(exp, exp_stmt, du, false);
	if (domdef == NULL) { return NULL; }
	if (domdef->is_stid() && ST_ofst(domdef) != exp->get_ofst()) {
		return NULL;
	}
	if (!domdef->is_stid() && !domdef->is_stpr()) {
		return comp_sc_by_anon_domdef(exp, domdef, change);
	}
	switch (IR_type(exp)) {
	case IR_LD:
		if (domdef->is_stpr() || (LD_idinfo(exp) != ST_idinfo(domdef))) {
			return NULL;
		}
		break;
	case IR_PR:
		if (domdef->is_stid() || PR_no(exp) != STPR_no(domdef)) {
			return NULL;
		}
		break;
	default: IS_TRUE(0, ("unsupport"));
	}

	VN * uni_vn = m_ir2vn.get(IR_id(domdef));
	if (uni_vn == NULL) {
		uni_vn = new_vn();
		VN_type(uni_vn) = VN_VAR;
		m_ir2vn.set(IR_id(domdef), uni_vn);
	}
	m_ir2vn.set(IR_id(exp), uni_vn);
	change = true;
	return uni_vn;
}


VN * IR_GVN::comp_vn(IR const* exp, bool & change)
{
	IS_TRUE0(exp);
	switch (IR_type(exp)) {
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND: //logical and &&
	case IR_LOR: //logical or ||
	case IR_BAND: //inclusive and &
	case IR_BOR: //inclusive or |
	case IR_XOR: //exclusive or
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ: //==
	case IR_NE: //!=
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		{
			VN * vn1 = comp_vn(BIN_opnd0(exp), change);
			VN * vn2 = comp_vn(BIN_opnd1(exp), change);
			if (vn1 == NULL || vn2 == NULL) {
				if (m_ir2vn.get(IR_id(exp)) != NULL) {
					m_ir2vn.set(IR_id(exp), NULL);
					change = true;
				}
				return NULL;
			}
			VN * x = register_bin_vn((IR_TYPE)IR_type(exp), vn1, vn2);
			if (m_ir2vn.get(IR_id(exp)) != x) {
				m_ir2vn.set(IR_id(exp), x);
				change = true;
			}
			return x;
		}
		break;
	case IR_BNOT: //bitwise not
	case IR_LNOT: //logical not
	case IR_NEG: //negative
		{
			VN * x = comp_vn(UNA_opnd0(exp), change);
			if (x == NULL) {
				if (m_ir2vn.get(IR_id(exp)) != NULL) {
					m_ir2vn.set(IR_id(exp), NULL);
					change = true;
				}
				return NULL;
			}
			x = register_uni_vn((IR_TYPE)IR_type(exp), x);
			if (m_ir2vn.get(IR_id(exp)) != x) {
				m_ir2vn.set(IR_id(exp), x);
				change = true;
			}
			return x;
		}
		break;
	case IR_CVT: //type convertion
		{
			VN * x = comp_vn(CVT_exp(exp), change);
			if (x == NULL) {
				if (m_ir2vn.get(IR_id(exp)) != NULL) {
					m_ir2vn.set(IR_id(exp), NULL);
					change = true;
				}
				return NULL;
			}
			x = register_uni_vn((IR_TYPE)IR_type(exp), x);
			if (m_ir2vn.get(IR_id(exp)) != x) {
				m_ir2vn.set(IR_id(exp), x);
				change = true;
			}
			return x;
		}
		break;
	case IR_LDA:
		{
			IR const* ldabase = LDA_base(exp);
			VN * basevn;
			if (IR_type(ldabase) == IR_ID) {
				MD const* emd = ldabase->get_exact_ref();
				if (emd == NULL) {
					//e.g: p = &"blabla", regard MD of "blabla" as inexact.
					IS_TRUE(ldabase->is_str(m_dm),
							("only string's MD can be inexact."));
					if (m_is_comp_lda_string) {
						emd = m_du->get_effect_use_md(ldabase);
						IS_TRUE(emd, ("string should have effect MD"));
						basevn = register_md_vn(emd);
					} else {
						basevn = NULL;
					}
				} else {
					IS_TRUE0(emd);
					basevn = register_md_vn(emd);
				}
			} else {
				basevn = comp_vn(LDA_base(exp), change);
			}
			if (basevn == NULL) {
				if (m_ir2vn.get(IR_id(exp)) != NULL) {
					m_ir2vn.set(IR_id(exp), NULL);
					change = true;
				}
				return NULL;
			}
			VN * ofstvn = register_int_vn(LDA_ofst(exp));
			VN * x = register_bin_vn(IR_LDA, basevn, ofstvn);
			if (m_ir2vn.get(IR_id(exp)) != x) {
				m_ir2vn.set(IR_id(exp), x);
				change = true;
			}
			return x;
		}
		break;
	//case IR_ID:
	case IR_LD:
	case IR_PR:
		return comp_sc(exp, change);
	case IR_ARRAY:
		return comp_array(exp, change);
	case IR_ILD:
		return comp_ild(exp, change);
	case IR_CONST:
		{
			VN * x = m_ir2vn.get(IR_id(exp));
			if (x != NULL) { return x; }
			if (exp->is_int(m_dm)) {
				x = register_int_vn(CONST_int_val(exp));
			} else if (exp->is_fp(m_dm)) {
				if (!m_is_vn_fp) {
					return NULL;
				}
				x = register_fp_vn(CONST_fp_val(exp));
			} else if (exp->is_str(m_dm)) {
				x = register_str_vn(CONST_str_val(exp));
			} else {
				IS_TRUE(0, ("unsupport const type"));
			}
			IS_TRUE0(x);
			m_ir2vn.set(IR_id(exp), x);
			change = true;
			return x;
		}
		break;
	default: break;
	}
	return NULL;
}


void IR_GVN::process_phi(IR const* ir, bool & change)
{
	VN * phivn = NULL;

	IR const* p = PHI_opnd_list(ir);
	if (p != NULL) {
		phivn = comp_vn(p, change);
		p = IR_next(p);
	}
	for (; p != NULL; p = IR_next(p)) {
		VN * opndvn = comp_vn(p, change);
		if (phivn != NULL && phivn != opndvn) {
			phivn = NULL;
		}
	}

	if (m_ir2vn.get(IR_id(ir)) != phivn) {
		IS_TRUE0(m_ir2vn.get(IR_id(ir)) == NULL);
		m_ir2vn.set(IR_id(ir), phivn);
		change = true;
	}
}


void IR_GVN::process_st(IR const* ir, bool & change)
{
	if (IR_type(ir) == IR_IST) {
		if (IR_type(IST_base(ir)) == IR_ARRAY) {
			comp_array_addr_ref(ir, change);
		} else {
			comp_vn(IST_base(ir), change);
		}
	}
	VN * x = comp_vn(ir->get_rhs(), change);
	if (x == NULL) { return; }

	//IST's vn may be set by its dominated use-stmt ILD.
	if (m_ir2vn.get(IR_id(ir)) != x) {
		IS_TRUE0(m_ir2vn.get(IR_id(ir)) == NULL);
		m_ir2vn.set(IR_id(ir), x);
		change = true;
	}
	return;
}


void IR_GVN::process_call(IR const* ir, bool & change)
{
	for (IR const* p = CALL_param_list(ir); p != NULL; p = IR_next(p)) {
		comp_vn(p, change);
	}

	VN * x = m_ir2vn.get(IR_id(ir));
	if (x == NULL) {
		x = new_vn();
		VN_type(x) = VN_VAR;
		change = true;
		m_ir2vn.set(IR_id(ir), x);
	}
	return;
}


void IR_GVN::process_region(IR const* ir, bool & change)
{
	IS_TRUE0(0); //TODO
}


void IR_GVN::process_bb(IR_BB * bb, bool & change)
{
	C<IR*> * ct;
	for (IR * ir = IR_BB_ir_list(bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&ct)) {
		switch (IR_type(ir)) {
		case IR_ST:
		case IR_STPR:
		case IR_IST:
			process_st(ir, change);
			break;
		case IR_CALL:
		case IR_ICALL:
			process_call(ir, change);
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			comp_vn(BR_det(ir), change);
			break;
		case IR_SWITCH:
			comp_vn(SWITCH_vexp(ir), change);
			break;
		case IR_IGOTO:
			comp_vn(IGOTO_vexp(ir), change);
			break;
		case IR_RETURN:
			comp_vn(RET_exp(ir), change);
			break;
		case IR_REGION:
			process_region(ir, change);
			break;
		case IR_PHI:
			process_phi(ir, change);
			break;
		case IR_GOTO: break;
		default: IS_TRUE0(0);
		}
	}
}


void IR_GVN::dump_ir2vn()
{
	if (g_tfile == NULL) return;
	for (INT k = 0; k <= m_ir2vn.get_last_idx(); k++) {
		VN * x = m_ir2vn.get(k);
		if (x != NULL) {
			fprintf(g_tfile,"\nIR%d : vn%d, %d", k, VN_id(x), (INT)VN_type(x));
		}
	}
	fflush(g_tfile);
}


void IR_GVN::dump_bb_labs(LIST<LABEL_INFO*> & lst)
{
	for (LABEL_INFO * li = lst.get_head(); li != NULL; li = lst.get_next()) {
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
}


void IR_GVN::dump_h1(IR const* k, VN * x)
{
	fprintf(g_tfile, "\n\t%s", IRTNAME(IR_type(k)));
	if (k->is_pr()) {
		fprintf(g_tfile, "%d", PR_no(k));
	}
	fprintf(g_tfile, " id:%d ", IR_id(k));
	if (x != NULL) {
		fprintf(g_tfile, "vn%d", VN_id(x));
	} else {
		fprintf(g_tfile, "--");
	}
}


void IR_GVN::dump_bb(UINT bbid)
{
	if (g_tfile == NULL) { return; }
	IR_BB * bb = m_ru->get_bb(bbid);
	IS_TRUE0(bb);

	CIR_ITER ii;
	fprintf(g_tfile, "\n-- BB%d ", IR_BB_id(bb));
	dump_bb_labs(IR_BB_lab_list(bb));
	fprintf(g_tfile, "\n");
	for (IR * ir = IR_BB_first_ir(bb);
		 ir != NULL; ir = IR_BB_next_ir(bb)) {
		dump_ir(ir, m_ru->get_dm());
		fprintf(g_tfile, "\n");
		VN * x = m_ir2vn.get(IR_id(ir));
		if (x != NULL) {
			fprintf(g_tfile, "vn%d", VN_id(x));
		}

		fprintf(g_tfile, " <- {");

		switch (IR_type(ir)) {
		case IR_ST:
			ii.clean();
			for (IR const* k = ir_iter_init_c(ST_rhs(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}
			break;
		case IR_STPR:
			ii.clean();
			for (IR const* k = ir_iter_init_c(STPR_rhs(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}
			break;
		case IR_IST:
			ii.clean();
			for (IR const* k = ir_iter_init_c(IST_rhs(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}

			ii.clean();
			for (IR const* k = ir_iter_init_c(IST_base(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}
			break;
		case IR_CALL:
		case IR_ICALL:
			{
				ii.clean();
				for (IR const* k = ir_iter_init_c(CALL_param_list(ir), ii);
					 k != NULL; k = ir_iter_next_c(ii)) {
					VN * x = m_ir2vn.get(IR_id(k));
					dump_h1(k, x);
				}
			}
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			ii.clean();
			for (IR const* k = ir_iter_init_c(BR_det(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}
			break;
		case IR_SWITCH:
			ii.clean();
			for (IR const* k = ir_iter_init_c(SWITCH_vexp(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}
			break;
		case IR_IGOTO:
			ii.clean();
			for (IR const* k = ir_iter_init_c(IGOTO_vexp(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}
			break;
		case IR_RETURN:
			ii.clean();
			for (IR const* k = ir_iter_init_c(RET_exp(ir), ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				VN * x = m_ir2vn.get(IR_id(k));
				dump_h1(k, x);
			}
			break;
		case IR_GOTO: break;
		case IR_REGION:
			IS_TRUE0(0); //TODO
			break;
		default: IS_TRUE0(0);
		}
		fprintf(g_tfile, " }");
	}
	fflush(g_tfile);
}


void IR_GVN::dump()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP GVN -- ru:'%s' ----==", m_ru->get_ru_name());
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		dump_bb(IR_BB_id(bb));
	}
	fflush(g_tfile);
}


//Return true if gvn is able to determine the result of 'ir'.
bool IR_GVN::calc_cond_must_val(IN IR const* ir,
								bool & must_true, bool & must_false)
{
	must_true = false;
	must_false = false;
	IS_TRUE0(ir->is_judge());
	VN const* v1 = m_ir2vn.get(IR_id(BIN_opnd0(ir)));
	VN const* v2 = m_ir2vn.get(IR_id(BIN_opnd1(ir)));
	if (v1 == NULL || v2 == NULL) return false;
	switch (IR_type(ir)) {
	case IR_LT:
	case IR_GT:
		if (v1 == v2) {
			must_true = false;
			must_false = true;
			return true;
		}
		break;
	case IR_LE:
	case IR_GE:
		if (v1 == v2) {
			must_true = true;
			must_false = false;
			return true;
		}
		break;
	case IR_NE:
		if (v1 == v2) {
			must_true = false;
			must_false = true;
			return true;
		}
		if (v1 != v2) {
			must_true = true;
			must_false = false;
			return true;
		}
		IS_TRUE0(0);
		break;
	case IR_EQ:
		if (v1 == v2) {
			must_true = true;
			must_false = false;
			return true;
		}
		if (v1 != v2) {
			must_true = false;
			must_false = true;
			return true;
		}
		IS_TRUE0(0);
		break;
	case IR_LAND:
	case IR_LOR:
	case IR_LNOT:
		break;
	default: IS_TRUE0(0);
	}
	return false;
}


bool IR_GVN::reperform(OPT_CTX & oc)
{
	clean();
	return perform(oc);
}


//GVN try to assign a value numbers to expressions.
bool IR_GVN::perform(OPT_CTX & oc)
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	if (bbl->get_elem_count() == 0) { return false; }

	START_TIMER_AFTER();
	m_ru->check_valid_and_recompute(&oc, OPT_DU_CHAIN, OPT_DU_REF, OPT_RPO,
									OPT_DOM, OPT_UNDEF);

	LIST<IR_BB*> * tbbl = m_cfg->get_bblist_in_rpo();
	IS_TRUE0(tbbl->get_elem_count() == bbl->get_elem_count());

	UINT count = 0;
	bool change = true;

	#ifdef DEBUG_GVN
	while (change && count < 10) {
		change = false;
	#endif
		for (IR_BB * bb = tbbl->get_head();
			 bb != NULL; bb = tbbl->get_next()) {
			process_bb(bb, change);
		} //end for BB
		count++;
	#ifdef DEBUG_GVN
	} //end while
	IS_TRUE0(!change && count <= 2);
	#endif

	//dump();
	END_TIMER_AFTER(get_opt_name());
	IS_TRUE0(verify());
	m_is_valid = true;
	return true;
}
//END IR_GVN
