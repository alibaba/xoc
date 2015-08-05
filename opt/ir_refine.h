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
#ifndef _IR_REFINE_H_
#define _IR_REFINE_H_

//Refine operation context variable.
#define RC_refine_div_const(r)		((r).u1.s1.refine_div_const)
#define RC_refine_mul_const(r)		((r).u1.s1.refine_mul_const)
#define RC_do_fold_const(r)			((r).u1.s1.do_fold_const)
#define RC_insert_cvt(r)			((r).u1.s1.insert_cvt)
#define RC_refine_stmt(r)			((r).u1.s1.refine_stmt)
#define RC_stmt_removed(r)			((r).u1.s1.stmt_has_been_removed)
class RF_CTX {
public:
	union {
		struct {
			//Pass info topdown. e.g: int a; a/2 => a>>1
			INT refine_div_const:1;

			//Pass info topdown. e.g: int a; a*2 => a<<1
			INT refine_mul_const:1;

			//Pass info topdown. True to do refinement to stmt.
			INT refine_stmt:1;

			//Pass info topdown. e.g: int a; a=2+3 => a=5
			INT do_fold_const:1;

			//Pass info topdown. True to insert IR_CVT automaticlly.
			INT insert_cvt:1;

			//Bottom up inform caller stmt has been removed from BB.
			INT stmt_has_been_removed:1;
		} s1;
		INT i1;
	} u1;

	RF_CTX()
	{
		RC_refine_div_const(*this) = true;
		RC_refine_mul_const(*this) = true;
		RC_refine_stmt(*this) = true;
		RC_do_fold_const(*this) = true;
		if (g_do_refine_auto_insert_cvt) {
			RC_insert_cvt(*this) = true;
		} else {
			RC_insert_cvt(*this) = false;
		}
		RC_stmt_removed(*this) = false;
	}
};

#endif
