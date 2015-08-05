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
//START IR_BB
//
UINT IR_BB::count_mem() const
{
	UINT count = sizeof(IR_BB);
	count += ir_list.count_mem();
	count += lab_list.count_mem();
	return count;
}


//Could ir be looked as a last stmt in basic block?
bool IR_BB::is_bb_down_boundary(IR * ir)
{
	IS_TRUE(ir->is_stmt_in_bb() || ir->is_lab(), ("illegal stmt in bb"));
	switch (IR_type(ir)) {
	case IR_CALL:
	case IR_ICALL: //indirective call
	case IR_GOTO:
	case IR_IGOTO:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_RETURN:
		return true;
	case IR_SWITCH:
		IS_TRUE(SWITCH_body(ir) == NULL,
				("Peel switch-body to enable switch in bb-list construction"));
		return true;
	default:
		return false;
	}
	return false;
}


void IR_BB::dump()
{
	if (g_tfile == NULL) return;
	g_indent = 0;
	note("\n----- BB%d ------", IR_BB_id(this));

	//Label Info list
	LABEL_INFO * li = IR_BB_lab_list(this).get_head();
	if (li != NULL) {
		note("\nLABEL:");
	}
	for (; li != NULL; li = IR_BB_lab_list(this).get_next()) {
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

	//Attributes
	note("\nATTR:");
	if (IR_BB_is_entry(this)) {
		note("entry_bb ");
	}
	//if (IR_BB_is_exit(this)) {
	//	note("exit_bb ");
	//}
	if (IR_BB_is_fallthrough(this)) {
		note("fall_through ");
	}
	if (IR_BB_is_target(this)) {
		note("branch_target ");
	}

	//IR list
	note("\nSTMT NUM:%d", IR_BB_ir_num(this));
	g_indent += 3;
	for (IR * ir = IR_BB_first_ir(this);
		ir != NULL; ir = IR_BB_ir_list(this).get_next()) {
		IS_TRUE0(IR_next(ir) == NULL && IR_prev(ir) == NULL);
		IS_TRUE0(ir->get_bb() == this);
		dump_ir(ir, m_ru->get_dm(), NULL, true, true, false);
	}
	g_indent -= 3;
	note("\n");
	fflush(g_tfile);
}


//Check that all basic blocks should only end with terminator IR.
void IR_BB::verify()
{
	UINT c = 0;
	C<IR*> * ct;
	for (IR * ir = IR_BB_ir_list(this).get_head(&ct);
		 ir != NULL; ir = IR_BB_ir_list(this).get_next(&ct)) {
		IS_TRUE0(IR_next(ir) == NULL && IR_prev(ir) == NULL);
		IS_TRUE0(ir->get_bb() == this);
		switch (IR_type(ir)) {
		case IR_ST:
		case IR_STPR:
		case IR_IST:
		case IR_PHI:
		case IR_REGION:
		case IR_CALL:
		case IR_ICALL:
		case IR_GOTO:
		case IR_IGOTO:
		case IR_TRUEBR:
		case IR_FALSEBR:
		case IR_RETURN:
		case IR_SWITCH:
			break;
		default: IS_TRUE(0, ("BB does not supported this kind of IR."));
		}

		if (ir->is_call() || ir->is_cond_br() ||
			ir->is_multicond_br() || ir->is_uncond_br()) {
			IS_TRUE(ir == IR_BB_last_ir(this), ("invalid bb boundary."));
		}

		c++;
	}
	IS_TRUE0(c == IR_BB_ir_num(this));
}
//END IR_BB


void dump_bbs(IR_BB_LIST * bbl, CHAR const* name)
{
	FILE * h = NULL;
	FILE * org_g_tfile = g_tfile;
	if (name == NULL) {
		h = g_tfile;
	} else {
		unlink(name);
		h = fopen(name, "a+");
		IS_TRUE(h != NULL, ("can not dump."));
		g_tfile = h;
	}
	if (h != NULL && bbl->get_elem_count() != 0) {
		fprintf(h, "\n==---- DUMP '%s' IR_BB_LIST ----==",
				bbl->get_head()->get_ru()->get_ru_name());
		for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
			bb->dump();
		}
	}
	fflush(h);
	if (h != org_g_tfile) {
		fclose(h);
	}
	g_tfile = org_g_tfile;
}


void dump_bbs_order(IR_BB_LIST * bbl)
{
	if (g_tfile == NULL) return;
	note("\n==---- DUMP  bbs order ----==\n");
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		note("%d ", IR_BB_id(bb));
	}
	note("\n");
	fflush(g_tfile);
}
