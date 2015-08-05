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
#include "callg.h"
#include "inliner.h"

//
//START INLINER
//
bool INLINER::is_call_site(IR * call, REGION * ru)
{
	IS_TRUE0(call->is_call());
	SYM * name = VAR_name(CALL_idinfo(call));
	CALL_NODE const* cn1 = m_callg->map_sym2cn(name);
	CALL_NODE const* cn2 = m_callg->map_ru2cn(ru);
	return cn1 == cn2;
}


/* 'caller': caller's region.
'caller_call': call site in caller.
'new_irs': indicate the duplicated IR list in caller. Note that
	these IR must be allocated in caller's region. */
IR * INLINER::replace_return_c(REGION * caller, IR * caller_call,
							   IR * new_irs, LABEL_INFO * el)
{
	IR * next = NULL;
	for (IR * x = new_irs; x != NULL; x = next) {
		next = IR_next(x);
		switch (IR_type(x)) {
		case IR_DO_WHILE:
		case IR_WHILE_DO:
		case IR_DO_LOOP: //loop with init , boundary , and step info
			LOOP_body(x) =
				replace_return_c(caller, caller_call, LOOP_body(x), el);
			break;
		case IR_IF:
			IF_truebody(x) =
				replace_return_c(caller, caller_call, IF_truebody(x), el);
			IF_falsebody(x) =
				replace_return_c(caller, caller_call, IF_falsebody(x), el);
			break;
		case IR_SWITCH:
			SWITCH_body(x) =
				replace_return_c(caller, caller_call, SWITCH_body(x), el);
			break;
		case IR_RETURN:
			if (!caller_call->has_return_val()) {
				if (el != NULL) {
					IR * go = caller->build_goto(el);
					insertbefore_one(&new_irs, x, go);
				}
				remove(&new_irs, x);
				caller->free_irs(x);
			} else {
				IR * send = RET_exp(x);
				UINT receive = CALL_prno(caller_call);
				IR * mv_lst = NULL;
				if (send != NULL) {
					IR * mv = caller->build_store_pr(receive,
													 IR_dt(caller_call), send);
					insertbefore_one(&mv_lst, mv_lst, mv);
				}
				RET_exp(x) = NULL;

				if (el != NULL) {
					IR * go = caller->build_goto(el);
					add_next(&mv_lst, go);
				}

				insertbefore(&new_irs, x, mv_lst);
				remove(&new_irs, x);
				IS_TRUE0(RET_exp(x) == NULL);
				caller->free_irs(x);
			}
			break;
		default: break;
		} //end switch
	}
	return new_irs;
}



void INLINER::ck_ru(IN REGION * ru, OUT bool & need_el,
					OUT bool & has_ret) const
{
	need_el = false;
	has_ret = false;
	LIST<IR const*> lst;
	IR const* irs = ru->get_ir_list();
	if (irs == NULL) { return; }
	for (IR const* x = irs; x != NULL; x = IR_next(x)) {
		switch (IR_type(x)) {
		case IR_DO_WHILE:
		case IR_WHILE_DO:
		case IR_DO_LOOP:
			lst.clean();
			for (IR const* k = ir_iter_init_c(LOOP_body(x), lst);
				 k != NULL; k = ir_iter_next_c(lst)) {
				if (IR_type(k) == IR_RETURN) {
					need_el = true;
					has_ret = true;
					break;
				}
			}
			break;
		case IR_IF:
			lst.clean();
			for (IR const* k = ir_iter_init_c(IF_truebody(x), lst);
				 k != NULL; k = ir_iter_next_c(lst)) {
				if (IR_type(k) == IR_RETURN) {
					need_el = true;
					has_ret = true;
					break;
				}
			}
			lst.clean();
			for (IR const* k = ir_iter_init_c(IF_falsebody(x), lst);
				 k != NULL; k = ir_iter_next_c(lst)) {
				if (IR_type(k) == IR_RETURN) {
					need_el = true;
					has_ret = true;
					break;
				}
			}
			break;
		case IR_SWITCH:
			lst.clean();
			for (IR const* k = ir_iter_init_c(SWITCH_body(x), lst);
				 k != NULL; k = ir_iter_next_c(lst)) {
				if (IR_type(k) == IR_RETURN) {
					need_el = true;
					has_ret = true;
					break;
				}
			}
			break;
		case IR_RETURN: has_ret = true; break;
		default: break;
		} //end switch
		if (need_el) {
			break;
		}
	}
}


IR * INLINER::replace_return(REGION * caller, IR * caller_call,
							 IR * new_irs, INLINE_INFO * ii)
{
	LABEL_INFO * el = NULL;
	if (INLINFO_need_el(ii)) {
		el = caller->gen_ilab();
	}
	if (INLINFO_has_ret(ii)) {
		new_irs = replace_return_c(caller, caller_call, new_irs, el);
	}
	if (el != NULL) {
		add_next(&new_irs, caller->build_label(el));
	}
	return new_irs;
}


bool INLINER::do_inline_c(REGION * caller, REGION * callee)
{
	IR * caller_irs = caller->get_ir_list();
	IR * callee_irs = callee->get_ir_list();
	if (caller_irs == NULL || callee_irs == NULL) { return false; }
	IR * next = NULL;
	bool change = false;
	IR * head = caller_irs;
	for (; caller_irs != NULL; caller_irs = next) {
		next = IR_next(caller_irs);
		if (IR_type(caller_irs) == IR_CALL &&
			is_call_site(caller_irs, callee)) {
			IR * new_irs_in_caller = caller->dup_irs_list(callee_irs);

			INLINE_INFO * ii = map_ru2ii(callee, false);
			if (ii == NULL) {
				bool need_el;
				bool has_ret;
				ck_ru(callee, need_el, has_ret);
				ii = map_ru2ii(callee, true);
				INLINFO_has_ret(ii) = has_ret;
				INLINFO_need_el(ii) = need_el;
 			}
			new_irs_in_caller = replace_return(caller, caller_irs, new_irs_in_caller, ii);
			insertafter(&caller_irs, new_irs_in_caller);
			remove(&head, caller_irs);
			change = true;
		}
	}
	caller->set_ir_list(head);
	return change;
}


//'cand': candidate that expected to be inlined.
void INLINER::do_inline(REGION * cand)
{
	CALL_NODE * cn = m_callg->map_ru2cn(cand);
	IS_TRUE0(cn);
	VERTEX * v = m_callg->get_vertex(CN_id(cn));
	IS_TRUE0(v);
	for (EDGE_C * el = VERTEX_in_list(v);
		 el != NULL; el = EC_next(el)) {
		REGION * caller = CN_ru(m_callg->map_vex2cn(EDGE_from(EC_edge(el))));
		if (caller != NULL) {
			do_inline_c(caller, cand);
		}
	}
}


//Evaluate whether ru can be inlining candidate.
bool INLINER::can_be_cand(REGION * ru)
{
	IR * ru_irs = ru->get_ir_list();
	if (RU_is_exp_inline(ru) && cnt_list(ru_irs) < g_inline_threshold) {
		return true;
	}
	return false;
}


bool INLINER::perform(OPT_CTX & oc)
{
	IS_TRUE0(OPTC_is_callg_valid(oc));
	REGION * top = m_ru_mgr->get_top_region();
	if (top == NULL) return false;
	IS_TRUE0(RU_type(top) == RU_PROGRAM);
	IR * irs = top->get_ir_list();
	UINT cn = 0;
	while (irs != NULL) {
		if (IR_type(irs) == IR_REGION) {
			REGION * ru = REGION_ru(irs);
			char const* s = ru->get_ru_name();
			if (can_be_cand(ru)) {
				do_inline(ru);
			}
		}
		irs = IR_next(irs);
	}
	return false;
}
//END INLINER
