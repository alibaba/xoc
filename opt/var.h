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
#ifndef __VAR_H__
#define __VAR_H__

class REGION_MGR;

//
//START VAR
//
/*
The size of VAR does not be recorded, because the memory size processed is
to be changed dynamically, so the size is related with the corresponding
IR's type that referred the VAR.
*/
//************************************************
//NOTE: Do *NOT* forget modify the bit-field in VAR if
//you remove/add flag here.
//************************************************
#define VAR_UNDEF				0x0
#define VAR_GLOBAL				0x1	//can be seen by all functions.

//This kind of variable only can be seen by current function or thread.
//It always be allocated in stack or thread local storage(TLS).
#define VAR_LOCAL				0x2

//This kind of variable can be seen by all functions in same file.
#define VAR_STATIC				0x4
#define VAR_READONLY			0x8 //var is readonly
#define VAR_VOLATILE			0x10 //var is volatile
#define VAR_HAS_INIT_VAL		0x20 //var with initialied value.
#define VAR_FAKE				0x40 //var is fake
#define VAR_FUNC_UNIT			0x80 //var is function unit.
#define VAR_FUNC_DECL			0x100 //var is function declaration.
#define VAR_IS_ARRAY			0x200 //var is array.
#define VAR_IS_FORMAL_PARAM		0x400 //var is formal parameter.
#define VAR_IS_SPILL			0x800 //var is spill location.
#define VAR_ADDR_TAKEN			0x1000 //var's address has been taken.
#define VAR_IS_PR				0x2000 //var is pr.
#define VAR_IS_RESTRICT			0x4000 //var is restrict.
#define VAR_IS_ALLOCABLE		0x8000 //var is allocable on memory.
//************************************************
//NOTE: Do *NOT* forget modify the bit-field in VAR if
//you remove/add flag here.
//************************************************

#define VAR_id(v)					((v)->id)
#define VAR_name(v)					((v)->name)
#define VAR_flag(v)					((v)->u2.flag)
#define VAR_str(v)					((v)->u1.string)
#define VAR_is_global(v)			((v)->u2.u2s1.is_global)
#define VAR_is_local(v)				((v)->u2.u2s1.is_local)
#define VAR_is_static(v)			((v)->u2.u2s1.is_static)
#define VAR_is_readonly(v)			((v)->u2.u2s1.is_readonly)
#define VAR_init_val_id(v)			((v)->u1.init_val_id)
#define VAR_has_init_val(v)			(VAR_init_val_id(v) != 0)
#define VAR_is_func_unit(v)			((v)->u2.u2s1.is_func_unit)
#define VAR_is_func_decl(v)			((v)->u2.u2s1.is_func_decl)
#define VAR_is_fake(v)				((v)->u2.u2s1.is_fake)
#define VAR_is_volatile(v)			((v)->u2.u2s1.is_volatile)
#define VAR_is_str(v)				(VAR_data_type(v) == D_STR)
#define VAR_is_pointer(v)			(VAR_data_type(v) == D_PTR)
#define VAR_is_array(v)				((v)->u2.u2s1.is_array)
#define VAR_is_formal_param(v)		((v)->u2.u2s1.is_formal_param)
#define VAR_is_spill(v)				((v)->u2.u2s1.is_spill)
#define VAR_is_addr_taken(v)		((v)->u2.u2s1.is_addr_taken)
#define VAR_is_pr(v)				((v)->u2.u2s1.is_pr)
#define VAR_is_restrict(v)			((v)->u2.u2s1.is_restrict)
#define VAR_allocable(v)			((v)->u2.u2s1.is_allocable)
#define VAR_elem_type(v)			((v)->elem_type)
#define VAR_is_vector(v)			(VAR_elem_type(v) != D_UNDEF)
#define VAR_data_type(v)			((v)->data_type)
#define VAR_data_size(v)			((v)->data_size)
#define VAR_align(v)				((v)->align)
#define VAR_pointer_base_size(v)	((v)->pointer_base_size)
class VAR {
public:
	UINT id; //unique id;
	SYM * name;
	union {
		//Record string contents if VAR is string.
		SYM * string;

		//Index to constant value table.
		//This value is 0 if no initial value.
		//Record constant content if VAR has constant initial value.
		UINT init_val_id;
	} u1;
	union {
		UINT flag; //Record variant properties of VAR.
		struct {
			UINT is_global:1; 		//VAR can be seen all program.
			UINT is_local:1; 		//VAR only can be seen in region.
			UINT is_static:1; 		//VAR only can be seen in file.
			UINT is_readonly:1;	 	//VAR is readonly.
			UINT is_volatile:1; 	//VAR is volatile.
			UINT has_init_val:1; 	//VAR has initial value.
			UINT is_fake:1; 		//VAR is fake.
			UINT is_func_unit:1; 	//VAR is function unit with body defined.
			UINT is_func_decl:1;	//VAR is function unit declaration.
			UINT is_array:1; 		//VAR is array
			UINT is_formal_param:1; //VAR is formal parameter.
			UINT is_spill:1; 		//VAR is spill location in function.
			UINT is_addr_taken:1; 	//VAR has been taken address.
			UINT is_pr:1; 			//VAR is pr.
			UINT is_restrict:1;		//VAR is restrict.

			//True if var should be allocate on memory or
			//false indicate it is only being a placeholder and do not
			//allocate in essence.
			UINT is_allocable:1;
		} u2s1;
	} u2;
	DATA_TYPE data_type; //basic type.
	DATA_TYPE elem_type; //vector-element type.

	//Base size of pointer. e.g:given double*, base size is 8bytes.
	UINT pointer_base_size;
	UINT data_size; //total size of current var.
	UINT align; //memory alignment of var.

	VAR();
	virtual ~VAR() {}
	virtual CHAR * dump_var_decl(OUT CHAR * buf, UINT buflen) { return NULL; }
	virtual void dump(FILE * h = NULL);
	virtual CHAR * dump(CHAR * buf);
};
//END VAR

typedef TMAP_ITER<UINT, VAR*> VTMAP_ITER;
typedef TAB_ITER<VAR*> VTAB_ITER;


class COMPARE_VAR {
public:
	bool is_less(VAR * t1, VAR * t2) const
	{
		#ifdef _DEBUG_
		return VAR_id(t1) < VAR_id(t2);
		#else
		return t1 < t2;
		#endif
	}

	bool is_equ(VAR * t1, VAR * t2) const
	{
		#ifdef _DEBUG_
		return VAR_id(t1) == VAR_id(t2);
		#else
		return t1 == t2;
		#endif
	}
};


class VAR_TAB : public TTAB<VAR*, COMPARE_VAR> {
public:
	void dump()
	{
		if (g_tfile == NULL) { return; }
		VTAB_ITER iter;
		for (VAR * v = get_first(iter); v != NULL; v = get_next(iter)) {
			v->dump(g_tfile);
		}
	}
};


//Map from SYM to VAR.
typedef TMAP<SYM*, VAR*> STR2VAR;

//Map from const SYM to VAR.
typedef TMAP<SYM const*, VAR*> CSTR2VAR;

//Map from VAR id to VAR.
//typedef TMAP<UINT, VAR*> ID2VAR;
typedef SVECTOR<VAR*> ID2VAR;

class VAR_MGR {
	size_t m_var_count;
	ID2VAR m_var_vec;
	STR2VAR m_str_tab;
	size_t m_str_count;
	LIST<UINT> m_freelist_of_varid;
	REGION_MGR * m_ru_mgr;
	DT_MGR * m_dm;

	inline void assign_var_id(VAR * v);
public:
	VAR_MGR(REGION_MGR * rm);
	virtual ~VAR_MGR() { destroy(); }
	void destroy()
	{
		for (INT i = 0; i <= m_var_vec.get_last_idx(); i++) {
			VAR * v = m_var_vec.get(i);
			if (v == NULL) { continue; }
			delete v;
		}
	}

	void dump(IN OUT CHAR * name = NULL);
	inline VAR * get_var(UINT id) { return m_var_vec.get(id); }
	inline ID2VAR * get_var_vec() { return &m_var_vec; }
	inline VAR * find_str_var(SYM * str)
	{
		VAR * v = m_str_tab.get(str);
		IS_TRUE0(v || VAR_is_str(v));
		return v;
	}

	virtual VAR * new_var()	{ return new VAR(); }

	//Free v's memory.
	inline void remove_var(VAR * v)
	{
		IS_TRUE0(VAR_id(v) != 0);
		m_freelist_of_varid.append_head(VAR_id(v));
		m_var_vec.set(VAR_id(v), NULL);
		delete v;
	}

	VAR * register_var(SYM * var_name, DATA_TYPE dt,
					   DATA_TYPE elem_type, UINT pointer_base_size,
					   UINT mem_size, UINT align, UINT flag);
	VAR * register_var(IN CHAR const* var_name,
					   DATA_TYPE dt, DATA_TYPE elem_type,
					   UINT pointer_base_size, UINT mem_size,
					   UINT align, UINT flag);
	VAR * register_var(SYM * var_name, UINT tyid, UINT align, UINT flag);
	VAR * register_var(CHAR const* var_name, UINT tyid, UINT align, UINT flag);
	VAR * register_str(IN CHAR const* var_name, IN SYM * s, UINT align);
};
#endif
