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
#ifndef _IR_LCSE_H_
#define _IR_LCSE_H_

/*
IR_LCSE
Perform Local Common Subexpression Elimination.
*/
class IR_LCSE : public IR_OPT {
protected:
	bool m_enable_filter; //filter determines which expression can be CSE.
	REGION * m_ru;
	DT_MGR * m_dm;
	IR_EXPR_TAB * m_expr_tab;
	IR_DU_MGR * m_du;
	BVEC<IR_EXPR*> * m_expr_vec;

	IR * hoist_cse(IR_BB * bb,  IR * ir_pos, IR_EXPR * ie);
	bool process_use(IN IR_BB * bb, IN IR * ir,
					 IN OUT BITSET & avail_ir_expr,
					 IN OUT SVECTOR<IR*> & map_expr2avail_pos,
					 IN OUT SVECTOR<IR*> & map_expr2avail_pr);
	bool process_def(IN IR_BB * bb, IN IR * ir,
					 IN OUT BITSET & avail_ir_expr,
					 IN OUT SVECTOR<IR*> & map_expr2avail_pos,
					 IN OUT SVECTOR<IR*> & map_expr2avail_pr,
					 IN MD_SET & tmp);
	bool process_br(IN IR_BB * bb, IN IR * ir,
					IN OUT BITSET & avail_ir_expr,
					IN OUT SVECTOR<IR*> & map_expr2avail_pos,
					IN OUT SVECTOR<IR*> & map_expr2avail_pr);
	IR * process_exp(IN IR_BB * bb, IN IR_EXPR * ie,
					 IN IR * stmt, IN OUT BITSET & avail_ir_expr,
					 IN OUT SVECTOR<IR*> & map_expr2avail_pos,
					 IN OUT SVECTOR<IR*> & map_expr2avail_pr);
	bool process_param_list(IN IR_BB * bb, IN IR * ir,
							IN OUT BITSET & avail_ir_expr,
							IN OUT SVECTOR<IR*> & map_expr2avail_pos,
							IN OUT SVECTOR<IR*> & map_expr2avail_pr);
	bool process_st_rhs(IN IR_BB * bb, IN IR * ir,
						IN OUT BITSET & avail_ir_expr,
						IN OUT SVECTOR<IR*> & map_expr2avail_pos,
						IN OUT SVECTOR<IR*> & map_expr2avail_pr);
public:
	IR_LCSE(REGION * ru);
	virtual ~IR_LCSE() {}
	bool can_be_candidate(IR * ir);
	virtual CHAR const* get_opt_name() const
	{ return "Local Command Subscript Elimination"; }

	OPT_TYPE get_opt_type() const { return OPT_LCSE; }

	inline void set_enable_filter(bool is_enable) { m_enable_filter = is_enable; }
	bool perform(OPT_CTX & oc);
};
#endif
