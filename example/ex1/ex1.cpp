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
#include "../opt/cominc.h"
#include "ex1.h"

List<Region*> g_ru_list;
static UINT generate_region(RegionMgr * rm)
{
	//Generate region for file.
	Region * topru = rm->allocRegion(RU_PROGRAM);
	rm->set_region(topru);
	topru->set_ru_var(rm->get_var_mgr()->registerVar(
										".file",
										rm->get_dm()->getMCType(0),
										0,
										VAR_GLOBAL|VAR_FAKE));

	//-----------
	//Generate region for func
	Region * func_ru = rm->allocRegion(RU_FUNC);
	g_ru_list.append_tail(func_ru);
	func_ru->set_ru_var(rm->get_var_mgr()->registerVar(
										".func.name",
										rm->get_dm()->getMCType(0),
										0,
										VAR_GLOBAL|VAR_FAKE));
	IR * ir = topru->buildRegion(func_ru);
	topru->addToIRList(ir);

	//----------
	//For simply, only generate a return IR.
	REGION_ru(ir)->addToIRList(REGION_ru(ir)->buildReturn(NULL));
	return ST_SUCC;
}


int main(int argc, char * argv[])
{
	g_dbg_mgr = new DbxMgr();
	RegionMgr * rm = new RegionMgr();
	rm->initVarMgr();

	printf("\nGenerate region");

	//Generate region.
	generate_region(rm);

	printf("\nProcess region");

	//Compile region.
	rm->process();

	for (Region * ru = g_ru_list.get_head();
		 ru != NULL; ru = g_ru_list.get_next()) {
		delete ru;
	}

	delete rm;
	delete g_dbg_mgr;
	g_dbg_mgr = NULL;
	printf("\nFinish\n");
	return 0;
}

