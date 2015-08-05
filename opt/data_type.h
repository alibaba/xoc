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
#ifndef __DATA_TYPE_H__
#define __DATA_TYPE_H__

class IR;

#define IS_INT(t)   			((t) >= D_I8 && (t) <= D_U128)
#define IS_SINT(t)  			((t) >= D_I8 && (t) <= D_I128)
#define IS_FP(t)    			((t) >= D_F32 && (t) <= D_F128)
#define IS_BOOL(t)  			((t) == D_B)
#define IS_MEM(t)   			((t) == D_MC)
#define IS_PTR(t)   			((t) == D_PTR)
#define IS_SIMPLEX(t)			(IS_INT(t) || IS_FP(t) || IS_BOOL(t) || t == D_STR)

/* Memory Type ,represented with bit length.
Is unsigned type indispensible?
Java, for example, gets rid of signed/unsigned mixup issues by eliminating
unsigned types. This goes down a little hard for type convertion, although
some argue that Java has demonstrated that unsigned types really aren't necessary.
But unsigned types are useful for implementing memory operation for that span
more than half of the memory address space, and useful for primitives
implementing multiprecision arithmetic, etc. */
typedef enum _DATA_TYPE {
	D_UNDEF = 0,
	D_B = 1, //boolean
	D_I8 = 2, //signed integer 8 bit
	D_I16 = 3,
	D_I32 = 4,
	D_I64 = 5,
	D_I128 = 6,
	D_U8 = 7, //unsigned integer 8 bit
	D_U16 = 8,
	D_U32 = 9,
	D_U64 = 10,
	D_U128 = 11,
	D_F32 = 12, //float point 32 bit
	D_F64 = 13, //float point 64 bit
	D_F80 = 14, //float point 96 bit
	D_F128 = 15, //float point 128 bit

	//Note all above types are scalar.

	D_MC = 16, //memory chunk, used in structure/union/block type.
	D_STR = 17, //strings
	D_PTR = 18, //pointer
	D_LAST = 19,
	//NOTE: Extend IR::rtype bit length if one extends type value large than 31.
} DATA_TYPE;

#define VOID_TY	0

class DTYPE_DESC {
public:
	DATA_TYPE dtype;
	CHAR const* name;
	INT bitsize;
};
#define DTDES_type(d)				((d)->dtype)
#define DTDES_name(d)				((d)->name)
#define DTDES_bitsize(d)			((d)->bitsize)

#define DTNAME(type)				(DTDES_name(&g_dtype_desc[type]))

//Define target bit size of WORD length.
#define WORD_BITSIZE	GENERAL_REGISTER_SIZE * BIT_PER_BYTE

//Data Type Descriptor.
#define DTD_rty(d)					((d)->rtype)

//Indicate the pointer base size.
#define DTD_ptr_base_sz(d)			((d)->u1.pointer_base_size)

//Indicate the size of memory chunk.
#define DTD_mc_sz(d)				((d)->u1.mc_size)

//Indicate the size of whole vector.
#define DTD_vec_sz(d)				DTD_mc_sz(d)

//Indicate the vector element size.
#define DTD_vec_ety(d)				((d)->s2.vector_elem_type)

//Return true if type is vector.
#define DTD_is_vec(d)				(DTD_rty(d) == D_MC && DTD_vec_ety(d) != D_UNDEF)

//Return true if type is pointer.
#define DTD_is_ptr(d)				(DTD_rty(d) == D_PTR)

//Return true if type is string.
#define DTD_is_str(d)				(DTD_rty(d) == D_STR)

//Return true if type is memory chunk.
#define DTD_is_mc(d)				(DTD_rty(d) == D_MC)

class DTD {
public:
	DATA_TYPE rtype;  //result type
	//USHORT rtype:5;

	union {
		//e.g  int * p, base size is 4,
		//long long * p, base size is 8
		//char * p, base size is 1.
		UINT pointer_base_size;

		//Record the BYTE size if 'rtype' is D_MC.
		//NOTE: If ir is pointer, its 'rtype' should NOT be D_MC.
		UINT mc_size;
	} u1;

	struct {
		//record the vector elem size if ir represent a vector.
		DATA_TYPE vector_elem_type;
	} s2;

	DTD()
	{
		rtype = D_UNDEF;
		u1.mc_size = 0;
		s2.vector_elem_type = D_UNDEF;
	}
};


//Container of DTD.
#define DTDC_dtd(c)			((c)->dtd)
#define DTDC_tyid(c)		((c)->tyid)
#define DTDC_kid(c)			((c)->kid)
#define DTDC_next(c)		((c)->next)
class DTD_C {
public:
	DTD * dtd;
	UINT tyid;
	DTD_C * kid;
	DTD_C * next;
	DTD_C * prev;
};


extern DTYPE_DESC const g_dtype_desc[];
class DT_MGR {
	SVECTOR<DTD*> m_dtd_tab;
	SMEM_POOL * m_pool;
	DTD_C * m_dtd_hash[D_LAST];
	UINT m_dtd_ty_count;
	UINT m_b;
	UINT m_i8;
	UINT m_i16;
	UINT m_i32;
	UINT m_i64;
	UINT m_i128;
	UINT m_u8;
	UINT m_u16;
	UINT m_u32;
	UINT m_u64;
	UINT m_u128;
	UINT m_f32;
	UINT m_f64;
	UINT m_f80;
	UINT m_f128;
	UINT m_str;
	UINT m_uint16x4;
	UINT m_int16x4;
	UINT m_uint32x4;
	UINT m_int32x4;
	UINT m_uint64x2;
	UINT m_int64x2;

	void * xmalloc(size_t size)
	{
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p);
		memset(p, 0, size);
		return p;
	}

	inline DTD_C * new_dtdc()
	{
		DTD_C * dtdc = (DTD_C*)xmalloc(sizeof(DTD_C));
		DTDC_dtd(dtdc) = (DTD*)xmalloc(sizeof(DTD));
		return dtdc;
	}
	UINT register_mc(DTD const* dtd);
	UINT register_vec(DTD_C * mc_head, DTD const* dtd);
	UINT register_ptr(DTD const* dtd);
	UINT register_simplex(DTD const* dtd);
public:
	DT_MGR()
	{
		m_dtd_tab.clean();
		m_pool = smpool_create_handle(sizeof(DTD) * 8, MEM_COMM);
		m_dtd_ty_count = 1;
		memset(m_dtd_hash, 0, sizeof(m_dtd_hash));
		m_b = get_simplex_tyid(D_B);
		m_i8 = get_simplex_tyid(D_I8);
		m_i16 = get_simplex_tyid(D_I16);
		m_i32 = get_simplex_tyid(D_I32);
		m_i64 = get_simplex_tyid(D_I64);
		m_i128 = get_simplex_tyid(D_I128);
		m_u8 = get_simplex_tyid(D_U8);
		m_u16 = get_simplex_tyid(D_U16);
		m_u32 = get_simplex_tyid(D_U32);
		m_u64 = get_simplex_tyid(D_U64);
		m_u128 = get_simplex_tyid(D_U128);
		m_f32 = get_simplex_tyid(D_F32);
		m_f64 = get_simplex_tyid(D_F64);
		m_f80 = get_simplex_tyid(D_F80);
		m_f128 = get_simplex_tyid(D_F128);
		m_str = get_simplex_tyid(D_STR);
		m_uint16x4 = get_mc_tyid(64, D_U16);
		m_int16x4 = get_mc_tyid(64, D_I16);
		m_uint32x4 = get_mc_tyid(128, D_U32);
		m_int32x4 = get_mc_tyid(128, D_I32);
		m_uint64x2 = get_mc_tyid(128, D_U64);
		m_int64x2 = get_mc_tyid(64, D_I64);
	}

	~DT_MGR()
	{
		smpool_free_handle(m_pool);
		m_pool = NULL;
	}

	//Exported Functions.
	UINT register_dtd(DTD const* dtd);
	CHAR * dump_dtd(IN DTD const* dtd, OUT CHAR * buf);
	void dumpf_dtd(DTD const* dtd);
	void dumpf_dtd(UINT tyid);
	void dump_dtd_hash();
	void dump_dtd_tab();
	void dump_hash_node(DTD_C * dtdc, INT indent);

	DATA_TYPE hoist_bs_dtype(UINT bit_size, bool is_signed) const;
	DATA_TYPE hoist_dtype(IN UINT bit_size, OUT UINT * hoisted_data_size);
	DATA_TYPE hoist_dtype(IN DATA_TYPE stype) const;
	UINT hoist_dtype_for_binop(IN IR const* opnd0, IN IR const* opnd1);

	//Return DATA_TYPE which 'bitsize' corresponding to
	inline DATA_TYPE get_fp_dtype(INT bitsize) const
	{
		switch (bitsize) {
		case 32: return D_F32;
	    case 64: return D_F64;
	    case 128: return D_F128;
		default:;
		}
		return D_UNDEF;
	}

	//Return DATA-TYPE according to given byte size.
	DATA_TYPE get_uint_dtype(INT bytesize) const
	{ return get_int_dtype(bytesize * BIT_PER_BYTE, false); }

	//Return DATA-TYPE according to given bit size and sign.
	//If bitsize is not equal to 1, 8, 16, 32, 64, 128, this
	//function will return D_MC.
	DATA_TYPE get_int_dtype(INT bitsize, bool is_signed) const
	{
		switch (bitsize) {
		case 1: return D_B;
		case 8: return is_signed ? D_I8 : D_U8;
		case 16: return is_signed ? D_I16 : D_U16;
		case 32: return is_signed ? D_I32 : D_U32;
		case 64: return is_signed ? D_I64 : D_U64;
		case 128: return is_signed ? D_I128 : D_U128;
		default: return D_MC;
		}
		return D_UNDEF;
	}

	//Return DATA-TYPE according to given bit size and sign.
	//Note the bit size will be aligned to power of 2.
	DATA_TYPE get_dtype(INT bit_size, bool is_signed) const
	{ return hoist_bs_dtype(bit_size, is_signed); }

	//Return byte size of a pointer.
	//e.g: 32bit processor return 4, 64bit processor return 8.
	UINT get_pointer_bytesize() const { return BYTE_PER_POINTER; }

	//Return DATA-TYPE that has identical byte-size to pointer.
	//e.g: 32bit processor return U4, 64bit processor return U8.
	DATA_TYPE get_pointer_size_int_dtype() const
	{ return get_int_dtype(BYTE_PER_POINTER * BIT_PER_BYTE, false); }

	//Return the byte size of pointer's base.
	UINT get_pointer_base_sz(UINT tyid)
	{
		IS_TRUE0(is_ptr(tyid));
		return DTD_ptr_base_sz(get_dtd(tyid));
	}

	//Return bits size of 'dtype' refers to.
	UINT get_dtype_bitsize(DATA_TYPE dtype) const
	{
		IS_TRUE(dtype != D_MC, ("this is memory chunk"));
		return DTDES_bitsize(&g_dtype_desc[dtype]);
	}

	//Return bytes size of 'dtype' refer to.
	UINT get_dtype_bytesize(DATA_TYPE dtype) const
	{
		IS_TRUE0(dtype != D_UNDEF);
		INT bitsize = get_dtype_bitsize(dtype);
		return bitsize < BIT_PER_BYTE ? BIT_PER_BYTE : bitsize / BIT_PER_BYTE;
	}

	//Retrieve DTD via 'type-index'.
	DTD const* get_dtd(UINT tyid) const
	{
		IS_TRUE0(tyid != 0);
		IS_TRUE0(m_dtd_tab.get(tyid));
		return m_dtd_tab.get(tyid);
	}

	//Return tyid accroding to DATA_TYPE.
	UINT get_simplex_tyid(DATA_TYPE dt)
	{
		IS_TRUE0(IS_SIMPLEX(dt));
		DTD d;
		DTD_rty(&d) = dt;
		return register_dtd(&d);
	}

	//Return tyid accroding to DATA_TYPE.
	inline UINT get_simplex_tyid_ex(DATA_TYPE dt)
	{
		switch (dt) {
		case D_B: return m_b;
		case D_I8: return m_i8;
		case D_I16: return m_i16;
		case D_I32: return m_i32;
		case D_I64: return m_i64;
		case D_I128: return m_i128;
		case D_U8: return m_u8;
		case D_U16: return m_u16;
		case D_U32: return m_u32;
		case D_U64: return m_u64;
		case D_U128: return m_u128;
		case D_F32: return m_f32;
		case D_F64: return m_f64;
		case D_F80: return m_f80;
		case D_F128: return m_f128;
		case D_STR: return m_str;
		default: IS_TRUE(0, ("not simplex type"));
		}
		return 0;
	}

	//Return vector type, which type is <vec_elem_num x vec_elem_ty>.
	UINT get_vec_tyid_ex2(UINT vec_elem_num, DATA_TYPE vec_elem_ty)
	{
		if (vec_elem_num == 4) {
			switch (vec_elem_ty) {
			case D_U16: return m_uint16x4;
			case D_I16: return m_int16x4;
			case D_U32: return m_uint32x4;
			case D_I32: return m_int32x4;
			default:;
			}
		}
		if (vec_elem_num == 2) {
			switch (vec_elem_ty) {
			case D_U64: return m_uint64x2;
			case D_I64: return m_int64x2;
			default:;
			}
		}
		return get_vec_tyid(vec_elem_num * get_dtype_bytesize(vec_elem_ty),
							vec_elem_ty);
	}

	//Return vector type, which type is :
	//	total_sz = <vec_elem_num x vec_elem_ty>.
	UINT get_vec_tyid_ex(UINT total_sz, DATA_TYPE vec_elem_ty)
	{
		if (total_sz == 64) {
			switch (vec_elem_ty) {
			case D_U16: return m_uint16x4;
			case D_I16: return m_int16x4;
			default:;
			}
		}
		if (total_sz == 128) {
			switch (vec_elem_ty) {
			case D_U32: return m_uint32x4;
			case D_I32: return m_int32x4;
			case D_U64: return m_uint64x2;
			case D_I64: return m_int64x2;
			default:;
			}
		}
		return get_vec_tyid(total_sz, vec_elem_ty);
	}

	//Register and return pointer type tyid accroding to pointer-base-size.
	UINT get_ptr_tyid(UINT pt_base_sz)
	{
		IS_TRUE0(pt_base_sz != 0);
		DTD d;
		DTD_rty(&d) = D_PTR;
		DTD_ptr_base_sz(&d) = pt_base_sz;
		return register_dtd(&d);
	}

	//Return vector type, and total_sz = <vec_elem_num x vec_elem_ty>.
	//e.g: int<16 x D_I32> means there are 16 elems in vector, each elem is
	//D_I32 type, and 'total_sz' is 64 bytes.
	UINT get_vec_tyid(UINT total_sz, DATA_TYPE vec_elem_ty)
	{
		IS_TRUE0(total_sz != 0 && vec_elem_ty != D_UNDEF);
		IS_TRUE0((total_sz % get_dtype_bytesize(vec_elem_ty)) == 0);
		DTD d;
		DTD_rty(&d) = D_MC;
		DTD_ptr_base_sz(&d) = total_sz;
		DTD_vec_ety(&d) = vec_elem_ty;
		return register_dtd(&d);
	}

	//Return memory chunk tyid accroding to chunk size and vector element tyid.
	UINT get_mc_tyid(UINT mc_sz, DATA_TYPE vec_elem_ty = D_UNDEF)
	{
		IS_TRUE0(mc_sz != 0);
		if (vec_elem_ty != D_UNDEF) {
			return get_vec_tyid(mc_sz, vec_elem_ty);
		}
		DTD d;
		DTD_rty(&d) = D_MC;
		DTD_ptr_base_sz(&d) = mc_sz;
		return register_dtd(&d);
	}

	//Return byte size according to given DTD.
	UINT get_dtd_bytesize(DTD const* dtd) const;

	//Return byte size according to given tyid.
	UINT get_dtd_bytesize(UINT tyid) const
	{ return get_dtd_bytesize(get_dtd(tyid)); }

	bool is_scalar(UINT tyid)
	{ return tyid >= D_B && tyid <= D_F128; }

	bool is_valid_dt(DATA_TYPE rt) const
	{ return rt >= D_B && rt <= D_PTR; }

	//Return true if tyid is signed.
	inline bool is_signed(UINT tyid) const
	{
		if ((tyid >= m_i8 && tyid <= m_i128) ||
			(tyid >= m_f32 && tyid <= m_f128)) {
			return true;
		}
		return false;
	}

	//Return true if tyid is signed integer.
	inline bool is_sint(UINT tyid) const
	{
		if ((tyid >= m_i8 && tyid <= m_i128)) {
			return true;
		}
		return false;
	}

	//Return true if tyid is unsigned integer.
	inline bool is_uint(UINT tyid) const
	{
		if ((tyid >= m_u8 && tyid <= m_u128)) {
			return true;
		}
		return false;
	}

	//Return true if tyid is float.
	inline bool is_fp(UINT tyid) const
	{
		if ((tyid >= m_f32 && tyid <= m_f128)) {
			return true;
		}
		return false;
	}
	bool is_bool(UINT tyid) const { return m_b == tyid; }
	bool is_str(UINT tyid) const { return m_str == tyid; }
	bool is_mem(UINT tyid) const { return DTD_is_mc(get_dtd(tyid)); }
	bool is_ptr(UINT tyid) const { return DTD_is_ptr(get_dtd(tyid)); }

	//Return true if data type is vector.
	bool is_vec(UINT tyid) const
	{ return DTD_is_vec(get_dtd(tyid)); }

	//Return true if the type indicate by 'tyid' can be used to represent the
	//pointer's addend. e.g:The pointer arith, int * p; p = p + (tyid)value.
	bool is_ptr_addend(UINT tyid)
	{ return !is_fp(tyid) && !is_mem(tyid) && !is_bool(tyid) && !is_ptr(tyid); }
};


//Record type index to facilitate type comparation.
class TYIDR {
public:
	UINT i8;
	UINT u8;
	UINT i16;
	UINT u16;
	UINT i32;
	UINT u32;
	UINT i64;
	UINT u64;
	UINT f32;
	UINT f64;
	UINT b;
	UINT ptr;
	UINT obj_lda_base_type;

	//vector.
	UINT uint16x4;
	UINT int16x4;
	UINT uint32x4;
	UINT int32x4;
	UINT uint64x2;
	UINT int64x2;
};
#endif
