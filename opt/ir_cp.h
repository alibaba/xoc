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
#ifndef _IR_CP_H_
#define _IR_CP_H_

//Record Context info during Copy Propagation.
#define CPC_change(c)			(c).change
#define CPC_need_recomp_aa(c)	(c).need_recompute_alias_info
class CP_CTX {
public:
	bool change;
	bool need_recompute_alias_info;

	CP_CTX()
	{
		change = false;
		need_recompute_alias_info = false;
	}

	//Perform bit or operation.
	void bor(CP_CTX & c)
	{
		change |= c.change;
		need_recompute_alias_info |= c.need_recompute_alias_info;
	}
};


//Only prop the simplex operation, include const, pr, lda, and cvt for simplex.
#define CP_PROP_SIMPLEX					1

//Prop unary and simplex operations, include const, pr, lda, cvt for simplex, ld,
//id, neg, bnot, lnot, ild.
#define CP_PROP_UNARY_AND_SIMPLEX		2

//Perform Copy Propagation
class IR_CP : public IR_OPT {
protected:
	REGION * m_ru;
	MD_SYS * m_md_sys;
	IR_DU_MGR * m_du;
	IR_CFG * m_cfg;
	MD_SET m_tmp;
	MD_SET_MGR * m_md_set_mgr;
	DT_MGR * m_dm;
	BYTE m_prop_kind:2;

	inline bool check_type_consistency(IR const* ir,
									   IR const* cand_expr) const;
	bool do_prop(IN IR_BB * bb, SVECTOR<IR*> & usevec);

	bool is_simp_cvt(IR const* ir) const;
	bool is_available(IR * def_ir, IR * occ, IR * use_ir);
	inline bool is_copy(IR * ir) const;

	bool perform_dom_tree(IN VERTEX * v, IN GRAPH & domtree);

	void replace_expr(IR * exp, IR const* cand_expr, IN OUT CP_CTX & ctx);
public:
	IR_CP(REGION * ru)
	{
		IS_TRUE0(ru != NULL);
		m_ru = ru;
		m_md_sys = ru->get_md_sys();
		m_du = ru->get_du_mgr();
		m_cfg = ru->get_cfg();
		m_md_set_mgr = ru->get_mds_mgr();
		m_dm = ru->get_dm();
		m_prop_kind = CP_PROP_UNARY_AND_SIMPLEX;
	}
	virtual ~IR_CP() {}

	//Check if ir is appropriate for propagation.
	virtual bool can_be_candidate(IR const* ir) const
	{
		if (m_prop_kind == CP_PROP_SIMPLEX) {
			return IR_type(ir) == IR_LDA || IR_type(ir) == IR_ID ||
				   IR_is_const(ir) || ir->is_pr() || is_simp_cvt(ir);
		}

		IS_TRUE0(m_prop_kind == CP_PROP_UNARY_AND_SIMPLEX);

		return IR_type(ir) == IR_LD ||
				IR_type(ir) == IR_LDA ||
				IR_type(ir) == IR_ID ||
				IR_is_const(ir) ||
				IR_type(ir) == IR_PR ||
				IR_type(ir) == IR_NEG ||
				IR_type(ir) == IR_BNOT ||
				IR_type(ir) == IR_LNOT ||
				IR_type(ir) == IR_ILD ||
				is_simp_cvt(ir);
		return false;
	}

	virtual CHAR const* get_opt_name() const { return "Copy Propagation"; }
	OPT_TYPE get_opt_type() const { return OPT_CP; }

	void set_prop_kind(UINT kind) { m_prop_kind = kind; }

	virtual bool perform(OPT_CTX & oc);
};
#endif

