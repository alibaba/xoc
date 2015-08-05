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

#if (WORD_LENGTH_OF_HOST_MACHINE == 32)
	#define PADDR(ir) (dump_addr ? fprintf(g_tfile, " 0x%lx",(ULONG)(ir)) : 0)
	 //fprintf(g_tfile, " 0x%.8x",(ir)) : 0)
#elif (WORD_LENGTH_OF_HOST_MACHINE == 64)
	#define PADDR(ir) (dump_addr ? fprintf(g_tfile, " 0x%lx",(ULONG)(ir)) : 0)
	//fprintf(g_tfile, " 0x%.16x",(ir)) : 0)
#else
	#define PADDR(ir) (IS_TRUE0(0))
#endif

IR_DESC const g_ir_desc[] = {
	{IR_UNDEF,		"undef",		0x0,	0,	0,												0					},
	{IR_CONST,		"const",		0x0,	0,	IRT_IS_LEAF,									sizeof(CCONST)		},
	{IR_ID,			"id",			0x0,	0,	IRT_IS_LEAF,									sizeof(CID)			},
	{IR_LD,			"ld",			0x0,	0,	IRT_IS_MEM_REF|IRT_IS_MEM_OPND|IRT_IS_LEAF,		sizeof(CLD)			},
	{IR_ILD,		"ild",			0x1,	1,	IRT_IS_UNA|IRT_IS_MEM_REF|IRT_IS_MEM_OPND,		sizeof(CILD)		},
	{IR_PR, 		"pr",			0x0,	0,	IRT_IS_MEM_REF|IRT_IS_MEM_OPND|IRT_IS_LEAF, 	sizeof(CPR) 		},
	{IR_ARRAY,		"array",		0x3,	2,	IRT_IS_MEM_REF|IRT_IS_MEM_OPND, 				sizeof(CARRAY)		},
	{IR_ST,			"st",			0x1,	1,	IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,		sizeof(CST)			},
	{IR_STPR,		"stpr",			0x1,	1,	IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,		sizeof(CSTPR)		},
	{IR_IST,		"ist",			0x3,	2,	IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,		sizeof(CIST)		},
	{IR_SETEPR, 	"setepr",		0x3,	2,	IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,		sizeof(CSETEPR) 	},
	{IR_GETEPR, 	"getepr",		0x3,	2,	IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,		sizeof(CGETEPR) 	},
	{IR_CALL,		"call",			0x1,	1,	IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,		sizeof(CCALL)		},
	{IR_ICALL,		"icall",		0x3,	2,	IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,		sizeof(CICALL)		},
	{IR_LDA,		"lda",			0x1,	1,	IRT_IS_UNA, 									sizeof(CLDA)		},
	{IR_ADD,		"add",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,	sizeof(CBIN)	},
	{IR_SUB,		"sub",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE,						sizeof(CBIN)	},
	{IR_MUL,		"mul",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,	sizeof(CBIN)	},
	{IR_DIV,		"div",			0x3,	2,	IRT_IS_BIN,										sizeof(CBIN)		},
	{IR_REM,		"rem",			0x3,	2,	IRT_IS_BIN,										sizeof(CBIN)		},
	{IR_MOD,		"mod",			0x3,	2,	IRT_IS_BIN,										sizeof(CBIN)		},
	{IR_LAND,		"land",			0x3,	2,	IRT_IS_BIN|IRT_IS_LOGICAL,						sizeof(CBIN)		},
	{IR_LOR,		"lor",			0x3,	2,	IRT_IS_BIN|IRT_IS_LOGICAL,						sizeof(CBIN)		},
	{IR_BAND,		"band",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,	sizeof(CBIN)	},
	{IR_BOR,		"bor",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE,						sizeof(CBIN)	},
	{IR_XOR,		"xor",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,	sizeof(CBIN)	},
	{IR_ASR,		"asr",			0x3,	2,	IRT_IS_BIN, 									sizeof(CBIN)		},
	{IR_LSR,		"lsr",			0x3,	2,	IRT_IS_BIN, 									sizeof(CBIN)		},
	{IR_LSL,		"lsl",			0x3,	2,	IRT_IS_BIN, 									sizeof(CBIN)		},
	{IR_LT, 		"lt",			0x3,	2,	IRT_IS_BIN|IRT_IS_RELATION, 					sizeof(CBIN)		},
	{IR_LE, 		"le",			0x3,	2,	IRT_IS_BIN|IRT_IS_RELATION, 					sizeof(CBIN)		},
	{IR_GT, 		"gt",			0x3,	2,	IRT_IS_BIN|IRT_IS_RELATION, 					sizeof(CBIN)		},
	{IR_GE, 		"ge",			0x3,	2,	IRT_IS_BIN|IRT_IS_RELATION, 					sizeof(CBIN)		},
	{IR_EQ, 		"eq",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE|IRT_IS_RELATION,	sizeof(CBIN)		},
	{IR_NE, 		"ne",			0x3,	2,	IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE|IRT_IS_RELATION,	sizeof(CBIN)		},
	{IR_BNOT,		"bnot",			0x1,	1,	IRT_IS_UNA,										sizeof(CUNA)		},
	{IR_LNOT,		"lnot",			0x1,	1,	IRT_IS_UNA|IRT_IS_LOGICAL,						sizeof(CUNA)		},
	{IR_NEG,		"neg",			0x1,	1,	IRT_IS_UNA,										sizeof(CUNA)		},
	{IR_CVT,		"cvt",			0x1,	1,	IRT_IS_UNA, 									sizeof(CCVT)		},
	{IR_GOTO,		"goto",			0x0,	0,	IRT_IS_STMT,									sizeof(CGOTO)		},
	{IR_IGOTO,		"igoto",		0x3,	2,	IRT_IS_STMT,									sizeof(CIGOTO)		},
	{IR_DO_WHILE,	"do_while",		0x3,	2,	IRT_IS_STMT,									sizeof(CDO_WHILE)	},
	{IR_WHILE_DO,	"while_do",		0x3,	2,	IRT_IS_STMT,									sizeof(CWHILE_DO)	},
	{IR_DO_LOOP,	"do_loop",		0xF,	4,	IRT_IS_STMT,									sizeof(CDO_LOOP)	},
	{IR_IF,			"if",			0x7,	3,	IRT_IS_STMT,									sizeof(CIF)			},
	{IR_LABEL,		"label",		0x0,	0,	IRT_IS_STMT,									sizeof(CLAB)		},
	{IR_SWITCH,		"switch",		0x7,	3,	IRT_IS_STMT,									sizeof(CSWITCH)		},
	{IR_CASE,		"case",			0x1,	1,	0,												sizeof(CCASE)		},
	{IR_TRUEBR,		"truebr",		0x1,	1,	IRT_IS_STMT,									sizeof(CTRUEBR)		},
	{IR_FALSEBR,	"falsebr",		0x1,	1,	IRT_IS_STMT,									sizeof(CFALSEBR)	},
	{IR_RETURN,		"return",		0x1,	1,	IRT_IS_STMT,									sizeof(CRET)		},
	{IR_SELECT,		"select",		0x7,	3,	0,												sizeof(CSELECT)		},
	{IR_BREAK,		"break",		0x0,	0,	IRT_IS_STMT,									sizeof(CBREAK)		},
	{IR_CONTINUE,	"continue",		0x0,	0,	IRT_IS_STMT,									sizeof(CCONTINUE)	},
	{IR_PHI,		"phi",			0x1,	1,	IRT_IS_STMT|IRT_HAS_RESULT|IRT_IS_MEM_REF,		sizeof(CPHI)		},
	{IR_REGION,		"region",		0x0,	0,	IRT_IS_STMT,									sizeof(CREGION)		},
	{IR_TYPE_NUM,	"LAST IR TYPE",	0x0,	0,	0,												0					},
};


#ifdef _DEBUG_
INT ck_kid_num_valid(IR const* ir, UINT n, CHAR const* filename, INT line)
{
	UINT x = IR_MAX_KID_NUM(ir);
	IS_TRUEL(n < x, filename, line,
			("%d is beyond maximum IR kids num %d", n, x));
	return n;
}


INT ck_kid_num_valid_unary(IR const* ir, UINT n, CHAR const* filename,
						   INT line)
{
	UINT x = IR_MAX_KID_NUM(ir);
	IS_TRUEL(n < x, filename, line,
			("%d is beyond maximum IR kids num %d", n, x));
	IS_TRUE0(ir->is_unary_op());
	return n;
}


INT ck_kid_num_valid_binary(IR const* ir, UINT n, CHAR const* filename,
							INT line)
{
	UINT x = IR_MAX_KID_NUM(ir);
	IS_TRUEL(n < x, filename, line,
			("%d is beyond maximum IR kids num %d", n, x));
	IS_TRUE0(ir->is_bin_op());
	return n;
}


INT ck_kid_num_valid_branch(IR const* ir, UINT n, CHAR const* filename,
							INT line)
{
	UINT x = IR_MAX_KID_NUM(ir);
	IS_TRUEL(n < x, filename, line,
			("%d is beyond maximum IR kids num %d", n, x));
	IS_TRUE0(ir->is_truebr() || ir->is_falsebr());
	return n;
}


INT ck_kid_num_valid_loop(IR const* ir, UINT n, CHAR const* filename, INT line)
{
	UINT x = IR_MAX_KID_NUM(ir);
	IS_TRUEL(n < x, filename, line,
			("%d is beyond maximum IR kids num %d", n, x));
	IS_TRUE0(IR_type(ir) == IR_WHILE_DO || IR_type(ir) == IR_DO_WHILE ||
			 IR_type(ir) == IR_DO_LOOP);
	return n;
}


INT ck_kid_num_valid_call(IR const* ir, UINT n, CHAR const* filename, INT line)
{
	UINT x = IR_MAX_KID_NUM(ir);
	IS_TRUEL(n < x, filename, line,
			("%d is beyond maximum IR kids num %d", n, x));
	IS_TRUE0(ir->is_call());
	return n;
}


INT ck_kid_num_irty(IR const* ir, UINT n, IR_TYPE irty,
					CHAR const* filename, INT line)
{
	UINT x = IR_MAX_KID_NUM(ir);
	IS_TRUEL(n < x, filename, line,
			("%d is beyond maximum IR kids num %d", n, x));
	IS_TRUE0(IR_type(ir) == irty);
	return n;
}


IR const* ck_irt(IR const* ir, IR_TYPE irt)
{
	IS_TRUE(IR_type(ir) == irt, ("current ir is not '%s'", IRTNAME(irt)));
	return ir;
}


IR const* ck_irt_br(IR const* ir)
{
	IS_TRUE0(IR_type(ir) == IR_TRUEBR || IR_type(ir) == IR_FALSEBR);
	return ir;
}


IR const* ck_irt_call(IR const* ir)
{
	IS_TRUE0(IR_type(ir) == IR_CALL || IR_type(ir) == IR_ICALL);
	return ir;
}


IR const* ck_irt_only_call(IR const* ir)
{
	IS_TRUE0(IR_type(ir) == IR_CALL);
	return ir;
}


IR const* ck_irt_only_icall(IR const* ir)
{
	IS_TRUE0(IR_type(ir) == IR_ICALL);
	return ir;
}


UINT ck_dim(IR const* ir, UINT n)
{
	UINT i = 0;
	for (IR const* sub = ARR_sub_list(ir); sub != NULL; sub = IR_next(sub)) {
		i++;
	}
	IS_TRUE0(n < i);
	return n;
}
#endif


//Dump ir list with a clause header.
void dump_irs_h(IR * ir_list , DT_MGR const* dm)
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n==---- DUMP IR LIST ----==\n");
	dump_irs(ir_list, dm, NULL);
}


//Dump IR, and both its kids and siblings.
void dump_irs(IR * ir_list, DT_MGR const* dm, CHAR * attr)
{
	if (g_tfile == NULL) { return; }
	bool first_one = true;
	while (ir_list != NULL) {
		if (first_one) {
			first_one = false;
			dump_ir(ir_list, dm, attr);
		} else {
			dump_ir(ir_list, dm);
		}
		ir_list = IR_next(ir_list);
	}
}


static void verify_ir(IR * ir, IR_ADDR_HASH * irh, DT_MGR const* dm)
{
	IS_TRUE0(irh != NULL);
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * k = ir->get_kid(i);
		if (k != NULL) {
			IS_TRUE0(IR_parent(k) == ir);
			verify_irs(k, irh, dm);
		}
	}

	//IR can not be used more than twice. Since there will cause
	//memory crash during free_ir().
	IS_TRUE(!irh->find(ir), ("IR has been used again"));
	irh->append(ir);
	ir->verify(dm);
}


//Check for IR and IR_BB sanity and uniqueness.
//Ensure that all IRs must be embedded into a basic block.
//Ensure that PHI must be the first stmt in basic block.
bool verify_ir_and_bb(IR_BB_LIST * bblst, DT_MGR const* dm)
{
	IR_ADDR_HASH irh;
	for (IR_BB * bb = bblst->get_head();
		 bb != NULL; bb = bblst->get_next()) {
		bool should_not_phi = false;

		C<IR*> * irct;
		for (IR * ir = IR_BB_ir_list(bb).get_head(&irct);
			 ir != NULL; ir = IR_BB_ir_list(bb).get_next(&irct)) {
			IS_TRUE0(IR_next(ir) == NULL && IR_prev(ir) == NULL);
			IS_TRUE0(IR_parent(ir) == NULL);
			IS_TRUE0(ir->get_bb() == bb);

			if (IR_type(ir) != IR_PHI) {
				should_not_phi = true;
			}

			if (should_not_phi) {
				IS_TRUE0(IR_type(ir) != IR_PHI);
			}

			verify_irs(ir, &irh, dm);
			IS_TRUE0(ir->is_stmt_in_bb());
		}
		bb->verify();
	}
	return true;
}


//Check for IR is lowest form.
bool verify_lowest_form(IR_BB_LIST * ir_bb_list, REGION * ru)
{
	for (IR_BB * bb = ir_bb_list->get_head();
		 bb != NULL; bb = ir_bb_list->get_next()) {
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			IS_TRUE0(ru->is_lowest_heigh(ir));
		}
	}
	return true;
}


//Function to verify stmt info after IR simplified.
bool verify_simp(IR * ir_list, SIMP_CTX & simp)
{
	if (simp.is_simp_cfg()) {
		for (IR * p = ir_list; p != NULL; p = IR_next(p)) {
			IS_TRUE0(p->is_stmt());
			IS_TRUE0(IR_parent(p) == NULL);
		}
	}
	return true;
}


//Check for IR sanity and uniqueness.
bool verify_irs(IR * ir, IR_ADDR_HASH * irh, DT_MGR const* dm)
{
	IR_ADDR_HASH * loc = NULL;
	if (irh == NULL) {
		loc = new IR_ADDR_HASH();
		irh = loc;
	}
	while (ir != NULL) {
		verify_ir(ir, irh, dm);
		ir = IR_next(ir);
	}
	if (loc != NULL) {
		delete loc;
	}
	return true;
}


void dump_irs(IN LIST<IR*> & ir_list, DT_MGR const* dm)
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP IR List ----==\n");
	IS_TRUE0(dm);
	for (IR * ir = ir_list.get_head();
		 ir != NULL; ir = ir_list.get_next()) {
		IS_TRUE0(IR_next(ir) == NULL && IR_prev(ir) == NULL);
		dump_ir(ir, dm);
	}
	fflush(g_tfile);
}


void dump_irs(IR_LIST & ir_list, DT_MGR const* dm)
{
	dump_irs((LIST<IR*>&)ir_list, dm);
}


static void ir_dump_lab(IR const* ir)
{
	LABEL_INFO * li = ir->get_label();
	if (LABEL_INFO_type(li) == L_ILABEL) {
		fprintf(g_tfile, "ilabel(" ILABEL_STR_FORMAT ")",
						 ILABEL_CONT(li));
	} else if (LABEL_INFO_type(li) == L_CLABEL) {
		fprintf(g_tfile, "clabel(" CLABEL_STR_FORMAT ")",
						 CLABEL_CONT(li));
	} else {
		IS_TRUE(0, ("unknown label type"));
	}
}


//Dump IR and all of its kids.
//'attr': miscellaneous string which following 'ir'.
void dump_ir(IN IR const* ir, DT_MGR const* dm, IN CHAR * attr,
			 bool dump_kid, bool dump_src_line, bool dump_addr)
{
	#ifdef _DEBUG_
	IS_TRUE0(dm);
	UINT dn = 4;
	if (g_tfile == NULL) return;
	if (ir == NULL) return;

	//Attribution string do NOT exceed length of 128 chars.
	static CHAR attr_buf[128];
	if (attr == NULL) {
		attr = attr_buf;
		*attr = 0;
	} else {
		sprintf(attr_buf, "%s", attr);
		attr = attr_buf;
	}

	CHAR * p = attr + strlen(attr);
	sprintf(p, " id:%d", IR_id(ir));
	if (IR_may_throw(ir)) {
		strcat(p, " throw");
	}
	if (IR_is_termiate(ir)) {
		strcat(p, " terminate");
	}
	if (IR_is_atomic(ir)) {
		strcat(p, " atom");
	}
	if (IR_is_read_mod_write(ir)) {
		strcat(p, " rmw");
	}
	if (IR_has_sideeffect(ir)) {
		strcat(p, " sideeffect");
	}

	//Record type info and var decl.
	static CHAR buf[MAX_BUF_LEN];
	static CHAR buf2[MAX_BUF_LEN];
	buf[0] = 0;
	buf2[0] = 0;
	if (g_dbg_mgr != NULL && dump_src_line) {
		g_dbg_mgr->print_src_line(ir);
	}
	DTD const* d = NULL;
	UINT ptsz = 0;
	if (IR_dt(ir) != VOID_TY) {
		d = ir->get_dtd(dm);
		ptsz = DTD_ptr_base_sz(d);
	}
	DT_MGR * xdm = const_cast<DT_MGR*>(dm);
	switch (IR_type(ir)) {
	case IR_ST:
		{
			CHAR tt[40];
			tt[0] = 0;
			CHAR * name = xstrcat(tt, 40, "%s",
								SYM_name(VAR_name(ST_idinfo(ir))));
			buf[0] = 0;

			//Dump operator and variable name.
			note("\nst %s:", name);
			fprintf(g_tfile, "%s", xdm->dump_dtd(d, buf));
			if (ST_ofst(ir) != 0) {
				fprintf(g_tfile, ":ofst<%d>", ST_ofst(ir));
			}

			//Dump declaration info if the frontend supplied.
			buf[0] = 0;
			if (ST_idinfo(ir)->dump_var_decl(buf, 40) != NULL) {
				fprintf(g_tfile, " decl:%s", buf);
			}

			PADDR(ir);
			fprintf(g_tfile, "%s", attr);

			if (dump_kid) {
				g_indent += dn;
				dump_irs(ST_rhs(ir), dm);
				g_indent -= dn;
			}
		}
		break;
	case IR_STPR:
		note("\nst $pr%d:%s", STPR_no(ir), xdm->dump_dtd(d, buf));
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);

		if (dump_kid) {
			g_indent += dn;
			dump_irs(STPR_rhs(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_IST:
		if (IST_ofst(ir) != 0) {
			note("\nist:%s:ofst<%d>", xdm->dump_dtd(d, buf), IST_ofst(ir));
		} else {
			note("\nist:%s", xdm->dump_dtd(d, buf));
		}

		//Dump IR address.
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);

		if (dump_kid) {
			g_indent += dn;
			dump_irs(IST_base(ir), dm, (CHAR*)" lhs");
			dump_irs(IST_rhs(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_LD:
		{
			CHAR tt[40];
			tt[0] = 0;

			//Dump variable info.
			CHAR * name = xstrcat(tt, 40, "%s",
								SYM_name(VAR_name(LD_idinfo(ir))));
			buf[0] = 0;
			if (LD_ofst(ir) != 0) {
				note("\nld '%s':%s:ofst<%d>", name,
					 xdm->dump_dtd(d, buf), LD_ofst(ir));
			} else {
				note("\nld '%s':%s", name, xdm->dump_dtd(d, buf));
			}

			//Dump declaration if frontend supplied.
			buf[0] = 0;
			if (LD_idinfo(ir)->dump_var_decl(buf, 40) != NULL) {
				fprintf(g_tfile, " decl:%s", buf);
			}

			//Dump IR address.
			PADDR(ir);
			fprintf(g_tfile, "%s", attr);
		}
		break;
	case IR_ILD:
		if (ILD_ofst(ir) != 0) {
			note("\nild:%s:ofst<%d>", xdm->dump_dtd(d, buf), ILD_ofst(ir));
		} else {
			note("\nild:%s", xdm->dump_dtd(d, buf));
		}
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(ILD_base(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_PR:
		note("\n$pr%d:%s", PR_no(ir), xdm->dump_dtd(d, buf));
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		break;
	case IR_ID:
		{
			buf[0] = 0;
			CHAR tt[40];
			tt[0] = 0;

			//Dump ID name.
			CHAR * name = xstrcat(tt, 40, "%s",
								SYM_name(VAR_name(ID_info(ir))));
			note("\nid '%s':%s", name, xdm->dump_dtd(d, buf));
			buf[0] = 0;
			if (ID_info(ir)->dump_var_decl(buf, 40) != NULL) {
				fprintf(g_tfile, " decl:%s", buf);
			}

			//Dump IR address.
			PADDR(ir);
			fprintf(g_tfile, "%s", attr);
			break;
		}
	case IR_CONST:
		if (ir->is_sint(dm)) {
			note("\nintconst %lld|0x%llx:%s",
				 CONST_int_val(ir),
				 CONST_int_val(ir),
				 xdm->dump_dtd(d, buf));
		} else if (ir->is_uint(dm)) {
			note("\nintconst %llu|0x%llx:%s",
				 CONST_int_val(ir),
				 CONST_int_val(ir),
				 xdm->dump_dtd(d, buf));
		} else if (ir->is_fp(dm)) {
			note("\nfpconst %f:%s",
				 CONST_fp_val(ir),
				 xdm->dump_dtd(d, buf));
		} else if (ir->is_bool(dm)) {
			note("\nboolconst %d:%s",
				 (UINT)CONST_int_val(ir),
				 xdm->dump_dtd(d, buf));
		} else if (ir->is_str(dm)) {
			CHAR tt[40];
			tt[0] = 0;
			CHAR * name = xstrcat(tt, 40, "%s", SYM_name(CONST_str_val(ir)));
			buf[0] = 0;
			note("\nstrconst \\\"%s....\\\":%s",
				 name, xdm->dump_dtd(d, buf));
		} else if (dm->is_mem(IR_dt(ir))) {
			//IS_TRUE(0, ("immediate value cannot be MEM block"));
			note("\nintconst %lld|0x%llx:%s",
				 CONST_int_val(ir),
				 CONST_int_val(ir),
				 xdm->dump_dtd(d, buf));
		} else {
			//Dump const even if it is illegal type, leave the assertion
			//work to verify().
			note("\nintconst %lld|0x%llx:%s",
				 CONST_int_val(ir),
				 CONST_int_val(ir),
				 xdm->dump_dtd(d, buf));
			//IS_TRUE(0, ("unsupport immediate value DATA_TYPE:%d",
			//		ir->get_rty(dm)));
		}
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		break;
	case IR_LOR: //logical or ||
	case IR_LAND: //logical and &&
	case IR_BOR: //inclusive or |
	case IR_BAND: //inclusive and &
	case IR_XOR: //exclusive or
	case IR_EQ: //==
	case IR_NE: //!=
	case IR_LT:
	case IR_GT:
	case IR_GE:
	case IR_LE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_ADD:
	case IR_SUB:
	case IR_DIV:
	case IR_MUL:
	case IR_MOD:
	case IR_REM:
		note("\n%s:%s", IRNAME(ir), xdm->dump_dtd(d, buf));
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(BIN_opnd0(ir), dm);
			dump_irs(BIN_opnd1(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_IF:
		note("\nif");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(IF_det(ir), dm);
			g_indent -= dn;

			note("\n{");
			g_indent += dn;
			dump_irs(IF_truebody(ir), dm);
			g_indent -= dn;
			note("\n}");

			if (IF_falsebody(ir)) {
				note("\n else");
				note("\n{");
				g_indent += dn;
				dump_irs(IF_falsebody(ir), dm);
				g_indent -= dn;
				note("\n}");
			}
		}
		break;
	case IR_DO_WHILE:
		note("\ndo_while");
		note("\nbody");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);

		if (dump_kid) {
			g_indent += dn;
			dump_irs(LOOP_body(ir), dm);
			g_indent -= dn;

			note("\ndet");
			g_indent += dn;
			dump_irs(LOOP_det(ir), dm);
			g_indent -= dn;

			note("\nend_do_while");
		}
		break;
	case IR_WHILE_DO:
		note("\nwhile_do");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);

		if (dump_kid) {
			g_indent += dn;
			dump_irs(LOOP_det(ir), dm);
			g_indent -= dn;

			note("\nbody");

			g_indent += dn;
			dump_irs(LOOP_body(ir), dm);
			g_indent -= dn;

			note("\nend_while_do");
		}
		break;
	case IR_DO_LOOP:
		note("\ndo_loop");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);

		if (dump_kid) {
			g_indent += dn;
			note("\ninit");

			g_indent += dn;
			dump_irs(LOOP_init(ir), dm);
			g_indent -= dn;

			note("\ndet");

			g_indent += dn;
			dump_irs(LOOP_det(ir), dm);
			g_indent -= dn;

			note("\nstep");

			g_indent += dn;
			dump_irs(LOOP_step(ir), dm);
			g_indent -= dn;

			note("\ndo_loop_body");

			g_indent += dn;
			dump_irs(LOOP_body(ir), dm);
			g_indent -= dn;
			note("\nend_do_loop");
		}
		break;
	case IR_BREAK:
		note("\nbreak");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		break;
	case IR_CONTINUE:
		note("\ncontinue");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		break;
	case IR_RETURN:
		note("\nreturn");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_ir(RET_exp(ir), dm, NULL, dump_kid,
					dump_src_line, dump_addr);
			g_indent -= dn;
		}
		break;
	case IR_GOTO:
		note("\ngoto ");
		ir_dump_lab(ir);
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		break;
	case IR_IGOTO:
		note("\nigoto");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(IGOTO_vexp(ir), dm);
			g_indent -= dn;

			note("\ncase_list");
			g_indent += dn;
			dump_irs(IGOTO_case_list(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_LABEL:
		{
			LABEL_INFO * li = LAB_lab(ir);
			if (LABEL_INFO_type(li) == L_ILABEL) {
				note("\nilabel(" ILABEL_STR_FORMAT ")",
					 ILABEL_CONT(LAB_lab(ir)));
			} else if (LABEL_INFO_type(li) == L_CLABEL) {
				note("\nclabel(" CLABEL_STR_FORMAT ")",
					 CLABEL_CONT(LAB_lab(ir)));
			} else if (LABEL_INFO_type(li) == L_PRAGMA) {
				note("\npragms(%s)", SYM_name(LABEL_INFO_pragma(LAB_lab(ir))));
			} else { IS_TRUE0(0); }
			PADDR(ir);
			if (LABEL_INFO_b1(li) != 0) {
				fprintf(g_tfile, "(");
			}
			if (LABEL_INFO_is_try_start(li)) {
				fprintf(g_tfile, "try_start ");
			}
			if (LABEL_INFO_is_try_end(li)) {
				fprintf(g_tfile, "try_end ");
			}
			if (LABEL_INFO_is_catch_start(li)) {
				fprintf(g_tfile, "catch_start ");
			}
			if (LABEL_INFO_is_used(li)) {
				fprintf(g_tfile, "used ");
			}
			if (LABEL_INFO_b1(li) != 0) {
				fprintf(g_tfile, ")");
			}
			fprintf(g_tfile, "%s", attr);
		}
		break;
	case IR_SELECT: //formulized log_OR_exp?exp:cond_exp
		note("\nselect:%s", xdm->dump_dtd(d, buf));

		PADDR(ir); fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(SELECT_det(ir), dm);
			g_indent -= dn;

			g_indent += dn;
			dump_irs(SELECT_trueexp(ir), dm, (CHAR*)" true_exp");
			g_indent -= dn;

			g_indent += dn;
			dump_irs(SELECT_falseexp(ir), dm, (CHAR*)" false_exp");
			g_indent -= dn;
		}
		break;
	case IR_CVT: //type convertion
		note("\ncvt:%s", xdm->dump_dtd(d, buf));

		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(CVT_exp(ir), dm, (CHAR*)" cvtbase");
			g_indent -= dn;
		}
		break;
	case IR_LDA: //Get address of a symbol
		if (LDA_ofst(ir) != 0) {
			note("\nlda:%s:ofst<%d>", xdm->dump_dtd(d, buf), LDA_ofst(ir));
		} else {
			note("\nlda:%s", xdm->dump_dtd(d, buf));
		}

		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(LDA_base(ir), dm);
			g_indent -= dn;
		}
		break;
  	case IR_NEG: // -123
		note("\nneg:%s", xdm->dump_dtd(d, buf));
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(UNA_opnd0(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_PHI:
		note("\nphi $pr%d:%s", PHI_prno(ir), xdm->dump_dtd(d, buf));

		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			IR * opnd = PHI_opnd_list(ir);
			while (opnd != NULL) {
				dump_ir(opnd, dm, NULL);
				opnd = IR_next(opnd);
			}
			g_indent -= dn;
		}
		break;
	case IR_BNOT: //bitwise not
		note("\nbnot:%s", xdm->dump_dtd(d, buf));

		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(UNA_opnd0(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_LNOT: //logical not
		note("\nlnot:%s", xdm->dump_dtd(d, buf));

		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(UNA_opnd0(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_SWITCH:
		note("\nswitch");
		if (SWITCH_deflab(ir) != NULL) {
			fprintf(g_tfile, ", deflab: ");
			ir_dump_lab(ir);
		}
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(SWITCH_vexp(ir), dm);
			g_indent -= dn;

			note("\ncase_list");
			g_indent += dn;
			dump_irs(SWITCH_case_list(ir), dm);
			g_indent -= dn;

			note("\nswitch_body");
			g_indent += dn;
			dump_irs(SWITCH_body(ir), dm);
			g_indent -= dn;
			note("\nend_switch");
		}
		break;
	case IR_CASE:
		{
			IS_TRUE0(CASE_vexp(ir));
			IS_TRUE0(CASE_lab(ir));
			note("\ncase %d, ", CONST_int_val(CASE_vexp(ir)));
			ir_dump_lab(ir);
			PADDR(ir);
			fprintf(g_tfile, "%s", attr);
		}
		break;
	case IR_ARRAY:
		if (ARR_ofst(ir) != 0) {
			note("\narray (%s:ofst<%d>, ety:%s)",
				 xdm->dump_dtd(d, buf),
				 ARR_ofst(ir),
				 xdm->dump_dtd(dm->get_dtd(ARR_elem_ty(ir)), buf2));
		} else {
			note("\narray (%s, ety:%s)",
				 xdm->dump_dtd(d, buf),
				 xdm->dump_dtd(dm->get_dtd(ARR_elem_ty(ir)), buf2));
		}

		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (ARR_sub_list(ir) != NULL && dump_kid) {
			CHAR tt[40];
			IS_TRUE0(ARR_elem_num_buf(ir));

			//Dump elem number.
			g_indent += dn;
			UINT dim = 0;
			note("\nenum[");
			for (IR const* sub = ARR_sub_list(ir); sub != NULL;) {
				fprintf(g_tfile, "%d", ARR_elem_num(ir, dim));
				sub = IR_next(sub);
				if (sub != NULL) {
					fprintf(g_tfile, ",");
				}
				dim++;
			}
			fprintf(g_tfile, "]");

			//Dump sub exp list.
			dim = 0;
			for (IR const* sub = ARR_sub_list(ir);
				 sub != NULL; sub = IR_next(sub)) {
				sprintf(tt, " dim%d", dim);
				dump_ir(sub, dm, (CHAR*)tt);
				dim++;
			}
			g_indent -= dn;
		}
		if (dump_kid) {
			g_indent += dn;
			dump_irs(ARR_base(ir), dm, (CHAR*)" array_base");
			g_indent -= dn;
		}
		break;
	case IR_CALL:
	case IR_ICALL:
		{
			if (ir->has_return_val()) {
				note("\n$pr%d:%s", CALL_prno(ir), xdm->dump_dtd(d, buf));
				note("\n= ");
			} else {
				note("\n");
			}

			if (IR_type(ir) == IR_CALL) {
				CHAR tt[40];
				tt[0] = 0;
				CHAR * name = xstrcat(tt, 40, "%s",
									  SYM_name(VAR_name(CALL_idinfo(ir))));
				fprintf(g_tfile, "call '%s' ", name);
				buf[0] = 0;
				if (CALL_idinfo(ir)->dump_var_decl(buf, 40) != NULL) {
					fprintf(g_tfile, "decl:%s", buf);
				}
			} else {
				fprintf(g_tfile, "icall ");
			}

			PADDR(ir);
			fprintf(g_tfile, "%s", attr);

			if (dump_kid) {
				if (IR_type(ir) == IR_ICALL) {
					g_indent += dn;
					dump_ir(ICALL_callee(ir), dm, (CHAR*)" callee",
							dump_kid, dump_src_line, dump_addr);
					g_indent -= dn;
				}

				//Dump parameter list.
				IR * tmp = CALL_param_list(ir);
				UINT i = 0;
				CHAR tmpbuf[30];
				while (tmp != NULL) {
					sprintf(tmpbuf, " param%d", i);
					g_indent += dn;
					dump_ir(tmp, dm, tmpbuf, dump_kid,
							dump_src_line, dump_addr);
					g_indent -= dn;
					i++;
					tmp = IR_next(tmp);
				}
			}
			break;
		}
	case IR_TRUEBR:
		note("\ntruebr ");
		ir_dump_lab(ir);
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(BR_det(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_FALSEBR:
		note("\nfalsebr ");
		ir_dump_lab(ir);
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		if (dump_kid) {
			g_indent += dn;
			dump_irs(BR_det(ir), dm);
			g_indent -= dn;
		}
		break;
	case IR_REGION:
		note("\nregion");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		break;
	case IR_UNDEF:
		note("\nundef!");
		PADDR(ir);
		fprintf(g_tfile, "%s", attr);
		break;
	default:
		IS_TRUE(0, ("unknown IR type:%s", IRNAME(ir)));
		return ;
	} //end switch
	fflush(g_tfile);
	#endif
} //end dump_ir


void irs_set_parent_pointer(IR * ir_list)
{
	while (ir_list != NULL) {
		ir_list->set_parent_pointer(true);
		ir_list = IR_next(ir_list);
	}
}


//Return the arthmetic precedence.
UINT get_arth_precedence(IR_TYPE ty)
{
	UINT p = 0;
	switch (ty) {
	case IR_ARRAY:
		p = 1;
		break;
	case IR_NEG:
	case IR_BNOT:
	case IR_LNOT:
	case IR_ILD:
	case IR_LDA:
	case IR_CVT:
		p = 2;
		break;
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
		p = 4;
		break;
	case IR_ADD:
	case IR_SUB:
		p = 5;
		break;
	case IR_LSL:
	case IR_ASR:
	case IR_LSR:
		p = 6;
		break;
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
		p = 7;
		break;
	case IR_EQ:
	case IR_NE:
		p = 8;
		break;
	case IR_BAND:
		p = 9;
		break;
	case IR_XOR:
		p = 10;
		break;
	case IR_BOR:
		p = 11;
		break;
	case IR_LAND:
		p = 12;
		break;
	case IR_LOR:
		p = 13;
		break;
	case IR_SELECT:
		p = 14;
		break;
	case IR_ST:
	case IR_STPR:
	case IR_SETEPR:
	case IR_GETEPR:
	case IR_IST:
	case IR_CALL:
	case IR_ICALL:
		p = 15;
		break;
	default: IS_TRUE(0, ("TODO"));
	}
	return p;
}
///



//
//START IR
//
//Check that IR cannot take a UNDEF type.
bool IR::verify(DT_MGR const* dm) const
{
	verify_kids();
	DTD const* d = NULL;
	DATA_TYPE dt = D_UNDEF;
	if (IR_dt(this) != VOID_TY) {
		d = dm->get_dtd(IR_dt(this));
		if (DTD_is_ptr(d)) {
			IS_TRUE0(DTD_ptr_base_sz(d) != 0);
		} else if (DTD_is_mc(d)) {
			IS_TRUE0(DTD_mc_sz(d) != 0);
		} else {
			/*
			IR_mc_size may be not zero.
			e.g: struct {int x;}s;
				int a = s.x;
				Here we get a IR_LD(s, ofst=0, mc_size=4)
			*/
			//IS_TRUE0(IR_mc_size(this) == 0);
		}

		dt = DTD_rty(d);
		if (DTD_vec_ety(d) != D_UNDEF) {
			//vector type.
			IS_TRUE0(dt == D_MC);

			IS_TRUE(IS_SIMPLEX(DTD_vec_ety(d)) || IS_PTR(DTD_vec_ety(d)),
					("illegal vector elem type"));

			UINT elemsize = dm->get_dtype_bytesize(DTD_vec_ety(d));
			IS_TRUE0(DTD_vec_sz(d) >= elemsize && DTD_vec_sz(d) % elemsize == 0);

			UINT ofst = get_ofst();
			if (ofst != 0) {
				IS_TRUE0((ofst % elemsize) == 0);
			}
		}
	}

	switch (IR_type(this)) {
	case IR_UNDEF: IS_TRUE(0, ("should not be undef")); break;
	case IR_CONST:
		if (!is_sint(dm) &&
			!is_uint(dm) &&
			!is_fp(dm) &&
			!is_bool(dm) &&
			//!dm->is_mem(IR_dt(this)) && //IR_CONST can not be MC.
			!is_str(dm)) {
			IS_TRUE(0, ("unsupport immediate value DATA_TYPE:%d",
					get_rty(dm)));
		}
		break;
	case IR_ID: break;
	case IR_LD:
		//src of LD might be small or big compare with REG.
		IS_TRUE0(LD_idinfo(this));
		IS_TRUE0(d);
		IS_TRUE(dt != D_UNDEF, ("size of load value cannot be zero"));
		break;
	case IR_ST:
		IS_TRUE0(d);
		IS_TRUE0(ST_idinfo(this));
		IS_TRUE0(ST_rhs(this)->is_exp());
		IS_TRUE(dt != D_UNDEF, ("size of store value cannot be zero"));
		IS_TRUE0(IR_next(ST_rhs(this)) == NULL &&
				 IR_prev(ST_rhs(this)) == NULL);
		if (DTD_is_vec(d)) {
			IS_TRUE0(DTD_vec_ety(d) != D_UNDEF);
			UINT elemsize = dm->get_dtype_bytesize(DTD_vec_ety(d));
			UINT rhssize = dm->get_dtd_bytesize(IR_dt(ST_rhs(this)));
			IS_TRUE0(elemsize >= rhssize);
		}
		break;
	case IR_STPR:
		IS_TRUE0(d);
		IS_TRUE0(this->get_rty(dm) != D_UNDEF);
		IS_TRUE0(STPR_no(this) > 0);
		IS_TRUE0(STPR_rhs(this)->is_exp());
		IS_TRUE(dt != D_UNDEF, ("size of store value cannot be zero"));
		IS_TRUE0(IR_next(STPR_rhs(this)) == NULL &&
				 IR_prev(STPR_rhs(this)) == NULL);
		break;
	case IR_ILD:
		IS_TRUE0(d);

		IS_TRUE0(ILD_base(this));

		IS_TRUE(ILD_base(this)->is_ptr(dm) ||
				IR_type(ILD_base(this)) == IR_ARRAY,
				("base must be pointer"));

		IS_TRUE0(ILD_base(this)->is_exp());

		IS_TRUE0(dt != D_UNDEF);

		IS_TRUE0(IR_next(ILD_base(this)) == NULL &&
				 IR_prev(ILD_base(this)) == NULL);
		break;
	case IR_IST:
		IS_TRUE0(d);
		IS_TRUE(IST_base(this)->is_ptr(dm) ||
				IR_type(IST_base(this)) == IR_ARRAY,
				("base must be pointer"));
		IS_TRUE0(IST_rhs(this)->is_exp());
		IS_TRUE(dt != D_UNDEF, ("size of istore value cannot be zero"));
		IS_TRUE0(IR_next(IST_base(this)) == NULL &&
				 IR_prev(IST_base(this)) == NULL);
		IS_TRUE0(IR_next(IST_rhs(this)) == NULL &&
				 IR_prev(IST_rhs(this)) == NULL);
		break;
	case IR_SETEPR:
		IS_TRUE0(d);

		IS_TRUE0(SETEPR_rhs(this) && SETEPR_ofst(this));

		IS_TRUE0(DTD_is_mc(d) || DTD_is_vec(d));

		if (DTD_is_vec(d)) {
			IS_TRUE0(DTD_vec_ety(d) != D_UNDEF);
			UINT elemsize = dm->get_dtype_bytesize(DTD_vec_ety(d));
			UINT rhssize = dm->get_dtd_bytesize(IR_dt(ST_rhs(this)));

			//Note if the rhssize less than elemsize, it will be hoist to
			//elemsize.
			IS_TRUE0(elemsize >= rhssize);
		}

		IS_TRUE0(SETEPR_rhs(this)->is_exp());

		IS_TRUE0(SETEPR_ofst(this)->is_exp());

		IS_TRUE(dt != D_UNDEF, ("size of istore value cannot be zero"));

		IS_TRUE0(IR_next(SETEPR_ofst(this)) == NULL &&
				 IR_prev(SETEPR_ofst(this)) == NULL);

		IS_TRUE0(IR_next(IST_rhs(this)) == NULL &&
				 IR_prev(IST_rhs(this)) == NULL);
		break;
	case IR_GETEPR:
		{
			IS_TRUE0(d);

			IR const* base = GETEPR_base(this);
			IS_TRUE0(base && GETEPR_ofst(this));
			DTD const* basedtd = dm->get_dtd(IR_dt(base));
			IS_TRUE0(DTD_is_mc(basedtd) || DTD_is_vec(basedtd));

			if (DTD_is_vec(basedtd)) {
				IS_TRUE0(DTD_vec_ety(basedtd) != D_UNDEF);

				UINT elemsize = dm->get_dtype_bytesize(DTD_vec_ety(basedtd));
				UINT geteprsize = dm->get_dtd_bytesize(d);

				IS_TRUE0(elemsize >= geteprsize);
			}
		}
		break;
	case IR_LDA:
		{
			IS_TRUE0(d);
			IR * base = LDA_base(this);
			IS_TRUE0(IR_type(base) == IR_ID || base->is_str(dm) ||
					 base->is_lab());
			IS_TRUE0(DTD_is_ptr(d));
			IS_TRUE0(IR_next(base) == NULL && IR_prev(base) == NULL);
		}
		break;
	case IR_CALL:
		IS_TRUE0(CALL_idinfo(this));

		//result type of call is the type of return value if it exist.
		IS_TRUE0(!((d != NULL) ^ (CALL_prno(this) != 0)));

		//Parameters should be expression.
		for (IR * p = CALL_param_list(this); p != NULL; p = IR_next(p)) {
			IS_TRUE0(p->is_exp());
		}
		break;
	case IR_ICALL:
		//result type of call is the type of return value if it exist.
		IS_TRUE0(!((d != NULL) ^ (CALL_prno(this) != 0)));

		//rtype of icall is the type of IR in return-value-list.
		IS_TRUE0(IR_type(ICALL_callee(this)) == IR_LD ||
				 ICALL_callee(this)->is_pr());

		IS_TRUE0(IR_next(ICALL_callee(this)) == NULL &&
				 IR_prev(ICALL_callee(this)) == NULL);

		for (IR * p = CALL_param_list(this); p != NULL; p = IR_next(p)) {
			IS_TRUE0(p->is_exp());
		}
		break;
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		IS_TRUE0(d);
		IS_TRUE0(BIN_opnd0(this) != NULL &&
				 BIN_opnd0(this)->is_exp() &&
				 BIN_opnd1(this) != NULL &&
				 BIN_opnd1(this)->is_exp());

		//Check that shift operations only have integer type.
		IS_TRUE0(BIN_opnd0(this)->is_int(dm) &&
				 BIN_opnd1(this)->is_int(dm));

		IS_TRUE0(IR_next(BIN_opnd0(this)) == NULL &&
				 IR_prev(BIN_opnd0(this)) == NULL);
		IS_TRUE0(IR_next(BIN_opnd1(this)) == NULL &&
				 IR_prev(BIN_opnd1(this)) == NULL);
		break;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_XOR:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_BAND:
	case IR_BOR:
		IS_TRUE0(d);
		IS_TRUE0(BIN_opnd0(this) != NULL &&
				 BIN_opnd0(this)->is_exp() &&
				 BIN_opnd1(this) != NULL &&
				 BIN_opnd1(this)->is_exp());
		IS_TRUE0(IR_next(BIN_opnd0(this)) == NULL &&
				 IR_prev(BIN_opnd0(this)) == NULL);
		IS_TRUE0(IR_next(BIN_opnd1(this)) == NULL &&
				 IR_prev(BIN_opnd1(this)) == NULL);
		break;
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
		IS_TRUE0(d);
		IS_TRUE0(UNA_opnd0(this) != NULL &&
				 UNA_opnd0(this)->is_exp());
		IS_TRUE0(IR_next(UNA_opnd0(this)) == NULL &&
				 IR_prev(UNA_opnd0(this)) == NULL);
		break;
	case IR_GOTO:
		IS_TRUE0(dt == D_UNDEF);
		IS_TRUE0(GOTO_lab(this));
		break;
	case IR_IGOTO:
		IS_TRUE0(dt == D_UNDEF);
		IS_TRUE(IGOTO_vexp(this), ("igoto vexp can not be NULL."));
		IS_TRUE(IGOTO_case_list(this), ("igoto case list can not be NULL."));
		IS_TRUE(IR_next(IGOTO_vexp(this)) == NULL &&
				IR_prev(IGOTO_vexp(this)) == NULL,
				("igoto vexp can NOT be in list."));
		break;
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
		IS_TRUE0(IR_next(LOOP_det(this)) == NULL &&
				 IR_prev(LOOP_det(this)) == NULL);
		break;
	case IR_IF:
		IS_TRUE0(IR_next(IF_det(this)) == NULL &&
				 IR_prev(IF_det(this)) == NULL);
		//Fall through.
	case IR_LABEL:
		IS_TRUE0(dt == D_UNDEF);
		break;
	case IR_SWITCH:
		IS_TRUE0(dt == D_UNDEF);
		IS_TRUE(SWITCH_vexp(this), ("switch vexp can not be NULL."));

		//SWITCH case list can be NULL.
		IS_TRUE0(IR_next(SWITCH_vexp(this)) == NULL &&
				 IR_prev(SWITCH_vexp(this)) == NULL);
		break;
	case IR_CASE:
		IS_TRUE0(dt == D_UNDEF);
		IS_TRUE0(CASE_lab(this));
		IS_TRUE0(IR_next(CASE_vexp(this)) == NULL &&
				 IR_prev(CASE_vexp(this)) == NULL);
		break;
	case IR_ARRAY:
		{
			IS_TRUE0(d);
			IR * base = ARR_base(this);
			IS_TRUE0(base->is_ptr(dm));
			IS_TRUE0(ARR_elem_ty(this) != 0);
			DTD const* ed = dm->get_dtd(ARR_elem_ty(this));
			IS_TRUE0(ed);

			if (ARR_ofst(this) != 0) {
				IS_TRUE0(dm->get_dtd_bytesize(d) + ARR_ofst(this) <=
						 dm->get_dtd_bytesize(ed));
			}

			IS_TRUE0(IR_next(ARR_base(this)) == NULL &&
					 IR_prev(ARR_base(this)) == NULL);

			IS_TRUE0(ARR_sub_list(this));
		}
		break;
	case IR_CVT:
		IS_TRUE0(d);
		IS_TRUE0(CVT_exp(this) != NULL &&
				 CVT_exp(this)->is_exp());
		IS_TRUE0(IR_next(CVT_exp(this)) == NULL &&
				 IR_prev(CVT_exp(this)) == NULL);
		break;
	case IR_PR:
		IS_TRUE0(d);
		IS_TRUE0(this->get_rty(dm) != D_UNDEF);
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		{
			IS_TRUE0(d == NULL);
			IS_TRUE0(BR_lab(this));
			IR * ret = BR_det(this);
			IS_TRUE0(ret && ret->is_judge());
			IS_TRUE0(IR_next(ret) == NULL && IR_prev(ret) == NULL);
		}
		break;
	case IR_RETURN:
		if (RET_exp(this) != NULL) {
			IS_TRUE0(RET_exp(this)->is_exp());
			IS_TRUE0(IR_next(RET_exp(this)) == NULL &&
					 IR_prev(RET_exp(this)) == NULL);
		}
		break;
	case IR_SELECT:
		//true_exp's type might not equal to false_exp's.
		IS_TRUE0(d);
		IS_TRUE0(SELECT_trueexp(this) && SELECT_trueexp(this)->is_exp());
		IS_TRUE0(SELECT_falseexp(this) && SELECT_falseexp(this)->is_exp());
		IS_TRUE0(SELECT_det(this) && SELECT_det(this)->is_judge());

		IS_TRUE0(IR_next(SELECT_trueexp(this)) == NULL &&
				 IR_prev(SELECT_trueexp(this)) == NULL);
		IS_TRUE0(IR_next(SELECT_falseexp(this)) == NULL &&
				 IR_prev(SELECT_falseexp(this)) == NULL);
		IS_TRUE0(IR_next(SELECT_det(this)) == NULL &&
				 IR_prev(SELECT_det(this)) == NULL);
		break;
	case IR_BREAK:
	case IR_CONTINUE:
		IS_TRUE0(dt == D_UNDEF);
		break;
	case IR_PHI:
		IS_TRUE0(d);
		IS_TRUE0(this->get_rty(dm) != D_UNDEF);
		IS_TRUE0(PHI_prno(this) > 0);
		IS_TRUE(dt != D_UNDEF, ("size of store value cannot be zero"));

		//PHI must have at least one opnd.
		IS_TRUE0(PHI_opnd_list(this) != NULL);
		break;
	case IR_REGION:
		IS_TRUE0(dt == D_UNDEF);
		break;
	default: IS_TRUE0(0);
	}
	return true;
}


bool IR::verify_kids() const
{
	ULONG kidbit = 1;
	for (UINT i = 0; i < IR_MAX_KID_NUM(this); i++, kidbit <<= 1) {
		IR * k = get_kid(i);
		if (k != NULL) {
			IS_TRUE0(IR_parent(k) == this);
		}
		if (!HAVE_FLAG(IRDES_kid_map(g_ir_desc[IR_type(this)]), kidbit)) {
			IS_TRUE(k == NULL,
					("IR_%s does not have No.%d kid", IRNAME(this), i));
		} else {
			//Here, ith kid cannot be NULL.
			/* CASE: Kind of node permit some of their kid to be NULL.
			For now include IR_IF, IR_RETURN, IR_DO_LOOP, etc. */
			if (k == NULL) {
				switch (IR_type(this)) {
				case IR_IF:
				case IR_DO_LOOP:
				case IR_WHILE_DO:
				case IR_SWITCH:
				case IR_DO_WHILE:
					if (i == 0) {
						IS_TRUE(k != NULL,
								("IR_%s miss kid%d", IRNAME(this), i));
					}
					break;
				case IR_ICALL:
					if (i == 2) {
						IS_TRUE(k != NULL,
								("IR_%s miss kid%d", IRNAME(this), i));
					}
					break;
				case IR_RETURN:
				case IR_CALL:
					break;
				default:
					IS_TRUE(k != NULL, ("IR_%s miss kid%d", IRNAME(this), i));
				}
			}
		}
	}
	return true;
}


/* Calculate the accumulated offset value from the base of array.
e.g: For given array long long p[10][20],
the offset of p[i][j] can be computed by i*20 + j, and
the offset of p[i] can be computed by i*20.

If all the indice are constant value, calcuate the value, storing
in 'ofst_val' and return true, otherwise return false, means the
ofst can not be determined. */
bool IR::calc_array_ofst(TMWORD * ofst_val, DT_MGR * dm) const
{
	if (IR_type(this) != IR_ARRAY) { return false; }
	IR const* ir = this;
	TMWORD aggr = 0;
	UINT dim = 0;
	for (IR const* s = ARR_sub_list(this); s != NULL; s = IR_next(s), dim++) {
		IS_TRUE0(ARR_elem_num_buf(this));
		if (!IR_is_const(s)) { return false; }

		IS_TRUE0(!s->is_fp(dm) && CONST_int_val(s) >= 0);

		#ifdef _VC2010_
		#define MARK_32BIT 0xFFFFffff00000000lu
		#else
		#define MARK_32BIT 0xFFFFffff00000000llu
		#endif
		IS_TRUE((((ULONGLONG)CONST_int_val(s)) &
				 (ULONGLONG)(LONGLONG)MARK_32BIT) == 0,
				("allow 32bit array offset."));

		IS_TRUE0(ARR_elem_num(this, dim) != 0);
		//Array index start at 0.
		if (dim == 0) {
			aggr = (TMWORD)CONST_int_val(s);
		} else {
			aggr += ARR_elem_num(this, dim - 1) * (TMWORD)CONST_int_val(s);
		}
	}
	aggr *= dm->get_dtd_bytesize(ARR_elem_ty(this));
	IS_TRUE0(ofst_val);
	*ofst_val = aggr;
	return true;
}


/*
Return true if ir-list are equivalent.
'is_cmp_kid': it is true if comparing kids as well.
*/
bool IR::is_irs_equal(IR const* irs, bool is_cmp_kid) const
{
	IR const* pthis = this;
	while (irs != NULL && pthis != NULL) {
		if (!pthis->is_ir_equal(irs, is_cmp_kid)) {
			return false;
		}
		irs = IR_next(irs);
		pthis = IR_next(pthis);
	}
	if ((irs != NULL) ^ (pthis != NULL)) {
		return false;
	}
	return true;
}


//Return true if ir are equivalent.
//'is_cmp_kid': it is true if comparing kids as well.
bool IR::is_ir_equal(IR const* src, bool is_cmp_kid) const
{
	IS_TRUE0(src != NULL);

	//Compare opcode.
	if (IR_type(this) != IR_type(src)) { return false; }

	switch (IR_type(src)) {
	case IR_CONST: //Constant value: include integer, float, string.
		if (CONST_int_val(this) != CONST_int_val(src)) return false;
		break;
	case IR_ID:
		if (ID_info(this) != ID_info(src)) return false;
		break;
	case IR_LD:
		if (LD_idinfo(this) != LD_idinfo(src) ||
			LD_ofst(this) != LD_ofst(src) ||
			IR_dt(this) != IR_dt(src)) {
			return false;
		}
		break;
	case IR_ILD:
		if (ILD_ofst(this) != ILD_ofst(src) ||
			IR_dt(this) != IR_dt(src)) {
			return false;
		}
		break;
 	case IR_ST:
		if (ST_idinfo(this) != ST_idinfo(src) ||
			ST_ofst(this) != ST_ofst(src) ||
			IR_dt(this) != IR_dt(src)) {
			return false;
		}
		break;
	case IR_STPR:
		if (IR_dt(this) != IR_dt(src) || STPR_no(this) != STPR_no(src)) {
			return false;
		}
		break;
	case IR_IST:
		if (IST_ofst(this) != IST_ofst(src) ||
			IR_dt(this) != IR_dt(src)) {
			return false;
		}
		break;
	case IR_LDA:
		if (LDA_ofst(this) != LDA_ofst(src) ||
			IR_dt(this) != IR_dt(src)) {
			return false;
		}
		break;
	case IR_CALL:
		if (CALL_idinfo(this) != CALL_idinfo(src)) return false;
		break;
	case IR_ICALL:
		break;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
	case IR_CVT:
		if (IR_dt(this) != IR_dt(src)) return false;
		break;
	case IR_GOTO:
	case IR_IGOTO:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
		break;
	case IR_LABEL:
		if (LAB_lab(this) != LAB_lab(src)) return false;
		break;
	case IR_SWITCH:
	case IR_CASE:
		break;
	case IR_ARRAY:
		if (ARR_ofst(this) != ARR_ofst(src) || IR_dt(this) != IR_dt(src)) {
			return false;
		}
		break;
	case IR_PR:
		if (IR_dt(this) != IR_dt(src) || PR_no(this) != PR_no(src)) {
			return false;
		}
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_SELECT:
		if (IR_dt(this) != IR_dt(src)) {
			return false;
		}
		break;
	case IR_RETURN:
		break;
	case IR_BREAK:
	case IR_CONTINUE:
		return true;
	case IR_PHI:
		return true;
	case IR_REGION:
		//One should implement comparation function for your own region.
		return false;
	default: IS_TRUE0(0);
	}

	//Compare kids.
	for (INT i = 0; i < IR_MAX_KID_NUM(this); i++) {
		IR * kid1 = get_kid(i);
		IR * kid2 = src->get_kid(i);
		if ((kid1 != NULL) ^ (kid2 != NULL)) {
			return false;
		}
		if (kid1 != NULL && !kid1->is_irs_equal(kid2, is_cmp_kid)) {
			return false;
		}
	}
	return true;
}


//Contructing IR forest.
//'recur': true to iterate kids.
void IR::set_parent_pointer(bool recur)
{
	for (INT i = 0; i < IR_MAX_KID_NUM(this); i++) {
		IR * kid = get_kid(i);
		if (kid != NULL) {
			while (kid != NULL) {
				IR_parent(kid) = this;
				if (recur) {
					kid->set_parent_pointer(recur);
				}
				kid = IR_next(kid);
			}
		}
	}
}


IR * IR::get_opnd_pr_list(UINT prno)
{
	IR * pr = NULL;
	IR * p = this; //this is header of list.
	while (p != NULL) {
		if ((pr = p->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		p = IR_next(p);
	}
	return NULL;
}


//Get opnd pr nonrecursively.
//'ii': iterator.
IR * IR::get_opnd_pr(UINT prno, IR_ITER & ii)
{
	IS_TRUE0(is_stmt());
	ii.clean();
	IR * newpr = NULL;
	for (IR * k = ir_iter_init(this, ii);
		 k != NULL; k = ir_iter_next(ii)) {
		if (k->is_pr() && PR_no(k) == prno && is_rhs(k)) {
			return k;
		}
	}
	return NULL;
}


//This function recursively retrieve the operand that
//indicates a pr which PR_no is 'prno'.
IR * IR::get_opnd_pr(UINT prno)
{
	IR * pr = NULL;
	switch (IR_type(this)) {
	case IR_CONST:
	case IR_ID:
	case IR_LD:
		return NULL;
	case IR_ST:
		return ST_rhs(this)->get_opnd_pr(prno);
	case IR_STPR:
		return STPR_rhs(this)->get_opnd_pr(prno);
	case IR_SETEPR:
		return SETEPR_rhs(this)->get_opnd_pr(prno);
	case IR_GETEPR:
		if ((pr = GETEPR_base(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		if ((pr = GETEPR_ofst(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return GETEPR_no(this) == prno ? this : NULL;
	case IR_ILD:
		return ILD_base(this)->get_opnd_pr(prno);
	case IR_IST:
		if ((pr = IST_base(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return IST_rhs(this)->get_opnd_pr(prno);
	case IR_LDA: return NULL;
	case IR_CALL:
	case IR_ICALL:
		if ((pr = CALL_param_list(this)->get_opnd_pr_list(prno)) != NULL) {
			return pr;
		}
		if (IR_type(this) == IR_ICALL) {
			return ICALL_callee(this)->get_opnd_pr(prno);
		}
		return NULL;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
		if ((pr = BIN_opnd0(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return BIN_opnd1(this)->get_opnd_pr(prno);
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
		return UNA_opnd0(this)->get_opnd_pr(prno);
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
		if ((pr = BIN_opnd0(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return BIN_opnd1(this)->get_opnd_pr(prno);
	case IR_GOTO: return NULL;
	case IR_IGOTO:
		if ((pr = IGOTO_vexp(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return NULL;
	case IR_DO_WHILE:
	case IR_WHILE_DO:
		if ((pr = LOOP_det(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return LOOP_body(this)->get_opnd_pr_list(prno);
	case IR_DO_LOOP:
		if ((pr = LOOP_det(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		if ((pr = LOOP_init(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		if ((pr = LOOP_step(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return LOOP_body(this)->get_opnd_pr_list(prno);
	case IR_IF:
	case IR_LABEL: return NULL;
	case IR_SWITCH:
		if ((pr = SWITCH_vexp(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return SWITCH_body(this)->get_opnd_pr_list(prno);
	case IR_CASE: return NULL;
	case IR_ARRAY:
		if ((pr = ARR_base(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		if ((pr = ARR_sub_list(this)->get_opnd_pr_list(prno)) != NULL) {
			return pr;
		}
		return NULL;
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		if ((pr = BIN_opnd0(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return BIN_opnd1(this)->get_opnd_pr(prno);
	case IR_CVT:
		return CVT_exp(this)->get_opnd_pr(prno);
	case IR_PR:
		return PR_no(this) == prno ? this : NULL;
	case IR_TRUEBR:
	case IR_FALSEBR:
		return BR_det(this)->get_opnd_pr(prno);
	case IR_SELECT:
		if ((pr = SELECT_det(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		if ((pr = SELECT_trueexp(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return SELECT_falseexp(this)->get_opnd_pr(prno);
	case IR_RETURN:
		if ((pr = RET_exp(this)->get_opnd_pr(prno)) != NULL) {
			return pr;
		}
		return NULL;
	case IR_BREAK:
	case IR_CONTINUE:
		return NULL;
	case IR_PHI:
		if ((pr = PHI_opnd_list(this)->get_opnd_pr_list(prno)) != NULL) {
			return pr;
		}
		return NULL;
	case IR_REGION: return NULL;
	default: IS_TRUE0(0);
	}
	return NULL;
}


//Return IR_PR or IR_STPR.
//Get result pr accroding to prno.
//NOTE if current ir is IR_STPR, return pointer to the stmt.
IR * IR::get_res_pr(UINT prno)
{
	switch (IR_type(this)) {
	case IR_STPR:
		if (STPR_no(this) == prno) { return this; }
		return NULL;
	case IR_SETEPR:
		if (SETEPR_no(this) == prno) { return this; }
		return NULL;
	case IR_GETEPR:
		if (GETEPR_no(this) == prno) { return this; }
		return NULL;
	case IR_ST:
	case IR_IST:
	case IR_GOTO:
	case IR_IGOTO:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_SWITCH:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_BREAK:
	case IR_CONTINUE:
	case IR_REGION:
		return NULL;
	case IR_CALL:
	case IR_ICALL:
		if (CALL_prno(this) == prno) { return this; }
		return NULL;
	case IR_CONST:
	case IR_ID:
	case IR_LD:
	case IR_ILD:
	case IR_LDA:
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_CASE:
	case IR_ARRAY:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
	case IR_CVT:
	case IR_PR:
	case IR_SELECT:
	case IR_RETURN:	//Does not have a result PR.
		IS_TRUE0(0);
	case IR_PHI:
		if (PHI_prno(this) == prno) { return this; }
		return NULL;
	default: IS_TRUE0(0);
	}
	return NULL;
}


//Copy MD that ir referenced accroding to 'mds'.
void IR::set_ref_md(MD const* md, REGION * ru)
{
	DU * du = get_du();
	if (du == NULL) {
		if (md == NULL) { return; }

		IS_TRUE0(ru);
		du = ru->alloc_du();
		set_du(du);
	}
	DU_md(du) = md;
}


//Copy the set of MD that ir referenced accroding to 'mds'.
void IR::set_ref_mds(MD_SET const* mds, REGION * ru)
{
	DU * du = get_du();
	if (du == NULL) {
		if (mds == NULL) { return; }

		IS_TRUE0(ru);
		du = ru->alloc_du();
		set_du(du);
	}
	DU_mds(du) = mds;
}


void IR::invert_land(REGION * ru)
{
	IS_TRUE0(ru);
	//a&&b => !a || !b
	IR * newop0 = ru->build_logical_not(BIN_opnd0(this));
	IR * newop1 = ru->build_logical_not(BIN_opnd1(this));
	IR_type(this) = IR_LOR;

	BIN_opnd0(this) = newop0;
	BIN_opnd1(this) = newop1;
	IR_parent(newop0) = this;
	IR_parent(newop1) = this;
}


void IR::invert_lor(REGION * ru)
{
	IS_TRUE0(ru);
	//a||b => !a && !b
	IR * newop0 = ru->build_logical_not(BIN_opnd0(this));
	IR * newop1 = ru->build_logical_not(BIN_opnd1(this));
	IR_type(this) = IR_LAND;

	BIN_opnd0(this) = newop0;
	BIN_opnd1(this) = newop1;
	IR_parent(newop0) = this;
	IR_parent(newop1) = this;
}
//END IR
