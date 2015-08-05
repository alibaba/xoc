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
#ifndef __CFS_OPT_H__
#define __CFS_OPT_H__

class IR_CFS_OPT : public IR_OPT {
protected:
	REGION * m_ru;
	DT_MGR * m_dm;
	bool trans_to_do_while_loop(IR ** head, IR * ir);
	bool is_non_branch_ir(IR * ir);
	bool trans_if1(IR ** head, IR * ir);
	bool trans_if2(IR ** head, IR * ir);
	bool trans_if3(IR ** head, IR * ir);
	bool hoist_loop(IR ** head, IR * ir);
	bool hoist_if(IR ** head, IR * ir);
public:
	IR_CFS_OPT(REGION * ru) { m_ru = ru; m_dm = ru->get_dm(); }
	~IR_CFS_OPT() {}

	void dump();

	virtual CHAR const* get_opt_name() const { return "IR_CFS_OPT"; }

	bool perform_cfs_optimization(IN OUT IR ** ir_list, IN SIMP_CTX const& sc);
	virtual bool perform(OPT_CTX & oc)
	{
		IS_TRUE0(0);
		return false;
	}
	virtual bool perform(IN SIMP_CTX const& simp);
};
#endif
