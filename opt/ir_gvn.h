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
#ifndef _IR_GVN_H_
#define _IR_GVN_H_

typedef enum _VN_TYPE {
	VN_UNKNOWN = 0,
	VN_OP,
	VN_VAR,
	VN_INT,
	VN_FP,
	VN_STR,
} VN_TYPE;


#define VN_id(v)		((v)->id)
#define VN_type(v)		((v)->vn_type)
#define VN_int_val(v)	((v)->u1.iv)
#define VN_fp_val(v)	((v)->u1.dv)
#define VN_str_val(v)	((v)->u1.str)
#define VN_op(v)		((v)->u1.op)
#define VN_is_cst(v)	(VN_type(v) == VN_INT || \
						 VN_type(v) == VN_FP || \
						 VN_type(v) == VN_STR)
class VN {
public:
	UINT id;
	VN_TYPE vn_type; //value type
	union {
		LONGLONG iv;
		double dv;
		SYM * str;
		IR_TYPE op; //operation
	} u1;

	VN() { clean();	}

	void clean()
	{
		id = 0;
		vn_type = VN_UNKNOWN;
		u1.iv = 0;
	}
};


class DOUBLE_HF : public HASH_FUNC_BASE<double> {

public:
	UINT get_hash_value(double d, UINT bucket_size) const
	{
		double * p = &d;
		LONGLONG * pl = (LONGLONG*)p;
		return *pl % bucket_size;
	}
};

typedef HMAP<double, VN*, DOUBLE_HF> FP2VN_MAP;
typedef HMAP<LONGLONG, VN*> LONGLONG2VN_MAP;
typedef HMAP<MD const*, VN*> MD2VN_MAP;


//Note: SYM_HASH_FUNC request bucket size must be the power of 2.
class SYM2VN_MAP : public HMAP<SYM*, VN*, SYM_HASH_FUNC> {
public:
	SYM2VN_MAP() : HMAP<SYM*, VN*, SYM_HASH_FUNC>(0) {}
};


class IR2VN : public SVECTOR<VN*> {
public:
	void set(INT i, VN * elem) { SVECTOR<VN*>::set(i, elem); }
};


//VN Expression of Scalar
class VNE_SC {
public:
	UINT mdid;
	UINT ofst;
	UINT sz;

	VNE_SC(UINT id, UINT o, UINT s)
	{
		mdid = id;
		ofst = o;
		sz = s;
	}

	bool is_equ(VNE_SC & ve)
	{
		return mdid == ve.mdid &&
			   ofst == ve.ofst &&
			   sz == ve.sz;
	}

	void copy(VNE_SC & ve)
	{ *this = ve; }

	void clean()
	{
		mdid = 0;
		ofst = 0;
		sz = 0;
	}
};


//VN Expression of ILD
class VNE_ILD {
public:
	UINT base_vn_id;
	UINT ofst;
	UINT sz;

	VNE_ILD(UINT vnid, UINT o, UINT s)
	{
		base_vn_id = vnid;
		ofst = o;
		sz = s;
	}

	bool is_equ(VNE_ILD & ve)
	{
		return base_vn_id == ve.base_vn_id &&
			   ofst == ve.ofst &&
			   sz == ve.sz;
	}

	void copy(VNE_ILD & ve)
	{ *this = ve; }

	void clean()
	{
		base_vn_id = 0;
		ofst = 0;
		sz = 0;
	}
};


//VN Expression of ARRAY
class VNE_ARR : public VNE_ILD {
public:
	UINT ofst_vn_id;

	VNE_ARR(UINT bvnid, UINT ovnid, UINT o, UINT s) : VNE_ILD(bvnid, o, s)
	{ ofst_vn_id = ovnid; }

	bool is_equ(VNE_ARR & ve)
	{ return VNE_ILD::is_equ(ve) && ofst_vn_id == ve.ofst_vn_id; }

	void copy(VNE_ARR & ve)
	{ *this = ve; }

	void clean()
	{
		VNE_ILD::clean();
		ofst_vn_id = 0;
	}
};


class VNE_SC_HF {
public:
	UINT get_hash_value(VNE_SC * x, UINT bucket_size) const
	{
		UINT n = (x->mdid << 20) | (x->ofst << 10) | x->sz;
		IS_TRUE0(is_power_of_2(bucket_size));
		return hash32bit(n) & (bucket_size - 1);
	}

	UINT get_hash_value(OBJTY v, UINT bucket_size) const
	{ return get_hash_value((VNE_SC*)v, bucket_size); }

	bool compare(VNE_SC * x1, OBJTY x2) const
	{ return x1->is_equ(*(VNE_SC*)x2); }

	bool compare(VNE_SC * x1, VNE_SC * x2) const
	{ return x1->is_equ(*x2); }
};


class SCVNE2VN : public HMAP<VNE_SC*, VN*, VNE_SC_HF> {
protected:
	SMEM_POOL * m_pool;
	LIST<VNE_SC*> m_free_lst;
public:
	SCVNE2VN(SMEM_POOL * pool, UINT bsize) :
		HMAP<VNE_SC*, VN*, VNE_SC_HF>(bsize)
	{
		IS_TRUE0(pool);
		m_pool = pool;
	}
	virtual ~SCVNE2VN() {}

	virtual VNE_SC * create(OBJTY v)
	{
		VNE_SC * ve = m_free_lst.remove_head();
		if (ve == NULL) {
			ve = (VNE_SC*)smpool_malloc_h(sizeof(VNE_SC), m_pool);
		}
		ve->copy(*(VNE_SC*)v);
		return ve;
	}

	virtual void clean()
	{
		INT c;
		for (VNE_SC * ve = get_first(c); ve != NULL; ve = get_next(c)) {
			ve->clean();
			m_free_lst.append_head(ve);
		}
		HMAP<VNE_SC*, VN*, VNE_SC_HF>::clean();
	}
};


class VNE_ILD_HF {
public:
	UINT get_hash_value(VNE_ILD * x, UINT bucket_size) const
	{
		UINT n = (x->base_vn_id << 20) | (x->ofst << 10) | x->sz;
		IS_TRUE0(is_power_of_2(bucket_size));
		return hash32bit(n) & (bucket_size - 1);
	}

	UINT get_hash_value(OBJTY v, UINT bucket_size) const
	{ return get_hash_value((VNE_ILD*)v, bucket_size); }

	bool compare(VNE_ILD * x1, OBJTY x2) const
	{ return x1->is_equ(*(VNE_ILD*)x2); }

	bool compare(VNE_ILD * x1, VNE_ILD * x2) const
	{ return x1->is_equ(*x2); }
};


class ILD_VNE2VN : public HMAP<VNE_ILD*, VN*, VNE_ILD_HF> {
protected:
	SMEM_POOL * m_pool;
	LIST<VNE_ILD*> m_free_lst;
public:
	ILD_VNE2VN(SMEM_POOL * pool, UINT bsize) :
		HMAP<VNE_ILD*, VN*, VNE_ILD_HF>(bsize)
	{
		IS_TRUE0(pool);
		m_pool = pool;
	}
	virtual ~ILD_VNE2VN() {}

	virtual VNE_ILD * create(OBJTY v)
	{
		VNE_ILD * ve = m_free_lst.remove_head();
		if (ve == NULL) {
			ve = (VNE_ILD*)smpool_malloc_h(sizeof(VNE_ILD), m_pool);
		}
		ve->copy(*(VNE_ILD*)v);
		return ve;
	}

	virtual void clean()
	{
		INT c;
		for (VNE_ILD * ve = get_first(c); ve != NULL; ve = get_next(c)) {
			ve->clean();
			m_free_lst.append_head(ve);
		}
		HMAP<VNE_ILD*, VN*, VNE_ILD_HF>::clean();
	}
};


class IR2SCVNE : public TMAP<IR const*, SCVNE2VN*> {
public:
	IR2SCVNE() {}
	~IR2SCVNE()
	{
		TMAP_ITER<IR const*, SCVNE2VN*> ii;
		SCVNE2VN * v;
		for (get_first(ii, &v); v != NULL; get_next(ii, &v)) {
			delete v;
		}
	}

	void clean()
	{
		TMAP_ITER<IR const*, SCVNE2VN*> ii;
		SCVNE2VN * v;
		for (get_first(ii, &v); v != NULL; get_next(ii, &v)) {
			v->clean();
		}
	}
};


class IR2ILDVNE : public TMAP<IR const*, ILD_VNE2VN*> {
public:
	IR2ILDVNE() {}
	~IR2ILDVNE()
	{
		TMAP_ITER<IR const*, ILD_VNE2VN*> ii;
		ILD_VNE2VN * vne2vn;
		for (get_first(ii, &vne2vn); vne2vn != NULL; get_next(ii, &vne2vn)) {
			delete vne2vn;
		}
	}

	void clean()
	{
		TMAP_ITER<IR const*, ILD_VNE2VN*> ii;
		ILD_VNE2VN * vne2vn;
		for (get_first(ii, &vne2vn); vne2vn != NULL; get_next(ii, &vne2vn)) {
			vne2vn->clean();
		}
	}
};


class VNE_ARR_HF {
public:
	UINT get_hash_value(VNE_ARR * x, UINT bucket_size) const
	{
		UINT n = (x->base_vn_id << 24) | (x->ofst_vn_id << 16) |
				 (x->ofst << 8) | x->sz;
		IS_TRUE0(is_power_of_2(bucket_size));
		return hash32bit(n) & (bucket_size - 1);
	}

	UINT get_hash_value(OBJTY v, UINT bucket_size) const
	{ return get_hash_value((VNE_ARR*)v, bucket_size); }

	bool compare(VNE_ARR * x1, OBJTY x2) const
	{ return x1->is_equ(*(VNE_ARR*)x2); }

	bool compare(VNE_ARR * x1, VNE_ARR * x2) const
	{ return x1->is_equ(*(VNE_ARR*)x2); }
};


class ARR_VNE2VN : public HMAP<VNE_ARR*, VN*, VNE_ARR_HF> {
protected:
	SMEM_POOL * m_pool;
	LIST<VNE_ARR*> m_free_lst;
public:
	ARR_VNE2VN(SMEM_POOL * pool, UINT bsize) :
		HMAP<VNE_ARR*, VN*, VNE_ARR_HF>(bsize)
	{
		IS_TRUE0(pool);
		m_pool = pool;
	}
	virtual ~ARR_VNE2VN() {}

	VNE_ARR * create(OBJTY v)
	{
		VNE_ARR * ve = m_free_lst.remove_head();
		if (ve == NULL) {
			ve = (VNE_ARR*)smpool_malloc_h(sizeof(VNE_ARR), m_pool);
		}
		ve->copy(*(VNE_ARR*)v);
		return ve;
	}

	void clean()
	{
		INT c;
		for (VNE_ARR * ve = get_first(c); ve != NULL; ve = get_next(c)) {
			ve->clean();
			m_free_lst.append_head(ve);
		}
		HMAP<VNE_ARR*, VN*, VNE_ARR_HF>::clean();
	}
};


class IR2ARRVNE : public TMAP<IR const*, ARR_VNE2VN*> {
public:
	IR2ARRVNE() {}
	virtual ~IR2ARRVNE()
	{
		TMAP_ITER<IR const*, ARR_VNE2VN*> ii;
		ARR_VNE2VN * v;
		for (get_first(ii, &v); v != NULL; get_next(ii, &v)) {
			delete v;
		}
	}

	void clean()
	{
		TMAP_ITER<IR const*, ARR_VNE2VN*> ii;
		ARR_VNE2VN * v;
		for (get_first(ii, &v); v != NULL; get_next(ii, &v)) {
			delete v;
		}
	}
};


class IR2IR : public HMAP<IR const*, IR const*,
						  HASH_FUNC_BASE2<IR const*> > {
public:
	IR2IR() : HMAP<IR const*, IR const*, HASH_FUNC_BASE2<IR const*> >(0) {}
};


//Perform Global Value Numbering.
class IR_GVN : public IR_OPT {
protected:
	REGION * m_ru;
	IR_DU_MGR * m_du;
	DT_MGR * m_dm;
	MD_SYS * m_md_sys;
	IR_CFG * m_cfg;
	VN * m_zero_vn;
	UINT m_vn_count;
	IR2VN m_ir2vn;
	SVECTOR<SVECTOR<SVECTOR<VN*>*>*> m_irt_vec;
	SMEM_POOL * m_pool;
	LONGLONG2VN_MAP m_ll2vn;
	FP2VN_MAP m_fp2vn;
	SYM2VN_MAP m_str2vn;
	LIST<IR const*> m_tmp;
	LIST<VN*> m_free_lst;
	LIST<SVECTOR<VN*>*> m_vec_lst;
	LIST<SVECTOR<VN*>*> m_vnvec_lst;
	MD2VN_MAP m_md2vn;
	IR2ILDVNE m_def2ildtab;
	IR2ARRVNE m_def2arrtab;
	IR2SCVNE m_def2sctab;
	IR2IR m_stmt2domdef;
	BYTE m_is_vn_fp:1;
	BYTE m_is_valid:1;
	BYTE m_is_comp_ild_vn_by_du:1;

	/*
	Set true if we want to compute the vn for LDA(ID(str)).
	And it's default value is false.
	NOTE if you are going to enable it, you should ensure
	REGION_MGR::m_is_regard_str_as_same_md is false, because this
	flag regard all strings as the same one to reduce the MD which
	generated by different VAR.

	e.g: LDA(ID("xxx")),  and LDA(ID("yyy")), if
	REGION_MGR::m_is_regard_str_as_same_md is true, ID("xxx") and
	ID("yyy") will refer to same MD, and the MD is inexact.
	*/
	BYTE m_is_comp_lda_string:1;

	VN * alloc_livein_vn(IR const* exp, MD const* emd, bool & change);
	void clean();
	VN * comp_sc_by_anon_domdef(IR const* ild, IR const* domdef,
								bool & change);
	VN * comp_ild_by_anon_domdef(IR const* ild, VN const* mlvn,
								 IR const* domdef, bool & change);
	VN * comp_array_by_anon_domdef(IR const* arr, VN const* basevn,
								   VN const* ofstvn, IR const* domdef,
								   bool & change);
	IR const* comp_dom_def(IR const* exp, IR const* exp_stmt,
						   SLIST<IR*> * defs);
	void comp_array_addr_ref(IR const* ir, bool & change);
	VN * comp_array(IR const* exp, bool & change);
	VN * comp_sc(IR const* exp, bool & change);
	VN * comp_ild(IR const* exp, bool & change);
	VN * comp_mem(IR const* exp, bool & change);
	VN * comp_vn(IR const* exp, bool & change);
	void * xmalloc(UINT size)
	{
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p);
		memset(p, 0, size);
		return p;
	}
	VN * register_qua_vn(IR_TYPE irt, VN const* v0, VN const* v1,
						 VN const* v2, VN const* v3);
	VN * register_tri_vn(IR_TYPE irt, VN const* v0, VN const* v1,
						 VN const* v2);
	VN * register_bin_vn(IR_TYPE irt, VN const* v0, VN const* v1);
	VN * register_uni_vn(IR_TYPE irt, VN const* v0);
	VN * register_md_vn(MD const* md);
	VN * register_int_vn(LONGLONG v);
	VN * register_fp_vn(double v);
	VN * register_str_vn(SYM * v);
	inline VN * new_vn()
	{
		VN * vn = m_free_lst.remove_head();
		if (vn == NULL) {
			vn = (VN*)xmalloc(sizeof(VN));
		} else {
			vn->clean();
		}
		VN_id(vn) = m_vn_count++;
		return vn;
	}

	void dump_h1(IR const* k, VN * x);
	void dump_bb_labs(LIST<LABEL_INFO*> & lst);
	void process_bb(IR_BB * bb, bool & change);
	void process_st(IR const* ir, bool & change);
	void process_call(IR const* ir, bool & change);
	void process_region(IR const* ir, bool & change);
	void process_phi(IR const* ir, bool & change);
public:
	IR_GVN(REGION * ru);
	virtual ~IR_GVN();
	bool calc_cond_must_val(IN IR const* ir,
							bool & must_true, bool & must_false);
	void dump();
	void dump_bb(UINT bbid);
	void dump_ir2vn();

	virtual CHAR const* get_opt_name() const
	{ return "Global Value Numbering"; }

	OPT_TYPE get_opt_type() const { return OPT_GVN; }

	bool is_triple(IR_TYPE i) const { return i == IR_ILD; }
	bool is_quad(IR_TYPE i) const { return i == IR_ARRAY; }
	bool is_valid() { return m_is_valid; }

	VN * map_ir2vn(IR const* ir)
	{ return m_ir2vn.get(IR_id(ir)); }

	void set_map_ir2vn(IR const* ir, VN * vn)
	{ m_ir2vn.set(IR_id(ir), vn); }
	void set_vn_fp(bool doit) { m_is_vn_fp = doit; }
	void set_valid(bool valid) { m_is_valid = valid; }
	void set_comp_ild_vn_by_du(bool doit) { m_is_comp_ild_vn_by_du = doit; }

	bool reperform(OPT_CTX & oc);
	bool verify();
	virtual bool perform(OPT_CTX & oc);
};
#endif
