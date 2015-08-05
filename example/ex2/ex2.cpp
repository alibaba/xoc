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
#include "../xoc/cominc.h"
#include "ex2.h"

REGION_MGR * g_region_mgr = NULL;

//
//START CVAR
//
CHAR * CVAR::dump_var_decl(OUT CHAR * buf)
{
	return NULL;
}
//END CVAR


//
//START CVAR_MGR
//
VAR * CVAR_MGR::new_var()
{
	return new CVAR();
}
//END CVAR_MGR


//
//START DBG_MGR
//
/*
Append src file line into dump file.
Only print statement line.
*/
static LONG g_cur_lineno = 0;
void CDBG_MGR::print_src_line(IR * ir)
{
	return;
}
//END DBG_MGR


static UINT generate_region(REGION_MGR * rm)
{
	//Generate region for file.
	REGION * topru = rm->new_region(RU_PROGRAM);
	rm->register_region(topru);
	topru->set_ru_var(rm->get_var_mgr()->register_var(
													".file",
													D_MC,
													D_UNDEF,
													0,
													0,
													0,
													VAR_GLOBAL|VAR_FAKE));

	//-----------
	//Generate region for func
	IR * funcru = topru->build_region(RU_FUNC,
								rm->get_var_mgr()->register_var(
													".func.name",
													D_MC,
													D_UNDEF,
													0,
													0,
													0,
													VAR_GLOBAL|VAR_FAKE));
	topru->add_to_ir_list(funcru);
	//----------
	//For simply, only generate a return IR.
	IR * r = IR_region(funcru)->build_return(NULL);
	IR_region(funcru)->add_to_ir_list(r);
	return ST_SUCC;
}


int main(int argc, char * argv[])
{
	g_dbg_mgr = new CDBG_MGR();
	REGION_MGR * rm = new REGION_MGR();
	rm->init_var_mgr();

	//Generate region.
	generate_region(rm);

	//Compile region.
	rm->process();

	delete rm;
	delete g_dbg_mgr;
	g_dbg_mgr = NULL;
	return 0;
}

