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

//
//START IR_EXPR_TAB
//
IR_EXPR_TAB::IR_EXPR_TAB(REGION * ru)
{
	m_expr_count = 0;
	m_ru = ru;
	m_dm = ru->get_dm();
	memset(m_level1_hash_tab, 0,
		   sizeof(IR_EXPR*) * IR_EXPR_TAB_LEVEL1_HASH_BUCKET);
	m_pool = smpool_create_handle(sizeof(IR_EXPR*) * 128, MEM_COMM);
	m_sc_pool = smpool_create_handle(sizeof(SC<IR_EXPR*>) * 4, MEM_CONST_SIZE);
	m_ir_expr_lst.set_pool(m_sc_pool);
}


IR_EXPR_TAB::~IR_EXPR_TAB()
{
	SC<IR_EXPR*> * sct;
	for (IR_EXPR * ie = m_ir_expr_lst.get_head(&sct);
		 ie != NULL; ie = m_ir_expr_lst.get_next(&sct)) {
		delete ie;
	}
	smpool_free_handle(m_pool);
	smpool_free_handle(m_sc_pool);
}


UINT IR_EXPR_TAB::count_mem()
{
	UINT count = 0;
	count += sizeof(m_expr_count);
	count += sizeof(m_ru);
	count += m_ir_expr_vec.count_mem();
	count += m_ir_expr_lst.count_mem();
	count += smpool_get_pool_size_handle(m_pool);
	count += sizeof(m_level1_hash_tab) * IR_EXPR_TAB_LEVEL1_HASH_BUCKET;
	count += m_map_ir2ir_expr.count_mem();
	count += m_md_set_mgr.count_mem();
	count += m_bs_mgr.count_mem();
	return count;
}


void * IR_EXPR_TAB::xmalloc(INT size)
{
	void * p = smpool_malloc_h(size, m_pool);
	IS_TRUE0(p);
	memset(p, 0, size);
	return p;
}


void IR_EXPR_TAB::clean_occ_list()
{
	INT last = m_ir_expr_vec.get_last_idx();
	for (INT i = 0; i <= last; i++) {
		IR_EXPR * ie = m_ir_expr_vec.get(i);
		if (ie == NULL) { continue; }
		IS_TRUE0(IR_EXPR_id(ie) == (UINT)i);
		IR_EXPR_occ_list(ie).clean();
	}
}


//Dump all IR expressions of region and its used MDs.
void IR_EXPR_TAB::dump_ir_expr_tab()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP IR_EXPR_TAB ----==");
	IR_DU_MGR * du_mgr = m_ru->get_du_mgr();
	INT last = m_ir_expr_vec.get_last_idx();
	for (INT i = 0; i <= last; i++) {
		IR_EXPR * ie = m_ir_expr_vec.get(i);
		if (ie == NULL) { continue; }
		IS_TRUE0(IR_EXPR_id(ie) == (UINT)i);
		fprintf(g_tfile, "\n\n----------- IR_EXPR(%d)", i);
		dump_ir(IR_EXPR_ir(ie), m_dm);
		fprintf(g_tfile, "\n\tOCC:");
		for (IR * occ = IR_EXPR_occ_list(ie).get_head();
			 occ != NULL; occ = IR_EXPR_occ_list(ie).get_next()) {
			fprintf(g_tfile, "IR%d", IR_id(occ));
			MD_SET const* use_mds = du_mgr->get_may_use(occ);
			if (use_mds != NULL) {
				fprintf(g_tfile, "(use:");
				use_mds->dump(m_ru->get_md_sys());
				fprintf(g_tfile, ")");
			}
			fprintf(g_tfile, ",");
		}
	}
	fflush(g_tfile);
}


//If 'ir' has been inserted in the table with an IR_EXPR, get that and return.
IR_EXPR * IR_EXPR_TAB::map_ir2ir_expr(IR const* ir)
{
	if (ir == NULL) return NULL;
	return m_map_ir2ir_expr.get(IR_id(ir));
}


void IR_EXPR_TAB::set_map_ir2ir_expr(IR const* ir, IR_EXPR * ie)
{
	m_map_ir2ir_expr.set(IR_id(ir), ie);
}


UINT IR_EXPR_TAB::compute_hash_key(IR const* ir)
{
	IS_TRUE0(ir != NULL);
	UINT hval = IR_type(ir) + (ir->get_ofst() + 1) + IR_dt(ir);
	if (IR_type(ir) == IR_ID) {
		VAR * id_info = ID_info(ir);
		/*
		SYM * name = VAR_name(id_info);
		CHAR * s = SYM_name(name);
		UINT v = 0 ;
		while (*s != 0) {
			v += (UINT)(*s++);
		}
		hval += v;
		*/
		hval += VAR_id(id_info) * 5;
	}
	return hval;
}


UINT IR_EXPR_TAB::compute_hash_key_for_tree(IR * ir)
{
	UINT hval = 0;
	m_iter.clean();
	for (IR const* x = ir_iter_init_c(ir, m_iter);
		 x != NULL; x = ir_iter_next_c(m_iter)) {
		hval += compute_hash_key(x);
	}
	return hval;
}


/* Append IR tree expression into HASH table and return the entry-info.
If 'ir' has already been inserted in the table with an IR_EXPR,
get that and return. */
IR_EXPR * IR_EXPR_TAB::append_expr(IR * ir)
{
	if (ir == NULL) return NULL;
	UINT key = compute_hash_key_for_tree(ir);

	//First level hashing.
	UINT level1_hashv = key % IR_EXPR_TAB_LEVEL1_HASH_BUCKET;
	IR_EXPR ** level2_hash_tab = m_level1_hash_tab[level1_hashv];
	if (level2_hash_tab == NULL) {
		//Generate level2
		level2_hash_tab	=
			(IR_EXPR *(*))xmalloc(sizeof(IR_EXPR*) *
								   IR_EXPR_TAB_LEVEL2_HASH_BUCKET);
		m_level1_hash_tab[level1_hashv] = level2_hash_tab;

		//Generate copy of 'ir'.
		IR_EXPR * ie = new_ir_expr();
		IR_EXPR_id(ie) = ++m_expr_count;
		IR_EXPR_ir(ie) = m_ru->dup_irs(ir);
		//dump_ir(IR_EXPR_ir(ie));
		m_ir_expr_vec.set(IR_EXPR_id(ie), ie);

		//Enter into 'ir'
		UINT level2_hashv = key % IR_EXPR_TAB_LEVEL2_HASH_BUCKET;
		level2_hash_tab[level2_hashv] = ie;
		return ie;
	}

	//Scanning in level2 hash tab.
	UINT level2_hashv = key % IR_EXPR_TAB_LEVEL2_HASH_BUCKET;
	IR_EXPR * ie = level2_hash_tab[level2_hashv];
	if (ie == NULL) {
		//Generate copy of 'ir'.
		ie = new_ir_expr();
		IR_EXPR_id(ie) = ++m_expr_count;
		IR_EXPR_ir(ie) = m_ru->dup_irs(ir);
		//dump_ir(IR_EXPR_ir(ie));
		m_ir_expr_vec.set(IR_EXPR_id(ie), ie);

		//Enter into 'ir'
		level2_hash_tab[level2_hashv] = ie;
		return ie;
	}

	//Scanning in IR_EXPR list in level2 hash tab.
	IR_EXPR * last = NULL;
	while (ie != NULL) {
		if (ir->is_ir_equal(IR_EXPR_ir(ie))) {
			return ie;
		}
		last = ie;
		ie = IR_EXPR_next(ie);
	} //end while

	//Generate copy of 'ir'.
	ie = new_ir_expr();
	IR_EXPR_id(ie) = ++m_expr_count;
	IR_EXPR_ir(ie) = m_ru->dup_irs(ir);
	//dump_ir(IR_EXPR_ir(ie));
	m_ir_expr_vec.set(IR_EXPR_id(ie), ie);

	//Enter into 'ir'
	IS_TRUE0(level2_hash_tab[level2_hashv] != NULL);
	insertafter_one(&last, ie);
	return ie;
}


IR_EXPR * IR_EXPR_TAB::new_ir_expr()
{
	IR_EXPR * ie = new IR_EXPR();
	m_ir_expr_lst.append_head(ie);
	return ie;
}


//Remove occurence of IR_EXPR.
IR * IR_EXPR_TAB::remove_occ(IR * occ)
{
	if (occ == NULL) return NULL;
	IS_TRUE0(occ->is_exp());
	IR_EXPR * ie = map_ir2ir_expr(occ);
	if (ie == NULL) return NULL;
	return IR_EXPR_occ_list(ie).remove(occ);
}


//Remove all expr for given stmt out of occ list in expr-tab.
void IR_EXPR_TAB::remove_occs(IR * ir)
{
	IS_TRUE0(ir->is_stmt());
	switch (IR_type(ir)) {
	case IR_ST:
		{
			IR * stv = ST_rhs(ir);
			if (IR_is_const(stv)) return;
			this->remove_occ(stv);
		}
		break;
	case IR_IST:
		{
			IR * stv = IST_rhs(ir);
			if (!IR_is_const(stv)) {
				this->remove_occ(stv);
			}

			IR * m = IST_base(ir);
			if (IR_is_const(m)) return;
			this->remove_occ(m);
		}
		break;
	case IR_CALL:
	case IR_ICALL:
		{
			IR * p = CALL_param_list(ir);
			while (p != NULL) {
				if (!IR_is_const(p)) {
					this->remove_occ(p);
				}
				p = IR_next(p);
			}
		}
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		this->remove_occ(BR_det(ir));
		break;
	case IR_SWITCH:
		IS_TRUE0(SWITCH_vexp(ir));
		if (!IR_is_const(SWITCH_vexp(ir))) {
			this->remove_occ(SWITCH_vexp(ir));
		}
		break;
	case IR_IGOTO:
		IS_TRUE0(IGOTO_vexp(ir));
		if (!IR_is_const(IGOTO_vexp(ir))) {
			this->remove_occ(IGOTO_vexp(ir));
		}
		break;
	case IR_RETURN:
		if (RET_exp(ir) != NULL) {
			if (!IR_is_const(RET_exp(ir))) {
				this->remove_occ(RET_exp(ir));
			}
		}
		break;
	case IR_GOTO:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_CASE:
	case IR_BREAK:
	case IR_CONTINUE:
	case IR_PHI:
		break;
	default: IS_TRUE0(0);
	}
}


//Remove IR tree expression out of HASH table and return the removed
//entry-info if it was existed.
IR_EXPR * IR_EXPR_TAB::remove_expr(IR * ir)
{
	UINT key = compute_hash_key_for_tree(ir);

	//First level hashing.
	UINT level1_hashv = key % IR_EXPR_TAB_LEVEL1_HASH_BUCKET;
	IR_EXPR ** level2_hash_tab = m_level1_hash_tab[level1_hashv];
	if (level2_hash_tab == NULL) {
		return NULL;
	}

	//Scanning in level2 hash tab.
	UINT level2_hashv = key % IR_EXPR_TAB_LEVEL2_HASH_BUCKET;
	IR_EXPR * ie = level2_hash_tab[level2_hashv];
	if (ie == NULL) {
		return NULL;
	}

	//Scanning in IR_EXPR list in level2 hash tab.
	while (ie != NULL) {
		if (ir->is_ir_equal(IR_EXPR_ir(ie))) {
			remove(&level2_hash_tab[level2_hashv], ie);
			m_ir_expr_vec.remove(IR_EXPR_id(ie), NULL);
			return ie;
		}
		ie = IR_EXPR_next(ie);
	} //end while

	return NULL;
}


/*
Return entry-info if expression has been entered into HASH table,
otherwise return NULL.
*/
IR_EXPR * IR_EXPR_TAB::find_expr(IR * ir)
{
	if (ir == NULL) return NULL;
	UINT key = compute_hash_key_for_tree(ir);

	//First level hashing.
	UINT level1_hashv = key % IR_EXPR_TAB_LEVEL1_HASH_BUCKET;
	IR_EXPR ** level2_hash_tab = m_level1_hash_tab[level1_hashv];
	if (level2_hash_tab == NULL) {
		return NULL;
	}

	//Scanning in level2 hash tab.
	UINT level2_hashv = key % IR_EXPR_TAB_LEVEL2_HASH_BUCKET;
	IR_EXPR * ie = level2_hash_tab[level2_hashv];
	if (ie == NULL) {
		return NULL;
	}

	//Scanning in IR_EXPR list in level2 hash tab.
	while (ie != NULL) {
		if (ir->is_ir_equal(IR_EXPR_ir(ie))) {
			return ie;
		}
		ie = IR_EXPR_next(ie);
	} //end while
	return NULL;
}


IR_EXPR * IR_EXPR_TAB::encode_expr(IN IR * ir, IN IR_BB * bb)
{
	if (ir == NULL) return NULL;

	IS_TRUE0(ir->is_exp());
	switch (IR_type(ir)) {
	case IR_ID:
	case IR_LD:
	case IR_LDA:
	case IR_CONST:
	case IR_PR:
		return NULL;
	case IR_ILD:
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND: //logical and &&
	case IR_LOR: //logical or ||
	case IR_BAND: //bit and &
	case IR_BOR: //bit or |
	case IR_XOR:
	case IR_BNOT: //bitwise not
	case IR_LNOT: //logical not
	case IR_NEG:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ARRAY:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_CVT: //type convertion
	case IR_SELECT: //formulized determinate-expr?exp:exp
		{
			IR_EXPR * ie = append_expr(ir);
			IS_TRUE(!IR_EXPR_occ_list(ie).find(ir),
					("process same IR repeated."));
			IR_EXPR_occ_list(ie).append_tail(ir);
			return ie;
		}
	default: IS_TRUE0(0);
	}
	return NULL;
}


/* Encode expression for single BB.
Scan IR statement literally, and encoding it for generating
the unique id for each individual expressions, and update
the 'GEN-SET' and 'KILL-SET' of IR-EXPR for BB as well as. */
void IR_EXPR_TAB::encode_bb(IR_BB * bb)
{
	C<IR*> * ct;
	for (IR * ir = IR_BB_ir_list(bb).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&ct)) {
		IS_TRUE0(ir->is_stmt());
		switch (IR_type(ir)) {
		case IR_ST:
			{
				IR_EXPR * ie = encode_expr(ST_rhs(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(ST_rhs(ir), ie);
				}
			}
			break;
		case IR_STPR:
			{
				IR_EXPR * ie = encode_expr(STPR_rhs(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(STPR_rhs(ir), ie);
				}
			}
			break;
		case IR_IST:
			{
				IR_EXPR * ie = encode_expr(IST_rhs(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(IST_rhs(ir), ie);
				}

				ie = encode_istore_memaddr(IST_base(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(IST_base(ir), ie);
				}
			}
			break;
		case IR_ICALL: //indirective call
			{
				IR_EXPR * ie = encode_expr(ICALL_callee(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(ICALL_callee(ir), ie);
				}
			}
		case IR_CALL:
			{
				IR * parm = CALL_param_list(ir);
				while (parm != NULL) {
					IR_EXPR * ie = encode_expr(parm, bb);
					if (ie != NULL) {
						set_map_ir2ir_expr(parm, ie);
					}
					parm = IR_next(parm);
				}
			}
			break;
		case IR_GOTO:
			break;
		case IR_IGOTO:
			{
				IR_EXPR * ie = encode_expr(IGOTO_vexp(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(IGOTO_vexp(ir), ie);
				}
			}
			break;
		case IR_DO_WHILE:
		case IR_WHILE_DO:
		case IR_DO_LOOP: //loop with init , boundary , and step info
		case IR_IF:
			IS_TRUE(0, ("High level IR should be simplified"));
			break;
		case IR_LABEL:
			break;
		case IR_CASE:
		case IR_REGION:
			break;
		case IR_TRUEBR:
		case IR_FALSEBR:
			{
				IR_EXPR * ie = encode_expr(BR_det(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(BR_det(ir), ie);
				}
			}
			break;
		case IR_SWITCH:
			{
				IR_EXPR * ie = encode_expr(SWITCH_vexp(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(SWITCH_vexp(ir), ie);
				}
			}
			break;
		case IR_RETURN:
			{
				IR_EXPR * ie = encode_expr(RET_exp(ir), bb);
				if (ie != NULL) {
					set_map_ir2ir_expr(RET_exp(ir), ie);
				}
			}
			break;
		case IR_PHI:
			break;
		default: IS_TRUE0(0);
		} //end switch
	} //end for IR
	//dump_ir_expr_tab();
}


void IR_EXPR_TAB::reperform(IN OUT OPT_CTX & oc)
{
	clean_occ_list();
	perform(oc);
}


/* Encode expression for a list of BB.
Scan IR statement literally, and encoding it for generating
the unique id for each individual expressions, and update
the 'GEN-SET' and 'KILL-SET' of IR-EXPR for BB as well as. */
bool IR_EXPR_TAB::perform(IN OUT OPT_CTX & oc)
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	C<IR_BB*> * cb;
	for (IR_BB * bb = bbl->get_head(&cb);
		 bb != NULL; bb = bbl->get_next(&cb)) {
		encode_bb(bb);
	}
	OPTC_is_expr_tab_valid(oc) = true;
	return true;
}
//END IR_EXPR_TAB
