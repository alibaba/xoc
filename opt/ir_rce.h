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
#ifndef _IR_RCE_H_
#define _IR_RCE_H_

//Perform Redundant Code Elimination.
class IR_RCE : public IR_OPT {
protected:
	REGION * m_ru;
	IR_CFG * m_cfg;
	IR_GVN * m_gvn;
	IR_DU_MGR * m_du;
public:
	IR_RCE(REGION * ru, IR_GVN * gvn)
	{
		IS_TRUE0(ru != NULL);
		m_ru = ru;
		m_gvn = gvn;
		m_cfg = ru->get_cfg();
		m_du = m_ru->get_du_mgr();
	}
	virtual ~IR_RCE() {}

	IR * calc_cond_must_val(IN IR * ir, OUT bool & must_true,
							OUT bool & must_false);

	void dump();
	virtual CHAR const* get_opt_name() const
	{ return "Redundant Code Elimination"; }

	OPT_TYPE get_opt_type() const { return OPT_RCE; }

	IR * process_st(IR * ir);
	IR * process_stpr(IR * ir);
	IR * process_branch(IR * ir, IN OUT bool & cfg_mod);
	bool perform_simply_rce(IN OUT bool & cfg_mod);
	virtual bool perform(OPT_CTX & oc);
};
#endif

