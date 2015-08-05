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
#include "if_opt.h"

/*
Recognizing simple IF control flow node
only recog such as:
	if (cond)
		single_BB
	else
		single_BB
	endif
  or:
	if (cond)
		single_BB
	endif

*/
static IF_TYPE get_simple_if_node_type(IR_CFG * cfg, IR_BB * bb)
{
	IS_TRUE(cfg && bb, ("need cfg"));
	IR_BB_LIST succs;
	cfg->get_succs(succs, bb);

	//IF cond node only has two succs
	if (succs.get_elem_count() != 2) {
		return NOT_SIMP_IF;
	}

	//IF cond node can not been circle-self
	if (succs.get_head_nth(0) == bb || succs.get_head_nth(1) == bb) {
		return NOT_SIMP_IF;
	}

	//It should optimize the two verbose succs edge
	if (succs.get_head() == succs.get_next()) {
		return NOT_SIMP_IF;
	}

	IR_BB * truebd = NULL;
	IR_BB * falsebd = NULL;
	if (IR_BB_is_fallthrough(succs.get_head()) &&
		IR_BB_is_target(succs.get_next())) {
		truebd = succs.get_head();
		falsebd = succs.get_next();
	} else if (IR_BB_is_fallthrough(succs.get_head_nth(1)) &&
				IR_BB_is_target(succs.get_head())) {
		falsebd = succs.get_head();
		truebd = succs.get_next();
	} else {
		IS_TRUE(0, ("illegal CFG"));
	}

	IR_BB_LIST succs_in_true;
	IR_BB_LIST succs_in_false;
	cfg->get_succs(succs_in_true, truebd);
	cfg->get_succs(succs_in_false, falsebd);

	//true body node only has one succs
	if (succs_in_true.get_elem_count() != 1) {
		return NOT_SIMP_IF;
	}

	//true body node can not been circle-self
	if (succs_in_true.get_head() == truebd) {
		return NOT_SIMP_IF;
	}

	/*If succ of true body is false body, such as
		if
		| \
		| true
		| /
	    false(actually is endif block)
	*/
	if (succs_in_true.get_head() == falsebd) {
		return SIMP_IF_THEN_TYPE;
	}

	//false body node only has one succs
	if (succs_in_false.get_elem_count() != 1) {
		return NOT_SIMP_IF;
	}

	//false body node can not been circle-self
	if (succs_in_false.get_head() == falsebd) {
		return NOT_SIMP_IF;
	}

	/*In the case of succ of true body is NOT false body, such as
		   if
		   | \
	   false  true
		   | /
	       endif
	*/
	if (succs_in_true.get_head() == succs_in_false.get_head()) {
		return SIMP_IF_ELSE_TYPE;
	}
	return NOT_SIMP_IF;
}


/*
Perform if optimization
*/
bool IR_CFG::if_opt(IR_BB * bb)
{
	IF_TYPE ift = NOT_SIMP_IF;
	if (NOT_SIMP_IF != (ift = get_simple_if_node_type(this, bb))) {
		/*
		TODO: Do misc if-opt here...
		*/
		return false;
	}
	return false;
}
