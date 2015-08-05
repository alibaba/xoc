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

VAR_MGR::VAR_MGR(REGION_MGR * rm)
{
	IS_TRUE0(rm);
	m_var_count = 1; //for enjoying bitset util
	m_str_count = 1;
	m_ru_mgr = rm;
	m_dm = rm->get_dm();
}


//
//START VAR
//
VAR::VAR()
{
	id = 0; //unique id;
	name = NULL;
	VAR_str(this) = 0;
	u2.flag = 0; //Record variant properties of VAR.
	data_type = D_UNDEF;
	pointer_base_size = 0;
	data_size = 0;
	elem_type = D_UNDEF;
	align = 0;
}


void VAR::dump(FILE * h)
{
	CHAR buf[MAX_BUF_LEN];
	buf[0] = 0;
	if (h == NULL) {
		h = g_tfile;
	}
	if (h == NULL) return;
	fprintf(h, "\n%s", dump(buf));
	fflush(h);
}



CHAR * VAR::dump(CHAR * buf)
{
	CHAR * tb = buf;
	CHAR * name = SYM_name(VAR_name(this));
	CHAR tt[43];
	if (xstrlen(name) > 43) {
		memcpy(tt, name, 43);
		tt[39] = '.';
		tt[40] = '.';
		tt[41] = '.';
		tt[42] = 0;
		name = tt;
	}
	buf += sprintf(buf, "VAR%d(%s):", VAR_id(this), name);
	if (HAVE_FLAG(VAR_flag(this), VAR_GLOBAL)) {
		strcat(buf, "global");
	} else if (HAVE_FLAG(VAR_flag(this), VAR_LOCAL)) {
		strcat(buf, "local");
	} else {
		IS_TRUE0(0);
	}

	if (HAVE_FLAG(VAR_flag(this), VAR_STATIC)) {
		strcat(buf, ",static");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_READONLY)) {
		strcat(buf, ",const");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_VOLATILE)) {
		strcat(buf, ",volatile");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_IS_RESTRICT)) {
		strcat(buf, ",restrict");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_HAS_INIT_VAL)) {
		strcat(buf, ",has_init_val");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_FUNC_UNIT)) {
		strcat(buf, ",func_unit");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_FUNC_DECL)) {
		strcat(buf, ",func_decl");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_FAKE)) {
		strcat(buf, ",fake");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_IS_FORMAL_PARAM)) {
		strcat(buf, ",formal_param");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_IS_SPILL)) {
		strcat(buf, ",spill_loc");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_ADDR_TAKEN)) {
		strcat(buf, ",addr_taken");
	}
	if (VAR_is_str(this)) {
		strcat(buf, ",str");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_IS_ARRAY)) {
		strcat(buf, ",array");
	}
	if (HAVE_FLAG(VAR_flag(this), VAR_IS_ALLOCABLE)) {
		strcat(buf, ",allocable");
	}
	if (VAR_is_pointer(this)) {
		buf += strlen(buf);
		sprintf(buf, ",pointer,pt_base_sz:%d", VAR_pointer_base_size(this));
	}
	buf += strlen(buf);

	sprintf(buf, ",%s", DTDES_name(&g_dtype_desc[VAR_data_type(this)]));
	if (VAR_data_type(this) > D_F128) {
		buf += strlen(buf);
		sprintf(buf, ",mem_size:%d", VAR_data_size(this));
	}

	if (VAR_align(this) != 0) {
		buf += strlen(buf);
		sprintf(buf, ",align:%d", VAR_align(this));
	}

	strcat(buf, ",decl:'");
	buf += strlen(buf);
	dump_var_decl(buf, 40);
	strcat(buf, "'");

	#ifdef _DEBUG_
	UINT tmpf = VAR_flag(this);
	REMOVE_FLAG(tmpf, VAR_GLOBAL);
	REMOVE_FLAG(tmpf, VAR_LOCAL);
	REMOVE_FLAG(tmpf, VAR_STATIC);
	REMOVE_FLAG(tmpf, VAR_READONLY);
	REMOVE_FLAG(tmpf, VAR_VOLATILE);
	REMOVE_FLAG(tmpf, VAR_HAS_INIT_VAL);
	REMOVE_FLAG(tmpf, VAR_FUNC_UNIT);
	REMOVE_FLAG(tmpf, VAR_FUNC_DECL);
	REMOVE_FLAG(tmpf, VAR_FAKE);
	REMOVE_FLAG(tmpf, VAR_IS_ARRAY);
	REMOVE_FLAG(tmpf, VAR_IS_FORMAL_PARAM);
	REMOVE_FLAG(tmpf, VAR_IS_SPILL);
	REMOVE_FLAG(tmpf, VAR_ADDR_TAKEN);
	REMOVE_FLAG(tmpf, VAR_IS_PR);
	REMOVE_FLAG(tmpf, VAR_IS_RESTRICT);
	REMOVE_FLAG(tmpf, VAR_IS_ALLOCABLE);
	IS_TRUE0(tmpf == 0);
	#endif
	return tb;
}
//END VAR


//
//START VAR_MGR
//
/*
Add VAR into VAR_TAB.
Call this function cafefully, and making sure the VAR is unique.
'var_name': name of the variable, it is optional.
*/
VAR * VAR_MGR::register_var(CHAR const* var_name, DATA_TYPE dt,
							DATA_TYPE elem_type, UINT pointer_base_size,
							UINT mem_size, UINT align, UINT flag)
{
	IS_TRUE0(var_name != NULL);
	SYM * sym = m_ru_mgr->add_to_symtab(var_name);
	return register_var(sym, dt, elem_type, pointer_base_size,
						mem_size, align, flag);
}


void VAR_MGR::assign_var_id(VAR * v)
{
	UINT id = m_freelist_of_varid.remove_head();
	if (id != 0) {
		VAR_id(v) = id;
	} else {
		VAR_id(v) = m_var_count++;
	}
	IS_TRUE(VAR_id(v) < 5000000, ("too many variables"));
	IS_TRUE0(m_var_vec.get(VAR_id(v)) == NULL);
	m_var_vec.set(VAR_id(v), v);
}


//'var_name': name of the variable, it is optional.
VAR * VAR_MGR::register_var(SYM * var_name, DATA_TYPE dt, DATA_TYPE elem_type,
							UINT pointer_base_size, UINT mem_size, UINT align,
							UINT flag)
{
	IS_TRUE0(var_name != NULL);
	VAR * v = new_var();
	VAR_name(v) = var_name;
	VAR_data_type(v) = dt;
	VAR_data_size(v) = mem_size;
	VAR_elem_type(v) = elem_type;
	VAR_align(v) = align;
	VAR_pointer_base_size(v) = pointer_base_size;
	VAR_flag(v) = flag;
	assign_var_id(v);
	return v;
}


//'var_name': name of the variable, it is optional.
VAR * VAR_MGR::register_var(SYM * var_name, UINT tyid, UINT align, UINT flag)
{
	IS_TRUE0(var_name != NULL && tyid != 0);
	DTD const* d = m_dm->get_dtd(tyid);
	IS_TRUE0(d);
	return register_var(var_name, DTD_rty(d), DTD_vec_ety(d),
						DTD_ptr_base_sz(d), m_dm->get_dtd_bytesize(d),
						align, flag);
}


//'var_name': name of the variable, it is optional.
VAR * VAR_MGR::register_var(CHAR const* var_name, UINT tyid,
							UINT align, UINT flag)
{
	IS_TRUE0(var_name != NULL && tyid != 0);
	SYM * sym = m_ru_mgr->add_to_symtab(var_name);
	return register_var(sym, tyid, align, flag);
}


/*
Return VAR if there is already related to 's',
otherwise create a new VAR.
'var_name': name of the variable, it is optional.
's': string's content.
*/
VAR * VAR_MGR::register_str(CHAR const* var_name, SYM * s, UINT align)
{
	IS_TRUE0(s != NULL);
	VAR * v;
	if ((v = m_str_tab.get(s)) != NULL) {
		return v;
	}
	v = new_var();
	CHAR buf[64];
	if (var_name == NULL) {
		sprintf(buf, ".rodata_%zu", m_str_count++);
		VAR_name(v) = m_ru_mgr->add_to_symtab(buf);
	} else {
		VAR_name(v) = m_ru_mgr->add_to_symtab(var_name);
	}
	VAR_str(v) = s;
	VAR_data_type(v) = D_STR;
	VAR_elem_type(v) = D_UNDEF;
	IS_TRUE0(sizeof(CHAR) == 1);
	VAR_data_size(v) = xstrlen(SYM_name(s)) + 1;
	VAR_align(v) = align;
	VAR_is_global(v) = 1; //store in .data or .rodata

	assign_var_id(v);
	m_str_tab.set(s, v);
	return v;
}


//Dump all variables registered.
void VAR_MGR::dump(CHAR * name)
{
	FILE * h = g_tfile;
	if (name != NULL) {
		h = fopen(name, "a+");
		IS_TRUE0(h);
	}
	if (h == NULL) return;
	fprintf(h, "\n\nVAR to DECL Mapping:");
	CHAR buf[MAX_BUF_LEN];

	for (INT i = 0; i <= m_var_vec.get_last_idx(); i++) {
		VAR * v = m_var_vec.get(i);
		if (v == NULL) { continue; }
		buf[0] = 0;
		fprintf(h, "\n%s", v->dump(buf));
	}

	fprintf(h, "\n");
	fflush(h);
	if (h != g_tfile) {
		fclose(h);
	}
}
//END VAR_MGR

