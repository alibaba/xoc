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
#include "anablock.h"
#include "anatypes.h"
#include "anaopcode.h"
#include "ananode.h"
#include "anadot.h" //for debug
#include "anaglobal.h"
#include "anamethod.h"
#include "anamem.h"
#include "anautility.h"
#include "anaprofile.h"

#include "libdex/DexFile.h"
#include "libdex/DexClass.h"
#include "libdex/DexOpcodes.h"
#include "liropcode.h"

#include "../opt/cominc.h"

#include "tcomf.h"
#include "anacomf.h"
#include "dx_mgr.h"
#include "../opt/prdf.h"
#include "dex.h"
#include "gra.h"
#include "dex_util.h"
#include "dex2ir.h"

inline static bool is_array_type(CHAR const* type_name)
{
	IS_TRUE0(type_name);
	return *type_name == '[';
}


inline static bool is_obj_type(CHAR const* type_name)
{
	IS_TRUE0(type_name);
	return *type_name == 'L';
}


//Add a variable of CLASS.
VAR * DEX2IR::add_var_by_name(CHAR const* name, UINT tyid)
{
	SYM * sym = m_ru_mgr->add_to_symtab(name);
	VAR * v = m_str2var.get(sym);
	if (v != NULL) { return v; }
	UINT flag = 0;
	SET_FLAG(flag, VAR_GLOBAL);
	v = m_vm->register_var(sym, tyid, 0, flag);
	m_str2var.set(sym, v);
	return v;
}


VAR * DEX2IR::add_string_var(CHAR const* string)
{
	SYM * sym = m_ru_mgr->add_to_symtab(string);
	VAR * v = m_str2var.get(sym);
	if (v != NULL) { return v; }
	v = m_vm->register_str(NULL, sym, 0);
	m_str2var.set(sym, v);
	return v;
}


//Add a variable of CLASS.
VAR * DEX2IR::add_class_var(UINT class_id, LIR * lir)
{
	DexClassDef const* class_info = dexGetClassDef(m_df, class_id);
	IS_TRUE0(class_info);
	CHAR const* class_name = dexStringByTypeIdx(m_df, class_info->classIdx);
	IS_TRUE0(class_name);
	return add_var_by_name(class_name, get_dt_tyid(lir));
}


/*
Add a variable of CLASS + FIELD.

'field_id': the global index in class-defining-tab of dexfile.
e.g: Given field_id is 6, the class-defining-tab is:
	class LSwitch; {
		 0th, field_1, TYPE:I
		 1th, field_2, TYPE:S
		 2th, field_3, TYPE:C
		 3th, field_4, TYPE:D
	}
	class LTT; {
		 4th, tt_f1, TYPE:I
		 5th, tt_f2, TYPE:I
	}
	class LUI; {
		 6th, u1, TYPE:S
		 7th, u2, TYPE:J
	}

	Field LUI;::u1 is the target.
*/
VAR * DEX2IR::add_field_var(UINT field_id, UINT tyid)
{
	DexFieldId const* fid = dexGetFieldId(m_df, field_id);
	IS_TRUE0(fid);
	CHAR const* class_name = dexStringByTypeIdx(m_df, fid->classIdx);
	CHAR const* field_name = get_field_name(m_df, field_id);
	IS_TRUE0(class_name && field_name);
	UINT cls_pos_id = compute_class_posid(fid->classIdx);
	DexClassDef const* class_info = dexGetClassDef(m_df, cls_pos_id);
	IS_TRUE0(class_info);
	CHAR buf[256];
	CHAR * pbuf = buf;
	UINT l = strlen(class_name) + strlen(field_name) + 10;
	if (l >= 256) {
		pbuf = (CHAR*)ALLOCA(l);
		IS_TRUE0(pbuf);
	}
	sprintf(pbuf, "%s::%s", class_name, field_name);
	return add_var_by_name(pbuf, tyid);
}


CHAR const* DEX2IR::get_var_type_name(UINT field_id)
{
	DexFieldId const* fid = dexGetFieldId(m_df, field_id);
	IS_TRUE0(fid);
	return dexStringByTypeIdx(m_df, fid->typeIdx);
}


VAR * DEX2IR::add_static_var(UINT field_id, UINT tyid)
{
	DexFieldId const* fid = dexGetFieldId(m_df, field_id);
	IS_TRUE0(fid);
	CHAR const* type_name = dexStringByTypeIdx(m_df, fid->typeIdx);
	CHAR const* class_name = dexStringByTypeIdx(m_df, fid->classIdx);
	CHAR const* field_name = get_field_name(m_df, field_id);
	IS_TRUE0(class_name && field_name && type_name);
	CHAR buf[256];
	CHAR * pbuf = buf;
	UINT l = strlen(class_name) + strlen(field_name) + strlen(type_name) + 10;
	if (l >= 256) {
		pbuf = (CHAR*)ALLOCA(l);
	}
	sprintf(pbuf, "%s::%s:%s", class_name, field_name, type_name);
	return add_var_by_name(pbuf, tyid);
}


static CHAR const* g_readonly_method [] = {
	"cos",
	"sin",
	"asin",
	"acos",
	"tan",
	"atan",
	"println",
};
UINT const g_readonly_method_num =
	sizeof(g_readonly_method) / sizeof(g_readonly_method[0]);


bool DEX2IR::is_readonly(CHAR const* method_name) const
{
	for (UINT i = 0; i < g_readonly_method_num; i++) {
		if (strcmp(method_name, g_readonly_method[i]) == 0) {
			return true;
		}
	}
	return false;
}


//'method_id': the global index in method-defining-tab of dexfile.
VAR * DEX2IR::add_func_var(UINT method_id, UINT tyid)
{
	DexMethodId const* mid = dexGetMethodId(m_df, method_id);
	IS_TRUE0(mid);
	CHAR const* class_name = dexStringByTypeIdx(m_df, mid->classIdx);
	CHAR const* method_name = dexStringById(m_df, mid->nameIdx);
	IS_TRUE0(class_name && method_name);
	CHAR buf[256];
	CHAR * pbuf = buf;
	UINT l = strlen(class_name) + strlen(method_name) + 10;
	if (l >= 256) {
		pbuf = (CHAR*)ALLOCA(l);
		IS_TRUE0(pbuf);
	}
	sprintf(pbuf, "%s::%s", class_name, method_name);
	VAR * v = add_var_by_name(pbuf, tyid);
	VAR_is_readonly(v) = is_readonly(method_name);
	return v;
}


/*
Return type-id.
Client Hotspot JVM on Windows.
Type					Size (bytes)
java.lang.Object		8
java.lang.Float			16
java.lang.Double		16
java.lang.Integer		16
java.lang.Long			16
java.math.BigInteger	56 (*)
java.lang.BigDecimal	72 (*)
java.lang.String		2*(Length) + 38(+/-)2
empty java.util.Vector	80
object reference		4
float array				4*(Length) + 14(+/-)2

For common 32-bit JVMs specifications a plain java.lang.
Object takes up 8 bytes, and the basic data types are
usually of the least physical size that can accommodate
the language requirements (except boolean takes up a whole byte):
java.lang.Object shell size in bytes:
public static final int OBJECT_SHELL_SIZE   = 8;
public static final int OBJREF_SIZE         = 4;
public static final int LONG_FIELD_SIZE     = 8;
public static final int INT_FIELD_SIZE      = 4;
public static final int SHORT_FIELD_SIZE    = 2;
public static final int CHAR_FIELD_SIZE     = 2;
public static final int BYTE_FIELD_SIZE     = 1;
public static final int BOOLEAN_FIELD_SIZE  = 1;
public static final int DOUBLE_FIELD_SIZE   = 8;
public static final int FLOAT_FIELD_SIZE    = 4;
*/
UINT DEX2IR::get_dt_tyid(LIR * ir)
{
	switch (LIR_dt(ir)) {
	case LIR_JDT_unknown:
		IS_TRUE0(0);
	case LIR_JDT_void	:
		return m_tr->i32;
	case LIR_JDT_int	:
		/*
		a Java int is 32 bits in all JVMs and on all platforms,
		but this is only a language specification requirement for the
		programmer-perceivable width of this data type.
		*/
		return m_tr->i32;
	case LIR_JDT_object	:
		return m_tr->ptr;
	case LIR_JDT_boolean:
		return m_tr->b;
	case LIR_JDT_byte	:
		return m_tr->u8;
	case LIR_JDT_char	:
		return m_tr->i8;
	case LIR_JDT_short	:
		return m_tr->i16;
	case LIR_JDT_float	:
		return m_tr->f32;
	case LIR_JDT_none	:
		return m_tr->u32;
	case LIR_wide:
		return m_tr->u64;
	case LIR_JDT_wide:
		return m_tr->u64;
	case LIR_JDT_long:
		return m_tr->i64;
	case LIR_JDT_double:
		return m_tr->f64;
	default: IS_TRUE0(0);
	}
	return 0;
}


void DEX2IR::set_map_v2blt(VAR * v, BLTIN_TYPE b)
{
	bool find;
	UINT k = m_map_var2blt.get(v, &find);
	if (!find) {
		m_map_var2blt.set(v, b);
	} else {
		IS_TRUE0(k == b);
	}
}


void DEX2IR::set_map_v2ofst(VAR * v, UINT ofst)
{
	bool find;
	UINT k = m_map_var2ofst.get(v, &find);
	if (!find) {
		m_map_var2ofst.set(v, ofst);
	} else {
		IS_TRUE0(k == ofst);
	}
}


//AABBBB
IR * DEX2IR::convert_sget(IN LIR * lir)
{
	//vA(res) <- field_id(op0)",
	UINT tyid = get_dt_tyid(lir);
	IR * res = gen_mapped_pr(LIR_res(lir), tyid);

	/*
	Identical PR may be reused by designating different typeid.
	e.g: PR1(D_PTR:24)
		 ...
		 PR1(D_U32)
		 ...
	TODO: Generate new PR instead of change the tyid.
	*/
	//VAR * v = add_static_var(LIR_op0(lir), m_tr->obj_lda_base_type);
	VAR * v = add_static_var(LIR_op0(lir), get_dt_tyid(lir));
	set_map_v2ofst(v, LIR_op0(lir));
	IR * rhs = NULL;
	/*
	if (LIR_dt(lir) == LIR_JDT_object) {
		IR * id = m_ru->build_id(v);
		rhs = m_ru->build_lda(id);
	} else {
		rhs = m_ru->build_load(v);
	}
	*/
	rhs = m_ru->build_load(v);
	IR_AI * ai = m_ru->new_ai();

	TBAA_AI * tbaa = (TBAA_AI*)xmalloc(sizeof(TBAA_AI));
	tbaa->init(IRAI_TBAA);
	tbaa->tyid = map_type2tyid(LIR_op0(lir));
	ai->set(IRAI_TBAA, tbaa);
	IR_ai(rhs) = ai;

	return m_ru->build_store_pr(PR_no(res), IR_dt(res), rhs);
}


UINT DEX2IR::map_type2tyid(UINT field_id)
{
	CHAR const* type_name = get_var_type_name(field_id);
	IS_TRUE0(type_name);
	UINT id = m_typename2tyid.get(type_name);
	if (id != 0) { return id; }

	id = m_tyid_count++;
	m_typename2tyid.set(type_name, id);
	return id;
}


//AABBBB
IR * DEX2IR::convert_sput(IN LIR * lir)
{
	//vA(res) -> field_id(op0)"
	IR * res = gen_mapped_pr(LIR_res(lir), get_dt_tyid(lir));

	/*
	Identical PR may be reused by designating different typeid.
	e.g: PR1(D_PTR:24)
		 ...
		 PR1(D_U32)
		 ...
	TODO: Generate new PR instead of change the tyid.
	*/
	IR_dt(res) = get_dt_tyid(lir);
	IS_TRUE0(IR_dt(res) != 0);
	VAR * v = add_static_var(LIR_op0(lir), get_dt_tyid(lir));
	set_map_v2ofst(v, LIR_op0(lir));
	return m_ru->build_store(v, res);
}


//AABBCC
//aput, OBJ, v0 -> (array_base_ptr)v1, (array_elem)v2
IR * DEX2IR::convert_aput(IN LIR * lir)
{
	UINT tyid = get_dt_tyid(lir);
	//st-val
	IR * res = gen_mapped_pr(LIR_res(lir), tyid);
	//array
	IR * base = gen_mapped_pr(LIR_op0(lir), m_tr->ptr);
	IR * ofst = gen_mapped_pr(LIR_op1(lir), m_tr->u32);
	TMWORD enbuf = 0;
	IR * array = m_ru->build_array(base, ofst, tyid, tyid, 1, &enbuf);

	//Even if array operation may throw exception in DEX, we still
	//set the may-throw property at its stmt but is not itself.
	//IR_may_throw(array) = true;

	//base type info.
	IR_AI * ai = m_ru->new_ai();
	TBAA_AI * tbaa = (TBAA_AI*)xmalloc(sizeof(TBAA_AI));
	tbaa->init(IRAI_TBAA);
	tbaa->tyid = DEX_DEFAULT_ARRAY_TYPE;
	ai->set(IRAI_TBAA, tbaa);
	IR_ai(base) = ai;

	IR * c = m_ru->build_istore(array, res, tyid);
	IR_may_throw(c) = true;
	if (m_has_catch) {
		IR * lab = m_ru->build_label(m_ru->gen_ilab());
		add_next(&c, lab);
	}
	return c;
}


//AABBCC
//aget, OBJ, v0 <- (array_base_ptr)v1, (array_elem)v2
IR * DEX2IR::convert_aget(IN LIR * lir)
{
	UINT tyid = get_dt_tyid(lir);
	//st-val
	IR * res = gen_mapped_pr(LIR_res(lir), tyid);

	//array
	IR * base = gen_mapped_pr(LIR_op0(lir), m_tr->ptr);
	IR * ofst = gen_mapped_pr(LIR_op1(lir), m_tr->u32);
	TMWORD enbuf = 0;
	IR * array = m_ru->build_array(base, ofst, tyid, tyid, 1, &enbuf);

	//Even if array operation may throw exception in DEX, we still
	//set the may-throw property at its stmt but is not itself.
	//IR_may_throw(array) = true;

	//base type info.
	IR_AI * ai = m_ru->new_ai();
	TBAA_AI * tbaa = (TBAA_AI*)xmalloc(sizeof(TBAA_AI));
	tbaa->init(IRAI_TBAA);
	tbaa->tyid = DEX_DEFAULT_ARRAY_TYPE;
	ai->set(IRAI_TBAA, tbaa);
	IR_ai(base) = ai;

	IR * c = m_ru->build_store_pr(PR_no(res), IR_dt(res), array);
	IR_may_throw(c) = true;
	if (m_has_catch) {
		IR * lab = m_ru->build_label(m_ru->gen_ilab());
		add_next(&c, lab);
	}
	return c;
}


//ABCCCC
IR * DEX2IR::convert_iput(IN LIR * lir)
{
	/*
	class UI {
		public UI(short x) { u1=x; }
		short u1;
		long u2;
	}
	iput-short v1, v0, LUI;.u1:S // field@0006

	v%d(res) -> v%d(op0, thiptr), field_id(op1)",
	*/
	IR * stv = gen_mapped_pr(LIR_res(lir), get_dt_tyid(lir));

	/*
	Identical PR may be reused by designating different typeid.
	e.g: PR1(D_PTR:24)
		 ...
		 PR1(D_U32)
		 ...
	TODO: Generate new PR instead of change the tyid.
	*/
	IR_dt(stv) = get_dt_tyid(lir);
	IS_TRUE0(IR_dt(stv) != 0);

	//Get 'this' pointer
	IR * addr = gen_mapped_pr(LIR_op0(lir), m_tr->ptr);

	/*
	UINT ofst = compute_field_ofst(LIR_op1(lir));
	IR * addr = m_ru->build_binary_op(IR_ADD,
								IR_dt(thisptr),
								thisptr,
								m_ru->build_imm_int(ofst, m_ptr_addend));
	*/
	IR * c = m_ru->build_istore(addr, stv, IR_dt(stv));
	IST_ofst(c) = LIR_op1(lir) * m_ofst_addend;

	IR_may_throw(c) = true;
	if (m_has_catch) {
		IR * lab = m_ru->build_label(m_ru->gen_ilab());
		add_next(&c, lab);
	}
	return c;
}


/*
AABBCC
cmpkind vAA <- vBB, vCC

DEX will be converted to:
	IR_CALL
		param0: cmp-kind.
		param1: vBB
		param2: vCC
		res: vAA
*/
IR * DEX2IR::convert_cmp(IN LIR * lir)
{
	//see d2d_l2d.c
	CMP_KIND ck = CMP_UNDEF;
	switch (LIR_opcode(lir)) {
	case LOP_CMPL:
		if (LIR_dt(lir) == LIR_CMP_float) {
			ck = CMPL_FLOAT;
		} else {
			IS_TRUE0(LIR_dt(lir) == LIR_CMP_double);
			ck = CMPL_DOUBLE;
		}
		break;
	case LOP_CMP_LONG:
		ck = CMP_LONG;
		break;
	case LOP_CMPG:
		if (LIR_dt(lir) == LIR_CMP_float) {
			ck = CMPG_FLOAT;
		} else {
			IS_TRUE0(LIR_dt(lir) == LIR_CMP_double);
			ck = CMPG_DOUBLE;
		}
		break;
	default: IS_TRUE0(0);
	}

	UINT tyid = 0;
	switch (ck) {
	case CMPL_FLOAT: tyid = m_tr->f32; break;
	case CMPG_FLOAT: tyid = m_tr->f32; break;
	case CMPL_DOUBLE: tyid = m_tr->f64; break;
	case CMPG_DOUBLE: tyid = m_tr->f64; break;
	case CMP_LONG: tyid = m_tr->i64; break;
	default: IS_TRUE0(0);
	}

	//Generate callee.
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_CMP_BIAS), m_tr->i32);
	set_map_v2blt(v, BLTIN_CMP_BIAS);
	IR * last = NULL;
	IR * p = NULL;

	//The first parameter is used to record cmp-kind.
	IR * kind = m_ru->build_imm_int(ck, m_tr->u32);
	add_next(&p, &last, kind);

	//The second parameter is opnd0 reigster.
	IR * r = gen_mapped_pr(LIR_op0(lir), tyid);
	add_next(&p, &last, r);

	//The third parameter is opnd1 reigster.
	r = gen_mapped_pr(LIR_op1(lir), tyid);
	add_next(&p, &last, r);

	IR * c = m_ru->build_call(v, p,
							gen_mapped_prno(LIR_res(lir), m_tr->i32),
							m_tr->i32);
	CALL_is_readonly(c) = true;
	IR_has_sideeffect(c) = false;
	CALL_is_intrinsic(c) = true;
	//Not throw exception.
	return c;
}


//cls_id_in_tytab: typeIdx into type-table for defining class.
UINT DEX2IR::compute_class_posid(UINT cls_id_in_tytab)
{
	bool find = false;
	UINT posid = m_map_tyid2posid.get(cls_id_in_tytab, &find);
	if (find) {
		return posid;
	}
	for (UINT i = 0; i < m_df->pHeader->classDefsSize; i++) {
		//Show section header.
		//dumpClassDef(pDexFile, i);

		//Dump class name.
		DexClassDef const* class_info = dexGetClassDef(m_df, i);
		IS_TRUE0(class_info);
		if (class_info->classIdx == cls_id_in_tytab) {
			m_map_tyid2posid.set(cls_id_in_tytab, i);
			return i;
		}
	}
	CHAR const* class_name = dexStringByTypeIdx(m_df, cls_id_in_tytab);
	IS_TRUE0(class_name);
	IS_TRUE(0, ("Not find!"));
	return 0;
}


/* 'field_id': the global index in class-defining-tab of dexfile.
e.g: Given field_id is 6, the class-defining-tab is:
	class LSwitch; {
		 0th, field_1, TYPE:INT
		 1th, field_2, TYPE:SHORT
		 2th, field_3, TYPE:CHAR
		 3th, field_4, TYPE:DOUBLE
	}
	class LTT; {
		 4th, tt_f1, TYPE:INT
		 5th, tt_f2, TYPE:LONG
	}
	class LUI; {
		 6th, u1, TYPE:SHORT
		 7th, u2, TYPE:OBJ
	}

	Field LUI;::u1 is the target.
*/
UINT DEX2IR::compute_field_ofst(UINT field_id)
{
	DexFieldId const* fid = dexGetFieldId(m_df, field_id);
	UINT cls_pos_id = compute_class_posid(fid->classIdx);
	DexClassDef const* class_info = dexGetClassDef(m_df, cls_pos_id);
	IS_TRUE0(class_info);
	CHAR const* class_name = dexStringByTypeIdx(m_df, fid->classIdx);
	CHAR const* field_name = get_field_name(m_df, field_id);

	//Get class-definition.
	BYTE const* encoded_data = dexGetClassData(m_df, class_info);
	DexClassData * class_data = dexReadAndVerifyClassData(&encoded_data, NULL);
	for (UINT i = 0; i < class_data->header.instanceFieldsSize; i++) {
		DexField * finfo = &class_data->instanceFields[i];
		if (finfo->fieldIdx == field_id) {
			return i * m_dm->get_dtype_bytesize(D_I64);
		}
		//dumpf_field(df, finfo, 4);
	}
	IS_TRUE0(0);
	return 0;
}


IR * DEX2IR::convert_iget(IN LIR * lir)
{
	//ABCCCC
	//v%d(res) <- v%d(op0), field_id(op1)",
	UINT tyid = get_dt_tyid(lir);
	IR * res = gen_mapped_pr(LIR_res(lir), tyid);

	//Get object pointer
	IR * obj = gen_mapped_pr(LIR_op0(lir), m_tr->ptr);

	/* How to describe the field offset of object-ptr?
	Since the field-idx(in LIR_op1(lir)) is unique in whole
	dex file, we can use it as offset value. Although it
	looks like a little bit tricky approach, but it works.
	UINT ofst = compute_field_ofst(LIR_op1(lir)); */
	UINT objofst = LIR_op1(lir) * m_ofst_addend;

	/*
	IR * addr = m_ru->build_binary_op(
							IR_ADD,
							IR_dt(obj),
							obj,
							m_ru->build_imm_int(objofst, m_ptr_addend));
	IR * ild = m_ru->build_iload(addr, get_dt_tyid(lir));
	*/

	IR * ild = m_ru->build_iload(obj, tyid);
	ILD_ofst(ild) = objofst; //ILD(MEM_ADDR+ofst)
	IR_may_throw(ild) = true;
	IR * c = m_ru->build_store_pr(PR_no(res), IR_dt(res), ild);

	IR_may_throw(c) = true;
	if (m_has_catch) {
		IR * lab = m_ru->build_label(m_ru->gen_ilab());
		add_next(&c, lab);
	}

	IR_AI * ai = m_ru->new_ai();
	TBAA_AI * tbaa = (TBAA_AI*)xmalloc(sizeof(TBAA_AI));
	tbaa->init(IRAI_TBAA);
	//tbaa->tyid = map_type2tyid(DEX_DEFAULT_OBJ_TYPE);
	tbaa->tyid = DEX_DEFAULT_OBJ_TYPE;
	ai->set(IRAI_TBAA, tbaa);
	IR_ai(obj) = ai;

	CHAR const* type_name = get_var_type_name(LIR_op1(lir));
	if (is_array_type(type_name)) {
		//The type of result value of ild is pointer to array type.
		IR_AI * ai = m_ru->new_ai();
		TBAA_AI * tbaa = (TBAA_AI*)xmalloc(sizeof(TBAA_AI));
		tbaa->init(IRAI_TBAA);
		tbaa->tyid = DEX_DEFAULT_ARRAY_TYPE;
		ai->set(IRAI_TBAA, tbaa);
		IR_ai(ild) = ai;
	} else if (is_obj_type(type_name)) {
		//The type of result value of ild is pointer to object type.
		IR_AI * ai = m_ru->new_ai();
		TBAA_AI * tbaa = (TBAA_AI*)xmalloc(sizeof(TBAA_AI));
		tbaa->init(IRAI_TBAA);
		tbaa->tyid = DEX_DEFAULT_OBJ_TYPE;
		ai->set(IRAI_TBAA, tbaa);
		IR_ai(ild) = ai;
	}

	return c;
}


//AABBCC
IR * DEX2IR::convert_bin_op_assign(IN LIR * lir)
{
	UINT tyid = get_dt_tyid(lir);
	UINT tyid2 = tyid;
	IR_TYPE ir_ty = IR_UNDEF;
	switch (LIR_opcode(lir)) {
	case LOP_ADD_ASSIGN : ir_ty = IR_ADD; break;
	case LOP_SUB_ASSIGN : ir_ty = IR_SUB; break;
	case LOP_MUL_ASSIGN : ir_ty = IR_MUL; break;
	case LOP_DIV_ASSIGN : ir_ty = IR_DIV; break;
	case LOP_REM_ASSIGN : ir_ty = IR_REM; break;
	case LOP_AND_ASSIGN : ir_ty = IR_BAND; break;
	case LOP_OR_ASSIGN  : ir_ty = IR_BOR; break;
	case LOP_XOR_ASSIGN : ir_ty = IR_XOR; break;
	case LOP_SHL_ASSIGN : ir_ty = IR_LSL;
		tyid2 = m_dm->get_simplex_tyid_ex(D_U32); break;
	case LOP_SHR_ASSIGN : ir_ty = IR_ASR;
		tyid2 = m_dm->get_simplex_tyid_ex(D_U32); break;
	case LOP_USHR_ASSIGN: ir_ty = IR_LSR;
		tyid2 = m_dm->get_simplex_tyid_ex(D_U32); break;
	default: IS_TRUE0(0);
	}
	IR * res = gen_mapped_pr(LIR_res(lir), tyid);
	IR * op0 = gen_mapped_pr(LIR_op0(lir), tyid2);
	IR * x = m_ru->build_binary_op(ir_ty, tyid, res, op0);
	IR * c = m_ru->build_store_pr(PR_no(res), IR_dt(res), x);
	if (ir_ty == IR_DIV || ir_ty == IR_REM) {
		#ifdef DIV_REM_MAY_THROW
		IR_may_throw(x) = true;
		IR_may_throw(c) = true;
		#endif
		if (m_has_catch) {
			IR * lab = m_ru->build_label(m_ru->gen_ilab());
			add_next(&c, lab);
		}
	}
	return c;
}


//AABBCC
IR * DEX2IR::convert_bin_op(IN LIR * lir)
{
	UINT tyid = get_dt_tyid(lir);
	IR_TYPE ir_ty = IR_UNDEF;
	switch (LIR_opcode(lir)) {
	case LOP_ADD : ir_ty = IR_ADD; break;
	case LOP_SUB : ir_ty = IR_SUB; break;
	case LOP_MUL : ir_ty = IR_MUL; break;
	case LOP_DIV : ir_ty = IR_DIV; break;
	case LOP_REM : ir_ty = IR_REM; break;
	case LOP_AND : ir_ty = IR_BAND; break;
	case LOP_OR  : ir_ty = IR_BOR; break;
	case LOP_XOR : ir_ty = IR_XOR; break;
	case LOP_SHL : ir_ty = IR_LSL; break;
	case LOP_SHR : ir_ty = IR_ASR; break;
	case LOP_USHR: ir_ty = IR_LSR; break;
	default: IS_TRUE0(0);
	}
	IR * res = gen_mapped_pr(LIR_res(lir), tyid);
	IR * op0 = gen_mapped_pr(LIR_op0(lir), tyid);
	IR * op1 = gen_mapped_pr(LIR_op1(lir), tyid);
	IR * x = m_ru->build_binary_op(ir_ty, tyid, op0, op1);
	IR * c = m_ru->build_store_pr(PR_no(res), IR_dt(res), x);
	if (ir_ty == IR_DIV || ir_ty == IR_REM) {
		#ifdef DIV_REM_MAY_THROW
		IR_may_throw(x) = true;
		IR_may_throw(c) = true;
		#endif
		if (m_has_catch) {
			IR * lab = m_ru->build_label(m_ru->gen_ilab());
			add_next(&c, lab);
		}
	}
	return c;
}


IR * DEX2IR::convert_bin_op_lit(IN LIR * lir)
{
	//LIRABCOp * p = (LIRABCOp*)ir;
	//if (is_s8(LIR_op1(ir))) {
		//8bit imm
	//} else {
		//16bit imm
	//}
	UINT tyid = get_dt_tyid(lir);
	IR_TYPE ir_ty = IR_UNDEF;
	switch (LIR_opcode(lir)) {
	case LOP_ADD_LIT : ir_ty = IR_ADD; break;
	case LOP_SUB_LIT : ir_ty = IR_SUB; break;
	case LOP_MUL_LIT : ir_ty = IR_MUL; break;
	case LOP_DIV_LIT : ir_ty = IR_DIV; break;
	case LOP_REM_LIT : ir_ty = IR_REM; break;
	case LOP_AND_LIT : ir_ty = IR_BAND; break;
	case LOP_OR_LIT  : ir_ty = IR_BOR; break;
	case LOP_XOR_LIT : ir_ty = IR_XOR; break;
	case LOP_SHL_LIT : ir_ty = IR_LSL; break;
	case LOP_SHR_LIT : ir_ty = IR_ASR; break;
	case LOP_USHR_LIT: ir_ty = IR_LSR; break;
	default: IS_TRUE0(0);
	}

	IR * res = gen_mapped_pr(LIR_res(lir), tyid);
	IR * op0 = gen_mapped_pr(LIR_op0(lir), tyid);
	IR * lit = m_ru->build_imm_int(LIR_op1(lir), tyid);
	IR * x = m_ru->build_binary_op(ir_ty, tyid, op0, lit);
	IR * c = m_ru->build_store_pr(PR_no(res), IR_dt(res), x);
	if ((ir_ty == IR_DIV || ir_ty == IR_REM) && CONST_int_val(lit) == 0) {
		IR_may_throw(x) = true;
		IR_may_throw(c) = true;
		if (m_has_catch) {
			IR * lab = m_ru->build_label(m_ru->gen_ilab());
			add_next(&c, lab);
		}
	}
	return c;
}


UINT DEX2IR::get_dexopcode(UINT flag)
{
	UINT flag1 = flag & 0x0f;
	UINT flag2 = flag & 0xf0;
	UINT dexopcode = 0; //defined in DexOpcodes.h
	switch (flag1) {
	case LIR_invoke_unknown:
		IS_TRUE0(0);
		switch (flag2) {
		case 0: dexopcode = OP_FILLED_NEW_ARRAY; break;
		case LIR_Range: dexopcode = OP_FILLED_NEW_ARRAY_RANGE; break;
		default: IS_TRUE0(0);
		}
		break;
	case LIR_invoke_virtual:
		switch (flag2) {
		case 0: dexopcode = OP_INVOKE_VIRTUAL; break;
		case LIR_Range: dexopcode = OP_INVOKE_VIRTUAL_RANGE; break;
		default: IS_TRUE0(0);
		}
		break;
	case LIR_invoke_super:
		switch(flag2) {
		case 0: dexopcode = OP_INVOKE_SUPER; break;
		case LIR_Range: dexopcode = OP_INVOKE_SUPER_RANGE; break;
		default: IS_TRUE0(0);
		}
		break;
	case LIR_invoke_direct:
		switch (flag2) {
		case 0: dexopcode = OP_INVOKE_DIRECT; break;
		case LIR_Range: dexopcode = OP_INVOKE_DIRECT_RANGE; break;
		default: IS_TRUE0(0);
		}
		break;
	case LIR_invoke_static:
		switch (flag2) {
		case 0: dexopcode = OP_INVOKE_STATIC; break;
		case LIR_Range: dexopcode = OP_INVOKE_STATIC_RANGE; break;
		default: IS_TRUE0(0);
		}
		break;
	case LIR_invoke_interface:
		switch (flag2) {
		case 0: dexopcode = OP_INVOKE_INTERFACE; break;
		case LIR_Range: dexopcode = OP_INVOKE_INTERFACE_RANGE; break;
		default: IS_TRUE0(0);
		}
		break;
	default:
		IS_TRUE0(0);
	}
	return dexopcode;
}


/*
invoke-direct {v0}, Ljava/lang/Object;.<init>:()V // method@0006
invoke-virtual
invoke-static
*/
IR * DEX2IR::convert_invoke(IN LIR * lir)
{
	UINT dexopcode = get_dexopcode(LIR_dt(lir));
	UINT ret_tyid = m_tr->i32; //TODO: ensure the return-value type.
	LIRInvokeOp * r = (LIRInvokeOp*)lir;

	//Generate callee.
	VAR * v = add_func_var(r->ref, ret_tyid);
	set_map_v2blt(v, BLTIN_INVOKE);

	//Only for Debug
	DexMethodId const* method_id = dexGetMethodId(m_df, r->ref);
	CHAR const* method_name = dexStringById(m_df, method_id->nameIdx);
	CHAR const* class_name = dexStringByTypeIdx(m_df, method_id->classIdx);
	DexProtoId const* proto_id = dexGetProtoId(m_df, method_id->protoIdx);
	CHAR const* paramty = dexStringById(m_df, proto_id->shortyIdx);
	IS_TRUE0(method_name && paramty);

	//Construct param list.
	IR * last = NULL;
	IR * param_list = NULL;

	//The first parameter is used to record invoke-kind.
	UINT k = LIR_dt(lir);
	bool is_range = HAVE_FLAG((k & 0xf0), LIR_Range);
	INVOKE_KIND ik = INVOKE_UNDEF;
	bool has_this = true;
	if (is_range) {
		switch (k & 0x0f) {
		case LIR_invoke_unknown: IS_TRUE0(0); break;
		case LIR_invoke_virtual: ik = INVOKE_VIRTUAL_RANGE; break;
		case LIR_invoke_direct: ik = INVOKE_DIRECT_RANGE; break;
		case LIR_invoke_super: ik = INVOKE_SUPER_RANGE; break;
		case LIR_invoke_interface: ik = INVOKE_INTERFACE_RANGE; break;
		case LIR_invoke_static:
			ik = INVOKE_STATIC_RANGE;
			has_this = false;
			break;
		default: IS_TRUE0(0);
		}
	} else {
		switch (k & 0x0f) {
		case LIR_invoke_unknown: IS_TRUE0(0); break;
		case LIR_invoke_virtual: ik = INVOKE_VIRTUAL; break;
		case LIR_invoke_direct: ik = INVOKE_DIRECT; break;
		case LIR_invoke_super: ik = INVOKE_SUPER; break;
		case LIR_invoke_interface: ik = INVOKE_INTERFACE; break;
		case LIR_invoke_static: ik = INVOKE_STATIC; has_this = false; break;
		default: IS_TRUE0(0);
		}
	}
	IR * kind = m_ru->build_imm_int(ik, m_tr->u32);
	add_next(&param_list, &last, kind);

	//The second parameter is used to record method-id.
	IR * midv = m_ru->build_imm_int(r->ref, m_tr->u32);
	add_next(&param_list, &last, midv);

	//Record invoke-parameters.
	paramty++;
	for (USHORT i = 0; i < r->argc; i++) {
		IR * param = NULL;
		if (has_this && i == 0) {
			param = gen_mapped_pr(r->args[i], m_tr->ptr);
		} else {
			switch (*paramty) {
			case 'V':
				param = gen_mapped_pr(r->args[i], m_tr->i32);
				break;
			case 'Z':
				param = gen_mapped_pr(r->args[i], m_tr->b);
				break;
			case 'B':
				param = gen_mapped_pr(r->args[i], m_tr->u8);
				break;
			case 'S':
				param = gen_mapped_pr(r->args[i], m_tr->i16);
				break;
			case 'C':
				param = gen_mapped_pr(r->args[i], m_tr->i8);
				break;
			case 'I':
				param = gen_mapped_pr(r->args[i], m_tr->i32);
				break;
			case 'J':
				param = gen_mapped_pr(r->args[i], m_tr->i64);
				i++;
				break;
			case 'F':
				param = gen_mapped_pr(r->args[i], m_tr->f32);
				break;
			case 'D':
				param = gen_mapped_pr(r->args[i], m_tr->f64);
				i++;
				break;
			case 'L':
				param = gen_mapped_pr(r->args[i], m_tr->ptr);
				break;
			case '[': IS_TRUE(0, ("should be convert to L")); break;
			default: IS_TRUE0(0);
			}
			paramty++;
		}
		add_next(&param_list, &last, param);
	}
	IS_TRUE0(*paramty == 0);
	IR * c = m_ru->build_call(v, param_list, 0, 0);
	IR_may_throw(c) = true;
	CALL_is_readonly(c) = VAR_is_readonly(v);
	IR_has_sideeffect(c) = true;
	return c;
}


//AABBBB
//new-instance vA <- CLASS-NAME
IR * DEX2IR::convert_new_instance(IN LIR * lir)
{
	/*
	REGION::buld_binary_op will detect the POINTER type, IR_ADD
	computes the POINTER addend size automatically.
	Here we hacked it by change POINTER base type size to 1 byte.
	*/
	UINT tyid = m_tr->ptr; //return value is referrence type.

	//AABBBB
	//new-instance v%d <- CLASS-NAME
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_NEW), tyid);
	set_map_v2blt(v, BLTIN_NEW);

	//return_value = new(CLASS-NAME)
	IR * class_type_id = m_ru->build_imm_int(LIR_op0(lir), m_tr->u32);
	IR * c = m_ru->build_call(v, class_type_id,
							  gen_mapped_prno(LIR_res(lir), tyid), tyid);
	IR_may_throw(c) = true;
	CALL_is_alloc_heap(c) = true;
	CALL_is_readonly(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = true;
	return c;
}


//ABCCCC
//new-array vA(res) <- vB(op0), LCAnimal(op1)
IR * DEX2IR::convert_new_array(IN LIR * lir)
{
	UINT tyid = m_tr->ptr; //return value is obj-ptr.
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_NEW_ARRAY), tyid);
	set_map_v2blt(v, BLTIN_NEW_ARRAY);

	//The first parameter is the number of array element.
	IR * param_list = gen_mapped_pr(LIR_op0(lir), tyid);

	//The class_name id.
	CHAR const* class_name = dexStringByTypeIdx(m_df, LIR_op1(lir));
	IS_TRUE0(class_name);

	//The second parameter is class-type-id.
	add_next(&param_list, m_ru->build_imm_int(LIR_op1(lir), m_tr->u32));

	//Call new_array(num_of_elem, class_name_id)
	IR * c =  m_ru->build_call(v, param_list,
							   gen_mapped_prno(LIR_res(lir), tyid), tyid);
	IR_may_throw(c) = true;
	CALL_is_alloc_heap(c) = true;
	CALL_is_readonly(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = true;
	return c;
}


IR * DEX2IR::convert_array_length(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_ARRAY_LENGTH), m_tr->ptr);
	set_map_v2blt(v, BLTIN_ARRAY_LENGTH);
	IR * c = m_ru->build_call(v,
							  gen_mapped_pr(LIR_op0(lir), m_tr->ptr),
							  gen_mapped_prno(LIR_res(lir), m_tr->i32),
							  m_tr->i32);

	IR_may_throw(c) = true;
	CALL_is_readonly(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = false;
	return c;
}


//Releases the monitor of the object referenced by vA.
IR * DEX2IR::convert_monitor_exit(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_MONITOR_EXIT), m_tr->ptr);
	set_map_v2blt(v, BLTIN_MONITOR_EXIT);
	IR * c = m_ru->build_call(v, gen_mapped_pr(LIR_res(lir), m_tr->i32), 0, 0);
	IR_may_throw(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = true;
	CALL_is_readonly(c) = true;
	return c;
}


//Obtains the monitor of the object referenced by vA.
IR * DEX2IR::convert_monitor_enter(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_MONITOR_ENTER), m_tr->ptr);
	set_map_v2blt(v, BLTIN_MONITOR_ENTER);
	IR * c = m_ru->build_call(v, gen_mapped_pr(LIR_res(lir), m_tr->i32), 0, 0);
	IR_may_throw(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = true;
	CALL_is_readonly(c) = true;
	return c;
}


IR * DEX2IR::convert_packed_switch(IN LIR * lir)
{
	LIRSwitchOp * p = (LIRSwitchOp*)lir;
	IS_TRUE0(LIR_dt(p) == 0x1);

	USHORT * pdata = p->data;

	UINT f = LIR_switch_kind(p);
	IS_TRUE0(f == 0x100);

	USHORT num_of_case = LIR_case_num(p);

	INT base_val = LIR_packed_switch_base_value(p);

	IR * case_list = NULL;
	IR * last = NULL;
	if (num_of_case > 0) {
		UINT tyid = m_tr->i32;
		UINT * pcase_entry = LIR_packed_switch_case_entry(p);
		for (USHORT i = 0; i < num_of_case; i++, base_val++) {
			UINT idx_of_insn = pcase_entry[i];
			LIR * tgt = LIRC_op(m_fu, idx_of_insn);
			IS_TRUE0(tgt);
			LIST<LABEL_INFO*> * labs = m_lir2labs.get(tgt);

			//There may be many labs over there.
			//IS_TRUE0(labs && labs->get_elem_count() == 1);

			LABEL_INFO * lab = labs->get_head();
			add_next(&case_list, &last,
				m_ru->build_case(m_ru->build_imm_int(base_val, tyid), lab));
		}
	}

	LABEL_INFO * defaultlab = m_ru->gen_ilab();
	IR * vexp = gen_mapped_pr(p->value, m_tr->i32);
	IR * ret_list = m_ru->build_switch(vexp, case_list, NULL, defaultlab);
	add_next(&ret_list, m_ru->build_label(defaultlab));
	return ret_list;
}


IR * DEX2IR::convert_fill_array_data(IN LIR * lir)
{
	LIRSwitchOp * l = (LIRSwitchOp*)lir;
	USHORT const* pdata = l->data;

	#ifdef _DEBUG_
	//pdata[0]: the magic number of code
	//0x100 PACKED_SWITCH, 0x200 SPARSE_SWITCH, 0x300 FILL_ARRAY_DATA
	IS_TRUE0(pdata[0] == 0x300);
	//pdata[1]: size of each element.
	//pdata[2]: the number of element.
	UINT size_of_elem = pdata[1];
	UINT num_of_elem = pdata[2];
	UINT data_size = num_of_elem * size_of_elem;
	#endif

	VAR * v = add_var_by_name(BLTIN_name(BLTIN_FILL_ARRAY_DATA), m_tr->ptr);
	set_map_v2blt(v, BLTIN_FILL_ARRAY_DATA);
	IR * call = m_ru->build_call(v, NULL, 0, 0);

	//The first parameter is array obj-ptr.
	IR * p = gen_mapped_pr(l->value, m_tr->ptr);

	//The second parameter record the pointer to data.
	add_next(&p, m_ru->build_imm_int((HOST_INT)pdata, m_tr->u32));

	CALL_param_list(call) = p;
	call->set_parent_pointer(false);
	IR_may_throw(call) = true;
	CALL_is_intrinsic(call) = true;
	IR_has_sideeffect(call) = true;
	CALL_is_readonly(call) = true;
	return call;
}


IR * DEX2IR::convert_sparse_switch(IN LIR * lir)
{
	LIRSwitchOp * p = (LIRSwitchOp*)lir;
	IS_TRUE0(LIR_dt(p) == 0x2);

	USHORT * pdata = p->data;
	UINT f = LIR_switch_kind(p);
	IS_TRUE0(f == 0x200);

	UINT num_of_case = LIR_case_num(p);
	IR * case_list = NULL;
	IR * last = NULL;
	if (num_of_case > 0) {
		UINT tyid = m_tr->i32;
		UINT * pcase_value =LIR_sparse_switch_case_value(p);

		UINT * pcase_entry = LIR_sparse_switch_case_entry(p);
		for (UINT i = 0; i < num_of_case; i++) {
			LIR * tgt = LIRC_op(m_fu, pcase_entry[i]);
			IS_TRUE0(tgt);
			LIST<LABEL_INFO*> * labs = m_lir2labs.get(tgt);

			//There may be many labs over there.
			//IS_TRUE0(labs && labs->get_elem_count() == 1);

			LABEL_INFO * lab = labs->get_head();
			add_next(&case_list, &last,
				m_ru->build_case(m_ru->build_imm_int(pcase_value[i], tyid),
								 lab));
		}
	}

	LABEL_INFO * defaultlab = m_ru->gen_ilab();
	IR * vexp = gen_mapped_pr(p->value, m_tr->i32);
	IR * ret_list = m_ru->build_switch(vexp, case_list, NULL, defaultlab);
	add_next(&ret_list, m_ru->build_label(defaultlab));
	return ret_list;
}


IR * DEX2IR::convert_instance_of(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_INSTANCE_OF), m_tr->ptr);
	set_map_v2blt(v, BLTIN_INSTANCE_OF);

	//first parameter
	IR * p = gen_mapped_pr(LIR_op0(lir), m_tr->ptr);

	//second one.
	add_next(&p, m_ru->build_imm_int(LIR_op1(lir), m_tr->i32));

	IR * c = m_ru->build_call(v, p,
							  gen_mapped_prno(LIR_res(lir), m_tr->i32),
							  m_tr->i32);
	IR_may_throw(c) = true;
	CALL_is_readonly(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = false;
	return c;
}


IR * DEX2IR::convert_check_cast(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_CHECK_CAST), m_tr->ptr);
	set_map_v2blt(v, BLTIN_CHECK_CAST);
	IR * p = gen_mapped_pr(LIR_res(lir), m_tr->i32);
	add_next(&p, m_ru->build_imm_int(LIR_op0(lir), m_tr->i32));
	IR * c = m_ru->build_call(v, p, 0, 0);
	IR_may_throw(c) = true;
	CALL_is_intrinsic(c) = true;
	CALL_is_readonly(c) = true;

	//It throw a ClassCastException if the reference in the given register
	//cannot be cast to the indicated.
	IR_has_sideeffect(c) = true;
	return c;
}


IR * DEX2IR::convert_filled_new_array(IN LIR * lir)
{
	/*
	AABBBBCCCC or ABCCCCDEFG
	e.g:
		A(argc), B,D,E,F,G(parampters), CCCC(class_tyid)
	*/
	LIRInvokeOp * r = (LIRInvokeOp*)lir;

	VAR * v = add_var_by_name(BLTIN_name(BLTIN_FILLED_NEW_ARRAY), m_tr->ptr);
	set_map_v2blt(v, BLTIN_FILLED_NEW_ARRAY);
	#ifdef _DEBUG_
	CHAR const* class_name = dexStringByTypeIdx(m_df, r->ref);
	IS_TRUE0(class_name);
	#endif
	//first parameter is invoke-kind.
	IR * p = m_ru->build_imm_int(LIR_dt(lir), m_tr->u32);

	//second one is class-id.
	IR * cid = m_ru->build_imm_int(r->ref, m_tr->i32);
	IR * last = p;
	add_next(&p, &last, cid);

	//and else parameters.
	for (UINT i = 0; i < r->argc; i++) {
		add_next(&p, &last, gen_mapped_pr(r->args[i], m_tr->i32));
	}
	IR * c = m_ru->build_call(v, p, 0, 0);
	IR_may_throw(c) = true;
	CALL_is_intrinsic(c) = true;
	CALL_is_readonly(c) = true;
	IR_has_sideeffect(c) = true;
	return c;
}


IR * DEX2IR::convert_throw(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_THROW), m_tr->ptr);
	set_map_v2blt(v, BLTIN_THROW);
	IR * c = m_ru->build_call(v, gen_mapped_pr(LIR_res(lir), m_tr->i32), 0, 0);
	IR_may_throw(c) = true;
	CALL_is_readonly(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = true;
	return c;
}


IR * DEX2IR::convert_const_class(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_CONST_CLASS), m_tr->ptr);
	set_map_v2blt(v, BLTIN_CONST_CLASS);
	IR * c = m_ru->build_call(v,
							  m_ru->build_imm_int(LIR_op0(lir), m_tr->i32),
							  gen_mapped_prno(LIR_res(lir), m_tr->i32),
							  m_tr->i32);
	IR_may_throw(c) = true;
	CALL_is_readonly(c) = true;
	CALL_is_intrinsic(c) = true;
	IR_has_sideeffect(c) = false;
	return c;
}


IR * DEX2IR::convert_move_exception(IN LIR * lir)
{
	VAR * v = add_var_by_name(BLTIN_name(BLTIN_MOVE_EXP), m_tr->ptr);
	set_map_v2blt(v, BLTIN_MOVE_EXP);
	IR * ir = m_ru->build_call(v, NULL,
							gen_mapped_prno(LIR_res(lir), m_tr->i32),
							m_tr->i32);
	CALL_is_readonly(ir) = true;
	CALL_is_intrinsic(ir) = true;
	IR_has_sideeffect(ir) = true;
	return ir;
	//Not throw exception.
}


IR * DEX2IR::convert_move_result(IN LIR * lir)
{
	UINT restyid = VOID_TY;
	switch (LIR_dt(lir)) {
	case LIR_JDT_unknown: restyid = m_tr->i32; break;
	case LIR_JDT_object: restyid = m_tr->ptr; break;
	case LIR_JDT_wide: restyid = m_tr->i64; break;
	default: IS_TRUE0(0);
	}

	VAR * v = add_var_by_name(BLTIN_name(BLTIN_MOVE_RES), m_tr->ptr);
	VAR_is_readonly(v) = true;
	set_map_v2blt(v, BLTIN_MOVE_RES);
	IR * x = m_ru->build_call(v, NULL,
						gen_mapped_prno(LIR_res(lir), restyid),
						restyid);
	CALL_is_readonly(x) = true;
	IR_has_sideeffect(x) = true;
	CALL_is_intrinsic(x) = true;
	return x;
}


IR * DEX2IR::convert_move(IN LIR * lir)
{
	UINT ty = LIR_dt(lir);
	UINT tyid = 0;
	switch (ty) {
	case LIR_JDT_unknown: //return pr.
		tyid = m_tr->i32;
		break;
	case LIR_JDT_object: //return object.
		tyid = m_tr->ptr;
		break;
	case LIR_JDT_wide: //return 64bits result
		tyid = m_tr->i64; //TODO: need pair?
		break;
	default: IS_TRUE0(0);
	}
	IR * tgt = gen_mapped_pr(LIR_res(lir), tyid);
	IR * src = gen_mapped_pr(LIR_op0(lir), tyid);
	return m_ru->build_store_pr(PR_no(tgt), IR_dt(tgt), src);
}


//Map Vreg to PR, and return PR.
//vid: Vreg id.
IR * DEX2IR::gen_mapped_pr(UINT vid, UINT tyid)
{
	IR * vx = m_v2pr.get(vid);
	if (vx == NULL) {
		vx = m_ru->build_pr(tyid);
		m_v2pr.set(vid, vx);
		m_pr2v.set(PR_no(vx), vid);
	}
	vx = m_ru->dup_ir(vx);
	IR_dt(vx) = tyid;
	return vx;
}


//Map Vreg to PR, then return the PR no.
//vid: Vreg id.
UINT DEX2IR::gen_mapped_prno(UINT vid, UINT tyid)
{
	IR * vx = m_v2pr.get(vid);
	if (vx == NULL) {
		vx = m_ru->build_pr(tyid);
		m_v2pr.set(vid, vx);
		m_pr2v.set(PR_no(vx), vid);
	}
	return PR_no(vx);
}


IR * DEX2IR::convert_ret(IN LIR * lir)
{
	IR * exp = NULL;
	switch (LIR_dt(lir)) {
	case LIR_JDT_unknown: //return pr. vAA
		exp = gen_mapped_pr(LIR_res(lir), m_tr->i32);
		break;
	case LIR_JDT_void:
		exp = NULL;
		break;
	case LIR_JDT_object: //return object.
		exp = gen_mapped_pr(LIR_res(lir), m_tr->ptr);
		break;
	case LIR_JDT_wide: //return 64bits result
		exp = gen_mapped_pr(LIR_res(lir), m_tr->i64);
		break;
	default: IS_TRUE0(0);
	}
	return m_ru->build_return(exp);
}


IR * DEX2IR::convert_unary_op(IN LIR * lir)
{
	UINT tyid = get_dt_tyid(lir);
	IR_TYPE ir_ty = IR_UNDEF;
	switch (LIR_opcode(lir)) {
	case LOP_NEG: ir_ty = IR_NEG; break;
	case LOP_NOT: ir_ty = IR_BNOT; break;
	default: IS_TRUE0(0);
	}
	IR * res = gen_mapped_pr(LIR_res(lir), tyid);
	IR * op0 = gen_mapped_pr(LIR_op0(lir), tyid);
	IR * x = m_ru->build_unary_op(ir_ty, tyid, op0);
	return m_ru->build_store_pr(PR_no(res), IR_dt(res), x);
	//Not throw exception.
}


IR * DEX2IR::convert_load_string_addr(IN LIR * lir)
{
	CHAR const* string = dexStringById(m_df, LIR_op0(lir));
	IS_TRUE0(string);
	VAR * v = add_string_var(string);
	set_map_v2ofst(v, LIR_op0(lir));
	IR * lda = m_ru->build_lda(m_ru->build_id(v));
	IR * res = gen_mapped_pr(LIR_res(lir), m_tr->ptr);
	return m_ru->build_store_pr(PR_no(res), IR_dt(res), lda);
}


IR * DEX2IR::convert_load_const(IN LIR * lir)
{
	UINT tyid = 0;
	switch (LIR_dt(lir)) {
	case LIR_JDT_unknown:
		tyid = m_tr->i32;
		break;
	case LIR_JDT_void	:
	case LIR_JDT_int	:
	case LIR_JDT_object	:
	case LIR_JDT_boolean:
	case LIR_JDT_byte	:
	case LIR_JDT_char	:
	case LIR_JDT_short	:
	case LIR_JDT_float	:
	case LIR_JDT_none	:
	case LIR_wide:
		IS_TRUE0(0);
		break;
	case LIR_JDT_wide:
		tyid = m_tr->i64;
		break;
	case LIR_JDT_long:
	case LIR_JDT_double:
	default:
		IS_TRUE0(0);
		break;
	}

	IR * res = gen_mapped_pr(LIR_res(lir), tyid);
	return m_ru->build_store_pr(PR_no(res), IR_dt(res),
							    m_ru->build_imm_int(LIR_int_imm(lir), tyid));
}


IR * DEX2IR::convert_goto(IN LIR * lir)
{
	LIR * tgt = LIRC_op(m_fu, ((LIRGOTOOp*)lir)->target);
	IS_TRUE0(tgt);
	LIST<LABEL_INFO*> * labs = m_lir2labs.get(tgt);
	IS_TRUE0(labs);
	return m_ru->build_goto(labs->get_head());
}


IR * DEX2IR::convert_cond_br(IN LIR * lir)
{
	IR_TYPE rt = IR_UNDEF;
	switch (LIR_dt(lir)) {
	case LIR_cond_EQ: rt = IR_EQ; break;
	case LIR_cond_LT: rt = IR_LT; break;
	case LIR_cond_GT: rt = IR_GT; break;
	case LIR_cond_LE: rt = IR_LE; break;
	case LIR_cond_GE: rt = IR_GE; break;
	case LIR_cond_NE: rt = IR_NE; break;
	}

	//Det
	UINT tyid = m_tr->i32;
	IR * op0 = NULL;
	IR * op1 = NULL;
	UINT liridx = 0;
	if (LIR_opcode(lir) == LOP_IF) {
		op0 = gen_mapped_pr(LIR_res(lir), tyid);
		op1 = gen_mapped_pr(LIR_op0(lir), tyid);
		liridx = LIR_op1(lir);
	} else if (LIR_opcode(lir) == LOP_IFZ) {
		op0 = gen_mapped_pr(LIR_res(lir), tyid);
		op1 = m_ru->build_imm_int(0, tyid);
		liridx = LIR_op0(lir);
	} else {
		IS_TRUE0(0);
	}
	IR * det = m_ru->build_binary_op(rt, m_tr->b, op0, op1);
	//Target label
	LIR * tgt = LIRC_op(m_fu, liridx);
	IS_TRUE0(tgt);
	LIST<LABEL_INFO*> * labs = m_lir2labs.get(tgt);
	IS_TRUE0(labs);
	return m_ru->build_branch(true, det, labs->get_head());
}


IR * DEX2IR::convert_cvt(IN LIR * lir)
{
	UINT tgt = 0, src = 0;
	switch (LIR_dt(lir)) {
	case LIR_convert_i2l: tgt = m_tr->i64; src = m_tr->i32; break;
	case LIR_convert_i2f: tgt = m_tr->f32; src = m_tr->i32; break;
	case LIR_convert_i2d: tgt = m_tr->f64; src = m_tr->i32; break;

	case LIR_convert_l2i: tgt = m_tr->i32; src = m_tr->i64; break;
	case LIR_convert_l2f: tgt = m_tr->f32; src = m_tr->i64; break;
	case LIR_convert_l2d: tgt = m_tr->f64; src = m_tr->i64; break;

	case LIR_convert_f2i: tgt = m_tr->i32; src = m_tr->f32; break;
	case LIR_convert_f2l: tgt = m_tr->i64; src = m_tr->f32; break;
	case LIR_convert_f2d: tgt = m_tr->f64; src = m_tr->f32; break;

	case LIR_convert_d2i: tgt = m_tr->i32; src = m_tr->f64; break;
	case LIR_convert_d2l: tgt = m_tr->i64; src = m_tr->f64; break;
	case LIR_convert_d2f: tgt = m_tr->f32; src = m_tr->f64; break;

	case LIR_convert_i2b: tgt = m_tr->b; src = m_tr->i32; break;
	case LIR_convert_i2c: tgt = m_tr->i8; src = m_tr->i32; break;
	case LIR_convert_i2s: tgt = m_tr->i16; src = m_tr->i32; break;
	default: IS_TRUE0(0);
	}

	IR * res = gen_mapped_pr(LIR_res(lir), tgt);
	IR * exp = gen_mapped_pr(LIR_op0(lir), src);
	IR * cvt = m_ru->build_cvt(exp, tgt);
	return m_ru->build_store_pr(PR_no(res), IR_dt(res), cvt);
}


IR * DEX2IR::convert(IN LIR * lir)
{
	switch (LIR_opcode(lir)) {
	case LOP_NOP:
		break;
	case LOP_CONST:
		return convert_load_const(lir);
	case LOP_RETURN:
		return convert_ret(lir);
	case LOP_THROW:
		return convert_throw(lir);
	case LOP_MONITOR_ENTER  :
		return convert_monitor_enter(lir);
	case LOP_MONITOR_EXIT   :
		return convert_monitor_exit(lir);
	case LOP_MOVE_RESULT    :
		return convert_move_result(lir);
	case LOP_MOVE_EXCEPTION :
		return convert_move_exception(lir);
	case LOP_GOTO		    :
		return convert_goto(lir);
	case LOP_MOVE		:
		return convert_move(lir);
	case LOP_NEG        :
	case LOP_NOT		:
		return convert_unary_op(lir);
	case LOP_CONVERT	:
		return convert_cvt(lir);
	case LOP_ADD_ASSIGN :
	case LOP_SUB_ASSIGN :
	case LOP_MUL_ASSIGN :
	case LOP_DIV_ASSIGN :
	case LOP_REM_ASSIGN :
	case LOP_AND_ASSIGN :
	case LOP_OR_ASSIGN  :
	case LOP_XOR_ASSIGN :
	case LOP_SHL_ASSIGN :
	case LOP_SHR_ASSIGN :
	case LOP_USHR_ASSIGN:
		return convert_bin_op_assign(lir);
	case LOP_ARRAY_LENGTH:
		return convert_array_length(lir);
	case LOP_IFZ         :
		return convert_cond_br(lir);
	case LOP_NEW_INSTANCE:
		return convert_new_instance(lir);
	case LOP_CONST_STRING:
		return convert_load_string_addr(lir);
	case LOP_CONST_CLASS :
		return convert_const_class(lir);
	case LOP_SGET        :
		return convert_sget(lir);
	case LOP_CHECK_CAST  :
		return convert_check_cast(lir);
	case LOP_SPUT        :
		return convert_sput(lir);
	case LOP_APUT        :
		return convert_aput(lir);
	case LOP_AGET      :
		return convert_aget(lir);
	case LOP_CMPL      :
	case LOP_CMP_LONG  :
	case LOP_CMPG:
		return convert_cmp(lir);
	case LOP_ADD       :
	case LOP_SUB       :
	case LOP_MUL       :
	case LOP_DIV       :
	case LOP_REM       :
	case LOP_AND       :
	case LOP_OR        :
	case LOP_XOR       :
	case LOP_SHL       :
	case LOP_SHR       :
	case LOP_USHR      :
		return convert_bin_op(lir);
	case LOP_IF        :
		return convert_cond_br(lir);
	case LOP_ADD_LIT   :
	case LOP_SUB_LIT   :
	case LOP_MUL_LIT   :
	case LOP_DIV_LIT   :
	case LOP_REM_LIT   :
	case LOP_AND_LIT   :
	case LOP_OR_LIT    :
	case LOP_XOR_LIT   :
	case LOP_SHL_LIT   :
	case LOP_SHR_LIT   :
	case LOP_USHR_LIT   :
		return convert_bin_op_lit(lir);
	case LOP_IPUT       :
		return convert_iput(lir);
	case LOP_IGET       :
		return convert_iget(lir);
	case LOP_NEW_ARRAY  :
		return convert_new_array(lir);
	case LOP_INSTANCE_OF:
		return convert_instance_of(lir);
	case LOP_TABLE_SWITCH:
		return convert_packed_switch(lir);
	case LOP_LOOKUP_SWITCH:
		return convert_sparse_switch(lir);
	case LOP_FILL_ARRAY_DATA:
		return convert_fill_array_data(lir);
	case LOP_INVOKE         :
		return convert_invoke(lir);
	case LOP_FILLED_NEW_ARRAY:
		return convert_filled_new_array(lir);
	case LOP_PHI:
		IS_TRUE0(0);
		break;
	default: IS_TRUE0(0);
	} //end switch

	if (g_tfile != NULL) {
		fflush(g_tfile);
	}
	return NULL;
}


void DEX2IR::mark_lab()
{
	for (INT i = 0; i < LIRC_num_of_op(m_fu); i++) {
		LIR * lir = LIRC_op(m_fu, i);
		IS_TRUE0(lir);
		if (LIR_opcode(lir) == LOP_IF) {
			LIR * tgt = LIRC_op(m_fu, LIR_op1(lir));
			IS_TRUE0(tgt);
			LIST<LABEL_INFO*> * lst = m_lir2labs.get(tgt);
			if (lst == NULL) {
				lst = new LIST<LABEL_INFO*>();
				m_lir2labs.set(tgt, lst);
				lst->append_tail(m_ru->gen_ilab());
			}
		} else if (LIR_opcode(lir) == LOP_IFZ) {
			LIR * tgt = LIRC_op(m_fu, LIR_op0(lir));
			IS_TRUE0(tgt);
			LIST<LABEL_INFO*> * lst = m_lir2labs.get(tgt);
			if (lst == NULL) {
				lst = new LIST<LABEL_INFO*>();
				m_lir2labs.set(tgt, lst);
				lst->append_tail(m_ru->gen_ilab());
			}
		} else if (LIR_opcode(lir) == LOP_GOTO) {
			LIR * tgt = LIRC_op(m_fu, ((LIRGOTOOp*)lir)->target);
			IS_TRUE0(tgt);
			LIST<LABEL_INFO*> * lst = m_lir2labs.get(tgt);
			if (lst == NULL) {
				lst = new LIST<LABEL_INFO*>();
				m_lir2labs.set(tgt, lst);
				lst->append_tail(m_ru->gen_ilab());
			}
		} else if (LIR_opcode(lir) == LOP_TABLE_SWITCH) {
			LIRSwitchOp * p = (LIRSwitchOp*)lir;
			IS_TRUE0(LIR_dt(p) == 0x1);
			USHORT * pdata = p->data;
			USHORT num_of_case = pdata[1];

			if (num_of_case > 0) {
				UINT * pcase_entry = (UINT*)&pdata[4];
				for (USHORT i = 0; i < num_of_case; i++) {
					LIR * tgt = LIRC_op(m_fu, pcase_entry[i]);
					IS_TRUE0(tgt);
					LIST<LABEL_INFO*> * lst = m_lir2labs.get(tgt);
					if (lst == NULL) {
						lst = new LIST<LABEL_INFO*>();
						m_lir2labs.set(tgt, lst);
						lst->append_tail(m_ru->gen_ilab());
					}
				}
			}
		} else if (LIR_opcode(lir) == LOP_LOOKUP_SWITCH) {
			LIRSwitchOp * p = (LIRSwitchOp*)lir;
			IS_TRUE0(LIR_dt(p) == 0x2);
			USHORT * pdata = p->data;
			//pdata[1]: the number of CASE entry.
			UINT num_of_case = pdata[1];
			if (num_of_case > 0) {
				BYTE * tp = (BYTE*)pdata;
				UINT * pcase_entry = (UINT*)&tp[4 + num_of_case * 4];
				for (UINT i = 0; i < num_of_case; i++) {
					LIR * tgt = LIRC_op(m_fu, pcase_entry[i]);
					IS_TRUE0(tgt);
					LIST<LABEL_INFO*> * lst = m_lir2labs.get(tgt);
					if (lst == NULL) {
						lst = new LIST<LABEL_INFO*>();
						m_lir2labs.set(tgt, lst);
						lst->append_tail(m_ru->gen_ilab());
					}
				}
			}
		} else if (LIR_opcode(lir) == LOP_MOVE_EXCEPTION) {
			/* Do not generate catch start label here, and it will
			 be made up during handle try-block.
			LIST<LABEL_INFO*> * lst = m_lir2labs.get(lir);
			if (lst == NULL) {
				lst = new LIST<LABEL_INFO*>();
				m_lir2labs.set(lir, lst);
				LABEL_INFO * lab = m_ru->gen_ilab();
				LABEL_INFO_is_catch_start(lab) = true;
				lst->append_tail(lab);
			}
			*/
		}
	}

	if (m_fu->triesSize != 0) {
		TRY_INFO * head = NULL, * lasti = NULL;
		for (UINT i = 0; i < m_fu->triesSize; i++) {
			LIROpcodeTry * each_try = m_fu->trys + i;
			TRY_INFO * ti = (TRY_INFO*)xmalloc(sizeof(TRY_INFO));
			add_next(&head, &lasti, ti);

			INT s = each_try->start_pc;
			IS_TRUE0(s >= 0 && s < LIRC_num_of_op(m_fu));
			LIR * lir = LIRC_op(m_fu, s);
			LIST<LABEL_INFO*> * lst = m_lir2labs.get(lir);
			if (lst == NULL) {
				lst = new LIST<LABEL_INFO*>();
				m_lir2labs.set(lir, lst);
			}
			LABEL_INFO * lab = m_ru->gen_ilab();
			LABEL_INFO_is_try_start(lab) = true;
			lst->append_tail(lab);
			ti->try_start = lab;

			s = each_try->end_pc;
			IS_TRUE0(s >= 0 && s <= LIRC_num_of_op(m_fu));
			lab = m_ru->gen_ilab();
			LABEL_INFO_is_try_end(lab) = true;
			if (s < LIRC_num_of_op(m_fu)) {
				lir = LIRC_op(m_fu, s);
				lst = m_lir2labs.get(lir);
				if (lst == NULL) {
					lst = new LIST<LABEL_INFO*>();
					m_lir2labs.set(lir, lst);
				}
				lst->append_tail(lab);
			} else {
				m_last_try_end_lab_list.append_tail(lab);
			}
			ti->try_end = lab;

			CATCH_INFO * last = NULL;
			for (UINT j = 0; j < each_try->catchSize; j++) {
				LIROpcodeCatch * each_catch = each_try->catches + j;
				CATCH_INFO * ci = (CATCH_INFO*)xmalloc(sizeof(CATCH_INFO));

				IS_TRUE0(each_catch->handler_pc < (UInt32)LIRC_num_of_op(m_fu));
				LIR * x = LIRC_op(m_fu, each_catch->handler_pc);
				IS_TRUE0(LIR_opcode(x) == LOP_MOVE_EXCEPTION);

				LABEL_INFO * lab = NULL;
				lst = m_lir2labs.get(x);
				if (lst == NULL) {
					lst = new LIST<LABEL_INFO*>();
					m_lir2labs.set(x, lst);
				} else {
					for (lab = lst->get_head();
						 lab != NULL; lab = lst->get_next()) {
						if (LABEL_INFO_is_catch_start(lab)) { break; }
					}

					//Multiple label may correspond to same LIR.
					//IS_TRUE0(lst->get_elem_count() == 1);
				}

				if (lab == NULL) {
					lab = m_ru->gen_ilab();
					LABEL_INFO_is_catch_start(lab) = true;
					lst->append_tail(lab);
				}

				ci->catch_start = lab;
				ci->kind = each_catch->class_type;
				add_next(&ti->catch_list, &last, ci);
				m_has_catch = true;
			}
		}
		m_ti = head;
	}
}


void DEX2IR::dump_lir2lab()
{
	if (g_tfile != NULL) {
		INT c;
		fprintf(g_tfile, "\n==-- DUMP LIR->LABEL --== ");

		TMAP_ITER<LIR*, LIST<LABEL_INFO*>*> iter;
		LIST<LABEL_INFO*> * lst;
		for (LIR * lir = m_lir2labs.get_first(iter, &lst);
			 lir != NULL; lir =	m_lir2labs.get_next(iter, &lst)) {
			IS_TRUE0(lst);
			fprintf(g_tfile, "\n");
			dump_lir2(lir, m_df, -1);
			for (LABEL_INFO * lab = lst->get_head();
				 lab != NULL; lab = lst->get_next()) {
				dump_lab(lab);
			}
		}
	}
}


extern bool g_dd;
//'succ': return true if convertion is successful.
IR * DEX2IR::convert(bool * succ)
{
	m_has_catch = false;
	mark_lab();
	IR * ir_list = NULL;
	IR * last = NULL;

	bool dump = false;
	if (dump && g_tfile != NULL) {
		fprintf(g_tfile, "\n\n==== DEX->IR CONVERT %s =====",
				m_ru->get_ru_name());
	}

	FILE * log = NULL;
	if (g_dd) {
		log = fopen("dex2ir.log", "a+");
		fprintf(log, "\n== %s ==", m_ru->get_ru_name());
	}

	if (dump) { dump_lir2lab(); }

	for (INT i = 0; i < LIRC_num_of_op(m_fu); i++) {
		LIR * lir = LIRC_op(m_fu, i);

		FILE * t = NULL;
		if (g_dd) {
			t = g_tfile;
			g_tfile = log;
		}

		//Do not use j as idx, since it will mess up
		//the diff with other log.
		if (dump) { dump_lir(lir, m_df, -1); }

		if (g_dd) {
			g_tfile = t;
		}

		LIST<LABEL_INFO*> * labs = m_lir2labs.get(lir);
		if (labs != NULL) {
			for (LABEL_INFO * l = labs->get_head();
				 l != NULL; l = labs->get_next()) {
				add_next(&ir_list, &last, m_ru->build_label(l));
			}
		}
		IR * newir = convert(lir);

		if (dump) {
			dump_lir(lir, m_df, i);
			dump_irs(newir, m_dm);
		}
		add_next(&ir_list, &last, newir);
	}
	for (LABEL_INFO * li = m_last_try_end_lab_list.get_head();
		 li != NULL; li = m_last_try_end_lab_list.get_next()) {
		add_next(&ir_list, &last, m_ru->build_label(li));
	}

	if (g_dd) {
		fclose(log);
	}
	*succ = true;
	return ir_list;
}
