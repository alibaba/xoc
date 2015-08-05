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
#ifndef _IR_LOOP_CVT_H_
#define _IR_LOOP_CVT_H_

/* Perform Loop Convertion.
Transform while-do loop to do-while loop.

e.g: Given loop I, the loop induction variable is i, loop upper bound is N.

while do loop is:
	bb1:
		if i > N, branch bb3;
	bb2:
		loop body;
		goto bb1;
	bb3:
		...

after convertion:

	bb1:
		if i > N, branch bb3;
	bb2:
		loop body;
		if i <= N, branch bb2;
	bb3:
		...
*/
class IR_LOOP_CVT : public IR_OPT {
protected:
	REGION * m_ru;
	IR_CFG * m_cfg;
	IR_DU_MGR * m_du;
	IR_ITER m_ii;

	bool is_while_do(LI<IR_BB> const* li, IR_BB ** gobackbb,
					UINT * succ1, UINT * succ2);
	bool try_convert(LI<IR_BB> * li, IR_BB * gobackbb,
					UINT succ1, UINT succ2);
	bool find_and_convert(LIST<LI<IR_BB>*> & worklst);
public:
	IR_LOOP_CVT(REGION * ru)
	{
		IS_TRUE0(ru != NULL);
		m_ru = ru;
		m_du = ru->get_du_mgr();
		m_cfg = m_ru->get_cfg();
	}
	virtual ~IR_LOOP_CVT() {}

	virtual CHAR const* get_opt_name() const { return "Loop Convertion"; }
	OPT_TYPE get_opt_type() const { return OPT_LOOP_CVT; }

	virtual bool perform(OPT_CTX & oc);
};
#endif
