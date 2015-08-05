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
#ifndef __DEX_TO_IR_H__
#define __DEX_TO_IR_H__

#define VARD_ci(vd)		((vd)->class_idx)
#define VARD_fi(vd)		((vd)->class_idx)
#define VARD_var(vd)	((vd)->var)
class VARD {
public:
	UINT class_idx;
	UINT field_idx;
	VAR * var;
};


//Map from LIR to LABEL.
typedef TMAP<LIR*, LIST<LABEL_INFO*>*> LIR2LABS;


class CATCH_INFO {
public:
	CATCH_INFO * prev;
	CATCH_INFO * next;
	LABEL_INFO * catch_start;
	UINT kind;
};


class TRY_INFO {
public:
	TRY_INFO * prev;
	TRY_INFO * next;
	LABEL_INFO * try_start;
	LABEL_INFO * try_end;
	CATCH_INFO * catch_list;
};


typedef enum _CMP_KIND {
	CMP_UNDEF = 0,
	CMPL_FLOAT,
	CMPG_FLOAT,
	CMPL_DOUBLE,
	CMPG_DOUBLE,
	CMP_LONG,
} CMP_KIND;

#define DEX_DEFAULT_ARRAY_TYPE	1
#define DEX_DEFAULT_OBJ_TYPE	2
#define DEX_USER_TYPE_START		3

//#define DIV_REM_MAY_THROW

class CMP_STR {
public:
	bool is_less(CHAR const* t1, CHAR const* t2) const
	{ return strcmp(t1, t2) < 0; }

	bool is_equ(CHAR const* t1, CHAR const* t2) const
	{ return strcmp(t1, t2) == 0; }
};

typedef TMAP<CHAR const*, UINT, CMP_STR> STR2UINT;

//In Actually, it does work to convert ANA IR to IR, but is not DEX.
//To wit, the class declared in class LIR2IR, that will be better.
class DEX2IR {
protected:
	REGION * m_ru;
	REGION_MGR * m_ru_mgr;
	DT_MGR * m_dm;
	DexFile * m_df;
	VAR_MGR * m_vm;
	LIRCode * m_fu;
	TYIDR const* m_tr;
	SMEM_POOL * m_pool;
	VAR2UINT m_map_var2ofst;
	VAR2UINT m_map_var2blt;
	LIR2LABS m_lir2labs;
	UINT m_tyid_count;
	STR2UINT m_typename2tyid;

	UINT m_ptr_addend;
	UINT m_ofst_addend;

	//Map from typeIdx of type-table to
	//positionIdx in file-class-def-table.
	TYID2POSID m_map_tyid2posid;
	CSTR2VAR m_str2var;
	TRY_INFO * m_ti;
	bool m_has_catch; //Set to true if region has catch block.
	LIST<LABEL_INFO*> m_last_try_end_lab_list;

	void * xmalloc(INT size)
	{
		IS_TRUE(size > 0, ("xmalloc: size less zero!"));
		IS_TRUE(m_pool != NULL, ("need pool!!"));
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p);
		memset(p, 0, size);
		return p;
	}
public:
	UINT2PR m_v2pr; //map from dex register v to IR_PR node.
	PRNO2UINT m_pr2v; //map from dex register v to IR_PR node.

	DEX2IR(IN REGION * ru, IN DexFile * df,
		   IN LIRCode * fu, IN TYIDR const* tr)
	{
		IS_TRUE0(ru && df && fu);
		m_ru = ru;
		m_ru_mgr = ru->get_ru_mgr();
		m_dm = ru->get_dm();
		m_vm = ru->get_var_mgr();
		m_df = df;
		m_fu = fu;
		m_tr = tr;
		m_ti = NULL;
		m_pool = smpool_create_handle(16, MEM_COMM);
		m_pr2v.init(MAX(4, get_nearest_power_of_2(fu->maxVars)));
		m_tyid_count = DEX_USER_TYPE_START;
		m_ptr_addend = m_dm->get_simplex_tyid(D_U32);
		m_ofst_addend = m_dm->get_dtype_bytesize(D_I64);
		m_pr2v.maxreg = fu->maxVars - 1;
		m_pr2v.paramnum = fu->numArgs;
	}

	~DEX2IR()
	{
		TMAP_ITER<LIR*, LIST<LABEL_INFO*>*> iter;
		LIST<LABEL_INFO*> * l;
		for (m_lir2labs.get_first(iter, &l);
			 l != NULL; m_lir2labs.get_next(iter, &l)) {
			delete l;
		}

		smpool_free_handle(m_pool);
	}

	VAR * add_var_by_name(CHAR const* name, UINT tyid);
	VAR * add_class_var(UINT class_id, LIR * lir);
	VAR * add_string_var(CHAR const* string);
	VAR * add_field_var(UINT field_id, UINT tyid);
	VAR * add_static_var(UINT field_id, UINT tyid);
	VAR * add_func_var(UINT method_id, UINT tyid);

	UINT compute_class_posid(UINT cls_id_in_tytab);
	UINT compute_field_ofst(UINT field_id);
	IR * convert_array_length(IN LIR * lir);
	IR * convert_load_string_addr(IN LIR * lir);
	IR * convert_cvt(IN LIR * lir);
	IR * convert_check_cast(IN LIR * lir);
	IR * convert_cond_br(IN LIR * lir);
	IR * convert_const_class(IN LIR * lir);
	IR * convert_filled_new_array(IN LIR * lir);
	IR * convert_fill_array_data(IN LIR * lir);
	IR * convert_goto(IN LIR * lir);
	IR * convert_load_const(IN LIR * lir);
	IR * convert_move_result(IN LIR * lir);
	IR * convert_move(IN LIR * lir);
	IR * convert_throw(IN LIR * lir);
	IR * convert_move_exception(IN LIR * lir);
	IR * convert_monitor_exit(IN LIR * lir);
	IR * convert_monitor_enter(IN LIR * lir);
	IR * convert_invoke(IN LIR * lir);
	IR * convert_instance_of(IN LIR * lir);
	IR * convert_new_instance(IN LIR * lir);
	IR * convert_packed_switch(IN LIR * lir);
	IR * convert_sparse_switch(IN LIR * lir);
	IR * convert_aput(IN LIR * lir);
	IR * convert_aget(IN LIR * lir);
	IR * convert_sput(IN LIR * lir);
	IR * convert_sget(IN LIR * lir);
	IR * convert_iput(IN LIR * lir);
	IR * convert_iget(IN LIR * lir);
	IR * convert_cmp(IN LIR * lir);
	IR * convert_unary_op(IN LIR * lir);
	IR * convert_bin_op_assign(IN LIR * lir);
	IR * convert_bin_op(IN LIR * lir);
	IR * convert_bin_op_lit(IN LIR * lir);
	IR * convert_ret(IN LIR * lir);
	IR * convert_new_array(IN LIR * lir);
	IR * convert(IN LIR * lir);
	IR * convert(bool * succ);

	void dump_lir2lab();

	UINT get_ofst_addend() const { return m_ofst_addend; }
	CHAR const* get_var_type_name(UINT field_id);
	UINT get_dexopcode(UINT flag);
	UINT get_dt_tyid(LIR * ir);
	IR * gen_mapped_pr(UINT vid, UINT tyid);
	UINT gen_mapped_prno(UINT vid, UINT tyid);
	inline PRNO2UINT * get_pr2v_map() { return &m_pr2v; }
	inline UINT2PR * get_v2pr_map() { return &m_v2pr; }
	inline VAR2UINT * get_var2ofst_map() { return &m_map_var2ofst; }
	inline VAR2UINT * get_var2blt_map() { return &m_map_var2blt; }
	inline TRY_INFO * get_try_info() { return m_ti; }

	bool is_readonly(CHAR const* method_name) const;
	bool has_catch() const { return m_has_catch; }

	UINT map_type2tyid(UINT field_id);
	void mark_lab();
	void set_map_v2blt(VAR * v, BLTIN_TYPE b);
	void set_map_v2ofst(VAR * v, UINT ofst);
};

#endif
