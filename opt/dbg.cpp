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

DBG_MGR * g_dbg_mgr = NULL;

void set_lineno(IR * ir, UINT lineno, REGION * ru)
{
	DBX_AI * da;
	IS_TRUE0(ru);
	if (IR_ai(ir) == NULL) {
		IR_ai(ir) = ru->new_ai();
		da = (DBX_AI*)smpool_malloc_h(sizeof(DBX_AI), ru->get_pool());
		IS_TRUE0(da);
		da->init();
		IR_ai(ir)->set(IRAI_DBX, (AI_BASE*)da);
	} else {
		IR_ai(ir)->init();
		da = (DBX_AI*)IR_ai(ir)->get(IRAI_DBX);
		if (da == NULL) {
			da = (DBX_AI*)smpool_malloc_h(sizeof(DBX_AI), ru->get_pool());
			IS_TRUE0(da);
			da->init();
			IS_TRUE0(da);
			IR_ai(ir)->set(IRAI_DBX, (AI_BASE*)da);
		}
	}
	DBX_lineno(&da->dbx) = lineno;
}


//Line number of source code that corresponding to the IR.
UINT get_lineno(IR const* ir)
{
	if (IR_ai(ir) == NULL || !IR_ai(ir)->is_init()) { return 0; }
	DBX_AI * da = (DBX_AI*)IR_ai(ir)->get(IRAI_DBX);
	if (da == NULL) { return 0; }

	return DBX_lineno(&da->dbx);
}


//Copy dbx from 'src' to 'tgt'.
void copy_dbx(IR * tgt, IR const* src, REGION * ru)
{
	IS_TRUE0(ru);
	if (IR_ai(src) == NULL) { return; }

	DBX_AI * src_da = (DBX_AI*)IR_ai(src)->get(IRAI_DBX);
	if (IR_ai(tgt) == NULL) {
		if (src_da == NULL) { return; }
		IR_ai(tgt) = ru->new_ai();
	}
	IS_TRUE0(IR_ai(tgt));
	if (src_da == NULL) {
		IR_ai(tgt)->set(IRAI_DBX, NULL);
		return;
	}

	DBX_AI * tgt_da = (DBX_AI*)IR_ai(tgt)->get(IRAI_DBX);
	if (tgt_da == NULL) {
		tgt_da = (DBX_AI*)smpool_malloc_h(sizeof(DBX_AI), ru->get_pool());
		IS_TRUE0(tgt_da);
		tgt_da->init();
		IR_ai(tgt)->set(IRAI_DBX, (AI_BASE*)tgt_da);
	}
	tgt_da->dbx.copy(src_da->dbx);
}


DBX * get_dbx(IR * ir)
{
	if (IR_ai(ir) == NULL) { return NULL; }
	DBX_AI * da = (DBX_AI*)IR_ai(ir)->get(IRAI_DBX);
	if (da == NULL) { return NULL; }
	return &da->dbx;
}
