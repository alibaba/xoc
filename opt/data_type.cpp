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

DTYPE_DESC const g_dtype_desc[] = {
	{D_UNDEF, "none",  0},
	{D_B,      "bool", 8}, //BOOL
	{D_I8,    "i8",    8}, //signed integer 8 bits
	{D_I16,   "i16",   16},
	{D_I32,   "i32",   32},
	{D_I64,   "i64",   64},
	{D_I128,  "i128",  128},

	{D_U8,    "u8",    8},//unsigned integer 8 bits
	{D_U16,   "u16",   16},
	{D_U32,   "u32",   32},
	{D_U64,   "u64",   64},
	{D_U128,  "u128",  128},

	{D_F32,   "f32",   32}, //float point 32 bits
	{D_F64,   "f64",   64},
	{D_F80,   "f80",   80},
	{D_F128,  "f128",  128},

	{D_MC,     "mc",  -1}, //memory chunk, for structures
	{D_STR,    "str",  BYTE_PER_POINTER * BIT_PER_BYTE}, //char strings is pointer
	{D_PTR,    "ptr",  BYTE_PER_POINTER * BIT_PER_BYTE} //pointer
};

/*
The hoisting rules are:
1. Return max bit size of DATA_TYPE between 'opnd0' and 'opnd1',
2. else return SIGNED if one of them is signed;
3. else return FLOAT if one of them is float,
4. else return UNSIGNED.

The C language rules are:
1. If any operand is of a integral type smaller than int? Convert to int.
2. Is any operand unsigned long? Convert the other to unsigned long.
3. (else) Is any operand signed long? Convert the other to signed long.
4. (else) Is any operand unsigned int? Convert the other to unsigned int.

NOTE: The function does NOT hoist vector type.
*/
UINT DT_MGR::hoist_dtype_for_binop(IN IR const* opnd0, IN IR const* opnd1)
{
	DTD const* d0 = get_dtd(IR_dt(opnd0));
	DTD const* d1 = get_dtd(IR_dt(opnd1));
	IS_TRUE0(!DTD_is_vec(d0) && !DTD_is_vec(d1));
	IS_TRUE0(!DTD_is_ptr(d0) && !DTD_is_ptr(d1));
	DATA_TYPE t0 = DTD_rty(d0);
	DATA_TYPE t1 = DTD_rty(d1);
	if (t0 == D_MC && t1 == D_MC) {
		IS_TRUE0(DTD_mc_sz(d0) == DTD_mc_sz(d1));
		IS_TRUE0(DTD_vec_ety(d0) == DTD_vec_ety(d1));
		return IR_dt(opnd0);
	}
	if (t0 == D_MC) {
		IS_TRUE0(DTD_mc_sz(d0) != 0);
		UINT ty_size = MAX(DTD_mc_sz(d0), get_dtd_bytesize(d1));
		if (ty_size == DTD_mc_sz(d0))
			return IR_dt(opnd0);
		return IR_dt(opnd1);
	}
	if (t1 == D_MC) {
		IS_TRUE0(DTD_mc_sz(d1) != 0);
		UINT ty_size = MAX(DTD_mc_sz(d1), get_dtd_bytesize(d0));
		if (ty_size == DTD_mc_sz(d1))
			return IR_dt(opnd1);
		return IR_dt(opnd0);
	}

	//Always hoist to longest integer type.
	//t0 = hoist_dtype(t0);
	//t1 = hoist_dtype(t1);

	//Generic data type.
	INT bitsize = MAX(get_dtype_bitsize(t0), get_dtype_bitsize(t1));
	DATA_TYPE res;
	if (IS_FP(t0) || IS_FP(t1)) {
		res = get_fp_dtype(bitsize);
	} else if (IS_SINT(t0) || IS_SINT(t1)) {
		res = get_int_dtype(bitsize, true);
	} else {
		res = get_int_dtype(bitsize, false);
	}
	IS_TRUE0(res != D_UNDEF);
	IS_TRUE0(IS_SIMPLEX(res));
	return get_simplex_tyid(res);
}


//Hoist DATA_TYPE up to upper bound of given bit length.
DATA_TYPE DT_MGR::hoist_dtype(IN UINT data_size, OUT UINT * hoisted_data_size)
{
	DATA_TYPE dt = D_UNDEF;
	if (data_size > get_dtype_bitsize(D_I128)) {
		//Memory chunk
		dt = D_MC;
		*hoisted_data_size = data_size;
	} else {
		dt = hoist_bs_dtype(data_size, false);
		*hoisted_data_size = get_dtype_bytesize(dt);
	}
	return dt;
}


//Hoist DATA_TYPE up to upper bound of given type.
DATA_TYPE DT_MGR::hoist_dtype(IN DATA_TYPE dt) const
{
	if (IS_INT(dt) &&
		get_dtype_bitsize(dt) < (BYTE_PER_INT * BIT_PER_BYTE)) {
		//Hoist to longest INT type.
		return hoist_bs_dtype(BYTE_PER_INT * BIT_PER_BYTE, IS_SINT(dt));
	}
	return dt;
}


DATA_TYPE DT_MGR::hoist_bs_dtype(UINT bit_size, bool is_signed) const
{
	DATA_TYPE m = D_UNDEF;
	if (bit_size > 1 && bit_size <= 8) {
		m = is_signed ? D_I8 : D_U8;
	} else if (bit_size > 8 && bit_size <= 16) {
		m = is_signed ? D_I16 : D_U16;
	} else if (bit_size > 16 && bit_size <= 32) {
		m = is_signed ? D_I32 : D_U32;
	} else if (bit_size > 32 && bit_size <= 64) {
		m = is_signed ? D_I64 : D_U64;
	} else if (bit_size > 64 && bit_size <= 128) {
		m = is_signed ? D_I128 : D_U128;
	} else if (bit_size == 1) {
		m = D_B;
	} else if (bit_size > 128) {
		m = D_MC;
	}
	return m;
}


//Return ty-idx in m_dtd_tab.
UINT DT_MGR::register_ptr(DTD const* dtd)
{
	IS_TRUE0(dtd);
	/*
	Insertion Sort by ptr-base-size in incrmental order.
	e.g: Given PTR, base_size=32,
		PTR, base_size=24
		PTR, base_size=128
		...
	=> after insertion.
		PTR, base_size=24
		PTR, base_size=32  //insert into here.
		PTR, base_size=128
		...
	*/
	UINT sz = DTD_ptr_base_sz(dtd);
	IS_TRUE0(sz != 0);
	DTD_C * p = m_dtd_hash[DTD_rty(dtd)];
	DTD_C * last = NULL;
	while (p != NULL) {
		last = p;
		if (sz == DTD_ptr_base_sz(DTDC_dtd(p))) {
			return DTDC_tyid(p);
		}
		if (sz < DTD_ptr_base_sz(DTDC_dtd(p))) {
			DTD_C * x = new_dtdc();
			//Add new item into hash.
			*DTDC_dtd(x) = *dtd;
			DTDC_tyid(x) = m_dtd_ty_count++;
			m_dtd_tab.set(DTDC_tyid(x), DTDC_dtd(x));
			//
			insertbefore_one(&m_dtd_hash[DTD_rty(dtd)], p, x);
			return DTDC_tyid(x);
		}
		p = DTDC_next(p);
	}

	//Add new item into hash.
	DTD_C * x = new_dtdc();
	*DTDC_dtd(x) = *dtd;
	DTDC_tyid(x) = m_dtd_ty_count++;
	m_dtd_tab.set(DTDC_tyid(x), DTDC_dtd(x));
	//
	add_next(&m_dtd_hash[DTD_rty(dtd)], &last, x);
	return DTDC_tyid(x);
}


/*
'mc_head': the first element of current D_MC type list,
	and its vec-type is D_UNDEF.
'dtd': it is D_MC type, and its vec-type is not D_UNDEF,
	namely, dtd is vector type.
	e.g: vector<I8,I8,I8,I8> dtd, which mc_size is 32, vec-type is D_I8
*/
UINT DT_MGR::register_vec(DTD_C * mc_head, DTD const* dtd)
{
	IS_TRUE0(dtd && mc_head);
	IS_TRUE(!DTD_is_vec(DTDC_dtd(mc_head)), ("mc_head can NOT be vector"));
	IS_TRUE0(DTD_is_vec(dtd));
	DTD_C ** head = &DTDC_kid(mc_head);
	DTD_C * last = NULL;
	DATA_TYPE ty = DTD_vec_ety(dtd);
	DTD_C * p = *head;
	while (p != NULL) {
		last = p;
		if (ty == DTD_vec_ety(DTDC_dtd(p))) {
			return DTDC_tyid(p);
		}
		if (ty < DTD_vec_ety(DTDC_dtd(p))) {
			DTD_C * x = new_dtdc();
			//Add new item into hash.
			*DTDC_dtd(x) = *dtd;
			DTDC_tyid(x) = m_dtd_ty_count++;
			m_dtd_tab.set(DTDC_tyid(x), DTDC_dtd(x));
			//
			insertbefore_one(head, p, x);
			return DTDC_tyid(x);
		}
		p = DTDC_next(p);
	}
	//Add new item into hash.
	DTD_C * x = new_dtdc();
	*DTDC_dtd(x) = *dtd;
	DTDC_tyid(x) = m_dtd_ty_count++;
	m_dtd_tab.set(DTDC_tyid(x), DTDC_dtd(x));
	//
	//'last' may being 'mc_head'.
	add_next(head, &last, x);
	return DTDC_tyid(x);
}


UINT DT_MGR::register_mc(DTD const* dtd)
{
	IS_TRUE0(dtd);
	/*
	Insertion Sort by mc-size in incrmental order.
	e.g:Given MC, mc_size=32
		MC, mc_size=24
		MC, mc_size=32 <= insert into here.
		MC, mc_size=128
		...
	*/
	UINT sz = DTD_mc_sz(dtd);
	IS_TRUE0(sz != 0);
	DTD_C * p = m_dtd_hash[DTD_rty(dtd)];
	DTD_C * last = NULL;
	while (p != NULL) {
		last = p;
		if (sz == DTD_mc_sz(DTDC_dtd(p))) {
			IS_TRUE0(!DTD_is_vec(DTDC_dtd(p)));
			if (DTD_is_vec(dtd)) {
				return register_vec(p, dtd);
			}
			return DTDC_tyid(p);
		}
		if (sz < DTD_mc_sz(DTDC_dtd(p))) {
			DTD_C * x = new_dtdc();
			/*
			Add new item into hash.
			This is the first item of current mc_size.
			So its vec_type mustbe D_UNDEF.
			e.g:
				MC,size=100,vec_ty=D_UNDEF
				MC,size=200,vec_ty=D_UNDEF
				    MC,size=200,vec_ty=D_I8
					MC,size=200,vec_ty=D_U8 //I8<U8
					MC,size=200,vec_ty=D_F32
				MC,size=300,vec_ty=D_UNDEF
				    MC,size=300,vec_ty=D_F32
				...
			*/
			*DTDC_dtd(x) = *dtd;
			DTD_vec_ety(DTDC_dtd(x)) = D_UNDEF;
			DTDC_tyid(x) = m_dtd_ty_count++;
			m_dtd_tab.set(DTDC_tyid(x), DTDC_dtd(x));
			insertbefore_one(&m_dtd_hash[DTD_rty(dtd)], p, x);
			if (DTD_is_vec(dtd)) {
				return register_vec(x, dtd);
			}
			return DTDC_tyid(x);
		}
		p = DTDC_next(p);
	}

	/*
	Add first item into hash as header element.
	This is the first item of current mc_size.
	So its vec_type mustbe D_UNDEF.
	e.g:
		MC,size=100,vec_ty=D_UNDEF
		MC,size=200,vec_ty=D_UNDEF
		    MC,size=200,vec_ty=D_I8
			MC,size=200,vec_ty=D_U8 //I8<U8
			MC,size=200,vec_ty=D_F32
		MC,size=300,vec_ty=D_UNDEF
		    MC,size=300,vec_ty=D_F32
		...
	*/
	DTD_C * x = new_dtdc();
	*DTDC_dtd(x) = *dtd;
	DTD_vec_ety(DTDC_dtd(x)) = D_UNDEF;
	DTDC_tyid(x) = m_dtd_ty_count++;
	m_dtd_tab.set(DTDC_tyid(x), DTDC_dtd(x));
	//
	add_next(&m_dtd_hash[DTD_rty(dtd)], &last, x);
	if (DTD_is_vec(dtd)) {
		return register_vec(x, dtd);
	}
	return DTDC_tyid(x);
}


//INT, UINT, FP, BOOL.
UINT DT_MGR::register_simplex(DTD const* dtd)
{
	IS_TRUE0(dtd);
	DTD_C ** head = &m_dtd_hash[DTD_rty(dtd)];
	if (*head == NULL) {
		*head = new_dtdc();
		DTD_C * x = *head;
		m_dtd_hash[DTD_rty(dtd)] = x;

		//Add new item into hash.
		*DTDC_dtd(x) = *dtd;
		DTDC_tyid(x) = m_dtd_ty_count++;
		m_dtd_tab.set(DTDC_tyid(x), DTDC_dtd(x));
		//
		return DTDC_tyid(x);
	}
	return DTDC_tyid(*head);
}


//Return ty-idx in m_dtd_tab.
UINT DT_MGR::register_dtd(DTD const* dtd)
{
	IS_TRUE0(dtd);
	#ifdef _DEBUG_
	if (DTD_rty(dtd) == D_PTR) {
		IS_TRUE0(DTD_ptr_base_sz(dtd) != 0);
	}
	if (DTD_rty(dtd) == D_MC) {
		IS_TRUE0(DTD_mc_sz(dtd) != 0);
	}
	if (DTD_vec_ety(dtd) != D_UNDEF) {
		IS_TRUE0(DTD_rty(dtd) == D_MC && DTD_mc_sz(dtd) != 0);
	}
	#endif
	if (IS_SIMPLEX(DTD_rty(dtd))) {
		return register_simplex(dtd);
	}
	if (DTD_rty(dtd) == D_PTR) {
		return register_ptr(dtd);
	}
	if (DTD_rty(dtd) == D_MC) {
		return register_mc(dtd);
	}
	IS_TRUE(0, ("unsupport data type"));
	return 0;
}


//READONLY
UINT DT_MGR::get_dtd_bytesize(DTD const* dtd) const
{
	IS_TRUE0(dtd);
	DATA_TYPE dt = DTD_rty(dtd);
	if (IS_SIMPLEX(dt)) {
		return get_dtype_bytesize(dt);
	}
	if (IS_PTR(dt)) {
		return get_pointer_bytesize();
	}
	if (IS_MEM(dt)) {
		return DTD_mc_sz(dtd);
	}
	return 0;
}


CHAR * DT_MGR::dump_dtd(IN DTD const* dtd, OUT CHAR * buf)
{
	IS_TRUE0(dtd);
	CHAR * p = buf;
	DATA_TYPE dt = DTD_rty(dtd);
	switch (dt) {
	case D_B:
	case D_I8:
	case D_I16:
	case D_I32:
	case D_I64:
	case D_I128:
	case D_U8:
	case D_U16:
	case D_U32:
	case D_U64:
	case D_U128:
	case D_F32:
	case D_F64:
	case D_F80:
	case D_F128:
	case D_STR:
	case D_PTR:
		sprintf(buf, "%s", DTNAME(dt));
		break;
	case D_MC:
		sprintf(buf, "%s<%d>", DTNAME(dt), get_dtd_bytesize(dtd));
		break;
	default:
		IS_TRUE(0, ("unsupport"));
	}
	if (IS_PTR(dt)) {
		buf += strlen(buf);
		sprintf(buf, "<%d>", DTD_ptr_base_sz(dtd));
	}
	if (DTD_is_vec(dtd)) {
		buf += strlen(buf);
		sprintf(buf, " ety:%s", DTNAME(DTD_vec_ety(dtd)));
	}
	return p;
}


void DT_MGR::dumpf_dtd(UINT tyid)
{
	dumpf_dtd(get_dtd(tyid));
}


void DT_MGR::dumpf_dtd(DTD const* dtd)
{
	if (g_tfile == NULL) return;
	CHAR buf[256];
	fprintf(g_tfile, "%s", dump_dtd(dtd, buf));
	fflush(g_tfile);
}


void DT_MGR::dump_hash_node(DTD_C * dtdc, INT indent)
{
	static CHAR buf[256];
	DATA_TYPE dt = DTD_rty(DTDC_dtd(dtdc));
	fprintf(g_tfile, "\n");
	if (IS_SIMPLEX(dt)) {
		while (indent-- > 0) { fprintf(g_tfile, " "); }
		fprintf(g_tfile, "%s tyid:%d",
				dump_dtd(DTDC_dtd(dtdc), buf), DTDC_tyid(dtdc));
	} else if (IS_PTR(dt)) {
		INT i = indent;
		while (i-- > 0) { fprintf(g_tfile, " "); }
		fprintf(g_tfile, "PTR_TAB:\n");
		DTD_C * p = dtdc;
		indent += 4;
		while (p != NULL) {
			INT i = indent;
			while (i-- > 0) { fprintf(g_tfile, " "); }
			fprintf(g_tfile, "%s tyid:%d",
					dump_dtd(DTDC_dtd(p), buf), DTDC_tyid(p));
			if (DTDC_kid(p) != NULL) {
				dump_hash_node(DTDC_kid(p), indent + 4);
			}
			fprintf(g_tfile, "\n");
			p = DTDC_next(p);
		}
	} else if (IS_MEM(dt)) {
		INT i = indent;
		while (i-- > 0) { fprintf(g_tfile, " "); }
		fprintf(g_tfile, "MC_TAB:\n");
		DTD_C * p = dtdc;
		indent += 4;
		while (p != NULL) {
			INT i = indent;
			while (i-- > 0) { fprintf(g_tfile, " "); }
			fprintf(g_tfile, "%s tyid:%d",
					dump_dtd(DTDC_dtd(p), buf), DTDC_tyid(p));
			if (DTDC_kid(p) != NULL) {
				dump_hash_node(DTDC_kid(p), indent + 4);
			}
			fprintf(g_tfile, "\n");
			p = DTDC_next(p);
		}
	}
}


void DT_MGR::dump_dtd_hash()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP DTD HASH ----==\n");
	for (UINT i = D_UNDEF + 1; i < D_LAST; i++) {
		DTD_C * header = m_dtd_hash[i];
		if (header == NULL) { continue; }
		dump_hash_node(header, 0);
	}
	fflush(g_tfile);
}


void DT_MGR::dump_dtd_tab()
{
	CHAR buf[256];
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP DTD GLOBAL TABLE ----==\n");
	for (INT i = 1; i <= m_dtd_tab.get_last_idx(); i++) {
		DTD * d = m_dtd_tab.get(i);
		IS_TRUE0(d);
		fprintf(g_tfile, "%s tyid:%d", dump_dtd(d, buf), i);
		fprintf(g_tfile, "\n");
	}
	fflush(g_tfile);
}

