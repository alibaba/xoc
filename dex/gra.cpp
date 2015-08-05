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
#include "prdf.h"
#include "dex.h"
#include "gra.h"

static CHAR const* g_fmt_name[] = {
	"",
	"F0",
	"FAB",
	"FABcv",
	"FAA",
	"FAAv",
	"FAAAAv",
	"FAABBBB",
	"FAABBBBv",
	"FAABBBBcv",
	"FAABBBBcvh",
	"FAABBCC",
	"FAABBCCcv",
	"FABCCCCv",
	"FABCCCCcv",
	"FAAAABBBB",
	"FAAAAAAAAv",
	"FAABBBBBBBBv",
	"FAABBBBBBBBcv",
	"FACDEFGBBBBv",
	"FAACCCCBBBBv",
	"FAABBBBBBBBBBBBBBBBcv",
};

#define PAIR_BYTES 8

class OP_MATCH {
public:
	bool match_bin_r_rr(IR_TYPE irt, IR * ir, IR ** res, IR ** op0, IR ** op1)
	{
		if (IR_type(ir) != IR_STPR) return false;
		if (res != NULL) { *res = ir; }

		IR * rhs = ST_rhs(ir);
		if (IR_type(rhs) != irt) return false;

		if (!BIN_opnd0(rhs)->is_pr()) return false;
		if (op0 != NULL) { *op0 = BIN_opnd0(rhs); }

		if (!BIN_opnd1(rhs)->is_pr()) return false;
		if (op1 != NULL) { *op1 = BIN_opnd1(rhs); }
		return true;
	}

	bool match_bin_r_rr(IR * ir, IR ** res, IR ** op0, IR ** op1)
	{
		if (IR_type(ir) != IR_STPR) return false;
		if (res != NULL) { *res = ir; }

		switch (IR_type(ST_rhs(ir))) {
		case IR_ADD:
		case IR_SUB:
		case IR_MUL:
		case IR_DIV:
		case IR_REM:
		case IR_MOD:
		case IR_LAND:
		case IR_LOR:
		case IR_BAND:
		case IR_BOR:
		case IR_XOR:
		case IR_ASR:
		case IR_LSR:
		case IR_LSL:
		case IR_LT:
		case IR_LE:
		case IR_GT:
		case IR_GE:
		case IR_EQ:
		case IR_NE:
			break;
		default: return false;
		}
		if (!BIN_opnd0(ST_rhs(ir))->is_pr()) return false;
		if (op0 != NULL) { *op0 = BIN_opnd0(ST_rhs(ir)); }
		if (!BIN_opnd1(ST_rhs(ir))->is_pr()) return false;
		if (op1 != NULL) { *op1 = BIN_opnd1(ST_rhs(ir)); }
		return true;
	}

	bool match_uni_r_r(IR_TYPE irt, IR * ir, IR ** res, IR ** op0)
	{
		if (IR_type(ir) != IR_STPR) { return false; }
		if (res != NULL) { *res = ir; }
		if (IR_type(ST_rhs(ir)) != irt) return false;
		if (!UNA_opnd0(ST_rhs(ir))->is_pr()) return false;
		if (op0 != NULL) { *op0 = UNA_opnd0(ST_rhs(ir)); }
		return true;
	}

	bool match_uni_r_r(IR * ir, IR ** res, IR ** op0)
	{
		if (IR_type(ir) != IR_STPR) return false;
		IR * rhs = ST_rhs(ir);
		switch (IR_type(rhs)) {
		case IR_BNOT:
		case IR_LNOT:
		case IR_NEG:
			break;
		default: return false;
		}
		if (res != NULL) { *res = ir; }
		if (!UNA_opnd0(rhs)->is_pr()) return false;
		if (op0 != NULL) { *op0 = UNA_opnd0(rhs); }
		return true;
	}

	bool match_mv_r_r(IR * ir, IR ** res, IR ** op0)
	{
		if (IR_type(ir) != IR_STPR) return false;
		if (res != NULL) { *res = ir; }
		if (!ST_rhs(ir)->is_pr()) return false;
		if (op0 != NULL) { *op0 = ST_rhs(ir); }
		return true;
	}
};


//
//START GLT
//
UINT GLT::comp_num_of_occ(GLTM & gltm)
{
	if (livebbs == NULL) { return 0; }
	SC<SEG*> * cur;
	UINT n = 0;
	for (INT i = livebbs->get_first(&cur);
		 i >= 0; i = livebbs->get_next(i, &cur)) {
		LTM * ltm = gltm.get_ltm(i);
		IS_TRUE0(ltm);
		LT * l = ltm->map_pr2lt(prno);
		IS_TRUE0(l);
		if (LT_occ(l) == NULL) { continue; }
		n += LT_occ(l)->get_elem_count();
	}
	return n;
}


//Set usable for local part.
void GLT::set_local_usable(GLTM & gltm)
{
	SC<SEG*> * sc;
	for (INT j = livebbs->get_first(&sc); j >= 0; j = livebbs->get_next(j, &sc)) {
		LTM * ltm = gltm.get_ltm(j);
		if (ltm == NULL) { continue; }
		LT * l = ltm->map_pr2lt(prno);
		IS_TRUE0(l); //glt miss local part
		LT_usable(l) = usable;
	}
}


//Set local part has same info with the global lt.
void GLT::set_local(GLTM & gltm)
{
	if (livebbs == NULL) { return; }
	SC<SEG*> * cur;
	for (INT i = livebbs->get_first(&cur);
		 i >= 0; i = livebbs->get_next(i, &cur)) {
		LTM * ltm = gltm.get_ltm(i);
		IS_TRUE(ltm, ("miss local part"));
		LT * l = ltm->map_pr2lt(prno);
		IS_TRUE(l, ("miss local part"));
		LT_phy(l) = phy;
		LT_rg_sz(l) = reg_group_size;
		LT_usable(l) = usable;
	}
}
//END GLT


//
//START LT
//
//Return global lt.
void LT::clean()
{
	range->clean();
	if (occ != NULL) {
		occ->clean();
	}
	return; //Do not clean other info.

	priority = 0.0;
	//usable = NULL;
	phy = REG_UNDEF;
	prefer_reg = REG_UNDEF;
	reg_group_size = 1;
	lt_group_pos = 0;
	if (lt_group != NULL) {
		lt_group->clean();
	}
}


GLT * LT::set_global(GLTM & gltm)
{
	GLT * g = gltm.map_pr2glt(prno);
	IS_TRUE0(g);
	GLT_phy(g) = phy;
	return g;
}


//Return true if lt has occurred in a branch.
bool LT::has_branch(LTM * ltm) const
{
	if (occ == NULL) { return false; }
	INT i = occ->get_last();
	if (i < 0) { return false; }
	IR * occ = ltm->get_ir(i);
	if (occ->is_cond_br() || occ->is_uncond_br() ||
		occ->is_multicond_br()) {
		return true;
	}
	return false;
}
//END LT


//
//START RSC
//
BITSET * RSC::get_4()
{
	if (m_4 == NULL) {
		m_4 = m_bsm->create(2);
		IS_TRUE0(FIRST_PHY_REG == 0);
		for (INT i = 0; i <= 15; i++) {
			m_4->bunion(i);
		}
	}
	return m_4;
}


BITSET * RSC::get_8()
{
	if (m_8 == NULL) {
		m_8 = m_bsm->create(8);
		IS_TRUE0(FIRST_PHY_REG == 0);
		for (INT i = 0; i <= 63; i++) {
			m_8->bunion(i);
		}
	}
	return m_8;
}


BITSET * RSC::get_16()
{
	if (m_16 == NULL) {
		m_16 = m_bsm->create(128);
		IS_TRUE0(FIRST_PHY_REG == 0);
		for (INT i = 0; i <= 1023; i++) {
			m_16->bunion(i);
		}
	}
	return m_16;
}


void RSC::comp_st_fmt(IR const* ir)
{
	IS_TRUE0(ir->is_stid() || ir->is_stpr());
	IR const* stv = ir->get_rhs();
	comp_ir_fmt(stv);
	if (ir->is_stpr()) {
		switch (IR_type(stv)) {
		case IR_CONST:
			//A, +B, load const
			m_ir2fmt.set(IR_id(ir), FABcv);
			break;
		case IR_LD:
			//AABBBB, sget
			m_ir2fmt.set(IR_id(ir), FAABBBB);
			break;
		case IR_ILD:
			//ABCCCC, iget
			m_ir2fmt.set(IR_id(ir), FABCCCCv);
			return;
		case IR_LDA:
			IS_TRUE0(IR_type(LDA_base(stv)) == IR_ID);
			IS_TRUE0(LDA_base(stv)->is_str(m_dm));
			//AABBBB
			m_ir2fmt.set(IR_id(ir), FAABBBBv);
			return;
		case IR_ADD:
		case IR_SUB:
		case IR_MUL:
		case IR_DIV:
		case IR_REM:
		case IR_MOD:
		case IR_BAND:
		case IR_BOR:
		case IR_XOR:
		case IR_ASR:
		case IR_LSR:
		case IR_LSL:
			{
				IR * op1 = BIN_opnd1(stv);
				IS_TRUE0(BIN_opnd0(stv)->is_pr());
				if (IR_type(op1) == IR_CONST) {
					IS_TRUE0(op1->is_int(m_dm));
					if (!is_s8((INT)CONST_int_val(op1))) {
						//vA, vB, CCCC+
						m_ir2fmt.set(IR_id(ir), FABCCCCv);
					} else {
						IS_TRUE0(is_s16((INT)CONST_int_val(op1)));
						//vAA, vAB, CC+
						m_ir2fmt.set(IR_id(ir), FAABBCCcv);
					}
				} else {
					//AABBCC, v0 = v1 op v2
					m_ir2fmt.set(IR_id(ir), FAABBCC);
				}
			}
			return;
		case IR_LT:
		case IR_LE:
		case IR_GT:
		case IR_GE:
		case IR_EQ:
		case IR_NE:
			//AABBCC, v0 = v1 op v2
			m_ir2fmt.set(IR_id(ir), FAABBCC);
			return;
		case IR_ARRAY:
			//AABBCC.
			//aget, OBJ, v0 <- (base)v1, (ofst)v2
			IS_TRUE0(ARR_base(stv)->is_pr() &&
					 ARR_sub_list(stv)->is_pr() &&
					 ((CARRAY*)stv)->get_dimn() == 1);
			m_ir2fmt.set(IR_id(ir), FAABBCC);
			return;
		case IR_BNOT:
		case IR_LNOT:
		case IR_NEG:
			//AB.
			IS_TRUE(UNA_opnd0(stv)->is_pr(), ("unary op base must be pr"));
			m_ir2fmt.set(IR_id(ir), FAB);
			return;
		case IR_CVT:
			//AB.
			IS_TRUE(CVT_exp(stv)->is_pr(), ("cvt base must be pr"));
			m_ir2fmt.set(IR_id(ir), FAB);
			return;
		case IR_PR:
			//AAAABBBB
			m_ir2fmt.set(IR_id(ir), FAAAABBBB);
			return;
		default: IS_TRUE0(0);
		}
	} else {
		IS_TRUE0(ir->is_stid());
		switch (IR_type(stv)) {
		case IR_PR:
			//AABBBB, sput
			m_ir2fmt.set(IR_id(ir), FAABBBB);
			break;
		case IR_ILD:
			//ABCCCC, iget
			m_ir2fmt.set(IR_id(ir), FABCCCCv);
			return;
		default: IS_TRUE0(0);
		}
	}
}


void RSC::comp_ist_fmt(IR const* ir)
{
	IS_TRUE0(IR_type(ir) == IR_IST);
	IR const* stv = IST_rhs(ir);
	comp_ir_fmt(stv);
	IS_TRUE0(stv->is_pr());

	IR const* lhs = IST_base(ir);
	if (lhs->is_pr()) {
		//ABCCCC
		//iput, vA(stv) -> vB(mlr), +CCCC(field_id)
		m_ir2fmt.set(IR_id(ir), FABCCCCv);
		return;
	} else if (IR_type(lhs) == IR_ARRAY) {
		IS_TRUE(((CARRAY*)lhs)->get_dimn() == 1,
				("dex supply only one dim array"));
		//AABBCC
		//aput, OBJ, vAA -> (array_base_ptr)vBB, (array_elem)vCC
		IR * base = ARR_base(lhs);
		IR * sub = ARR_sub_list(lhs);
		IS_TRUE0(base->is_pr() && sub->is_pr());
		m_ir2fmt.set(IR_id(ir), FAABBCC);
		return;
	}
	IS_TRUE0(0);
}


void RSC::comp_call_fmt(IR const* ir)
{
	IS_TRUE0(ir->is_call());
	if (IR_type(ir) == IR_ICALL) {
		comp_ir_fmt(ICALL_callee(ir));
	}
	if (IR_type(ir) == IR_CALL && CALL_is_intrinsic(ir)) {
		VAR const* v = CALL_idinfo(ir);
		IS_TRUE0(v);
		BLTIN_TYPE blt = m_str2intri.get(SYM_name(VAR_name(v)));
		IS_TRUE0(blt != BLTIN_UNDEF);
		switch (blt) {
		case BLTIN_NEW:
			m_ir2fmt.set(IR_id(ir), FAABBBBv);
			return;
		case BLTIN_NEW_ARRAY:
			m_ir2fmt.set(IR_id(ir), FABCCCCv);
			return;
		case BLTIN_MOVE_EXP:
		case BLTIN_MOVE_RES:
			m_ir2fmt.set(IR_id(ir), FAA);
			return;
		case BLTIN_THROW:
			IS_TRUE0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
			m_ir2fmt.set(IR_id(ir), FAA);
			return;
		case BLTIN_CHECK_CAST:
			IS_TRUE0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
			m_ir2fmt.set(IR_id(ir), FAABBBBv);
			return;
		case BLTIN_FILLED_NEW_ARRAY:
			{
			//first parameter is invoke-kind.
			IR * p = CALL_param_list(ir);
			IS_TRUE0(IR_type(p) == IR_CONST);

			//second one is class-id.
			p = IR_next(p);
			IS_TRUE0(IR_type(p) == IR_CONST);

			p = IR_next(p);
			for (; p != NULL; p = IR_next(p)) {
				IS_TRUE0(p->is_pr());
			}
			m_ir2fmt.set(IR_id(ir), FACDEFGBBBBv);
			return;
			}
		case BLTIN_FILL_ARRAY_DATA:
			IS_TRUE0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
			m_ir2fmt.set(IR_id(ir), FAABBBBBBBBv);
			return;
		case BLTIN_CONST_CLASS:
			m_ir2fmt.set(IR_id(ir), FAABBBBv);
			return;
		case BLTIN_ARRAY_LENGTH:
			IS_TRUE0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
			m_ir2fmt.set(IR_id(ir), FAB);
			return;
		case BLTIN_MONITOR_ENTER:
		case BLTIN_MONITOR_EXIT:
			IS_TRUE0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
			m_ir2fmt.set(IR_id(ir), FAA);
			return;
		case BLTIN_INSTANCE_OF:
			IS_TRUE0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
			m_ir2fmt.set(IR_id(ir), FABCCCCv);
			return;
		case BLTIN_CMP_BIAS:
			{
				IR const* p = CALL_param_list(ir);
				IS_TRUE0(p && IR_type(p) == IR_CONST);
				p = IR_next(p);
				IS_TRUE0(p && p->is_pr() && IR_next(p) && IR_next(p)->is_pr());
				m_ir2fmt.set(IR_id(ir), FAABBCC);
				return;
			}
		default:IS_TRUE(0, ("Unknown intrinsic"));
		}
	}
	IR const* p = CALL_param_list(ir);
	IS_TRUE0(p && IR_type(p) == IR_CONST);
	INVOKE_KIND ik = (INVOKE_KIND)CONST_int_val(p);
	p = IR_next(p);
	IS_TRUE0(p && IR_type(p) == IR_CONST);
	p = IR_next(p);
	switch (ik) {
	case INVOKE_UNDEF:
	case INVOKE_VIRTUAL:
	case INVOKE_SUPER:
	case INVOKE_DIRECT:
	case INVOKE_STATIC:
	case INVOKE_INTERFACE:
		{
			UINT c = cnt_list(p);
			IS_TRUE0(c <= 5);
			m_ir2fmt.set(IR_id(ir), FACDEFGBBBBv);
		}
		break;
	case INVOKE_VIRTUAL_RANGE:
	case INVOKE_SUPER_RANGE:
	case INVOKE_DIRECT_RANGE:
	case INVOKE_STATIC_RANGE:
	case INVOKE_INTERFACE_RANGE:
		m_ir2fmt.set(IR_id(ir), FAACCCCBBBBv);
		break;
	default: IS_TRUE0(0);
	}

	for (; p != NULL; p = IR_next(p)) {
		IS_TRUE0(p->is_pr());
	}
}


void RSC::comp_ir_fmt(IR const* ir)
{
	switch (IR_type(ir)) {
	case IR_CONST:
	case IR_ID:
	case IR_LD:
		return;
	case IR_ST:
	case IR_STPR:
		comp_st_fmt(ir);
		return;
	case IR_ILD:
		//ABCCCC
		//v%d(res) <- v%d(op0), field_id(op1)
		comp_ir_fmt(ILD_base(ir));
		if (ILD_base(ir)->is_pr()) {
			m_ir2fmt.set(IR_id(ir), FABCCCCv);
		} else { IS_TRUE0(0); }
		return;
	case IR_IST:
		comp_ist_fmt(ir);
		return;
	case IR_LDA:
		return;
	case IR_CALL:
	case IR_ICALL:
		comp_call_fmt(ir);
		return;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
	case IR_BAND:
	case IR_BOR:
	case IR_XOR:
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ:
	case IR_NE:
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		comp_ir_fmt(BIN_opnd0(ir));
		comp_ir_fmt(BIN_opnd1(ir));
		return;
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG:
		comp_ir_fmt(UNA_opnd0(ir));
		return;
	case IR_GOTO:
	case IR_LABEL:
		m_ir2fmt.set(IR_id(ir), F0);
		return;
	case IR_SWITCH:
		comp_ir_fmt(SWITCH_vexp(ir));
		m_ir2fmt.set(IR_id(ir), FAABBBBBBBBv);
		return;
	case IR_ARRAY:
		IS_TRUE0(((CARRAY*)ir)->get_dimn() == 1);
		comp_ir_fmt(ARR_base(ir));
		comp_ir_fmt(ARR_sub_list(ir));
		return;
	case IR_CVT:
		comp_ir_fmt(CVT_exp(ir));
		return;
	case IR_PR:
		return;
	case IR_TRUEBR:
	case IR_FALSEBR:
		{
			IR * det = BR_det(ir);
			comp_ir_fmt(BR_det(ir));
			IS_TRUE0(det->is_relation());
			if (IR_type(BIN_opnd1(det)) == IR_CONST) {
				//AABBBB
				IS_TRUE0(BIN_opnd0(det)->is_pr());
				m_ir2fmt.set(IR_id(ir), FAABBBBv);
			} else {
				IS_TRUE0(BIN_opnd0(det)->is_pr());
				IS_TRUE0(BIN_opnd1(det)->is_pr());
				//ABCCCC
				m_ir2fmt.set(IR_id(ir), FABCCCCv);
			}
		}
		return;
	case IR_RETURN:
		IS_TRUE0(cnt_list(RET_exp(ir)) <= 1);
		if (RET_exp(ir) == NULL) {
			m_ir2fmt.set(IR_id(ir), F0);
		} else {
			IS_TRUE0(RET_exp(ir)->is_pr());
			m_ir2fmt.set(IR_id(ir), FAA);
		}
		return;
	case IR_SELECT:
		comp_ir_fmt(SELECT_det(ir));
		comp_ir_fmt(SELECT_trueexp(ir));
		comp_ir_fmt(SELECT_falseexp(ir));
		IS_TRUE0(0);
		return;
	case IR_REGION:
	default: IS_TRUE0(0);
	}
}


void RSC::comp_ir_constrain()
{
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		for (IR const* ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			comp_ir_fmt(ir);
		}
	}
}


void RSC::dump_ir_fmt()
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n==------- DUMP IR FMT --------==");
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		fprintf(g_tfile, "\n-- BB%d --", IR_BB_id(bb));
		for (IR const* ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			FMT f = m_ir2fmt.get(IR_id(ir));
			fprintf(g_tfile, "\nFMT:%s", g_fmt_name[f]);
			dump_ir(ir, m_ru->get_dm());
		}
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


void RSC::dump_glt_usable()
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n=== DUMP GLT Usable Regs: ===");
	SVECTOR<GLT*> * gltv = m_gltm->get_gltvec();
	bool dump_bit = false;
	for (INT i = 0; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		fprintf(g_tfile, "\nGLT%d(pr%d):", GLT_id(g), GLT_prno(g));
		if (GLT_prefer_reg(g) != REG_UNDEF) {
			fprintf(g_tfile, "prefer_reg=%d", LT_prefer_reg(g));
		} else {
			BITSET const* usable = GLT_usable(g);
			if (usable == NULL || usable->is_empty()) {
				fprintf(g_tfile, "--");
				continue;
			}

			if (dump_bit) {
				for (INT i = usable->get_first();
					 i >= 0; i = usable->get_next(i)) {
					fprintf(g_tfile, "%d,", i);
				}
			} else {
				fprintf(g_tfile, "%d~%d",
						usable->get_first(), usable->get_last());
			}
		}
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


void RSC::dump_bb(UINT bbid)
{
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n-- BB%d Usable Regs --", bbid);
	LTM * ltm = m_gltm->get_ltm(bbid);
	IS_TRUE0(ltm);
	SVECTOR<LT*> * ltvec = ltm->get_lt_vec();
	bool dump_bit = false;
	for (INT i = 0; i <= ltvec->get_last_idx(); i++) {
		LT const* lt = ltvec->get(i);
		if (lt == NULL) { continue; }
		fprintf(g_tfile, "\nLT%d(pr%d):", LT_uid(lt), LT_prno(lt));
		if (LT_prefer_reg(lt) != REG_UNDEF) {
			fprintf(g_tfile, "prefer_reg=%d", LT_prefer_reg(lt));
		} else {
			BITSET const* usable = LT_usable(lt);
			if (usable == NULL || usable->is_empty()) {
				fprintf(g_tfile, "--");
				continue;
			}
			if (dump_bit) {
				for (INT i = usable->get_first();
					 i >= 0; i = usable->get_next(i)) {
					fprintf(g_tfile, "%d,", i);
				}
			} else {
				fprintf(g_tfile, "%d~%d", usable->get_first(), usable->get_last());
			}
		}
	}
	fflush(g_tfile);
}


void RSC::dump()
{
	if (g_tfile == NULL) { return; }
	dump_ir_fmt();
	dump_glt_usable();
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		dump_bb(IR_BB_id(bb));
	}
	fflush(g_tfile);
}


void RSC::init_usable()
{
	m_usable[FAB][1] = get_4();
	m_usable[FAB][0] = get_4();

	m_usable[FABcv][1] = get_4();
	m_usable[FABcv][0] = NULL;

	m_usable[FAA][1] = get_8();
	m_usable[FAA][0] = get_8();

	m_usable[FAAv][1] = NULL;
	m_usable[FAAv][0] = NULL;

	m_usable[FAAAAv][1] = NULL;
	m_usable[FAAAAv][0] = NULL;

	m_usable[FAABBBB][1] = get_8();
	m_usable[FAABBBB][0] = get_16();

	m_usable[FAABBBBv][1] =
	m_usable[FAABBBBcv][1] =
	m_usable[FAABBBBcvh][1] =
	m_usable[FAABBCC][1] =
	m_usable[FAABBCCcv][1] = get_8();

	m_usable[FAABBBBv][0] =
	m_usable[FAABBBBcv][0] =
	m_usable[FAABBBBcvh][0] =
	m_usable[FAABBCC][0] =
	m_usable[FAABBCCcv][0] = get_8();

	m_usable[FABCCCCv][1] =
	m_usable[FABCCCCcv][1] = get_4();
	m_usable[FABCCCCv][0] =
	m_usable[FABCCCCcv][0] = get_4();

	m_usable[FAAAABBBB][1] = get_16();
	m_usable[FAAAABBBB][0] = get_16();

	m_usable[FAAAAAAAAv][1] = NULL;
	m_usable[FAAAAAAAAv][0] = NULL;

	m_usable[FAABBBBBBBBv][1] = get_8();
	m_usable[FAABBBBBBBBv][0] = get_8();

	m_usable[FAABBBBBBBBcv][1] = get_8();
	m_usable[FAABBBBBBBBcv][0] = get_8();

	m_usable[FACDEFGBBBBv][1] = get_4();
	m_usable[FACDEFGBBBBv][0] = get_4();

	m_usable[FAACCCCBBBBv][1] = get_16();
	m_usable[FAACCCCBBBBv][0] = get_16();

	m_usable[FAABBBBBBBBBBBBBBBBcv][1] = get_16();
	m_usable[FAABBBBBBBBBBBBBBBBcv][0] = get_16();

	IS_TRUE0(FAABBBBBBBBBBBBBBBBcv + 1 == FNUM);
}


//Compute usable for lt.
void RSC::comp_lt_usable(LT * lt, LTM * ltm)
{
	BITSET * occ = LT_occ(lt);
	if (occ == NULL) { return; }

	BITSET const* usable = NULL;
	bool first_alloc = true;
	for (INT i = occ->get_first(); i >= 0; i = occ->get_next(i)) {
		IR const* ir = ltm->get_ir(i);
		IS_TRUE0(ir);
		BITSET * x = get_usable(get_fmt(ir), lt->is_def(i));
		if (x == NULL) { continue; }
		if (usable == NULL) {
			if (first_alloc) {
				first_alloc = false;
				usable = x;
			} else {
				;//usable is empty!
			}
		} else if (usable == x || usable->is_contain(*x, true)) {
			//Shrink
			usable = x;
		}
	}

	if (LT_is_global(lt)) {
		GLT * glt = map_lt2glt(lt);
		IS_TRUE0(glt && GLT_usable(glt));
		if (GLT_usable(glt)->is_contain(*usable, true)) {
			//Shrink
			GLT_usable(glt) = usable;
		}

		//Note all local part of global lt will be set after local computation
		//complete. The assignment to LT_usable() may be redundant.
		LT_usable(lt) = GLT_usable(glt);
	} else {
		LT_usable(lt) = usable;
	}
}


//Verify each stmt has instruction constrain.
bool RSC::verify_fmt()
{
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		for (IR const* ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			FMT f = m_ir2fmt.get(IR_id(ir));
			IS_TRUE0(f != FUNDEF);
		}
	}
	return true;
}


//'omit_constrain': true if all constrain is omit, and
//					only initialize usable set to the largest.
void RSC::comp_local_usage(LTM * ltm, bool only_local, bool omit_constrain)
{
	SVECTOR<LT*> * ltvec = ltm->get_lt_vec();
	for (INT i = 1; i <= ltvec->get_last_idx(); i++) {
		LT * lt = ltvec->get(i);
		if (lt == NULL) { continue; }
		if (only_local && LT_is_global(lt)) { continue; }
		if (omit_constrain) {
			LT_usable(lt) = get_16();
			continue;
		}
		comp_lt_usable(lt, ltm);
	}
}


//Recompute all global and local lt constrain.
//Note an explictly clean() is dispensable before this call.
void RSC::perform(bool omit_constrain)
{
	comp_ir_constrain();
	IS_TRUE0(verify_fmt());

	//Initialize global lt usable-regs.
	SVECTOR<GLT*> * gltv = m_gltm->get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		GLT_usable(g) = get_16();
	}

	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm->map_bb2ltm(bb);
		if (ltm == NULL) { continue; }
		comp_local_usage(ltm, false, omit_constrain);
	}
	if (omit_constrain) { return; }

	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		g->set_local(*m_gltm);
	}
}
//END RSC


//
//START GLTM Global Life Time Manager
//
GLTM::GLTM(REGION * ru, PRDF * prdf, RA * ra)
{
	m_glt_count = 1;
	m_ru = ru;
	m_ra = ra;
	m_rsc = ra->get_rsc();
	m_dm = ru->get_dm();
	m_prdf = prdf;
	m_is_consider_local_interf = false;
	m_pool = smpool_create_handle(sizeof(GLT) * 10, MEM_COMM);
}


//Localize glt to several lts.
void GLTM::localize(GLT * g)
{
	IS_TRUE0(!GLT_is_param(g));
	DBITSETC * bbs = GLT_bbs(g);
	IS_TRUE(bbs, ("should not localize an empty glt"));
	SC<SEG*> * sc;
	UINT prno = GLT_prno(g);
	for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
		LTM * ltm = get_ltm(j);
		if (ltm == NULL) { continue; }
		SBITSETC * livein = m_prdf->get_livein(j);
		SBITSETC * liveout = m_prdf->get_liveout(j);
		if (livein != NULL) {
			livein->diff(prno, m_prdf->get_sbs_mgr());
		}
		if (liveout != NULL) {
			liveout->diff(prno, m_prdf->get_sbs_mgr());
		}
		LT * gl = ltm->map_pr2lt(prno);
		IS_TRUE0(gl); //glt miss local part.
		//Note the usable-regs of local part do not changed.

		bool has_occ = true;
		if (LT_occ(gl) == NULL || LT_occ(gl)->is_empty()) {
			ltm->remove_lt(gl);
			has_occ = false;
		} else {
			LT_is_global(gl) = false;
		}

		ltm->clean();
		ltm->build(true, NULL, m_cii);

		ltm->get_ig()->erase();
		ltm->get_ig()->build();
		//m_rsc->comp_local_usage(ltm, true); Does it need?

		if (has_occ) {
			//Set perfer reg.
			gl = ltm->map_pr2lt(prno); //recheck
			IS_TRUE0(!LT_is_global(gl));
			LT_prefer_reg(gl) = FIRST_PHY_REG;
		}
	}
	m_pr2glt.set(prno, NULL);
	m_gltid2glt_map.set(GLT_id(g), NULL);
	m_sbs_mgr.free_dbitsetc(bbs);
}


//Build glt that lifetime just likes 'cand'.
//Note the function only build global part of lt.
GLT * GLTM::build_glt_like(IR * pr, GLT * cand)
{
	GLT * newglt = new_glt(PR_no(pr));
	DBITSETC * bbs = GLT_bbs(cand);
	if (bbs != NULL) {
		GLT_bbs(newglt) = m_sbs_mgr.create_dbitsetc();
		GLT_bbs(newglt)->copy(*bbs, m_sbs_mgr);
		SC<SEG*> * sc;
		UINT candprno = GLT_prno(cand);
		UINT prno = GLT_prno(newglt);
		for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
			SBITSETC * livein = m_prdf->get_livein(j);
			SBITSETC * liveout = m_prdf->get_liveout(j);
			if (livein != NULL && livein->is_contain(candprno)) {
				livein->bunion(prno, m_prdf->get_sbs_mgr());
			}
			if (liveout != NULL && liveout->is_contain(candprno)) {
				liveout->bunion(prno, m_prdf->get_sbs_mgr());
			}
		}
	}
	return newglt;
}


//Verify the consistency of global lt and local part.
bool GLTM::verify()
{
	for (INT i = 1; i <= m_gltid2glt_map.get_last_idx(); i++) {
		GLT * g = m_gltid2glt_map.get(i);
		if (g == NULL || GLT_bbs(g) == NULL) { continue; }
		SC<SEG*> * cur;
		for (INT j = GLT_bbs(g)->get_first(&cur);
			 j >= 0; j = GLT_bbs(g)->get_next(j, &cur)) {
			LTM * ltm = m_bb2ltmgr.get(j);
			IS_TRUE0(ltm);
			LT * l = ltm->map_pr2lt(GLT_prno(g));
			IS_TRUE0(l && LT_is_global(l));
			IS_TRUE0(LT_phy(l) == GLT_phy(g));

			//Global lt's reg group size may be larger than local part.
			IS_TRUE0(LT_rg_sz(l) <= GLT_rg_sz(g));
		}
	}
	return true;
}


//Dump global lt info.
void GLTM::dumpg()
{
	if (g_tfile == NULL) return;
	UINT max_name_len = 7;
	BITSET prs;
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		for (IR const* o = IR_BB_first_ir(bb);
			 o != NULL; o = IR_BB_next_ir(bb)) {
			m_cii.clean();
			for (IR const* k = ir_iter_init_c(o, m_cii);
				 k != NULL; k = ir_iter_next_c(m_cii)) {
				if (k->is_pr()) {
					prs.bunion(PR_no(k));
				}
			}
		}
	}
	INT maxprno = MAX(0, prs.get_last());

	UINT maxr = 0;
	for (INT i = prs.get_first(); i >= 0; i = prs.get_next(i)) {
		GLT * g = map_pr2glt(i);
		if (g != NULL && g->has_allocated()) {
			maxr = MAX(maxr, GLT_phy(g));
		}
	}

	CHAR litbuf[32];
	UINT num = 0; //number of allocated register.
	while (maxr != 0) {
		maxr /= 10;
		num++;
	}
	sprintf(litbuf, "v%%-%dd", num);

	CHAR litbuf2[32];
	UINT num2 = 0; //number of pr.
	while (maxprno != 0) {
		maxprno /= 10;
		num2++;
	}
	sprintf(litbuf2, "pr%%-%dd", num2);

	fprintf(g_tfile,
			"\n=== DUMP Global Life Time = maxreg:%d = paramnum:%d ===",
			m_ra->m_maxreg, m_ra->m_param_num);
	if (m_ra->m_param_num > 0) {
		fprintf(g_tfile, "{");
		for (INT i = 0; i <= m_params.get_last_idx(); i++) {
			INT pr = m_params.get(i);
			if (pr != 0) {
				fprintf(g_tfile, "pr%d", pr);
				GLT * g = map_pr2glt(pr);
				IS_TRUE0(g); //may be pr is local.
				if (g->has_allocated()) {
					fprintf(g_tfile, "(v%d)", GLT_phy(g));
				}
				fprintf(g_tfile, ",");
			} else {
				fprintf(g_tfile, "--,");
			}
		}
		fprintf(g_tfile, "}");
	}
	fprintf(g_tfile, "\n");

	CHAR buf[64];
	for (INT i = prs.get_first(); i >= 0; i = prs.get_next(i)) {
		GLT * g = map_pr2glt(i);
		if (g == NULL) { continue; }

		//Print prno.
		fprintf(g_tfile, "\n");
		fprintf(g_tfile, litbuf2, GLT_prno(g));
		fprintf(g_tfile, " ");

		//Print phy.
		if (g != NULL) {
			fprintf(g_tfile, "[");
			if (GLT_phy(g) == REG_UNDEF) {
				UINT h = 0;
				while (h <= num) {
					fprintf(g_tfile, "-");
					h++;
				}
			} else {
				fprintf(g_tfile, litbuf, GLT_phy(g));
			}
			fprintf(g_tfile, "]");
		}

		//Print live BB.
		DBITSETC * livebbs = GLT_bbs(g);
		if (livebbs == NULL || livebbs->is_empty()) { continue; }
		INT start = 0;
		SC<SEG*> * sc;
		for (INT u = livebbs->get_first(&sc);
			 u >= 0; u = livebbs->get_next(u, &sc)) {
			for (INT j = start; j < u; j++) {
				sprintf(buf, "%d,", j);
				for (UINT k = 0; k < strlen(buf); k++) {
					fprintf(g_tfile, " ");
				}
			}
			fprintf(g_tfile, "%d,", u);
			start = u + 1;
		}
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


//Dump global and local lt info.
void GLTM::dump()
{
	dumpg();
	if (g_tfile == NULL) { return; }
	fprintf(g_tfile, "\n=== DUMP Local Life Time ===");
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * lltmgr = map_bb2ltm(bb);
		lltmgr->dump();
	}
	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}


//Dump local lt info.
void GLTM::dumpl(UINT bbid)
{
	LTM * l = m_bb2ltmgr.get(bbid);
	IS_TRUE0(l);
	l->dump();
}


LTM * GLTM::map_bb2ltm(IR_BB * bb)
{
	LTM * l = m_bb2ltmgr.get(IR_BB_id(bb));
	if (l == NULL) {
		//Allocated object managed by RA_MGR, and do not delete it youself.
		l = new LTM(bb, m_prdf, this, m_pool);
		m_bb2ltmgr.set(IR_BB_id(bb), l);
	}
	return l;
}


/*
Every OR which refering sr must be assigned to same cluster, therefore
the only time to record cluster information is the first meeting with sr.
*/
GLT * GLTM::new_glt(UINT prno)
{
	GLT * glt = (GLT*)xmalloc(sizeof(GLT));
	GLT_id(glt) = m_glt_count++;
	GLT_prno(glt) = prno;
	GLT_phy(glt) = REG_UNDEF;
	GLT_prefer_reg(glt) = REG_UNDEF;
	GLT_rg_sz(glt) = 1;
	GLT_freq(glt) = 1.0;
	m_pr2glt.set(prno, glt);
	m_gltid2glt_map.set(GLT_id(glt), glt);
	return glt;
}


void GLTM::rename_glt(GLT * g)
{
	IR * newpr = NULL;
	SC<SEG*> * cur;
	if (GLT_bbs(g) == NULL) { return; }

	for (INT i = GLT_bbs(g)->get_first(&cur);
		 i >= 0; i = GLT_bbs(g)->get_next(i, &cur)) {
		LTM * ltm = m_bb2ltmgr.get(i);
		IS_TRUE0(ltm);
		LT * l = ltm->map_pr2lt(GLT_prno(g));
		IS_TRUE0(l);
		ltm->rename_lt(l, &newpr);
	}
	if (newpr != NULL) {
		GLT_prno(g) = PR_no(newpr);
		m_pr2glt.set(GLT_prno(g), g);
	}
	m_ru->free_ir(newpr);
}


//If local PR is same with global PR, rename local PR.
//If there are multiple local lifetime corresponded to same PR, rename them.
void GLTM::rename_local()
{
	TMAP<UINT, LT*> prno2lt;
	BITSET met;
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_bb2ltmgr.get(IR_BB_id(bb));
		if (ltm == NULL) { continue; }
		ltm->rename(prno2lt, met);
	}
}


void GLTM::rename()
{
	rename_local();
	TMAP<UINT, GLT*> prno2glt;
	for (INT i = 1; i <= m_gltid2glt_map.get_last_idx(); i++) {
		GLT * g = m_gltid2glt_map.get(i);
		IS_TRUE0(g);
		GLT * prior = prno2glt.get(GLT_prno(g));
		if (prior == NULL) {
			prno2glt.set(GLT_prno(g), g);
		} else if (prior != g) {
			rename_glt(g);
		}
	}
}


//Build global life time and set map between PR and Vreg parameter.
void GLTM::build(bool build_group_part)
{
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	INT vreg = 	m_ra->m_vregnum - m_ra->m_param_num;
	for (UINT i = 0; i < m_ra->m_param_num; i++, vreg++) {
		IS_TRUE0(vreg >= 0 && vreg < (INT)m_ra->m_vregnum);
		IR * pr = m_ra->m_v2pr->get(vreg);
		if (pr == NULL) {
			//Some parameter has no use.
			continue;
		}
		UINT prno = PR_no(pr);
		GLT * glt = map_pr2glt(prno);
		if (glt == NULL) {
			glt = new_glt(prno);
		}
		GLT_is_param(glt) = true;
		GLT_param_pos(glt) = i;
		m_params.set(i, prno);
	}
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		SBITSETC * livein = m_prdf->get_livein(IR_BB_id(bb));
		SBITSETC * liveout = m_prdf->get_liveout(IR_BB_id(bb));
		if (livein != NULL) {
			SC<SEG*> * cur;
			for (INT i = livein->get_first(&cur);
				 i >= 0; i = livein->get_next(i, &cur)) {
				GLT * glt = map_pr2glt(i);
				if (glt == NULL) {
					glt = new_glt(i);
				}
				if (GLT_bbs(glt) == NULL) {
					GLT_bbs(glt) = m_sbs_mgr.create_dbitsetc();
				}
				GLT_bbs(glt)->bunion(IR_BB_id(bb), m_sbs_mgr);
			}
		}

		if (liveout != NULL) {
			SC<SEG*> * cur;
			for (INT i = liveout->get_first(&cur);
				 i >= 0; i = liveout->get_next(i, &cur)) {
				GLT * glt = map_pr2glt(i);
				if (glt == NULL) {
					glt = new_glt(i);
				}
				if (GLT_bbs(glt) == NULL) {
					GLT_bbs(glt) = m_sbs_mgr.create_dbitsetc();
				}
				GLT_bbs(glt)->bunion(IR_BB_id(bb), m_sbs_mgr);
			}
		}

		LTM * lltmgr = map_bb2ltm(bb);
		lltmgr->build(m_is_consider_local_interf, NULL, m_cii);
	}
	rename();

	if (!build_group_part) {
		return;
	}

	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * lltmgr = map_bb2ltm(bb);
		lltmgr->build_group(m_cii);
	}
	rename_local(); //renaming group part lts.
}
//END GLTM


//
//START GIG
//
void GIG::dump_vcg(CHAR const* name)
{
	if (name == NULL) {
		name = "graph_global_if.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!",name));
	fprintf(h, "graph: {"
			  "title: \"GRAPH\"\n"
			  "shrink:  15\n"
			  "stretch: 27\n"
			  "layout_downfactor: 1\n"
			  "layout_upfactor: 1\n"
			  "layout_nearfactor: 1\n"
			  "layout_splinefactor: 70\n"
			  "spreadlevel: 1\n"
			  "treefactor: 0.500000\n"
			  "node_alignment: center\n"
			  "orientation: top_to_bottom\n"
			  "late_edge_labels: no\n"
			  "display_edge_labels: yes\n"
			  "dirty_edge_labels: no\n"
			  "finetuning: no\n"
			  "nearedges: no\n"
			  "splines: yes\n"
			  "ignoresingles: no\n"
			  "straight_phase: no\n"
			  "priority_phase: no\n"
			  "manhatten_edges: no\n"
			  "smanhatten_edges: no\n"
			  "port_sharing: no\n"
			  "crossingphase2: yes\n"
			  "crossingoptimization: yes\n"
			  "crossingweight: bary\n"
			  "arrow_mode: free\n"
			  "layoutalgorithm: mindepthslow\n"
			  "node.borderwidth: 2\n"
			  "node.color: lightcyan\n"
			  "node.textcolor: black\n"
			  "node.bordercolor: blue\n"
			  "edge.color: darkgreen\n");

	CHAR buf[MAX_OR_SR_NAME_BUF_LEN];
	//Print node
	INT c;
	for (VERTEX const* v = m_vertices.get_first(c);
		 v != NULL;  v = m_vertices.get_next(c)) {
		GLT * glt = m_gltm->get_glt(VERTEX_id(v));
		sprintf(buf, "GLT%d(pr%d):", VERTEX_id(v), GLT_prno(glt));
		fprintf(h, "\nnode: { title:\"%d\" label:\"%s\" "
				   "shape:circle fontname:\"courB\" color:gold}",
				VERTEX_id(v), buf);
	}

	//Print edge
	for (EDGE const* e = m_edges.get_first(c);
		 e != NULL;  e = m_edges.get_next(c)) {
		fprintf(h, "\nedge: { sourcename:\"%d\" targetname:\"%d\" %s}",
				VERTEX_id(EDGE_from(e)),
				VERTEX_id(EDGE_to(e)),
				m_is_direction ? "" : "arrowstyle:none" );
	}
	fprintf(h, "\n}\n");
	fclose(h);
}


//Return true if phy has been occupied by g's neighbours.
//'nis': for tmp use.
bool GIG::is_interf_with_neighbour(GLT * g, SBITSET & nis, UINT phy)
{
	IS_TRUE0(g && phy < 65000);
	nis.clean();
	get_neighbor_set(nis, GLT_id(g));
	SC<SEG*> * cur;
	for (INT ltid = nis.get_first(&cur);
		 ltid >= 0; ltid = nis.get_next(ltid, &cur)) {
		GLT * g = m_gltm->get_glt(ltid);
		IS_TRUE0(g);
		if (!g->has_allocated()) { continue; }
		if (GLT_phy(g) == phy) { return true; }
		if (GLT_rg_sz(g) > 1) {
			IS_TRUE0(GLT_rg_sz(g) == RG_PAIR_SZ);
			if ((UINT)GLT_phy(g) + 1 == phy) { return true; }
		}
	}
	return false;
}


bool GIG::is_interf(IN GLT * glt1, IN GLT * glt2)
{
	if (GLT_id(glt1) == GLT_id(glt2)) return true;
	if (GLT_bbs(glt1) == NULL || GLT_bbs(glt2) == NULL) {
		return false;
	}
	if (!GLT_bbs(glt1)->is_intersect(*GLT_bbs(glt2))) {
		return false;
	}
	if (!m_is_consider_local_interf) { return true; }
	DBITSETC * bs1 = GLT_bbs(glt1);
	DBITSETC * bs2 = GLT_bbs(glt2);
	UINT pr1 = GLT_prno(glt1);
	UINT pr2 = GLT_prno(glt2);
	SC<SEG*> * sc;
	for (INT i = bs1->get_first(&sc); i >= 0; i = bs1->get_next(i, &sc)) {
		if (!bs2->is_contain(i)) { continue; }
		IR_BB * bb = m_cfg->get_bb(i);
		IS_TRUE0(bb != NULL);
		LTM * ltmgr = m_gltm->map_bb2ltm(bb);
		LT * lt1 = ltmgr->map_pr2lt(pr1);
		LT * lt2 = ltmgr->map_pr2lt(pr2);
		IS_TRUE0(lt1 != NULL);
		IS_TRUE0(lt2 != NULL);
		if (lt1->is_intersect(lt2)) {
			return true;
		}
	}
	return false;
}


//Set glt interfered with a list of glt.
void GIG::set_interf_with(UINT gltid, LIST<UINT> & lst)
{
	IS_TRUE(m_vertices.find((OBJTY)gltid), ("not on graph"));
	UINT n = lst.get_elem_count();
	for (UINT i = lst.get_head(); n > 0; i = lst.get_next(), n--) {
		IS_TRUE(m_vertices.find((OBJTY)i), ("not on graph"));
		add_edge(gltid, i);
	}
}


void GIG::build()
{
	//Check interference
	SVECTOR<GLT*> * pr2glt = m_gltm->get_pr2glt_map();
	INT n = pr2glt->get_last_idx();
	for (INT i = 0; i <= n; i++) {
		GLT * lt1 = pr2glt->get(i);
		if (lt1 == NULL) { continue; }
		add_vertex(GLT_id(lt1));
		for (INT j = i + 1; j <= n; j++) {
			GLT * lt2 = pr2glt->get(j);
			if (lt2 == NULL) { continue; }
			if (is_interf(lt1, lt2)) {
				add_edge(GLT_id(lt1), GLT_id(lt2));
			}
		}
	}
}
//END GIG


//
//START IG
//
bool IG::is_interf(LT const* lt1, LT const* lt2) const
{
	if (lt1 == lt2) { return true; }
	if (LT_range(lt1) == NULL || LT_range(lt2) == NULL) {
		return false;
	}
	if (LT_range(lt1)->is_intersect(*LT_range(lt2))) {
		return true;
	}
	return false;
}


void IG::build()
{
	IS_TRUE0(m_ltm);
	SVECTOR<LT*> * vec = m_ltm->get_lt_vec();
	INT n = vec->get_last_idx();
	for (INT i = 1; i <= n; i++) {
		LT const* lt1 = vec->get(i);
		if (lt1 == NULL) { continue; }
		add_vertex(LT_uid(lt1));
		for (INT j = i + 1; j <= n; j++) {
			LT const* lt2 = vec->get(j);
			if (lt2 == NULL) { continue; }
			if (is_interf(lt1, lt2)) {
				add_edge(LT_uid(lt1), LT_uid(lt2));
			}
		}
	}
}


void IG::get_neighbor(OUT LIST<LT*> & nis, LT * lt) const
{
	nis.clean();
	//Ensure VERTEX_HASH::find is readonly.
	IG * pthis = const_cast<IG*>(this);
	VERTEX * vex  = pthis->m_vertices.find((OBJTY)LT_uid(lt));
	if (vex == NULL) return;

	EDGE_C * el = VERTEX_in_list(vex);
	while (el != NULL) {
		INT v = VERTEX_id(EDGE_from(EC_edge(el)));
		LT * ni = m_ltm->get_lt(v);
		IS_TRUE0(ni);
		if (!nis.find(ni)) {
			nis.append_tail(ni);
		}
		el = EC_next(el);
	}

	el = VERTEX_out_list(vex);
	while (el != NULL) {
		INT v = VERTEX_id(EDGE_to(EC_edge(el)));
		LT * ni = m_ltm->get_lt(v);
		IS_TRUE0(ni);
		if (!nis.find(ni)) {
			nis.append_tail(ni);
		}
		el = EC_next(el);
	}
}


void IG::dump_vcg(CHAR const* name)
{
	if (name == NULL) {
		name = "graph_local_if.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h, ("%s create failed!!!",name));
	fprintf(h, "graph: {"
			  "title: \"GRAPH\"\n"
			  "shrink:  15\n"
			  "stretch: 27\n"
			  "layout_downfactor: 1\n"
			  "layout_upfactor: 1\n"
			  "layout_nearfactor: 1\n"
			  "layout_splinefactor: 70\n"
			  "spreadlevel: 1\n"
			  "treefactor: 0.500000\n"
			  "node_alignment: center\n"
			  "orientation: top_to_bottom\n"
			  "late_edge_labels: no\n"
			  "display_edge_labels: yes\n"
			  "dirty_edge_labels: no\n"
			  "finetuning: no\n"
			  "nearedges: no\n"
			  "splines: yes\n"
			  "ignoresingles: no\n"
			  "straight_phase: no\n"
			  "priority_phase: no\n"
			  "manhatten_edges: no\n"
			  "smanhatten_edges: no\n"
			  "port_sharing: no\n"
			  "crossingphase2: yes\n"
			  "crossingoptimization: yes\n"
			  "crossingweight: bary\n"
			  "arrow_mode: free\n"
			  "layoutalgorithm: mindepthslow\n"
			  "node.borderwidth: 2\n"
			  "node.color: lightcyan\n"
			  "node.textcolor: black\n"
			  "node.bordercolor: blue\n"
			  "edge.color: darkgreen\n");

	CHAR buf[MAX_OR_SR_NAME_BUF_LEN];
	IS_TRUE0(m_ltm);
	//Print node
	INT c;
	for (VERTEX const* v = m_vertices.get_first(c);
		 v != NULL;  v = m_vertices.get_next(c)) {
		LT * lt = m_ltm->get_lt(VERTEX_id(v));
		sprintf(buf, "LT%d(pr%d):", VERTEX_id(v), LT_prno(lt));
		fprintf(h, "\nnode: { title:\"%d\" label:\"%s\" shape:circle fontname:\"courB\" color:gold}",
				VERTEX_id(v), buf);
	}

	//Print edge
	for (EDGE const* e = m_edges.get_first(c);
		 e != NULL;  e = m_edges.get_next(c)) {
		fprintf(h, "\nedge: { sourcename:\"%d\" targetname:\"%d\" %s}",
				VERTEX_id(EDGE_from(e)),
				VERTEX_id(EDGE_to(e)),
				m_is_direction ? "" : "arrowstyle:none" );
	}
	fprintf(h, "\n}\n");
	fclose(h);
}
//END IG


//
//START LTM
//
LTM::LTM(IR_BB * bb, PRDF * prdf, GLTM * gltm, SMEM_POOL * pool)
{
	m_bb = bb;
	m_pool = pool;
	m_prdf = prdf;
	m_gltm = gltm;
	m_dm = gltm->m_dm;
	m_ru = gltm->m_ru;
	m_ra = gltm->m_ra;
	m_lt_count = 1;
	m_pr2v = m_ra->m_pr2v;
	m_v2pr = m_ra->m_v2pr;
	m_ig.set_ltm(this);
}


/*
Generate spill instruction at 'pos',
pos may be the liveout/livein pos.

Spilling for Def.
	e.g:
		sr1 = ...

	after spilling:

	sr1 = ...
	[spill_loc] = sr1

NOTICE:
	1. Each of spilling code generated are executed unconditionally.
	   Allow to spill unallocated 'lt'.
	2. position in LT should be updated.
*/
IR * LTM::gen_spill(LT * lt, INT pos)
{
	IR * spill_loc = m_ru->build_pr(m_dm->get_simplex_tyid(D_I32));
	IR * ltpr = m_ru->build_pr(m_dm->get_simplex_tyid(D_I32));
	PR_no(ltpr) = LT_prno(lt);
	IR * spill = m_ru->build_store_pr(PR_no(spill_loc),
									  IR_dt(spill_loc), ltpr);
	if (pos == (INT)get_first_pos()) {
		//Prepend store at start pos of BB.
		IR_BB_ir_list(m_bb).append_head(spill);
	} else if (pos == (INT)get_last_pos()) {
		//Append store before the last non-boundary stmt of BB.
		//e.g: reload should insert be call/branch.
		IR * lastir = IR_BB_last_ir(m_bb);
		if (lastir != NULL && lastir->is_call()) {
			IR_BB_ir_list(m_bb).append_tail(spill);
		} else {
			IS_TRUE(!lastir->is_cond_br() && !lastir->is_uncond_br(),
					("How to spill after branch instruction."));
			IR_BB_ir_list(m_bb).append_tail_ex(spill);
		}
	} else {
		IR * marker = get_ir(pos);
		IS_TRUE0(marker);
		IR_BB_ir_list(m_bb).insert_after(spill, marker);
	}
	m_ra->get_rsc()->comp_ir_fmt(spill);
	return spill_loc;
}


/*
Generate spill instruction after 'marker' and swap pr.
Return spill location.
'stpr': indicate the result register to be spilled.
	It is also stmt marker to indicate where the spill instruction insert.
'prno': prno that to be spilled.
'spill_loc': spill location.

e.g: pr1 = pr2 + 3
	=>
	 [spill_loc] = pr2 + 3
	 pr1 = [spill_loc]
*/
IR * LTM::gen_spill_swap(IR * stmt, UINT prno, UINT prdt, IR * spill_loc)
{
	IS_TRUE0(stmt && (stmt->is_stpr() || stmt->is_call()) && prdt != VOID_TY);

	//Generate and insert spilling operation.
	if (spill_loc == NULL) {
		spill_loc = m_ru->build_pr(prdt);
	}
	IR * spill = m_ru->build_store_pr(prno, prdt, spill_loc);
	m_ra->get_rsc()->comp_ir_fmt(spill);
	IR_BB_ir_list(m_bb).insert_after(spill, stmt);

	//Replace orginal PR to spill_loc.
	if (stmt->is_stpr()) {
		STPR_no(stmt) = PR_no(spill_loc);
	} else if (CALL_prno(stmt) == prno) {
		CALL_prno(stmt) = PR_no(spill_loc);
	}
	return spill_loc;
}


/* Generate spill instruction after 'marker'.
Return spill location.
'orgpr': register to be spilled.
'spill_loc': spill location.

e.g: pr1 = pr2 + 3
	=>
	 pr1 = pr2 + 3
	 [spill_loc] = pr1
*/
IR * LTM::gen_spill(UINT prno, UINT dt, IR * marker, IR * spill_loc)
{
	IS_TRUE0(prno > 0 && dt > 0 && marker && marker->is_stmt());
	if (spill_loc == NULL) {
		spill_loc = m_ru->build_pr(dt);
	}
	IR * spill = m_ru->build_store_pr(PR_no(spill_loc), IR_dt(spill_loc),
									  m_ru->build_pr_dedicated(prno, dt));
	IR_BB_ir_list(m_bb).insert_after(spill, marker);
	m_ra->get_rsc()->comp_ir_fmt(spill);
	return spill_loc;
}


/* Generate reload instruction at 'pos',
pos may be the liveout/livein pos.

Reloading for Use.
	e.g:
		... = sr1
	after reloading:
		sr2 = [spill_loc]
		... = sr2
Return the new sr generated.

'ors': if it is NOT NULL, return the ORs generated.

NOTICE:
	Each reloads are executed unconditionally. */
IR * LTM::gen_reload(LT * lt, INT pos, IR * spill_loc)
{
	IR * ltpr = m_ru->build_pr(IR_dt(spill_loc));
	if (LT_is_global(lt)) {
		//Keep original PR unchanged.
		PR_no(ltpr) = LT_prno(lt);
	}
	IR * reload = m_ru->build_store_pr(PR_no(ltpr), IR_dt(ltpr), spill_loc);
	m_ra->get_rsc()->comp_ir_fmt(reload);
	if (pos == (INT)get_first_pos()) {
		//Prepend reload at start pos of BB.
		IR_BB_ir_list(m_bb).append_head(reload);
	} else if (pos == (INT)get_last_pos()) {
		//Append reload before the last non-boundary stmt of BB.
		IR * lastir = IR_BB_last_ir(m_bb);
		if (lastir != NULL && lastir->is_call()) {
			IR_BB_ir_list(m_bb).append_tail(reload);
		} else {
			IS_TRUE(!lastir->is_cond_br() && !lastir->is_uncond_br(),
					("How to spill after branch instruction."));
			if (!lastir->is_return()) {
				//If last ir is return, the reload is dispensable.
				IR_BB_ir_list(m_bb).append_tail_ex(reload);
			}
		}
	} else {
		IR * marker = get_ir(pos);
		IS_TRUE0(marker);
		IR_BB_ir_list(m_bb).insert_before(reload, marker);
	}
	return ltpr;
}


/* Generate reload instruction before 'marker', from 'spill_loc' to 'newpr'.
Reloading for Use.
	e.g:
		... = sr1
	after reloading:
		sr1 = [spill_loc]
		... = sr1
Return the result pr of reloading.
NOTICE:
	Each of reloading code generated are executed unconditionally. */
IR * LTM::gen_reload(IR * newpr, IR * marker, IR * spill_loc)
{
	IS_TRUE0(newpr && newpr->is_pr() &&
			 marker && spill_loc && spill_loc->is_pr());
	IR * reload = m_ru->build_store_pr(PR_no(newpr), IR_dt(newpr),
									   m_ru->dup_ir(spill_loc));
	m_ra->m_rsc.comp_ir_fmt(reload);

	IR_BB * irbb = marker->get_bb();
	IS_TRUE0(irbb);
	IR_BB_ir_list(irbb).insert_before(reload, marker);
	return newpr;
}


/* Generate reload instruction before 'marker', and
swap spill_loc and newpr.
Return the result pr of reloading. This pr need a phy register.
	e.g:
		... = sr1  //the stmt is marker.
	after reloading:
		[spill_loc] = sr1
		... = [spill_loc] //the stmt is marker.

NOTE: Caller is responsible for keeping spill_loc's id unique. */
IR * LTM::gen_reload_swap(IR * orgpr, IR * marker)
{
	IS_TRUE0(marker && marker->is_stmt());
	IS_TRUE0(orgpr && orgpr->is_pr());
	UINT spill_prno = m_ru->build_prno(IR_dt(orgpr));
	IR * reload = m_ru->build_store_pr(spill_prno, IR_dt(orgpr),
									   m_ru->dup_ir(orgpr));
	m_ra->m_rsc.comp_ir_fmt(reload);

	IR_BB * irbb = marker->get_bb();
	IS_TRUE0(irbb);
	IR_BB_ir_list(irbb).insert_before(reload, marker);

	//To speed up compiling, we only change the prno as tricky.
	PR_no(orgpr) = spill_prno;
	return orgpr;
}


/*
Every OR which refering sr must be assigned to same cluster, therefore
the only time to record cluster information is the first meeting with sr.
*/
LT * LTM::new_lt(UINT prno)
{
	IS_TRUE(m_max_lt_len > 0, ("Life time length is overrange."));
	IS_TRUE0(prno >= 1);
	LT * lt = (LT*)xmalloc(sizeof(LT));
	LT_uid(lt) = m_lt_count++;
	LT_range(lt) = m_gltm->get_bs_mgr()->create(0);
	m_prno2lt.set(prno, lt);
	m_lt_vec.set(LT_uid(lt), lt);

	LT_prno(lt) = prno;
	LT_phy(lt) = REG_UNDEF;
	LT_prefer_reg(lt) = REG_UNDEF;
	LT_ltg(lt) = NULL;
	LT_ltg_pos(lt) = 0;
	LT_rg_sz(lt) = 1;
	return lt;
}


/*
Life times which got same physical register with 'sr' must
record the current occurrence.
*/
void LTM::record_phy_reg_occ(LT * lt, UINT pos, IN BITSET & lived_lt)
{
	if (lt->has_allocated()) {
		/*
		Record the occurrence before the lived life-time be
		removed out of 'lived_lt'.
		*/
		for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
			LT * lived = get_lt(i);
			IS_TRUE0(lived);
			if (lt->has_allocated() &&
				LT_phy(lived) == LT_phy(lt)) {
				//Lifetime of lt died at current position.
				LT_range(lived)->bunion(pos);
			}
		}
	}
}


void LTM::process_res_group_part(IR const* ir, UINT pos, OUT BITSET & lived_lt)
{
	IS_TRUE0(ir && ir->is_stpr());
	IS_TRUE0(is_pair(ir));
	LT * lt = map_pr2lt(STPR_no(ir));
	IS_TRUE0(lt);

	bool find = false;
	UINT vreg = m_pr2v->get(STPR_no(const_cast<IR*>(ir)), &find);
	IR * sib;
	if (find) {
		vreg++; //ir is always the lowest part of group.
		sib = m_v2pr->get(vreg);
		if (sib == NULL) {
			//low part is mapped during dex2ir, but high part may be not appear.
			//So its related IR is NULL.
			sib = m_ru->build_pr(m_dm->get_simplex_tyid_ex(D_U32));
		} else {
			sib = m_ru->dup_ir(sib);
			IR_dt(sib) = m_dm->get_simplex_tyid_ex(D_U32);
		}
	} else if (LT_ltg(lt) != NULL) {
		//ir is generated by renaming. lt already has grouped.
		sib = m_ru->build_pr(m_dm->get_simplex_tyid_ex(D_U32));
		LT * pair = LT_ltg(lt)->get(1);
		IS_TRUE0(pair);
		PR_no(sib) = LT_prno(pair);
	} else {
		//ir is generated by renaming.
		sib = m_ru->build_pr(m_dm->get_simplex_tyid_ex(D_U32));
	}

	LT * siblt = process_res_pr(PR_no(sib), pos, lived_lt);
	IS_TRUE0(siblt);
	gen_group(lt, siblt);
}


LT * LTM::process_res_pr(UINT prno, UINT pos, OUT BITSET & lived_lt)
{
	LT * lt = map_pr2lt(prno);
	if (lt == NULL) {
		lt = new_lt(prno);
	}
	if (LT_occ(lt) == NULL) {
		LT_occ(lt) = m_gltm->get_bs_mgr()->create(0);
	}
	LT_occ(lt)->bunion(pos);
	LT_range(lt)->bunion(pos);
	record_phy_reg_occ(lt, pos, lived_lt);
	lived_lt.diff(LT_uid(lt));

	//Phy register definition.
	if (lt->has_allocated()) {
		for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
			LT const* lived = get_lt(i);
			IS_TRUE0(lived);
			if (lived->has_allocated() &&
				lt->is_reg_equal(lived)) {
				lived_lt.diff(LT_uid(lived));
			}
		}//end for
	}
	return lt;
}


IR * LTM::gen_mapped_pr(UINT vid, UINT tyid)
{
	IR * vx = m_ra->m_v2pr->get(vid);
	if (vx == NULL) {
		vx = m_ru->build_pr(tyid);
		m_ra->m_v2pr->set(vid, vx);
		m_pr2v->set(PR_no(vx), vid);
	}
	vx = m_ru->dup_ir(vx);
	IR_dt(vx) = tyid;
	return vx;
}


static bool is_range_call(IR const* ir, DT_MGR const* dm)
{
	if (IR_type(ir) != IR_CALL) { return false; }
	//The first parameter is used to record invoke-kind.
	IR const* p = CALL_param_list(ir);
	if (p == NULL || IR_type(p) != IR_CONST || !p->is_uint(dm)) {
		return false;
	}
	CHAR const* fname = SYM_name(VAR_name(CALL_idinfo(ir)));
	IS_TRUE0(fname);

	if (*fname == '#') {
		//It is an intrinsic call.
		return false;
	}

	INVOKE_KIND ik = (INVOKE_KIND)CONST_int_val(p);
	switch (ik) {
	case INVOKE_UNDEF: IS_TRUE0(0);
	case INVOKE_VIRTUAL_RANGE:
	case INVOKE_DIRECT_RANGE:
	case INVOKE_SUPER_RANGE:
	case INVOKE_INTERFACE_RANGE:
	case INVOKE_STATIC_RANGE:
		return true;
	case INVOKE_VIRTUAL:
	case INVOKE_SUPER:
	case INVOKE_DIRECT:
	case INVOKE_STATIC:
	case INVOKE_INTERFACE:
		return false;
	default: IS_TRUE0(0);
	}
	return false;
}


void LTM::gen_range_call_group(IR const* ir)
{
	IS_TRUE0(is_range_call(ir, m_dm));
	IR * p;
	for (p = CALL_param_list(ir); p != NULL; p = IR_next(p)) {
		if (!p->is_pr()) { continue; }
		break;
	}
	if (p == NULL) { return; }

	LTG * ltg = m_gltm->m_ltgmgr.map_ir2ltg(IR_id(ir));
	if (ltg == NULL) {
		ltg = m_gltm->m_ltgmgr.create();
		m_gltm->m_ltgmgr.set_map_ir2ltg(IR_id(ir), ltg);
	} else {
		ltg->clean();
	}
	ltg->ty = LTG_RANGE_PARAM;

	INT idx = 0;
	for (;p != NULL; p = IR_next(p)) {
		IS_TRUE0(p->is_pr());
		LT * lt = map_pr2lt(PR_no(p));
		IS_TRUE0(lt);
		LT_ltg_pos(lt) = idx;
		LT_ltg(lt) = ltg;
		ltg->set(idx, lt);
		idx++;
	}
}


void LTM::gen_group(LT * first, LT * second)
{
	if (LT_ltg(first) != NULL) {
		IS_TRUE0(LT_ltg(second));
		IS_TRUE0(LT_ltg(first) == LT_ltg(second));
		IS_TRUE0(LT_ltg_pos(first) == 0);
		IS_TRUE0(LT_ltg_pos(second) == 1);
		return;
	}
	IS_TRUE0(LT_ltg(second) == NULL);
	LT_ltg(first) = m_gltm->m_ltgmgr.create();
	LT_ltg(second) = LT_ltg(first);
	LT_ltg(first)->ty = LTG_REG_PAIR;

	LT_ltg_pos(first) = 0;
	LT_ltg(first)->set(0, first);

	LT_ltg_pos(second) = 1;
	LT_ltg(first)->set(1, second);
}


void LTM::process_use_group_part(IR const* ir, UINT pos, OUT BITSET & lived_lt)
{
	IS_TRUE0(ir->is_pr());
	IS_TRUE0(is_pair(ir));
	LT * lt = map_pr2lt(PR_no(ir));
	IS_TRUE0(lt);

	bool find = false;
	UINT vreg = m_pr2v->get(PR_no(const_cast<IR*>(ir)), &find);
	IR * sib;
	if (find) {
		vreg++; //ir is always the lowest part of group.
		//sib = gen_mapped_pr(vreg, m_dm->get_simplex_tyid_ex(D_U32));
		sib = m_v2pr->get(vreg);
		if (sib == NULL) {
			//low part is mapped during dex2ir, but high part may be not appear.
			//So its related IR is NULL.
			sib = m_ru->build_pr(m_dm->get_simplex_tyid_ex(D_U32));
		} else {
			sib = m_ru->dup_ir(sib);
			IR_dt(sib) = m_dm->get_simplex_tyid_ex(D_U32);
		}
	} else if (LT_ltg(lt) != NULL) {
		//ir is generated by renaming. lt already has grouped.
		sib = m_ru->build_pr(m_dm->get_simplex_tyid_ex(D_U32));
		LT * pair = LT_ltg(lt)->get(1);
		IS_TRUE0(pair);
		PR_no(sib) = LT_prno(pair);
	} else {
		//ir is generated by renaming.
		sib = m_ru->build_pr(m_dm->get_simplex_tyid_ex(D_U32));
	}

	LT * siblt = process_use_pr(sib, pos, lived_lt);
	IS_TRUE0(siblt);
	gen_group(lt, siblt);
}


LT * LTM::process_use_pr(IR const* ir, UINT pos, OUT BITSET & lived_lt)
{
	IS_TRUE0(ir->is_pr());
	UINT prno = PR_no(ir);
	LT * lt = map_pr2lt(prno);
	if (lt == NULL) {
		lt = new_lt(prno);
	}
	LT_range(lt)->bunion(pos);
	if (LT_occ(lt) == NULL) {
		LT_occ(lt) = m_gltm->get_bs_mgr()->create(0);
	}
	LT_occ(lt)->bunion(pos);
	record_phy_reg_occ(lt, pos, lived_lt);
	lived_lt.bunion(LT_uid(lt));
	return lt;
}


//Set lt to be register group. If it belong to global lt, also set the
//flag to the related global lt.
void LTM::process_rg(LT * lt)
{
	IS_TRUE0(lt);
	LT_rg_sz(lt) = RG_PAIR_SZ;
	if (LT_is_global(lt)) {
		GLT * g = m_gltm->m_pr2glt.get(LT_prno(lt));
		IS_TRUE0(g);

		//If global lt has allocate, then local part need to
		//make sure it must be paired.
		IS_TRUE(!g->has_allocated() || GLT_rg_sz(g) >= RG_PAIR_SZ,
				("glt has assigned register, but is not paired."));
		GLT_rg_sz(g) = RG_PAIR_SZ;
	}
}


void LTM::process_res(IN IR * ir, INT pos, IN OUT BITSET & lived_lt,
					  bool group_part)
{
	IS_TRUE0(ir->is_stmt());

	//Keep the track of live points at DEF for each lived PR.
	for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
	 	LT * lt = get_lt(i);
		IS_TRUE0(lt);
		LT_range(lt)->bunion(pos);
	}
	switch (IR_type(ir)) {
	case IR_ST: break;
	case IR_STPR:
		if (group_part) {
			if (is_pair(ir)) {
				process_res_group_part(ir, pos, lived_lt);
			}
		} else {
			LT * lt = process_res_pr(STPR_no(ir), pos, lived_lt);
			if (is_pair(ir)) {
				process_rg(lt);
			}
		}
		break;
	case IR_IST: break;
	case IR_CALL:
	case IR_ICALL:
		if (group_part) {
			if (is_pair(ir)) {
				process_res_group_part(ir, pos, lived_lt);
			}
		} else {
			LT * lt = process_res_pr(CALL_prno(ir), pos, lived_lt);
			if (is_pair(ir)) {
				process_rg(lt);
			}
		}
		break;
	case IR_GOTO:
	case IR_SWITCH:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_RETURN:
		break;
	case IR_PHI:
		if (group_part) {
			if (is_pair(ir)) {
				process_res_group_part(ir, pos, lived_lt);
			}
		} else {
			LT * lt = process_res_pr(PHI_prno(ir), pos, lived_lt);
			if (is_pair(ir)) {
				process_rg(lt);
			}
		}
		break;
	default: IS_TRUE0(0);
	}
}


//'group_part': set to true if user is going to scan and handle
//group register info.
void LTM::process_use(IN IR * ir, CIR_ITER & cii, INT pos,
					  IN OUT BITSET & lived_lt, bool group_part)
{
	IS_TRUE0(ir->is_stmt());

	//Keep the track of live points at USE for each live sr
	for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
		LT * lt = get_lt(i);
		IS_TRUE0(lt);
		LT_range(lt)->bunion(pos);
	}

	cii.clean();
	for (IR const* k = ir_iter_rhs_init_c(ir, cii);
		 k != NULL; k = ir_iter_rhs_next_c(cii)) {
		if (!k->is_pr()) { continue; }

		if (group_part) {
			if (is_pair(k)) {
				process_use_group_part(k, pos, lived_lt);
			}
		} else {
			LT * lt = process_use_pr(k, pos, lived_lt);
			if (is_pair(k)) {
				process_rg(lt);
			}
		}
	}

	if (is_range_call(ir, m_dm)) {
		gen_range_call_group(ir);
	}
}


bool LTM::has_pair_res(IR * ir)
{
	IS_TRUE0(ir->is_stmt());
	switch(IR_type(ir)) {
	case IR_STPR:
	case IR_CALL:
	case IR_ICALL:
		if (ir->get_dt_size(m_dm) == PAIR_BYTES) {
			return true;
		}
		break;
	default:;
	}
	return false;
}


IR * LTM::gen_dedicate_pr(UINT phy)
{
	IS_TRUE(0, ("Target Dependent Code"));
	return NULL;
}


//Process the function/region exit BB.
void LTM::process_func_exit_bb(IN OUT LIST<LT*> * liveout_exitbb,
							   IN OUT BITSET & lived_lt,
							   BITSET const& retval_regset,
							   UINT pos)
{
	IS_TRUE0(liveout_exitbb);
	for (INT phy = retval_regset.get_first();
		 phy != -1; phy = retval_regset.get_next(phy)) {
		IR * x = gen_dedicate_pr(phy);
		//TODO: map PR to phy.
		UINT prno = PR_no(x);
		LT * lt = map_pr2lt(prno);
		if (lt == NULL) {
			lt = new_lt(prno);
		}
		lived_lt.bunion(prno);
		LT_range(lt)->bunion(pos);
		liveout_exitbb->append_tail(lt);
		m_prdf->set_pr_to_liveout(m_bb, prno);
	}
}


//Process the live in sr.
//'always_consider_glt': true if build local lt for global lt
//		even if glt has not assigned register.
void LTM::process_livein(OUT BITSET & lived_lt, UINT pos,
						 bool always_consider_glt)
{
	SBITSETC * livein = m_prdf->get_livein(IR_BB_id(m_bb));
	SC<SEG*> * cur;
	for (INT i = livein->get_first(&cur);
		 i != -1; i = livein->get_next(i, &cur)) {
		GLT * glt = m_gltm->map_pr2glt(i);
		IS_TRUE0(glt);
		if (glt->has_allocated() || always_consider_glt) {
			LT * lt = map_pr2lt(i);
			if (lt == NULL) {
				lt = new_lt(i);
			}
			LT_rg_sz(lt) = MAX(LT_rg_sz(lt), GLT_rg_sz(glt));
			if (LT_rg_sz(lt) > GLT_rg_sz(glt)) {
				//Local part need lt must be paired.
				IS_TRUE(!glt->has_allocated(),
						("glt has assigned register, but is not paired."));
				GLT_rg_sz(glt) = LT_rg_sz(lt);
			}
			LT_is_global(lt) = true; //If lt has created, mark it as global.
			lived_lt.bunion(LT_uid(lt));
		}
	}

	//Keep tracking of live points for each lived PR.
	for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
		LT * lt = get_lt(i);
		IS_TRUE0(lt);
		LT_range(lt)->bunion(pos);
	}
}


void LTM::process_liveout(IN OUT BITSET & lived_lt, UINT pos,
						  bool always_consider_glt)
{
	SBITSETC * liveout = m_prdf->get_liveout(IR_BB_id(m_bb));

	SC<SEG*> * cur;
	for (INT i = liveout->get_first(&cur);
		 i != -1; i = liveout->get_next(i, &cur)) {
		GLT * glt = m_gltm->map_pr2glt(i);
		IS_TRUE0(glt);
		if (glt->has_allocated() || always_consider_glt) {
			//If global pr has not assign a register,
			//we say it does not interfere local pr.
			LT * lt = map_pr2lt(i);
			if (lt == NULL) {
				lt = new_lt(i);
			}
			IS_TRUE0(LT_rg_sz(lt) <= GLT_rg_sz(glt));
			LT_rg_sz(lt) = GLT_rg_sz(glt);

			LT_is_global(lt) = true; //If lt has created, mark it as global.
			lived_lt.bunion(LT_uid(lt));
		}
	}

	//Keep tracking of live points for each lived PR.
	for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
		LT * lt = get_lt(i);
		IS_TRUE0(lt);
		LT_range(lt)->bunion(pos);
	}
}


void LTM::build_group(CIR_ITER & cii)
{
	IS_TRUE(m_bb != NULL, ("Basic block is NULL"));
	//Add two point for live in exposed use and live out exposed use.
	m_max_lt_len = IR_BB_ir_num(m_bb) * 2 + 2 + get_first_pos();
	BITSET * lived_lt = m_gltm->get_bs_mgr()->create();
	C<IR*> * ct;
	IR * ir = IR_BB_ir_list(m_bb).get_tail(&ct);
	UINT pos = m_max_lt_len - 2;
	for (; ir != NULL; ir = IR_BB_ir_list(m_bb).get_prev(&ct), pos--) {
		process_res(ir, pos, *lived_lt, true);
		pos--;
		process_use(ir, cii, pos, *lived_lt, true);
	}
	IS_TRUE0(pos == get_first_pos());
	m_gltm->get_bs_mgr()->free(lived_lt);
}


/*
'consider_glt': if true to build local life time for unallocated glt.
'lived_lt': for tmp use.
'liveout_exitbb_lts': record life times which lived out of the function exit BB.
'tmp': for tmp use.

NOTE: If bb is empty, we also need to generate lifetime for live in/out
	global pr.
*/
void LTM::build(bool consider_glt, LIST<LT*> * liveout_exitbb_lts,
				CIR_ITER & cii)
{
	IS_TRUE(m_bb != NULL, ("Basic block is NULL"));
	//Add two point for live in exposed use and live out exposed use.
	m_max_lt_len = IR_BB_ir_num(m_bb) * 2 + 2 + get_first_pos();
	BITSET * lived_lt = m_gltm->get_bs_mgr()->create();
	if (liveout_exitbb_lts != NULL) {
		liveout_exitbb_lts->clean();
	}

	UINT pos = m_max_lt_len - 1;
	process_liveout(*lived_lt, pos, consider_glt);

	#ifdef HAS_COND_DEF
	if (IR_BB_is_exit(m_bb)) {
		//Keep USE point of special register in exit BB.
		//Append lt of each return value of register.
		BITSET * retval_rs = m_tm->get_retval_regset();
		process_func_exit_bb(liveout_exitbb_lts, lived_lt, *retval_rs, pos);
	}
	#endif

	IR * ir;
	C<IR*> * ct;
	for (pos = m_max_lt_len - 2, ir = IR_BB_ir_list(m_bb).get_tail(&ct);
		 ir != NULL; ir = IR_BB_ir_list(m_bb).get_prev(&ct), pos--) {
		m_pos2ir.set(pos, ir);

		//Must-DEF point terminates current life time. But May-DEF is not!
		process_res(ir, pos, *lived_lt, false);
		pos--;
		m_pos2ir.set(pos, ir);
		process_use(ir, cii, pos, *lived_lt, false);
	}

	IS_TRUE0(pos == get_first_pos());
	process_livein(*lived_lt, pos, consider_glt);

	/*
	//Append the FIRST_POS to complete all remainder life times.
	for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
	 	LT * lt = get_lt(i);
		IS_TRUE0(lt);
		LT_pos(lt)->bunion(pos);
	}
	*/

	#ifdef _DEBUG_
	//Life time verification.
	for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		if (!LT_is_global(lt) && !lt->has_allocated()) {
			/*
			For the sake of the weak implementation of Code Expansion Phase,
			do not check the existence of the first def-point for local SR,
			even if it does not have in some case. Because, Code Expansion
			Phase might generate redundant SR reference.
			While lt's SR has been assigned a physical register, the life
			time should be able to represent that register.

			IS_TRUE(LT_pos(lt)->get_first() > get_first_pos(),
					("Local life time has not live in point"));
			*/
			IS_TRUE(LT_range(lt)->get_first() <
					(INT)(m_max_lt_len - 1),
					("Local life time has not live in point"));
		}
	}
	#endif
	//lt in liveout_exitbb_lts may be removed.
	revise_special_lt(liveout_exitbb_lts);
	m_gltm->get_bs_mgr()->free(lived_lt);
}


void LTM::clean()
{
	for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		lt->clean();
	}
	m_pos2ir.clean();
	//Do not clean map between pr from lt.
}


void LTM::revise_lt_case_1(LT * lt)
{
	bool is_def;
	INT f = LT_range(lt)->get_first();
	INT first_concrete_occ = lt->get_forward_occ(f, &is_def, f);
	IS_TRUE(first_concrete_occ > (INT)get_first_pos(),
			("empty life tiem, have no any occ!"));
	BITSET * tmp = m_gltm->m_bs_mgr.create();
	BITSET * bs = LT_range(lt);
	bs->get_subset_in_range(first_concrete_occ, bs->get_last(), *tmp);
	bs->copy(*tmp);
	//Ignore the vector part of the lt, since we only iter lt via bs.
	m_gltm->m_bs_mgr.free(tmp);
}


void LTM::revise_special_lt(LIST<LT*> * lts)
{
	INT firstpos = get_first_pos();
	for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		if (!LT_is_global(lt) &&
			LT_range(lt)->get_first() == firstpos) {
            /*
			Local PR has occurred at LT_FIRST_POS!
			lt might be assigned register already. Apart from that,
			there are followed reasons for the situation at present:
			CASE 1: Local PR that only has USE point. That becasuse Code
				Generation Phase might generate redundant PR reference code,
				or the DEF of local PR is conditional execution.
			*/
			revise_lt_case_1(lt);
		} //end if
	} //end for

	/*
	Remove lt which live-through exit bb that neither have any
	use/def occurrence nor live-in the exit bb.
	So far, we only found this case in exit bb. Any else?
	For the sake of that, we only check exit bb for speeding up compiling.
	*/
	if (IR_BB_is_exit(m_bb) && lts != NULL) {
		for (LT * lt = lts->get_head(); lt != NULL; lt = lts->get_next()) {
			if (!is_livein(LT_prno(lt)) &&
				(LT_occ(lt) == NULL || LT_occ(lt)->get_elem_count() == 0)) {
				remove_lt(lt);
			}
		}
	}
}


void LTM::rename_use(IR * ir, LT * l, IR ** newpr)
{
	LTG * gr = LT_ltg(l);
	switch (IR_type(ir)) {
	case IR_IST:
		rename_use(IST_base(ir), l, newpr);
		rename_use(IST_rhs(ir), l, newpr);
		break;
	case IR_ST:
		rename_use(ST_rhs(ir), l, newpr);
		break;
	case IR_STPR:
		rename_use(STPR_rhs(ir), l, newpr);
		break;
	case IR_ICALL:
		rename_use(ICALL_callee(ir), l, newpr);
	case IR_CALL:
		{
			IR * next;
			for (IR * p = CALL_param_list(ir); p != NULL; p = next) {
				next = IR_next(p);
				if (p->is_pr()) {
					if (PR_no(p) == LT_prno(l)) {
						if (*newpr == NULL) {
							*newpr = m_ru->build_pr(IR_dt(p));
						}
						IR * newp = m_ru->dup_ir(*newpr);
						IR_dt(newp) = IR_dt(p);
						replace(&CALL_param_list(ir), p, newp);
						IR_parent(newp) = ir;
						m_ru->free_ir(p);
					} else if (gr != NULL &&
							   gr->is_member(PR_no(p))) {
						if (*newpr == NULL) {
							*newpr = m_ru->build_pr(IR_dt(p));
						}
					}
				}
			}
		}
		break;
	case IR_TRUEBR:
	case IR_FALSEBR:
		rename_use(BR_det(ir), l, newpr);
		break;
	case IR_SWITCH:
		rename_use(SWITCH_vexp(ir), l, newpr);
		break;
	case IR_RETURN:
		{
			IS_TRUE0(cnt_list(RET_exp(ir)) <= 1);
			IR * rv = RET_exp(ir);
			if (rv != NULL && rv->is_pr()) {
				if (PR_no(rv) == LT_prno(l)) {
					if (*newpr == NULL) {
						*newpr = m_ru->build_pr(IR_dt(rv));
					}
					IR * x = m_ru->dup_ir(*newpr);
					IR_dt(x) = IR_dt(rv);
					RET_exp(ir) = x;
					IR_parent(x) = ir;
					m_ru->free_ir(rv);
				} else if (gr != NULL &&
						   gr->is_member(PR_no(rv))) {
					if (*newpr == NULL) {
						*newpr = m_ru->build_pr(IR_dt(rv));
					}
				}
			}
		}
		break;
	case IR_REGION:
		IS_TRUE0(0);
		break;
	case IR_GOTO: break;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND: //logical and &&
	case IR_LOR: //logical or ||
	case IR_BAND: //inclusive and &
	case IR_BOR: //inclusive or |
	case IR_XOR: //exclusive or
	case IR_LT:
	case IR_LE:
	case IR_GT:
	case IR_GE:
	case IR_EQ: //==
	case IR_NE: //!=
	case IR_ASR:
	case IR_LSR:
	case IR_LSL:
		rename_use(BIN_opnd0(ir), l, newpr);
		rename_use(BIN_opnd1(ir), l, newpr);
		break;
	case IR_BNOT: //bitwise not
	case IR_LNOT: //logical not
	case IR_NEG: //negative
		rename_use(UNA_opnd0(ir), l, newpr);
		break;
	case IR_CVT: //type convertion
		rename_use(CVT_exp(ir), l, newpr);
		break;
	case IR_LDA:
		IS_TRUE0(!LDA_base(ir)->is_pr());
		break;
	case IR_ID:
	case IR_LD:
		break;
	case IR_PR:
		{
			IR * p = IR_parent(ir);
			IS_TRUE0(p); //only process PR here for that has a parent.
			for (INT i = 0; i < IR_MAX_KID_NUM(p); i++) {
				IR * t = p->get_kid(i);
				if (t == NULL || !t->is_pr()) { continue; }
				if (PR_no(t) == LT_prno(l)) {
					if (*newpr == NULL) {
						*newpr = m_ru->build_pr(IR_dt(t));
					}
					IR * x = m_ru->dup_ir(*newpr);
					IR_dt(x) = IR_dt(t);
					p->set_kid(i, x);
					IR_parent(x) = p;
					m_ru->free_ir(t);
				} else if (gr != NULL && gr->is_member(PR_no(t))) {
					if (*newpr == NULL) {
						*newpr = m_ru->build_pr(IR_dt(t));
					}
				}
			}
		}
		break;
	case IR_ARRAY:
		IS_TRUE0(((CARRAY*)ir)->get_dimn() == 1);
		rename_use(ARR_base(ir), l, newpr);
		rename_use(ARR_sub_list(ir), l, newpr);
		break;
	case IR_ILD:
		rename_use(ILD_base(ir), l, newpr);
		break;
	case IR_CONST: break;
	default: IS_TRUE0(0);
	}
}


//Rename lifetime's PR with new PR.
void LTM::rename_lt(LT * l, IR ** newpr)
{
	IS_TRUE0(l && newpr);
	BITSET * occ = LT_occ(l);
	if (occ == NULL) { return; }
	for (INT i = occ->get_first(); i >= 0; i = occ->get_next(i)) {
		IR * ir = m_pos2ir.get(i);
		IS_TRUE0(ir);
		if (l->is_def(i)) {
			switch (IR_type(ir)) {
			case IR_STPR:
				{
					UINT prno = ir->get_prno();
					if (prno == LT_prno(l)) {
						if (*newpr == NULL) {
							//Generate new PR no.
							*newpr = m_ru->build_pr(IR_dt(ir));
						}
						ir->set_prno(PR_no(*newpr));
					} else {
						//l is a member of group.
						IS_TRUE0(LT_ltg(l) != NULL);
						IS_TRUE0(LT_ltg(l)->is_member(prno));
						if (*newpr == NULL) {
							*newpr = m_ru->build_pr(IR_dt(ir));
						}
					}
				}
				break;
			case IR_CALL:
			case IR_ICALL:
					if (CALL_prno(ir) == LT_prno(l)) {
						if (*newpr == NULL) {
							*newpr = m_ru->build_pr(IR_dt(ir));
						}
						CALL_prno(ir) = PR_no(*newpr);
					} else {
						//l is a member of group.
						IS_TRUE0(LT_ltg(l) != NULL);
						IS_TRUE0(LT_ltg(l)->is_member(CALL_prno(ir)));
						if (*newpr == NULL) {
							*newpr = m_ru->build_pr(IR_dt(ir));
						}
					}
				break;
			default: IS_TRUE(0, ("no def to PR"));
			}
		} else {
			rename_use(ir, l, newpr);
		}
	}
	if (*newpr != NULL) {
		LT_prno(l) = PR_no(*newpr);
		m_prno2lt.aset(LT_prno(l), l);
	}
}


//If local PR is same with global PR, rename local PR.
//If there are multiple local lifetime corresponded to same PR, rename them.
void LTM::rename(TMAP<UINT, LT*> & prno2lt, BITSET & met)
{
	prno2lt.clean(); //for tmp use
	for (INT i = 1; i <= m_lt_vec.get_last_idx(); i++) {
		LT * l = m_lt_vec.get(i);
		if (l == NULL) { continue; }
		if (!LT_is_global(l)) {
			if (m_gltm->m_pr2glt.get(LT_prno(l)) != NULL) {
				//local lifetime has same PR with global lifetime.
				IR * newpr = NULL;
				rename_lt(l, &newpr);
				if (newpr != NULL) {
					met.bunion(PR_no(newpr));
				}
				continue;
			}
			if (met.is_contain(LT_prno(l))) {
				//local lifetime has same PR with local lifetime in other bb.
				IR * newpr = NULL;
				rename_lt(l, &newpr);
				if (newpr != NULL) {
					met.bunion(PR_no(newpr));
				}
				continue;
			}
		}

		LT * prior = prno2lt.get(LT_prno(l));
		if (prior == NULL) {
			prno2lt.set(LT_prno(l), l);
			met.bunion(LT_prno(l));
		} else if (prior != l) {
			//Do not rename global lifetime in LTM.
			IR * newpr = NULL;
			if (LT_is_global(l) && !LT_is_global(prior)) {
				prno2lt.aset(LT_prno(l), l);
				rename_lt(prior, &newpr);
			} else {
				IS_TRUE(!LT_is_global(l) || !LT_is_global(prior),
						("glt wit same pr"));
				rename_lt(l, &newpr);
			}
			if (newpr != NULL) {
				met.bunion(PR_no(newpr));
			}
			m_ru->free_ir(newpr);
		}
	}
}


void LTM::remove_lt(LT * lt)
{
	m_lt_vec.set(LT_uid(lt), NULL);
	m_prno2lt.aset(LT_prno(lt), NULL);
	m_gltm->get_bs_mgr()->free(LT_range(lt));
	m_gltm->get_bs_mgr()->free(LT_occ(lt));
	LT_range(lt) = NULL;
	LT_occ(lt) = NULL;
	m_ig.remove_vertex(LT_uid(lt));
}


void LTM::dump_allocated(FILE * h, BITSET & visited)
{
	//Print local life time.
	//Position start from 0, end at maxlen-1.
	fprintf(h, "\nPOS:");
	for (INT i = 1; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		visited.bunion(LT_uid(lt));
		if (LT_is_global(lt)) {
			fprintf(h, "\n GLT(%3d):", LT_uid(lt));
		} else {
			fprintf(h, "\n  LT(%3d):", LT_uid(lt));
		}

		//Collects position info.
		CHAR * pos_marker = (CHAR*)malloc(m_max_lt_len);
		memset(pos_marker, 0, sizeof(CHAR) * m_max_lt_len);
		for (INT j = LT_range(lt)->get_first();
			 j >= 0; j = LT_range(lt)->get_next(j)) {
			IS_TRUE0(j < (INT)m_max_lt_len);
			pos_marker[j] = 1;
		}

		//Dump life time.
		for (UINT k = 0; k < m_max_lt_len; k++) {
			if (pos_marker[k] == 0) {
				fprintf(h, "   ,");
			} else {
				fprintf(h, "%3d,", k);
			}
		}
		free(pos_marker);

		//Dump prno, phy.
		fprintf(h, "	[pr%d]", LT_prno(lt));
		if (lt->has_allocated()) {
			fprintf(h, "(");
			for (INT z = 0; z < LT_rg_sz(lt); z++) {
				fprintf(h, "v%d", LT_phy(lt) + z);
				if (z != (LT_rg_sz(lt) - 1)) {
					fprintf(h, ",");
				}
			}
			fprintf(h, ")");
		} else {
			fprintf(h, "(");
			for (INT z = 0; z < LT_rg_sz(lt); z++) {
				fprintf(h, "-");
				if (z != (LT_rg_sz(lt) - 1)) {
					fprintf(h, ",");
				}
			}
			fprintf(h, ")");
		}

		if (LT_ltg(lt) != NULL) {
			fprintf(h, ",");
			LTG * ltg = LT_ltg(lt);
			switch (ltg->ty) {
			case LTG_RANGE_PARAM:
				fprintf(h, "rg"); break;
			case LTG_REG_PAIR:
				fprintf(h, "pg"); break;
			default: IS_TRUE0(0);
			}

			//Dump ltg's prno, ltid, phy.
			fprintf(h, "<");
			for (INT i = 0; i <= ltg->get_last_idx(); i++) {
				LT * l = ltg->get(i);
				IS_TRUE0(l);
				fprintf(h, "pr%d(lt%d)", LT_prno(l), LT_uid(l));
				if (l->has_allocated()) {
					INT sz = LT_rg_sz(l);

					fprintf(h, "(");
					for (INT z = 0; z < sz; z++) {
						fprintf(h, "v%d", LT_phy(l) + z);
						if (z != (LT_rg_sz(l) - 1)) {
							fprintf(h, ",");
						}
					}
					fprintf(h, ")");
				}
				fprintf(h, " ");
			}
			fprintf(h, ">");
			//
		}
	}

	fprintf(h, "\nDESC:");
	for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		if (LT_is_global(lt)) {
			fprintf(h, "\n GLT(%3d):", LT_uid(lt));
		} else {
			fprintf(h, "\n  LT(%3d):", LT_uid(lt));
		}

		//Dump occurrences.
		INT start = LT_range(lt)->get_first();
		INT end = LT_range(lt)->get_last();
		for (INT j = get_first_pos(); j < (INT)m_max_lt_len; j++) {
			if (LT_occ(lt) != NULL && LT_occ(lt)->is_contain(j)) {
				if (lt->is_def(j)) {
					fprintf(h, "DEF,");
				} else {
					fprintf(h, "USE,");
				}
			} else {
				if (j >= start && j <= end) {
					fprintf(h, "   ,");
				} else {
					fprintf(h, "    ");
				}
			}
		}

		//Dump prno, phy.
		fprintf(h, "	[pr%d]", LT_prno(lt));
		if (lt->has_allocated()) {
			fprintf(h, "(");
			for (INT z = 0; z < LT_rg_sz(lt); z++) {
				fprintf(h, "v%d", LT_phy(lt) + z);
				if (z != LT_rg_sz(lt) - 1) {
					fprintf(h, ",");
				}
			}
			fprintf(h, ")");
		} else {
			fprintf(h, "(");
			for (INT z = 0; z < LT_rg_sz(lt); z++) {
				fprintf(h, "-");
				if (z != (LT_rg_sz(lt) - 1)) {
					fprintf(h, ",");
				}
			}
			fprintf(h, ")");
		}

		if (LT_ltg(lt) != NULL) {
			fprintf(h, ",");
			LTG * ltg = LT_ltg(lt);
			switch (ltg->ty) {
			case LTG_RANGE_PARAM:
				fprintf(h, "rg"); break;
			case LTG_REG_PAIR:
				fprintf(h, "pg"); break;
			default: IS_TRUE0(0);
			}

			//Dump ltg's prno, ltid, phy.
			fprintf(h, "<");
			for (INT i = 0; i <= ltg->get_last_idx(); i++) {
				LT * l = ltg->get(i);
				IS_TRUE0(l);
				fprintf(h, "pr%d(lt%d)", LT_prno(l), LT_uid(l));
				if (l->has_allocated()) {
					INT sz = LT_rg_sz(l);

					fprintf(h, "(");
					for (INT z = 0; z < sz; z++) {
						fprintf(h, "v%d", LT_phy(l) + z);
						if (z != (LT_rg_sz(l) - 1)) {
							fprintf(h, ",");
						}
					}
					fprintf(h, ")");
				}
				fprintf(h, " ");
			}
			fprintf(h, ">");
			//
		}
	}
}


void LTM::dump_unallocated(FILE * h, BITSET & visited)
{
	//Print unallocated life time.
	bool doit = false;
	for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		if (!visited.is_contain(LT_uid(lt))) {
			doit = true;
			break;
		}
	}
	if (!doit) { return; }

	fprintf(h, "\nUnallocated:");
	for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		if (visited.is_contain(LT_uid(lt))) { continue; }
		fprintf(h, "\nPOS:");
		fprintf(h, "\n  LT(%3d):", LT_uid(lt));

		//Collects position info.
		CHAR * pos_marker = (CHAR*)malloc(m_max_lt_len);
		memset(pos_marker, 0, sizeof(CHAR) * m_max_lt_len);
		for (INT j = LT_range(lt)->get_first();
			 j >= 0; j = LT_range(lt)->get_next(j)) {
			IS_TRUE0(j < (INT)m_max_lt_len);
			pos_marker[j] = 1;
		}

		//Position start from 0, end at maxlen-1.
		for (UINT k = 0; k < m_max_lt_len; k++) {
			if (pos_marker[k] == 0) {
				fprintf(h, "   ,");
			} else {
				fprintf(h, "%3d,", k);
			}
		}
		free(pos_marker);
		fprintf(h, "	[pr%d]", LT_prno(lt));

		//Collects position info
		if (LT_occ(lt) == NULL) { continue; }
		INT last_idx = LT_occ(lt)->get_last();
		IS_TRUE(last_idx == -1 || last_idx < (INT)m_max_lt_len,
				("Depiction of life time long than the finial position"));

		fprintf(h, "\nDESC:");
		INT start = LT_occ(lt)->get_first();
		INT end = LT_occ(lt)->get_last();
		for (INT j = get_first_pos(); j < (INT)m_max_lt_len; j++) {
			if (LT_occ(lt) != NULL && LT_occ(lt)->is_contain(j)) {
				if (lt->is_def(j)) {
					fprintf(h, "DEF,");
				} else {
					fprintf(h, "USE,");
				}
			} else {
				if (j >= start && j <= end) {
					fprintf(h, "   ,");
				} else {
					fprintf(h, "	");
				}
			}
		}
		fprintf(h, "	[pr%d]", LT_prno(lt));
	} //end for
}


void LTM::dump()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n--- BB%d Local Life Time ---", m_bb->id);

	//Print live-in PR.
	fprintf(g_tfile, "\nlivein:");
	SBITSETC * livein = m_prdf->get_livein(IR_BB_id(m_bb));
	SC<SEG*> * cur;
	for (INT i = livein->get_first(&cur);
		 i != -1; i = livein->get_next(i, &cur)) {
		fprintf(g_tfile, "pr%d, ", i);
	}

	//Print live-out PR.
	fprintf(g_tfile, "\nliveout:");
	SBITSETC * liveout = m_prdf->get_liveout(IR_BB_id(m_bb));
	for (INT i = liveout->get_first(&cur);
		 i != -1; i = liveout->get_next(i, &cur)) {
		fprintf(g_tfile, "pr%d, ", i);
	}

	//Print local life times base info.
	UINT c = 0;
	for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
		LT * lt = m_lt_vec.get(i);
		if (lt == NULL) { continue; }
		c = MAX(c, LT_uid(lt));
		//fprintf(g_tfile, "\n\tLT(%d):", LT_uid(lt));
		//fprintf(g_tfile, "[pr%d]:", LT_prno(lt));
	}

	BITSET visited(c + 1);
	dump_allocated(g_tfile, visited);
	//dump_unallocated(g_tfile, visited);
	fflush(g_tfile);
}
//END LTM


//
//START BBRA
//
BBRA::BBRA(IR_BB * bb, RA * ra)
{
	IS_TRUE0(ra && bb);
	m_bb = bb;
	m_ra = ra;
	m_rsc = ra->get_rsc();
	m_gltm = ra->get_gltm();
	m_ru = ra->m_ru;
	IS_TRUE0(m_gltm);
	m_ltm = m_gltm->map_bb2ltm(bb);
	IS_TRUE0(m_ltm);
	m_ig = m_ltm->get_ig();
	m_tmp_lts = NULL;
	m_tmp_lts2 = NULL;
	m_tmp_uints = NULL;
}


/*
Compute priority list and sort life times with descending order of priorities.

'lts': list of LT.
'prios': list of LT, which elements are sorted in descending order of priority.

We use some heuristics factors to evaluate the
priorities of each of life times:
	1. Life time in critical path will be put in higher priority.
	2. Life time whose symbol register referenced in
		high density will have higher priority.
	3. Life time whose usable registers are fewer, the priority is higher.

	TO BE ESTIMATED:
		Longer life time has higher priority.
*/
void BBRA::build_prio_list(IN LIST<LT*> const& lts, OUT LIST<LT*> & prios)
{
	C<LT*> * ct;
	for (LT * lt = lts.get_head(&ct); lt != NULL; lt = lts.get_next(&ct)) {
		LT_priority(lt) = comp_prio(lt);

		//Search for appropriate position to place.
		LT * t;
		C<LT*> * ct2;
		for (t = prios.get_head(&ct2); t != NULL; t = prios.get_next(&ct2)) {
			if (LT_priority(t) < LT_priority(lt)) {
				break;
			}
		}
		if (t == NULL) {
			prios.append_tail(lt);
		} else {
			prios.insert_before(lt, ct2);
		}
	}
}


float BBRA::comp_prio(LT const* lt)
{
	IS_TRUE0(!LT_is_global(lt));
	if (LT_range(lt) == NULL) return 0.0;
	BITSET const* occ = LT_occ(lt);
	IS_TRUE0(occ);
	float prio = (float)occ->get_elem_count();
	BITSET const* usable = LT_usable(lt);
	if (usable != NULL) {
		if (m_rsc->get_4() == usable) {
			prio += 1000;
		} else if (m_rsc->get_8() == usable) {
			prio += 800;
		} else if (m_rsc->get_16() == usable) {
			prio += 400;
		}
	}

	if (LT_rg_sz(lt) > 1) {
		prio *= (float)(LT_rg_sz(lt) * 2);
	}

	/* occ in truebr/falsebr/switch/goto should has highest priority
	e.g: br pr1,pr2
	If there is not avaiable register for parameters, we
	need to spill other LT. Here we can not spill live through
	global lt, because a reload need to be inserted at the end
	of BB. That violates the constrain of BB.
	In order to avoid the rarely situation, lt with branch should get a
	highest priority. */
	if (lt->has_branch(m_ltm)) {
		prio *= 1000;
	}
	return prio;
}


//'prio' list should be sorted in descending order.
void BBRA::alloc_prio_list(OUT LIST<LT*> & prios, LIST<UINT> & nis)
{
	C<LT*> * ct, * next_ct;
	for (prios.get_head(&ct), next_ct = ct; ct != NULL; ct = next_ct) {
		LT * lt = C_val(ct);
		prios.get_next(&next_ct);
		IS_TRUE0(!lt->has_allocated());
		if (!assign_reg(lt, nis)) {
			continue;
		}
		IS_TRUE0(lt->has_allocated());
		m_ra->update_lt_maxreg(lt);
		prios.remove(ct);
	}
}


/* Return true if allocation was successful, otherwise return false.
When register assigned to 'g', it must be deducted from
the usable_register_set of all its neighbors.

'unusable': for tmp use.
'ig': interference graph. */
bool BBRA::assign_reg(LT * l, LIST<UINT> & nis)
{
	IS_TRUE0(!LT_is_global(l));
	IS_TRUE0(!l->has_allocated());
	BITSET const* usable = LT_usable(l);
	if (usable == NULL) { return false; }
	BITSET * unusable = m_gltm->get_bs_mgr()->create();

	//Deduct the used register by neighbors.
	nis.clean();
	bool on = m_ig->get_neighbor_list(nis, LT_uid(l));
	IS_TRUE0(on);
	UINT n = nis.get_elem_count();
	for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
		LT const* ni = m_ltm->get_lt(i);
		IS_TRUE0(ni);
		if (!ni->has_allocated()) { continue; }
		unusable->bunion(LT_phy(ni));
		if (LT_rg_sz(ni) != 1) {
			IS_TRUE0(LT_rg_sz(ni) == RG_PAIR_SZ);
			unusable->bunion(LT_phy(ni) + 1);
		}
	}

	//Select preference register.
	UINT pref = LT_prefer_reg(l);
	if (pref != REG_UNDEF && !unusable->is_contain(pref)) {
		if (LT_rg_sz(l) > 1) {
			IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
			if (!unusable->is_contain(pref + 1)) {
				LT_phy(l) = pref;
				m_gltm->get_bs_mgr()->free(unusable);
				IS_TRUE0(usable->is_contain(pref));
				IS_TRUE0(!m_ra->is_cross_param(pref, LT_rg_sz(l)));
				return true;
			}
		} else {
			LT_phy(l) = pref;
			m_gltm->get_bs_mgr()->free(unusable);
			IS_TRUE0(usable->is_contain(pref));
			return true;
		}
	}

	//Avoid allocating the registers which
	//are neighbors the most preferable or anticipated.
	n = nis.get_elem_count();
	for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
		LT const* ni = m_ltm->get_lt(i);
		IS_TRUE0(ni);
		if (ni->has_allocated()) { continue; }
		//Avoid select the reg which ni preferable.
		UINT r = LT_prefer_reg(ni);
		if (r != REG_UNDEF) {
			unusable->bunion(r);
			if (LT_rg_sz(ni) != 1) {
				IS_TRUE0(LT_rg_sz(ni) == RG_PAIR_SZ);
				unusable->bunion(r + 1);
			}
		}
	}

	/*
	UINT reg = REG_UNDEF;
	BITSET const* anti = m_ltm->get_anti_regs(l, false);
	if (anti != NULL) {
		//Allocate register that 'g' anticipated.
		for (INT r = anti->get_first(); r >= 0; r = anti->get_next(r)) {
			if (usable->is_contain(r)) {
				LT_phy(l) = r;
				return true;
			}
		}
	}
	*/

	//Fine, you can allocate any register for it except the unusables.
	//Preferred the minmum.
	INT m = usable->get_last();
	bool succ = false;
	for (INT i = FIRST_PHY_REG; i <= m; i++) {
		if (!unusable->is_contain(i)) {
			if (LT_rg_sz(l) > 1) {
				IS_TRUE(LT_rg_sz(l) == RG_PAIR_SZ, ("to support more size"));
				if (!unusable->is_contain(i + 1) &&
					!m_ra->is_cross_param(i, LT_rg_sz(l))) {
					LT_phy(l) = i;
					succ = true;
					break;
				}
			} else {
				LT_phy(l) = i;
				succ = true;
				break;
			}
		}
	}
	m_gltm->get_bs_mgr()->free(unusable);
	IS_TRUE0(!succ || usable->is_contain(LT_phy(l)));
	return succ;
}


bool BBRA::can_be_split(LT const* lt) const
{
	UINT c = LT_range(lt)->get_elem_count();
	if (c == 1) { return false; }
	if (c == 2) {
		INT pos1 = LT_occ(lt)->get_first();
		INT pos2 = LT_occ(lt)->get_next(pos1);
		IS_TRUE0(pos1 != -1 && pos2 != -1);
		IS_TRUE(LT_occ(lt)->get_next(pos2) == -1, ("More than 2"));
		if (pos2 == pos1 + 1) {
			//We canot benefit from spilling this lifetime.
			return false;
		}
	}
	return true;
}


//Return true if there is hole in lifetime of 'lt',
//and 'startpos', 'endpos' represented the start and end position of hole.
bool BBRA::get_max_hole(OUT INT * startpos, OUT INT * endpos, LT const* lt)
{
	*startpos = 0;
	*endpos = 0;

	INT maxlen = 0;
	INT start = 0;
	INT next_i = -1;
	INT first = m_ltm->get_first_pos();
	INT last = m_ltm->get_last_pos();
	INT range_last = LT_range(lt)->get_last();
	for (INT i = LT_range(lt)->get_first(), start = i;
		 i >= 0 && i <= range_last; i = next_i) {

		next_i = i + 1;
		if (!LT_range(lt)->is_contain(i)) {
			next_i = LT_range(lt)->get_next(i); //'next_i' may be -1
			start = next_i;
			continue;
		}

		if ((i == first && LT_range(lt)->is_contain(first)) || //life time live in.
			(i == last && LT_range(lt)->is_contain(last)) || //life time live out.
			(LT_occ(lt) != NULL && LT_occ(lt)->is_contain(i))) {
			if (i > start) {
				if (maxlen < (i - start)) {
					maxlen = i - start;
					*startpos = start;
					*endpos = i;
				}
			}
			start = i;
		}
	}

	if (*startpos != *endpos) {
		return true;
	}
	return false;
}


/*
Calculate the number of lifetimes which only living in the 'hole'.
Only compute the longest hole for each of life times.
*/
void BBRA::compute_lt_residein_hole(OUT LIST<LT*> & reside_in, LT const* lt)
{
	INT hole_startpos, hole_endpos;
	get_max_hole(&hole_startpos, &hole_endpos, lt);

	BITSET * hole = m_gltm->get_bs_mgr()->create();
	LT_range(lt)->get_subset_in_range(hole_startpos, hole_endpos, *hole);

	SVECTOR<LT*> * ltvec = m_ltm->get_lt_vec();
	for (INT i = 1; i <= ltvec->get_last_idx(); i++) {
		LT * l = ltvec->get(i);
		if (l == NULL) { continue; }
		if (l == lt) { continue; }
		if (hole->is_contained_in_range(LT_range(l)->get_first(),
										LT_range(l)->get_last(),
										true)) {
			reside_in.append_tail(l);
		}
	}
	m_gltm->get_bs_mgr()->free(hole);
}


//Return true if l is global live through lifetime.
//There is not any occ in BB for the lifetime.
bool BBRA::is_live_through(LT const* l) const
{
	if (!LT_is_global(l)) { return false; }
	if (LT_occ(l) == NULL || LT_occ(l)->is_empty()) {
		return true;
	}
	return false;
}


//Return true if the phy of cand satisified lt's constrain.
bool BBRA::satisfied_constrain(LT * lt, LT * cand)
{
	if (!cand->has_allocated()) { return false; }
	IS_TRUE0(LT_usable(lt));
	return LT_usable(lt)->is_contain(LT_phy(cand));
}


/* Determining which one should be spilled.
	Computing spill cost:
	The quotient is bigger, the spill cost is less,
	is also the one we expect to spill.
	<cost = number of uncolored neighbors of 'ni' / 'ni's priority>

Return the spilling candidate life time selected.
And 'has_hole' will be set to TRUE, if we could find a
lifetime hole which contained several
shorter lifetimes in it. */
LT * BBRA::comp_split_cand(LT * lt, bool & has_hole, LIST<LT*> * tmp,
						   LIST<LT*> * tmp2)
{
	IS_TRUE0(tmp && tmp2);
	LT * best = NULL, * better = NULL;
	has_hole = false;

	//Calculates the benefits if 'ni' is deal with as 'action' descriptive.
	LIST<LT*> * ni_list = tmp; //neighbor list of 'lt'

	//Inspecting all of neighbors of 'lt' even itself.
	m_ig->get_neighbor(*ni_list, lt);
	//if (try_self) {
	//	ni_list.append_head(lt); //May be we should spill 'lt' itself as well.
	//}

	double * ni_cost =
		(double*)ALLOCA(ni_list->get_elem_count() * sizeof(double));

	//1. Computing the cost of each of neighbours in terms of life time
	//	 priority and the benefit when we split the
	//   life time.
	INT i = 0;
	LIST<LT*> * ni_ni_list = tmp2; //neighbor list of 'ni'
	for (LT * ni = ni_list->get_head();
		 ni != NULL; ni = ni_list->get_next(), i++) {
		if (!satisfied_constrain(lt, ni)) { continue; }
		if (is_live_through(ni)) {
			has_hole = true;
			return ni;
		}
		ni_cost[i] = 0.0;
		//if (!can_be_split(ni)) { continue; }

		m_ig->get_neighbor(*ni_ni_list, ni);
		UINT uncolored_nini = 0;
		for (LT * nini = ni_ni_list->get_head();
			 nini != NULL; nini = ni_ni_list->get_next()) {
			if (!nini->has_allocated()) {
				uncolored_nini++;
			}
		}

		double c = (uncolored_nini + EPSILON) / (LT_priority(ni) + EPSILON);
		ni_cost[i] = c;
	}

	double const* nic = ni_cost;

	//Selecting policy.
	//2. Choosing the best one as split-candidate that
	//	 constains the most life times
	//   which unallocated register till now.
	INT most = 0, most_idx = -1;
	INT minor = 0, minor_idx = -1;
	i = 0;
	LIST<LT*> * residein_lts = tmp2;
	for (LT * ni = ni_list->get_head();
		 ni != NULL; ni = ni_list->get_next(), i++) {
		if (!satisfied_constrain(lt, ni)) { continue; }
		if (!can_be_split(ni)) { continue; }

		residein_lts->clean();
		compute_lt_residein_hole(*residein_lts, ni);

		//Find lifetimes which did not assign register yet.
		INT lt_num = 0;
		for (LT * x = residein_lts->get_head();
			 x != NULL; x = residein_lts->get_next()) {
			if (!x->has_allocated()) {
				lt_num++;
			}
		}

		if (most < lt_num ||
			(most_idx != -1 && most == lt_num &&
			 nic[most_idx] < nic[i])) {
			most_idx = i;
			best = ni;
			most = lt_num;
		}

		//Find the inferior split candidate.
		//We say it is the split-cand if the number of life
		//times which residing in its hole to be the most.
		if (minor < (INT)residein_lts->get_elem_count() ||
			(minor_idx != -1 &&
			 minor == (INT)residein_lts->get_elem_count() &&
			 nic[minor_idx] < nic[i])) {
			minor_idx = i;
			better = ni;
			minor = residein_lts->get_elem_count();
		}
	}
	if (best == NULL && better != NULL) { //The alternative choose.
		best = better;
	}
	if (best != NULL) {
		//Simply select the lift time with largest hole.
		has_hole = true;
		return best;
	}

	//3. If the first step failed, we choose candidate in terms of
	//	 life time priority and the benefit when we split the candidate.
	i = 0;

	//We can obtain some benefits via the adjustment of 'action'.
	double maximal_cost = 0.0;
	ni_ni_list = tmp2; //neighbor list of 'ni'
	for (LT * ni = ni_list->get_head(); ni != NULL; ni = ni_list->get_next(), i++) {
		if (!can_be_split(ni)) { continue; }
		m_ig->get_neighbor(*ni_ni_list, ni);
		UINT uncolored_nini = 0;
		for (LT * nini = ni_ni_list->get_head();
			 nini != NULL; nini = ni_ni_list->get_next()) {
			if (!nini->has_allocated()) {
				uncolored_nini++;
			}
		}

		double c = nic[i];
		if (best == NULL || maximal_cost < c) {
			if (ni != lt) {
				//Avoid the followed case:
				//same start pos, or same end pos.
				if ((LT_range(ni)->get_first() == LT_range(lt)->get_first()) ||
					(LT_range(ni)->get_last() == LT_range(lt)->get_last())) {
					continue;
				}
			}
			maximal_cost = c;
			best = ni;
		}
	}
	IS_TRUE(best != NULL, ("Not any spill candidate."));
	return best;
}


/*
Return true if there is hole in lifetime of 'owner' that
'inner' can be lived in, and 'startpos','endpos' represented the hole.
*/
bool BBRA::find_hole(OUT INT & startpos, OUT INT & endpos,
					 LT const* owner, LT const* inner)
{
	INT start = 0;
	startpos = 0;
	endpos = 0;

	BITSET * owner_range = LT_range(owner);
	INT lastbit = owner_range->get_last();
	INT firstpos = m_ltm->get_first_pos();
	INT lastpos = m_ltm->get_last_pos();
	IR const* lastir = IR_BB_last_ir(m_bb);
	if (lastir != NULL &&
		(lastir->is_cond_br() ||
		 lastir->is_multicond_br() ||
		 lastir->is_uncond_br())) {
		lastpos -= 2;
	}

	BITSET * owner_occ = LT_occ(owner);
	INT next_i;
	for (INT i = owner_range->get_first(), start = i;
		 i >= 0 && i <= lastbit; i = next_i) {
		next_i = i + 1;

		//Current pos is not a point of life time.
		if (!owner_range->is_contain(i)) {
			next_i = owner_range->get_next(i); //'next_i' may be -1
			start = next_i;
			continue;
		}

		//owner is livein lt.
		if ((i == firstpos && owner_range->is_contain(firstpos)) ||
			((i == (INT)lastpos) && owner_range->is_contain(lastpos)) ||
			(owner_occ != NULL && owner_occ->is_contain(i))) {

			if (i > start) {
				if (LT_range(inner)->is_contained_in_range(start, i, true)) {
					startpos = start;
					endpos = i;
					return true;
				}
			}
			start = i;
		}//end if
	}//end for
	return false;
}


/* Given two position within lifetime 'lt', tring to choose the most
appropriate split point and inserting the spill/reload code at them.
'is_pos1_spill': if true indicate that a spilling is needed at pos1,
	otherwise to insert a reload.
'is_pos2_spill': if true indicate that a spilling is needed at pos2,
	otherwise to insert a reload.
'lt': split candidate, may be global and local lifetime.

e.g: Given pos1, pos2, both of them are USE.
	We need to find the DEF to insert the spill code. And choosing
	the best USE between 'pos1' and 'pos2' to insert reload code.
	While both positions are useless, we do not insert any code in
	those positions, and set 'pos1' and 'pos2' to -1. */
void BBRA::select_reasonable_split_pos(OUT INT & pos1, OUT INT & pos2,
									   OUT bool & is_pos1_spill,
									   OUT bool & is_pos2_spill,
									   LT * lt)
{
	IS_TRUE(lt && pos1 >= 0 && pos2 > 0 && pos1 < pos2, ("Illegal hole"));
	INT p1 = pos1, p2 = pos2;
	bool is_p1_def = false, is_p2_def = false;

	INT firstpos = m_ltm->get_first_pos();
	INT lastpos = m_ltm->get_last_pos();
	BITSET * occ = LT_occ(lt);
	if ((pos1 == firstpos && pos2 == lastpos) || occ == NULL) {
		//live through lt.
		is_pos1_spill = true; //need spill
		is_pos2_spill = false; //need reload
		return;
	}

	//Compute the status the pos shows.
	bool proc = true;
	while (proc) {
		if (p1 == firstpos) {
			is_p1_def = true;
			break;
		} else {
			if (!occ->is_contain(p1)) {
				/* CASE: image.c:copy_rdopt_data:BB1
				 	live in and out gsr: GSR277
				 	first pos:0
				 	last pos:83
				 	LT(33): 0,,...,28,29,30...83
				There is a invalid region in between 0~28,and in actually,
				position 28 has not any PI corresponding to!
				Since GSR238(a7) also allocate the same register as
				GSR277 and it has a def at position 28.
				LTM handled the situation conservatively.

				But I thought this is a GRA bug:
					See BB5 for more details:
						SR284, SR282, SR283 <- cmp GSR1294(a6) (0)
						GSR277(a7)[A1] <- SR283[P1] SR278
						GSR277(a7)[A1] <- SR282[P1] SR280
						br L22

				GSR277 was cond-defined and should be considered as an USE,
				but data-flow solver cannot distingwish that because of
				the cond-def. */
				p1--;
				pos1 = p1;
				continue;
			}
			if (lt->is_def(p1)) {
				is_p1_def = true;
			} else {
				is_p1_def = false;
			}
			break;
		}
	}

	proc = true;
	while (proc) {
		if (p2 == (INT)lastpos) {
			is_p2_def = false;
			break;
		} else {
			if (!occ->is_contain(p2)) {
				p2++;
				pos2 = p2;
				continue;
			}
			if (lt->is_def(p2)) {
				IR * ir = m_ltm->get_ir(p2);
				IS_TRUE0(ir);
				if (IR_type(ir) == IR_SELECT) {
					/* CASE: 20020402-3.c:blockvector_for_pc_sect():BB10
						gsr275(a3) lived in and lived out.
						first pos:0
						last pos:12, cond def

						sr268[A1] :- lw_m sr97(p0)[P1] gsr263(a4)[A1] (0x0)
						sr266(d10)[D2] :- lw_m sr97(p0)[P1] sr268[A1] (0x8)
						sr267(d2)[D1] :- lw_m sr97(p0)[P1] sr268[A1] (0xc)
						...
						gsr275(a3)[A1] gsr271(p7)[P1] gsr272(p6)[P1] :- sgtu_m sr270(p1)[P1] ...

					The spliting candidate is GSR275.
					Although the operator at postition p2 is a DEF,
					but it was a conditional DEF! So we regard position p2 as
					an USE in order to insert a reloading before the cond DEF and
					add a spilling followed the FIRST position to supply the spill
					temp memory location.
					result code can be:
						FIRST position
						sw_m gsr275(a3)[A1], gra_spill_temp
						...
						gsr275(a3)[A1] = lw_m gra_spill_temp
						gsr275(a3)[A1] gsr271(p7)[P1] gsr272(p6)[P1] :- sgtu_m sr270(p1)[P1] ...
					*/
					is_p2_def = false;
				} else {
					is_p2_def = true;
				}
			} else {
				is_p2_def = false;
			}
			break;
		}
	}

	//4 plots
	if (is_p1_def && !is_p2_def) { 		//def ... use
		is_pos1_spill = true; //need spill
		is_pos2_spill = false; //need reload
		return;
	} else if (is_p1_def && is_p2_def) { //def ... def
		pos1 = pos2 = -1; //do not need spill and reload.
		return;
	} else if (!is_p1_def) {
		if (is_p2_def) { 				//use ... def
			//pos2 do not need reload.
			pos2 = -1;
		} else { 						//use ... use
			is_pos2_spill = false; //need reload at pos2
		}

		//Find the DEF of pos1.
		pos1 = lt->get_backward_def_occ(p1, firstpos);
		if (pos1 != -1) {
			is_pos1_spill = true; //spill at pos1
		} else if ( //LT_is_dedicated(lt) ||
				   LT_is_global(lt)) { //Might be live-in lifetime
			pos1 = p1;
			is_pos1_spill = true; //spill at pos1
		} else {
			IS_TRUE(0, ("local use without DEF, dead use?"));
		}
		return;
	}
	IS_TRUE(0, ("Should we here?"));
}


/* Return true if 'prno' is the one of operands of 'ir' ,
and is also the result.

'prno': can be NULL. And if it is NULL, we only try to
get the index-info of the same opnd and result. */
bool BBRA::is_opnd_samewith_result(IR * ir)
{
	IS_TRUE0(0);
	return false;
}


void BBRA::rename_result(IR * ir, UINT old_prno, IR * newpr)
{
	IS_TRUE0(0);
}


void BBRA::rename_opnd(IR * ir, UINT old_prno, IR * newpr)
{
	IS_TRUE0(0);
}


/* Rename opnds in between 'start' and 'end' occurrencens within lifetime.
'start': start pos in lifetime, can NOT be the livein pos.
'end': end pos in lifetime, can NOT be the liveout pos. */
void BBRA::rename_opnds_in_range(LT * lt, IR * newpr, INT start, INT end)
{
	IS_TRUE0(lt && newpr && newpr->is_pr());
	IS_TRUE0(0);
	INT firstpos = m_ltm->get_first_pos();
	INT lastpos = m_ltm->get_last_pos();
	if (start == -1) { start = firstpos; }
	if (end == -1) { end = lastpos; }
	IS_TRUE0(start >= firstpos && start <= lastpos);
	IS_TRUE0(end >= firstpos && end <= lastpos);
	IS_TRUE0(start <= end);

	BITSET * occ = LT_occ(lt);
	IS_TRUE0(occ);
	start = MAX(start, occ->get_first());
	end = MIN(end, occ->get_last());
	if (!occ->is_contain(start)) {
		start = occ->get_next(start);
	}
	if (start == -1) { return; }
	for (INT i = start; i <= end; i = occ->get_next(i)) {
		IS_TRUE(i >= 0, ("out of boundary"));
		IR * ir = m_ltm->get_ir(i);
		IS_TRUE0(ir);
		if (lt->is_def(i)) {
			rename_result(ir, LT_prno(lt), newpr);
		} else {
			rename_opnd(ir, LT_prno(lt), newpr);
		}
	}
}


/* Generate spilling and reloading code at position 'start' and 'end' of life time
'lt' respectively.

'lt': split candidate, may be local and global lifetimes.

NOTICE:
	Neglact 'start' if it equals -1, and similar for 'end'. */
void BBRA::split_lt_at(INT start, INT end, bool is_start_spill,
					   bool is_end_spill, LT * lt)
{
	IS_TRUE0(lt);
	INT firstpos = m_ltm->get_first_pos();
	INT lastpos = m_ltm->get_last_pos();
	IR * spill_loc = NULL;
	if (start != -1) {
		if (start == firstpos) {
			spill_loc = m_ltm->gen_spill(lt, start);
		} else {
			IR * ir = m_ltm->get_ir(start);
			IS_TRUE0(ir);
			if (is_start_spill) { //Store to memory
				spill_loc = m_ltm->gen_spill(lt, start);
			} else { //Reload from memory
				IS_TRUE(0, ("Reload at the start position "
							"of Hole? It will be performance Gap!"));
				//m_ltm->gen_reload(lt, start);
			}
		}
	}

	if (end != -1) {
		if (end == lastpos) {
			IS_TRUE0(spill_loc);
			IR * newpr = m_ltm->gen_reload(lt, end, spill_loc);
			if (LT_is_global(lt)) {
				IS_TRUE(PR_no(newpr) == LT_prno(lt),
						("Should not rename global register, since that "
						 "global information needs update."));
			}
		} else {
			IR * ir = m_ltm->get_ir(end);
			IS_TRUE0(ir);
			if (is_end_spill) { //Store to memory
				//I think hereon that operations should be reloading!
				IS_TRUE(0,
					("Store at the end position of Hole? Performance Gap!"));
				//m_ltm->gen_spill(lt, end);
			} else { //Reload from memory
				IS_TRUE0(spill_loc);
				IR * newpr = m_ltm->gen_reload(lt, end, spill_loc);
				if (PR_no(newpr) != LT_prno(lt)) {
					//Do renaming.
					INT forward_def = lt->get_forward_def_occ(end, firstpos);

					//May be same result as operand.
					if (forward_def != -1 && forward_def == (end + 1)) {
						IR * occ = m_ltm->get_ir(end);
						IS_TRUE(m_ltm->get_ir(forward_def) == occ,
								("o should be same result and operand."));
						if (!is_opnd_samewith_result(occ)) {
							//Generate new sr again.
							newpr = m_ru->build_pr(IR_dt(newpr));
						}

						//Rename all follows REFs.
						forward_def = -1;
					}
					if (forward_def != -1) {
						rename_opnds_in_range(lt, newpr, end, forward_def - 1);
					} else {
						rename_opnds_in_range(lt, newpr, end, -1);
					}
				}
			}
		}
	}
}


bool BBRA::split(LT * lt)
{
	IS_TRUE0(!LT_is_global(lt)); //glt already has alllocated.
	bool has_hole;
	LT * cand = comp_split_cand(lt, has_hole, m_tmp_lts, m_tmp_lts2);
	IS_TRUE0(cand);
	if (has_hole && cand != lt) {
		INT start, end;
		bool find = find_hole(start, end, cand, lt);
		if (find) {
			bool is_start_spill, is_end_spill;
			select_reasonable_split_pos(start, end, is_start_spill,
										is_end_spill, cand);
			split_lt_at(start, end, is_start_spill, is_end_spill, cand);
			return true;
		}
	}


	//realloc(lt);
	/*
	bool has_hole = false;
	LIFE_TIME * cand =
		compute_best_spill_candidate(lt, ig, mgr, true, &has_hole);

	INT hole_startpos, hole_endpos;
	bool split_hole = false;
	if (has_hole && cand != lt) {
		split_hole =
			get_residedin_hole(&hole_startpos, &hole_endpos, cand, lt, mgr);
	}

	if (split_hole) {
		bool is_start_spill, is_end_spill;
		select_reasonable_split_pos(&hole_startpos,
									&hole_endpos,
									&is_start_spill,
									&is_end_spill,
									cand, mgr);
		split_lt_at(hole_startpos,
					hole_endpos,
					is_start_spill,
					is_end_spill,
					cand, mgr);
	} else if (lt == cand) {
		split_one_lt(lt, prio_list,
					uncolored_list, mgr,
					ig, spill_location, action);
	} else if (!can_be_spilled(lt, mgr)) {
		split_one_lt(cand, prio_list,
					uncolored_list, mgr,
					ig, spill_location, action);
	} else {
		split_two_lts(lt, cand, prio_list,
					uncolored_list, mgr, ig,
					spill_location, action);
	}

	show_phase("---Split,before ReAllocate_LifeTime");
	reallocate_lifetime(prio_list, uncolored_list,
						mgr, ddg, layerddg, ig, cri);
	for (LIFE_TIME * tmplt = uncolored_list.get_head();
		 tmplt != NULL; tmplt = uncolored_list.get_next()) {
		if (HAVE_FLAG(m_cur_phase, PHASE_FINIAL_FIXUP_DONE)) {
			//Should not change regfile again.
			action.set_action(tmplt, ACTION_SPLIT);
		} else {
			action.set_action(tmplt, ACTION_BFS_REASSIGN_REGFILE);
		}
	}
	show_phase("---Split finished");
	*/
	return true;
}


void BBRA::dump_prio(LIST<LT*> & prios)
{
	if (g_tfile == NULL) { return; }
	for (LT * l = prios.get_head(); l != NULL; l = prios.get_next()) {
		fprintf(g_tfile, "\nLT%d(pr%d)(prio=%f)",
				LT_uid(l), LT_prno(l), LT_priority(l));
		if (LT_is_global(l)) {
			fprintf(g_tfile, "(global)");
		}
		if (l->has_allocated()) {
			fprintf(g_tfile, "(phy=v%d)", LT_phy(l));
		}
		if (LT_prefer_reg(l) != REG_UNDEF) {
			fprintf(g_tfile, "(prefer=v%d)", LT_prefer_reg(l));
		}
	}
	fflush(g_tfile);
}


bool BBRA::solve(LIST<LT*> & prios)
{
	IS_TRUE0(m_tmp_lts && m_tmp_uints);
	for (;prios.get_elem_count() > 0;) {
		LT * lt = prios.remove_head();
		IS_TRUE(!lt->has_branch(m_ltm),
		("Branch should be allocated first, we can not split at branch"));
		bool succ = split(lt);
		IS_TRUE0(succ);
		m_ltm->clean();
		m_ltm->build(true, NULL, *m_tmp_cii);
		m_rsc->comp_local_usage(m_ltm, true, m_omit_constrain);

		LIST<LT*> * unalloc = m_tmp_lts;
		collect_unalloc(*unalloc);
		if (unalloc->get_elem_count() == 0) { return true; }

		m_ltm->get_ig()->erase();
		m_ltm->get_ig()->build();

		prios.clean();
		build_prio_list(*unalloc, prios);
		dump_prio(prios);

		LIST<UINT> * nis = m_tmp_uints;
		alloc_prio_list(prios, *nis);
	}
	return false;
}


void BBRA::collect_unalloc(LIST<LT*> & unalloc)
{
	SVECTOR<LT*> * vec = m_ltm->get_lt_vec();
	unalloc.clean();
	for (INT i = 1; i <= vec->get_last_idx(); i++) {
		LT * lt = vec->get(i);
		if (lt == NULL || lt->has_allocated()) { continue; }
		IS_TRUE(!LT_is_global(lt), ("glt should be allocated already"));
		unalloc.append_head(lt);
	}
}


bool BBRA::perform(LIST<LT*> & prios)
{
	LTM * ltm = m_ra->m_gltm.map_bb2ltm(m_bb);
	IS_TRUE0(ltm);

	IS_TRUE0(m_tmp_lts);
	LIST<LT*> * unalloc = m_tmp_lts;
	collect_unalloc(*unalloc);
	if (unalloc->get_elem_count() == 0) { return true; }

	prios.clean();
	build_prio_list(*unalloc, prios);

	IS_TRUE0(m_tmp_uints);
	LIST<UINT> * nis = m_tmp_uints;
	alloc_prio_list(prios, *nis);
	if (prios.get_elem_count() != 0) {
		//Use m_tmp_lts, m_tmp_lts2, m_tmp_bs
		solve(prios);
	}
	return true;
}
//END BBRA


static int gcount = 0;
static bool gdebug()
{
	return 1;
}


//
//START RA
//
void RA::alloc_local(LIST<UINT> & nis, bool omit_constrain)
{
	LIST<LT*> unalloc;
	LIST<LT*> tmp;
	LIST<LT*> prios;
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		BBRA l(bb, this);
		l.set_tmp_lts(&unalloc);
		l.set_tmp_lts2(&tmp);
		l.set_tmp_uints(&nis);
		l.set_tmp_cii(&m_cii);
		l.set_omit_constrain(omit_constrain);
		l.perform(prios);
	}
}


//Alloc phy-register for local lt which has specific constrain.
void RA::alloc_local_spec(LIST<UINT> & nis)
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	SVECTOR<IR*> need_to_alloc;
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		IR * lastir = IR_BB_last_ir(bb);
		if (lastir == NULL || (!lastir->is_cond_br() &&
			!lastir->is_uncond_br()	&& !lastir->is_multicond_br())) {
			continue;
		}

		m_ii.clean();
		LTM * ltm = m_gltm.get_ltm(IR_BB_id(bb));
		need_to_alloc.clean();
		UINT idx = 0;
		bool need_rebuild = false;
		for (IR * k = ir_iter_init(lastir, m_ii);
			 k != NULL; k = ir_iter_next(m_ii)) {
			if (!k->is_pr()) { continue; }

			LT * l = ltm->map_pr2lt(PR_no(k));
			IS_TRUE0(l);
			if (LT_is_global(l) && !l->has_allocated()) {
				//Can not leave the work to gra.
				IR * newk = insert_move_before(lastir, k);
				need_to_alloc.set(idx, newk);
				idx++;
				need_rebuild = true;
				continue;
			}
			if (l->has_allocated()) {
				IS_TRUE0(LT_usable(l) && LT_usable(l)->is_contain(LT_phy(l)));
				continue;
			}
			need_to_alloc.set(idx, k);
			idx++;
		}

		if (idx == 0) { continue; }
		if (need_rebuild) {
			//TODO: add lt incremental.
			ltm->clean();
			ltm->build(true, NULL, m_cii);
			ltm->get_ig()->erase();
			ltm->get_ig()->build();
		}

		BBRA lra(bb, this);
		lra.set_omit_constrain(false);
		for (UINT i = 0; i < idx; i++) {
			IR * x = need_to_alloc.get(i);
			IS_TRUE0(x);
			LT * lx = ltm->map_pr2lt(PR_no(x));
			IS_TRUE0(lx);
			m_rsc.comp_lt_usable(lx, ltm);
			lra.assign_reg(lx, nis);
			IS_TRUE(lx->has_allocated(), ("not enough phy"));
			IS_TRUE0(LT_usable(lx)->is_contain(LT_phy(lx)));
			update_lt_maxreg(lx);
		}
	}
}


//TODO: compute freq.
float RA::compute_prio(GLT * g)
{
	if (GLT_bbs(g) == NULL) { return 0.0; }
	float prio;

	//TODO: enable it.
	//prio = ((float)g->comp_num_of_occ(m_gltm)) /
	//		(float)GLT_bbs(g)->get_elem_count();
	//prio *= GLT_freq(g);

	//Check to see if glt has branch occ.
	UINT cross_branch = 0;
	DBITSETC * bbs = GLT_bbs(g);
	SC<SEG*> * sc;
	for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
		LTM * ltm = m_gltm.get_ltm(j);
		IS_TRUE0(ltm); //glt miss local part.
		LT * gl = ltm->map_pr2lt(GLT_prno(g));
		if (gl->has_branch(ltm)) {
			cross_branch++;
		}
	}

	prio = (float)GLT_bbs(g)->get_elem_count();
	BITSET const* usable = GLT_usable(g);
	IS_TRUE(usable, ("miss usable-regs info"));
	if (m_rsc.get_4() == usable) {
		prio += 1600;
	} else if (m_rsc.get_8() == usable) {
		prio += 800;
	} else if (m_rsc.get_16() == usable) {
		prio += 100;
	}
	if (GLT_rg_sz(g) > 1) {
		prio *= (float)(GLT_rg_sz(g) * 2);
	}
	if (cross_branch > 0) {
		prio *= 1000 * cross_branch;
	}
	return prio;
}


/* Compute priority list and sort life times with descending order of priorities.
We use some heuristics factors to evaluate the
priorities of each of life times:
	1. Life time in critical path will be put in higher priority.
	2. Life time whose symbol register referenced in
		high density will have higher priority.
	3. Life time whose usable registers are fewer, the priority is higher.

	TO BE ESTIMATED:
		Longer life time has higher priority.
*/
void RA::build_prio_list(OUT LIST<GLT*> & prios)
{
	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL || GLT_is_param(g) || g->has_allocated()) {
			//Allocate parameter elsewhere.
			continue;
		}
		GLT_prio(g) = compute_prio(g);

		//Search for appropriate position to place.
		GLT * t;
		C<GLT*> * ct;
		for (t = prios.get_head(&ct); t != NULL; t = prios.get_next(&ct)) {
			if (GLT_prio(t) < GLT_prio(g)) {
				break;
			}
		}
		if (t == NULL) {
			prios.append_tail(g);
		} else {
			prios.insert_before(g, ct);
		}
	}
}


void RA::diff_local_neighbour_used(GLT * g, LIST<UINT> & nis, BITSET * unusable)
{
	DBITSETC * bbs = GLT_bbs(g);
	if (bbs == NULL) { return; }

	SC<SEG*> * sc;
	for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
		LTM * ltm = m_gltm.get_ltm(j);
		IS_TRUE0(ltm); //glt miss local part.

		LT * gl = ltm->map_pr2lt(GLT_prno(g));
		IS_TRUE0(gl); //glt miss local part.

		IG * ig = ltm->get_ig();
		IS_TRUE0(ig);

		nis.clean();
		bool on = ig->get_neighbor_list(nis, LT_uid(gl));
		IS_TRUE0(on);
		UINT n = nis.get_elem_count();
		for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
			LT * ni = ltm->get_lt(i);
			IS_TRUE0(ni);
			if (LT_is_global(ni) || !ni->has_allocated()) { continue; }
			if (LT_rg(ni) != NULL) {
				IS_TRUE0(LT_rg(ni)->get(0) == LT_phy(ni));
				for (UINT i = 0; i < LT_rg(ni)->rnum; i++) {
					IS_TRUE0(LT_rg(ni)->get(i) != REG_UNDEF);
					unusable->bunion(LT_rg(ni)->get(i));
				}
			} else {
				unusable->bunion(LT_phy(ni));
				if (LT_rg_sz(ni) != 1) {
					IS_TRUE0(LT_rg_sz(ni) == RG_PAIR_SZ);
					unusable->bunion(LT_phy(ni) + 1);
				}
			}
		}
	}
}


/* Return true if allocation was successful, otherwise return false.
When register assigned to 'g', it must be deducted from
the usable_register_set of all its neighbors.

'unusable': for tmp use.
*/
bool RA::assign_reg(GLT * g, LIST<UINT> & nis, LIST<UINT> & nis2)
{
	BITSET const* usable = GLT_usable(g);
	if (usable == NULL) { return false; }
	BITSET * unusable = m_gltm.get_bs_mgr()->create();

	//Avoid allocate the register used by global neighbors.
	nis.clean();
	bool on = m_ig.get_neighbor_list(nis, GLT_id(g));
	IS_TRUE0(on);
	UINT n = nis.get_elem_count();
	for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
		GLT * ni = m_gltm.get_glt(i);
		IS_TRUE0(ni);
		if (!ni->has_allocated()) { continue; }
		if (GLT_rg(ni) != NULL) {
			IS_TRUE0(GLT_rg(ni)->get(0) == GLT_phy(ni));
			for (UINT i = 0; i < GLT_rg(ni)->rnum; i++) {
				IS_TRUE0(GLT_rg(ni)->get(i) != REG_UNDEF);
				unusable->bunion(GLT_rg(ni)->get(i));
			}
		} else {
			unusable->bunion(GLT_phy(ni));
			if (GLT_rg_sz(ni) != 1) {
				IS_TRUE0(GLT_rg_sz(ni) == RG_PAIR_SZ);
				unusable->bunion(GLT_phy(ni) + 1);
			}
		}
	}

	//Avoid allocate the register used by local neighbors.
	diff_local_neighbour_used(g, nis2, unusable);

	//Select preference register.
	UINT pref_reg = GLT_prefer_reg(g);
	if (pref_reg != REG_UNDEF && !unusable->is_contain(pref_reg)) {
		if (GLT_rg_sz(g) > 1) {
			IS_TRUE0(GLT_rg_sz(g) == RG_PAIR_SZ);
			if (!unusable->is_contain(pref_reg + 1)) {
				GLT_phy(g) = pref_reg;
				m_gltm.get_bs_mgr()->free(unusable);
				IS_TRUE0(usable->is_contain(pref_reg));
				IS_TRUE0(!is_cross_param(pref_reg, GLT_rg_sz(g)));
				return true;
			}
		} else {
			GLT_phy(g) = pref_reg;
			m_gltm.get_bs_mgr()->free(unusable);
			IS_TRUE0(usable->is_contain(pref_reg));
			return true;
		}
	}

	//Avoid allocating the registers which
	//are neighbors anticipated.
	n = nis.get_elem_count();
	for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
		GLT * ni = m_gltm.get_glt(i);
		IS_TRUE0(ni);
		if (ni->has_allocated()) { continue; }
		BITSET const* ni_usable = GLT_usable(ni);
		IS_TRUE0(ni_usable);

		//Avoid select the reg which ni preferable.
		UINT p = GLT_prefer_reg(ni);
		if (p != REG_UNDEF) {
			unusable->bunion(p);
			if (GLT_rg_sz(ni) != 1) {
				IS_TRUE0(GLT_rg_sz(ni) == RG_PAIR_SZ);
				unusable->bunion(p + 1);
			}
		}

		/*
		//Only aware of those usable registers more fewer than current.
		//This rule will incur the lt to be allocated some higher phy.
		//e.g: Lt will be assigned v256 or more higher.
		if (ni_usable != NULL &&
			ni_usable->get_elem_count() < usable->get_elem_count()) {
			unusable->bunion(*ni_usable);
		}
		*/
	}

	/*
	UINT reg = REG_UNDEF;
	BITSET * anti = m_gltm.get_anti_regs(g, false);
	if (anti != NULL) {
		//Allocate register that 'g' anticipated.
		for (INT r = anti->get_first(); r >= 0; r = anti->get_next(r)) {
			if (usable->is_contain(r)) {
				GLT_phy(g) = r;
				return true;
			}
		}
	}
	*/

	bool succ = false;
	INT m = usable->get_last();
	for (INT i = FIRST_PHY_REG; i <= m; i++) {
		if (!unusable->is_contain(i)) {
			if (GLT_rg_sz(g) != 1) {
				IS_TRUE(GLT_rg_sz(g) == RG_PAIR_SZ, ("to support more size"));
				if (!unusable->is_contain(i + 1) &&
					!is_cross_param(i, GLT_rg_sz(g))) {
					GLT_phy(g) = i;
					succ = true;
					break;
				}
			} else {
				GLT_phy(g) = i;
				succ = true;
				break;
			}
		}
	}
	m_gltm.get_bs_mgr()->free(unusable);
	IS_TRUE0(!succ || usable->is_contain(GLT_phy(g)));
	return succ;
}


void RA::alloc_param()
{
	UINT vreg = m_maxreg;
	m_param_reg_start = m_maxreg;
	for (UINT i = 0; i < m_param_num; i++, vreg++) {
		UINT prno = m_gltm.m_params.get(i);
		if (prno == 0) { continue; }
		GLT * g = m_gltm.map_pr2glt(prno);
		IS_TRUE0(g && !g->has_allocated());
		GLT_phy(g) = vreg;
		IS_TRUE0(GLT_usable(g) && GLT_usable(g)->is_contain(vreg));
		g->set_local(m_gltm);
	}
	m_maxreg += m_param_num;
}


UINT RA::comp_reserve_reg(IR_ITER & ii, LIST<IR*> & resolve_list)
{
	UINT max_rescount = 0;
	resolve_list.clean();
	LIST<IR_BB*> * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.get_ltm(IR_BB_id(bb));
		if (ltm == NULL) { continue; }
		for (IR * ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			ii.clean();
			FMT fmt = m_rsc.get_fmt(ir);
			IS_TRUE0(fmt != FUNDEF);
			UINT rescount = 0;

			if (ir->is_stpr() || ir->is_call_has_retval()) {
				LT * l = ltm->map_pr2lt(ir->get_prno());
				BITSET const* usable = m_rsc.get_usable(fmt, true);
				IS_TRUE(usable, ("stmt miss usable-regs info"));
				if (LT_rg_sz(l) > 1) {
					IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
					if (!usable->is_contain(LT_phy(l) + 1)) {
						rescount += 2;
					}
				} else if (!usable->is_contain(LT_phy(l))) {
					rescount++;
				}
			}

			for (IR * k = ir_iter_rhs_init(ir, ii);
				 k != NULL; k = ir_iter_rhs_next(ii)) {
				if (!k->is_pr()) { continue; }

				LT * l = ltm->map_pr2lt(PR_no(k));
				IS_TRUE0(l);

				BITSET const* usable = m_rsc.get_usable(fmt, false);
				IS_TRUE(usable, ("stmt miss usable-regs info"));
				if (LT_rg_sz(l) > 1) {
					IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
					if (!usable->is_contain(LT_phy(l) + 1)) {
						rescount += 2;
					}
				} else if (!usable->is_contain(LT_phy(l))) {
					rescount++;
				}
			}

			IS_TRUE0(rescount <= 16);
			if (rescount != 0) {
				resolve_list.append_tail(ir);
			}
			max_rescount = MAX(max_rescount, rescount);
		}
	}
	return max_rescount;
}


//Return true if PR need to be spilled.
bool RA::check_if_need_spill(UINT prno, FMT fmt, LTM const* ltm)
{
	LT * l = ltm->map_pr2lt(prno);
	BITSET const* usable = m_rsc.get_usable(fmt, true);
	IS_TRUE(usable, ("stmt miss usable-regs info"));

	if (!usable->is_contain(LT_phy(l))) {
		return true;
	}

	if (LT_rg_sz(l) > 1) {
		IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
		if (!usable->is_contain(LT_phy(l) + 1)) {
			return true;
		}
	}
	return false;
}


//Revise resource constrain.
void RA::revise_rsc()
{
	UINT resc = 0;
	LIST<IR*> resolve_list;
	while (true) {
		UINT c = comp_reserve_reg(m_ii, resolve_list);
		UINT ofst = 0;
		if (c <= resc) {
			break;
		}
		ofst = c - resc;
		resc = c;
		shift_reg(ofst);
	}
	if (resc == 0) { return; }

	BITSET visitbb;
	IR2INT pr2phy;
	for (IR * ir = resolve_list.get_head();
		 ir != NULL; ir = resolve_list.get_next()) {
		IR_BB * irbb = ir->get_bb();
		LTM * ltm = m_gltm.get_ltm(IR_BB_id(irbb));
		visitbb.bunion(IR_BB_id(irbb));
		FMT fmt = m_rsc.get_fmt(ir);
		IS_TRUE0(fmt != FUNDEF);
		m_ii.clean();

		//Indicate the phy reg num of lhs/rhs which is going to assign.
		INT lhsn = 0;
		INT rhsn = 0;

		if (ir->is_stpr() || ir->is_call_has_retval()) {
			IS_TRUE(lhsn < 1, ("multiple def"));
			UINT prno = ir->get_prno();
			if (check_if_need_spill(prno, fmt, ltm)) {
				IR * spill_loc = ltm->gen_spill_swap(ir, prno,
													IR_dt(ir), NULL);
				IS_TRUE0(spill_loc->get_stmt());
				pr2phy.set(spill_loc, lhsn);
				LT * l = ltm->map_pr2lt(prno);
				lhsn += LT_rg_sz(l);
			}
		}

		for (IR * k = ir_iter_rhs_init(ir, m_ii);
			 k != NULL; k = ir_iter_rhs_next(m_ii)) {
			if (!k->is_pr()) { continue; }

			LT * l = ltm->map_pr2lt(PR_no(k));
			IS_TRUE0(l);

			BITSET const* usable = m_rsc.get_usable(fmt, false);
			IS_TRUE(usable, ("stmt miss usable-regs info"));

			if (usable->is_contain(LT_phy(l))) {
				if (LT_rg_sz(l) > 1) {
					IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
					if (usable->is_contain(LT_phy(l) + 1)) {
						continue;
					}
				} else {
					continue;
				}
			}

			IR * spill_loc = ltm->gen_reload_swap(k, ir);
			IS_TRUE0(spill_loc->get_stmt());
			pr2phy.set(spill_loc, rhsn);
			IS_TRUE0(LT_rg_sz(l) == 1 || LT_rg_sz(l) == 2);
			rhsn += LT_rg_sz(l);
		}
		IS_TRUE0(lhsn <= (INT)resc && rhsn <= (INT)resc);
	}

	//Rebuild lt for new PR which generated by above step.
	for (INT bbid = visitbb.get_first();
		 bbid >= 0; bbid = visitbb.get_next(bbid)) {
		LTM * ltm = m_gltm.get_ltm(bbid);
		IS_TRUE0(ltm);
		ltm->clean();
		ltm->build(false, NULL, m_cii);
		#ifdef _DEBUG_
		//The interf graph is useless after all resolved. Only for verify used.
		ltm->get_ig()->erase();
		ltm->get_ig()->build();
		#endif
	}

	//Bind the new PR with the specified phy register.
	TMAP_ITER<IR*, INT> iter2;
	INT phy;
	for (IR * ir = pr2phy.get_first(iter2, &phy);
		 ir != NULL; ir = pr2phy.get_next(iter2, &phy)) {
		IR * stmt = ir->get_stmt();
		IS_TRUE0(stmt && stmt->get_bb());
		LTM * ltm = m_gltm.get_ltm(IR_BB_id(stmt->get_bb()));
		IS_TRUE0(ltm);
		LT * l = ltm->map_pr2lt(PR_no(ir));
		IS_TRUE0(l && LT_prno(l) == PR_no(ir) && !l->has_allocated());
		LT_phy(l) = phy;
	}
}


void RA::shift_reg(UINT ofst)
{
	IS_TRUE0(ofst < 60000);
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.map_bb2ltm(bb);
		if (ltm == NULL) { continue; }
		SVECTOR<LT*> * lvec = ltm->get_lt_vec();
		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
			LT * l = lvec->get(i);
			if (l == NULL) { continue; }
			IS_TRUE0(l->has_allocated());
			LT_phy(l) += ofst;
		}
	}

	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		IS_TRUE0(g->has_allocated());
		GLT_phy(g) += ofst;
	}

	m_maxreg += ofst;
}


void RA::rotate_reg()
{
	IS_TRUE0(m_param_reg_start == 0);
	if (m_param_num == 0 || m_gltm.m_params.get_last_idx() == -1) {
		return;
	}
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	UINT regn = m_maxreg + 1;
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.map_bb2ltm(bb);
		if (ltm == NULL) { continue; }
		SVECTOR<LT*> * lvec = ltm->get_lt_vec();
		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
			LT * l = lvec->get(i);
			if (l == NULL) { continue; }
			IS_TRUE0(l->has_allocated());

			if (LT_phy(l) < m_param_num) {
				LT_phy(l) = regn - m_param_num + LT_phy(l);
			} else {
				LT_phy(l) = LT_phy(l) - m_param_num;
			}
		}
	}
	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		IS_TRUE0(g->has_allocated());

		if (GLT_phy(g) < m_param_num) {
			GLT_phy(g) = regn - m_param_num + GLT_phy(g);
		} else {
			GLT_phy(g) = GLT_phy(g) - m_param_num;
		}
	}
}


void RA::revise_param()
{
	if (m_param_num == 0 ||
		m_param_reg_start + m_param_num - 1 == m_maxreg ||
		m_gltm.m_params.get_last_idx() == -1) { //not any param in use.
		//Parameter already in place.
		return;
	}
	UINT vreg = m_maxreg - m_param_num + 1;
	UINT lastp = m_param_reg_start + m_param_num - 1;
	m_param_reg_start = vreg;
	if (vreg <= lastp) {
		vreg = lastp + 1;
	}
	LIST<LT*> nis;
	BITSET visit;

	#ifdef _DEBUG_
	BITSET params;
	for (UINT i = 0; i < m_param_num; i++) {
		UINT prno = m_gltm.m_params.get(i);
		params.bunion(prno);
	}
	#endif

	//Swap phy for each lived bb.
	UINT i;
	for (i = 0; vreg <= m_maxreg; i++, vreg++) {
		UINT prno = m_gltm.m_params.get(i);
		if (prno == 0) { continue; }
		GLT * g = m_gltm.map_pr2glt(prno);
		IS_TRUE0(g && g->has_allocated());
		DBITSETC * bbs = GLT_bbs(g);
		if (bbs == NULL) { continue; }
		SC<SEG*> * sc;

		UINT gphy = GLT_phy(g);
		for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
			LTM * ltm = m_gltm.get_ltm(j);
			if (ltm == NULL) { continue; }
			LT * gl = ltm->map_pr2lt(GLT_prno(g));
			IS_TRUE0(gl); //glt miss local part.

			nis.clean();
			ltm->get_ig()->get_neighbor(nis, gl);
			for (LT * ni = nis.get_head(); ni != NULL; ni = nis.get_next()) {
				if (LT_phy(ni) != vreg) { continue; }
				LT_phy(ni) = gphy;
				#ifdef _DEBUG_
				//This neighbour should not be parameter.
				IS_TRUE0(!params.is_contain(LT_prno(ni)));
				#endif
				if (LT_is_global(ni) && !visit.is_contain(LT_prno(ni))) {
					GLT * niglt = ni->set_global(m_gltm);
					niglt->set_local(m_gltm);
					visit.bunion(LT_prno(ni));
				}
			}
		}
		GLT_phy(g) = m_param_reg_start + i;
		g->set_local(m_gltm);
	}

	//Settle the remainder parameters in.
	for (; i < m_param_num; i++) {
		UINT prno = m_gltm.m_params.get(i);
		if (prno == 0) { continue; }
		GLT * g = m_gltm.map_pr2glt(prno);
		IS_TRUE0(g && g->has_allocated());
		GLT_phy(g) = m_param_reg_start + i;
		g->set_local(m_gltm);
	}
}


//'prio' list should be sorted in descending order.
//'nis': for tmp used.
void RA::alloc_prio_list(OUT LIST<GLT*> & prios, OUT LIST<GLT*> & unalloc,
						 LIST<UINT> & nis, LIST<UINT> & nis2)
{
	C<GLT*> * ct, * next_ct;
	for (prios.get_head(&ct), next_ct = ct; ct != NULL; ct = next_ct) {
		GLT * g = C_val(ct);
		prios.get_next(&next_ct);
		IS_TRUE0(!g->has_allocated());
		if (!assign_reg(g, nis, nis2)) {
			//The elem in uncolored list are already sorted in
			//descending priority order.
			unalloc.append_tail(g);
		} else {
			IS_TRUE0(g->has_allocated());
			update_glt_maxreg(g);
		}
		prios.remove(ct);
	}
}


//Split glt into several section.
//Return the spill location.
IR * RA::split(GLT * g)
{
	IS_TRUE0(!GLT_is_param(g));
	DBITSETC * bbs = GLT_bbs(g);
	IS_TRUE(bbs, ("should not select an empty candidate to split"));
	SC<SEG*> * sc;
	IR * spill_loc = NULL;
	for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
		LTM * ltm = m_gltm.get_ltm(j);
		if (ltm == NULL) { continue; }
		LT * gl = ltm->map_pr2lt(GLT_prno(g));
		IS_TRUE0(gl); //glt miss local part.
		if (LT_occ(gl) == NULL) { continue; }
		for (INT i = LT_occ(gl)->get_first();
			 i >= 0; i = LT_occ(gl)->get_next(i)) {
			IR * occ = ltm->get_ir(i);
			IS_TRUE(occ, ("illegal occ info."));
			if (gl->is_def(i)) {
				if (occ->is_stpr()) {
					IS_TRUE0(STPR_no(occ) == LT_prno(gl));
					spill_loc = ltm->gen_spill(STPR_no(occ),
											   IR_dt(occ), occ, spill_loc);
				} else {
					IR * orgpr = occ->get_res_pr(LT_prno(gl));
					IS_TRUE0(orgpr && orgpr->is_pr() &&
							 PR_no(orgpr) == LT_prno(gl));
					spill_loc = ltm->gen_spill(PR_no(orgpr),
											   IR_dt(orgpr), occ, spill_loc);
				}
			} else {
				IR * tgtpr = occ->get_opnd_pr(LT_prno(gl));
				IS_TRUE0(tgtpr && tgtpr->is_pr() &&
						 PR_no(tgtpr) == LT_prno(gl));
				if (spill_loc == NULL) {
					spill_loc = m_ru->dup_ir(tgtpr);
				}
				ltm->gen_reload(tgtpr, occ, spill_loc);
			}
		}
	}
	return spill_loc;
}


//'nis': for tmp used.
void RA::solve_conflict(OUT LIST<GLT*> & unalloc, LIST<UINT> & nis)
{
	for (GLT * g = unalloc.get_head(); g != NULL; g = unalloc.get_next()) {
		IR * spill_loc = split(g);
		IS_TRUE0(spill_loc);
		GLT * slglt = m_gltm.build_glt_like(spill_loc, g);
		IS_TRUE0(slglt);
		GLT_usable(slglt) = m_rsc.get_16();

		nis.clean();
		bool on = m_ig.get_neighbor_list(nis, GLT_id(g));
		IS_TRUE0(on);
		m_ig.add_glt(slglt);
		m_ig.set_interf_with(GLT_id(slglt), nis);
		m_ig.remove_glt(g);
		m_gltm.localize(g);
		slglt->set_local_usable(m_gltm);
	}
}


//If current global lt has allocated phy register, then update
//the information to local part to make global and local
//are coherent.
void RA::update_local()
{
	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		g->set_local(m_gltm);
	}
}


//Verify all glt has been assigned phy.
//Verify local part is consistent with glt.
bool RA::verify_glt(bool check_alloc)
{
	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		IS_TRUE0(!check_alloc || g->has_allocated());

		DBITSETC * bbs = GLT_bbs(g);
		if (bbs == NULL) { continue; }

		SC<SEG*> * sc;
		for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
			LTM * ltm = m_gltm.get_ltm(j);

			//If local part not exist, remove the bit out of lived-bbs.
			IS_TRUE(ltm, ("miss local part"));
			LT * lt = ltm->map_pr2lt(GLT_prno(g));
			IS_TRUE(lt, ("miss local part"));
			IS_TRUE0(LT_is_global(lt));
			IS_TRUE0(LT_phy(lt) == GLT_phy(g));
			IS_TRUE0(LT_rg_sz(lt) == GLT_rg_sz(g));
		}
	}
	return true;
}


//Return true if l intefered with parameters.
bool RA::overlap_param(LT const* l) const
{
	IS_TRUE0(l->has_allocated());
	return LT_phy(l) >= m_param_reg_start &&
		   LT_phy(l) < m_param_reg_start + m_param_num;
}


/* Try to compute the maximum number of registers that
satisfied register group constrains when range starts at 'rangestart'.
'occupied': phy that has assigned to neighbours of ltg member.
'assigend': phy that has assigned to ltg member. */
UINT RA::comp_satisfied_regn(UINT rangestart, LTG const* ltg, UINT rgsz,
							 BITSET const& occupied, BITSET const& assigned,
							 BITSET const& liveout_phy)
{
	IS_TRUE0(rangestart != REG_UNDEF);
	UINT nsat = 0;
	if (is_cross_param(rangestart, rgsz)) {
		return 0;
	}

	UINT start = rangestart;
	UINT end = rangestart + rgsz - 1;
	UINT ofst;
	for (INT i = 0; i <= ltg->get_last_idx(); i++, rangestart += ofst) {
		LT * l = ltg->get(i);
		IS_TRUE0(l);
		ofst = LT_rg_sz(l);
		if (l->has_allocated()) {
			if (LT_phy(l) == rangestart) {
				nsat += LT_rg_sz(l);
				continue;
			}
			if (LT_phy(l) >= start && LT_phy(l) <= end) {
				return 0;
			}
			continue;
		}

		if (occupied.is_contain(rangestart) ||
			assigned.is_contain(rangestart)) {
			/* 'l' can not obtained the expect one.
			The tryrange must not contain the
			phy which has assigned to ltg member. */
			return 0;
		}

		if (LT_rg_sz(l) > 1) {
			IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
			if (occupied.is_contain(rangestart + 1) ||
				assigned.is_contain(rangestart + 1)) {
				//l can not obtained the expect one.
				return 0;
			}
		}

		/*
		if (is_cross_liveout_phy(rangestart, LT_rg_sz(l), liveout_phy)) {
			//The generated move will override the liveout phy.
			return 0;
		}
		*/

		IS_TRUE0(!is_cross_liveout_phy(rangestart, LT_rg_sz(l), liveout_phy));
		nsat += LT_rg_sz(l);
	}
	return nsat;
}


/* Compute the number of registers that satisfied register
group constrains when range starts at 'rangestart'.
NOTE: This function must find a legal range even if
inserting move. */
UINT RA::comp_regn(UINT rangestart, UINT rangeend, LTG const* ltg,
				   BITSET const& occupied, BITSET const& assigned,
				   BITSET const& liveout_phy)
{
	IS_TRUE0(rangestart != REG_UNDEF);
	UINT start = rangestart;
	UINT nsat = 0;
	for (INT i = 0; i <= ltg->get_last_idx(); i++) {
		LT * l = ltg->get(i);
		IS_TRUE0(l);
		if (occupied.is_contain(rangestart)) {
			//l can not obtained the expect one.
			return 0;
		}

		if (LT_rg_sz(l) > 1) {
			IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
			if (occupied.is_contain(rangestart + 1)) {
				//l can not obtained the expect one.
				return 0;
			}
		}

		if (l->has_allocated() && LT_phy(l) != rangestart) {
			if (LT_phy(l) >= start && LT_phy(l) <= rangeend) {
				//Will incur generating overlap-copy.
				return 0;
			}
			if (assigned.is_contain(rangestart)) {
				/*
				The tryrange can not contain the
				phy which has assigned to ltg member.
				*/
				return 0;
			}
			if (LT_rg_sz(l) > 1) {
				IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
				if (assigned.is_contain(rangestart)) {
					return 0;
				}
			}
			IS_TRUE0(!is_cross_liveout_phy(rangestart,
					 LT_rg_sz(l), liveout_phy));
		}

		/*
		if (is_cross_liveout_phy(rangestart, LT_rg_sz(l), liveout_phy)) {
			//The generated move will override the liveout phy.
			return 0;
		}
		*/

		nsat += LT_rg_sz(l);
		rangestart += LT_rg_sz(l);
	}
	return nsat;
}


/* Return true if the range of register overlapped with
live out phy.
e.g: liveout registers are r0,r1,r10, reigster range is r7~r11,
that is overlap. */
bool RA::is_cross_liveout_phy(UINT reg_start, UINT rgsz,
							  BITSET const& liveout_phy)
{
	for (UINT i = reg_start; i < rgsz; i++) {
		if (liveout_phy.is_contain(i)) {
			return true;
		}
	}
	return false;
}


//Try to find a properly range from those lts which has assigned register.
INT RA::try_reuse_appeared(LTG const* ltg, BITSET const& occupied,
						   BITSET const& assigned, BITSET const& liveout_phy)
{
	UINT best_nsat = 0; //The most number of satisfied.
	INT best_range_start = -1;
	INT phyofst = 0;
	UINT rgsz = ltg->get_rg_sz();
	INT frac_start = ((INT)FIRST_PHY_REG) - 1;
	for (INT i = 0; i <= ltg->get_last_idx(); i++) {
		LT const* l = ltg->get(i);
		IS_TRUE0(l);
		INT rangestart = -1;
		if (l->has_allocated()) {
			rangestart = ((INT)LT_phy(l)) - phyofst;
		}

		if (rangestart < 0) {
			//Try range start from the fraction bewteen the phy
			//which has been used.
			for (INT i = frac_start + 1; i < 1023; i++) {
				if (!occupied.is_contain(i) && !assigned.is_contain(i)) {
					frac_start = i;
					break;
				}
			}
			rangestart = frac_start;
		}

		//UINT rangend = rangestart + ltg->get_last_idx();
		UINT nsat = comp_satisfied_regn(rangestart, ltg, rgsz,
										occupied, assigned, liveout_phy);
		UINT old = best_nsat;
		best_nsat = MAX(best_nsat, nsat);
		if (best_nsat != old) {
			best_range_start = rangestart;
		}

		if (best_nsat == rgsz) { break; }
		phyofst += (INT)LT_rg_sz(l);
	}

	//best range is permited to cross liveout phy only if there is
	//no move inserted to rememdy the phy.
	IS_TRUE0(!is_cross_param(best_range_start, rgsz));

	if ((float)best_nsat >= ((float)rgsz * 0.6) && (best_nsat < rgsz)) {
		int a = 0;
	}
	if ((float)best_nsat < ((float)rgsz * 0.6)) {
	//if (best_nsat < rgsz) {
		return -1;
	}
	return best_range_start;
}


INT RA::try_extend(LTG const* ltg, BITSET const& occupied,
				   BITSET const& liveout_phy, BITSET const& assigned)
{
	//There is not any properly range can be allocated.
	UINT tryrangestart = m_param_reg_start + m_param_num;
	UINT count = 0;
	UINT rgsz = ltg->get_rg_sz();
	while (count < (65535 - rgsz)) {
		if (is_cross_param(tryrangestart, rgsz)) {
			continue;
		}
		UINT tryend = tryrangestart + rgsz - 1;
		INT nsat = (INT)comp_regn(tryrangestart, tryend, ltg,
								  occupied, assigned, liveout_phy);
		if ((UINT)nsat == rgsz) {
			return tryrangestart;
		}

		count++;
		tryrangestart++;
	}
	IS_TRUE(0, ("invoke-range not allocable!"));
	return -1;
}


/* Insert move before ir, and replace origial kid 'src'
in 'stmt' with new kid.
Return the new kid.
'stmt': stmt of source operand.
'src': source operand of move, after the replacement, this
	operand will be freed.
*/
IR * RA::insert_move_before(IR * stmt, IR * src)
{
	IS_TRUE0(stmt->is_kids(src));
	IR * newkid = m_ru->build_pr(IR_dt(src));
	IR * mv = m_ru->build_store_pr(PR_no(newkid), IR_dt(newkid),
								   m_ru->dup_ir(src));
	m_rsc.comp_ir_fmt(mv);

	IR_BB * irbb = stmt->get_bb();
	IS_TRUE0(irbb);
	IR_BB_ir_list(irbb).insert_before(mv, stmt);
	stmt->replace_kid(src, newkid, true);
	m_ru->free_irs(src);
	return newkid;
}


/* Check if the phy of ltg member is continuous, and
insert move if necessary.
Assign phy if there are members are not assigned.
'visited': for tmp use
'nis': for tmp use. */
void RA::remedy_ltg(LTG * ltg, IR * ir, LTM * ltm,
					SBITSET & nis, BITSET & visited, UINT rangestart)
{
	IR * param = CALL_param_list(ir);
	IS_TRUE0(param);
	while (param != NULL && !param->is_pr()) { param = IR_next(param); }
	IS_TRUE0(param && cnt_list(param) == ((UINT)ltg->get_last_idx()) + 1);

	TMAP<UINT, UINT> prno2phy;
	bool insert = false;
	IR * next = NULL;
	visited.clean();
	UINT org_rangestart = rangestart;

	for (INT i = 0; i <= ltg->get_last_idx(); i++, param = next) {
		next = IR_next(param);
		LT * l = ltg->get(i);
		IS_TRUE0(l);
		if (l->has_allocated()) {
			visited.bunion(i);
			if (LT_phy(l) != rangestart) {
				IR * x = insert_move_before(ir, param);
				LT_ltg(l) = NULL; //l is not ltg member any more.

				//x will be local lt.
				prno2phy.set(PR_no(x), rangestart);
				insert = true;
			}
			IS_TRUE0(get_maxreg() >= LT_phy(l));
		}
		rangestart += LT_rg_sz(l);
	}
	IS_TRUE0(param == NULL);

	rangestart = org_rangestart;
	param = CALL_param_list(ir);
	while (param != NULL && !param->is_pr()) { param = IR_next(param); }
	IS_TRUE0(param && cnt_list(param) == ((UINT)ltg->get_last_idx()) + 1);

	for (INT i = 0; i <= ltg->get_last_idx(); i++, param = next) {
		next = IR_next(param);
		LT * l = ltg->get(i);
		IS_TRUE0(l && LT_usable(l));
		if (visited.is_contain(i)) {
			rangestart += LT_rg_sz(l);
			continue;
		}

		/*
		If l has allocated, it may be the same pr with previous parameter,
		which case incurred by copy propagtion.
		*/
		bool need_move = false;
		if (l->has_allocated()) {
			#ifdef _DEBUG_
			bool find = false;
			for (INT w = 0; w <= ltg->get_last_idx(); w++) {
				LT * wl = ltg->get(w);
				if (LT_prno(wl) == LT_prno(l)) {
					find = true;
					break;
				}
			}
			IS_TRUE0(find); //Illegal case.
			#endif
			need_move = true;
		}

		if (need_move ||
			(LT_is_global(l) && m_ig.is_interf_with_neighbour(
								get_glt(LT_prno(l)), nis, rangestart))) {
			/* Only insert a move, leave l to local/global allocator.
			   call pr1
			=>
			   x = pr1
			   call x
			*/
			IR * x = insert_move_before(ir, param);
			LT_ltg(l) = NULL; //l is not ltg member any more.

			//x will be local lt.
			prno2phy.set(PR_no(x), rangestart);
			insert = true;
		} else if (LT_usable(l)->is_contain(rangestart)) {
			LT_phy(l) = rangestart;
			if (LT_is_global(l)) {
				GLT * g = l->set_global(m_gltm);
				g->set_local(m_gltm);
			}
			update_lt_maxreg(l);
		} else {
			/* Only insert a move, leave l to local/global allocator.
			   call pr1
			=>
			   x = pr1
			   call x
			*/
			IR * x = insert_move_before(ir, param);
			LT_ltg(l) = NULL; //l is not ltg member any more.

			//x is local lt.
			prno2phy.set(PR_no(x), rangestart);
			insert = true;
		}
		rangestart += LT_rg_sz(l);
	}
	IS_TRUE0(param == NULL);

	if (insert) {
		//Assigned phy to the swapping vreg.
		ltm->clean();

		//The local part of glt must be rebuild even if it is
		//still not allocated.
		ltm->build(true, NULL, m_cii);
		ltm->get_ig()->erase();
		ltm->get_ig()->build();
		TMAP_ITER<UINT, UINT> iter2;
		UINT phy;
		for (UINT prno = prno2phy.get_first(iter2, &phy);
			 prno != 0; prno = prno2phy.get_next(iter2, &phy)) {
			LT * l = ltm->map_pr2lt(prno);
			IS_TRUE0(l);
			LT_phy(l) = phy;
			update_lt_maxreg(l);
			m_rsc.comp_lt_usable(l, ltm);
			IS_TRUE0(LT_usable(l) && LT_usable(l)->is_contain(phy));
		}
	}
}


void RA::dump_ltg()
{
	if (g_tfile == NULL) { return; }
	TMAP_ITER<UINT, LTG*> iter;
	LTG * ltg;
	fprintf(g_tfile, "\n=== DUMP LTG === %s ===\n", m_ru->get_ru_name());
	for (UINT id = m_gltm.m_ltgmgr.get_first(iter, &ltg);
		 id != 0; id = m_gltm.m_ltgmgr.get_next(iter, &ltg)) {
		IR * ir = m_ru->get_ir(id);
		IS_TRUE0(ir && is_range_call(ir, m_gltm.m_dm) && ltg);
		IR_BB * bb = ir->get_bb();
		dump_ir(ir, m_dm);
		LTM * ltm = m_gltm.map_bb2ltm(bb);
		IS_TRUE0(ltm);
		ltm->dump();
	}
	fflush(g_tfile);
}


void RA::assign_ltg(LTG * ltg, IR * ir)
{
	INT best = -1;
	IS_TRUE0(ltg && ltg->ty == LTG_RANGE_PARAM && ir->is_call());
	IS_TRUE0(ir->get_bb());
	IR_BB * bb = ir->get_bb();
	//IS_TRUE0(IR_BB_last_ir(bb) == ir);
	LTM * ltm = m_gltm.map_bb2ltm(bb);
	IS_TRUE0(ltm);

	BITSET assigned; //the phys that ltg has assigned.
	bool all_assigned = true;
	SBITSET nis(m_gltm.m_sbs_mgr.get_seg_mgr());
	SBITSET gnis(m_gltm.m_sbs_mgr.get_seg_mgr());
	IG * ig = ltm->get_ig();
	IS_TRUE0(ig);
	BITSET liveout_phy;

	for (INT i = 0; i <= ltg->get_last_idx(); i++) {
		LT * l = ltg->get(i);
		IS_TRUE0(l);
		ig->get_neighbor_set(nis, LT_uid(l));

		/*
		if (LT_is_global(l)) {
			GLT * g = m_gltm.map_pr2glt(LT_prno(l));
			IS_TRUE0(g);
			m_ig.get_neighbor_set(gnis, GLT_id(g));
		}
		*/

		if (l->has_allocated()) {
			IS_TRUE0(LT_range(l));
			if (ltm->is_liveout(l)) {
				liveout_phy.bunion(LT_phy(l));
				if (LT_rg_sz(l) > 1) {
					IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
					liveout_phy.bunion(LT_phy(l) + 1);
				}
			}

			assigned.bunion(LT_phy(l));

			if (LT_rg_sz(l) > 1) {
				IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
				assigned.bunion(LT_phy(l) + 1);
			}
		} else {
			all_assigned = false;
		}
	}

	if (all_assigned) {
		//Check if continuous.
		INT rangestart;
		bool is_cont = true;
		for (INT i = 0; i <= ltg->get_last_idx(); i++) {
			LT * l = ltg->get(i);
			IS_TRUE0(l && l->has_allocated());
			if (i != 0 && LT_phy(l) != rangestart) {
				//The second condition handle followed case,
				//two lt assigned same phy:
 				//	GLT( 13):  [pr16](v1), rg<pr16(lt13)(v1) pr19(lt16)(v1) >
				is_cont = false;
				break;
			}
			IS_TRUE0(LT_rg_sz(l) >= 1);
			rangestart = LT_phy(l) + LT_rg_sz(l);
		}
		if (is_cont) {
			return;
		}
	}

	//Compute unusable set.
	BITSET occupied; //occupied vreg by other lt which is not member in group.
	SC<SEG*> * cur;

	//Compute the phy occupied by local neighbours.
	for (INT ltid = nis.get_first(&cur);
		 ltid >= 0; ltid = nis.get_next(ltid, &cur)) {
		LT * l = ltm->get_lt(ltid);
		IS_TRUE0(l);
		if (!l->has_allocated()) { continue; }
		occupied.bunion(LT_phy(l));
		if (LT_rg_sz(l) != 1) {
			IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
			occupied.bunion(LT_phy(l) + 1);
		}
	}

	/*
	for (INT gltid = gnis.get_first(&cur);
		 gltid >= 0; gltid = gnis.get_next(gltid, &cur)) {
		GLT * g = m_gltm.get_glt(gltid);
		IS_TRUE0(g);
		if (!g->has_allocated()) { continue; }

		occupied.bunion(GLT_phy(g));
		if (GLT_rg_sz(g) != 1) {
			IS_TRUE0(GLT_rg_sz(g) == RG_PAIR_SZ);
			occupied.bunion(GLT_phy(g) + 1);
		}

	}
	*/

	//Compute the phy occupied by global neighbours.
	for (INT i = 0; i <= ltg->get_last_idx(); i++) {
		LT * l = ltg->get(i);
		IS_TRUE0(l);
		if (!LT_is_global(l)) { continue; }

		GLT * g = m_gltm.map_pr2glt(LT_prno(l));
		IS_TRUE0(g);
		UINT prno = GLT_prno(g);
		SC<SEG*> * sc;
		DBITSETC * bbs = GLT_bbs(g);
		if (bbs == NULL) { continue; }

		for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
			LTM * nltm = m_gltm.get_ltm(j);
			if (nltm == NULL) { continue; }

			LT * gl = nltm->map_pr2lt(prno);
			IS_TRUE0(gl); //global lt miss local part.
			IG * nig = nltm->get_ig();
			IS_TRUE0(nig);
			nis.clean();
			nig->get_neighbor_set(nis, LT_uid(gl));

			//Compute the phy occupied by local part.
			SC<SEG*> * cur2;
			for (INT ltid = nis.get_first(&cur2);
				 ltid >= 0; ltid = nis.get_next(ltid, &cur2)) {
				LT * l = nltm->get_lt(ltid);
				IS_TRUE0(l);
				if (!l->has_allocated()) { continue; }

				occupied.bunion(LT_phy(l));
				if (LT_rg_sz(l) != 1) {
					IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
					occupied.bunion(LT_phy(l) + 1);
				}
			}
		}
	}
	occupied.diff(assigned);

	INT best_range_start = try_reuse_appeared(ltg, occupied,
											  assigned, liveout_phy);
	if (best_range_start == -1) {
		//Find a properly range.
		best_range_start = try_extend(ltg, occupied, liveout_phy, assigned);
	}
	remedy_ltg(ltg, ir, ltm, nis, occupied, best_range_start);
}


/* Verify lt's phy satisfied the constrains of occ.
Note
  * This function should be invoked after all lts has allocated.
  * This function does not check LT's usable set, only check instructions.
*/
bool RA::verify_rsc()
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.map_bb2ltm(bb);
		if (ltm == NULL) { continue; }

		SVECTOR<LT*> * lvec = ltm->get_lt_vec();
		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
			LT * l = lvec->get(i);
			if (l == NULL) { continue; }
			IS_TRUE0(l->has_allocated());
			if (LT_occ(l) == NULL) { continue; }

			//Do more check for each occ of lt.
			for (INT j = LT_occ(l)->get_first();
				 j >= 0; j = LT_occ(l)->get_next(j)) {
				IR * occ = ltm->get_ir(j);
				IS_TRUE(occ, ("occ and pos info are not match."));
				FMT fmt = m_rsc.m_ir2fmt.get(IR_id(occ));
				IS_TRUE0(fmt != FUNDEF);
				BITSET * usable = m_rsc.get_usable(fmt, l->is_def(j));
				IS_TRUE(usable, ("stmt miss usable-regs info"));
				IS_TRUE(usable->is_contain(LT_phy(l)), ("phy is not legal"));
			}
		}
	}
	return true;
}


//Verify if phy is usable to each lt.
//Verify the local part usable-set must be consistent with global.
bool RA::verify_usable()
{
	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.map_bb2ltm(bb);
		if (ltm == NULL) { continue; }
		SVECTOR<LT*> * lvec = ltm->get_lt_vec();
		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
			LT * l = lvec->get(i);
			if (l == NULL) { continue; }
			if (LT_occ(l) != NULL) {
				IS_TRUE0(LT_usable(l));
				if (l->has_allocated()) {
					IS_TRUE(LT_usable(l)->is_contain(LT_phy(l)),
							("phy is unusable to lt"));
				}
				if (LT_is_global(l)) {
					GLT * g = m_gltm.map_pr2glt(LT_prno(l));
					IS_TRUE0(g);
					IS_TRUE0(GLT_usable(g) == LT_usable(l));
				}
			}
		}
	}
	return true;
}


//Verify each pr correspond to unique phy.
//Verify max reg assigned.
//Verify usable regs.
bool RA::verify_reg(bool check_usable, bool check_alloc)
{
	PRNO2UINT prno2v(get_nearest_power_of_2((UINT)57));
	UINT maxreg = 0;
	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }
		if (!check_alloc && !g->has_allocated())  { continue; }
		IS_TRUE0(g->has_allocated());

		maxreg = MAX(maxreg, (UINT)GLT_phy(g));
		if (GLT_rg_sz(g) > 1) {
			IS_TRUE0(GLT_rg_sz(g) == RG_PAIR_SZ);
			maxreg = MAX(maxreg, (UINT)GLT_phy(g) + 1);
		}
	}

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.map_bb2ltm(bb);
		if (ltm == NULL) { continue; }
		SVECTOR<LT*> * lvec = ltm->get_lt_vec();
		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
			LT * l = lvec->get(i);
			if (l == NULL) { continue; }
			if (!check_alloc && !l->has_allocated())  { continue; }
			IS_TRUE0(l->has_allocated());

			if (LT_occ(l) != NULL && check_usable) {
				IS_TRUE0(LT_usable(l));
				IS_TRUE0(LT_usable(l)->is_contain(LT_phy(l)));
			}

			maxreg = MAX(maxreg, (UINT)LT_phy(l));
			if (LT_rg_sz(l) > 1) {
				IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
				maxreg = MAX(maxreg, (UINT)LT_phy(l) + 1);
			}
			bool find;
			UINT v = prno2v.get(LT_prno(l), &find);
			if (find) {
				//each prno is corresponding to a unqiue vreg.
				IS_TRUE0(v == LT_phy(l));
			} else {
				prno2v.set(LT_prno(l), LT_phy(l));
			}
		}
	}

	//Used phy may less than maxreg.
	IS_TRUE0(maxreg <= get_maxreg());
	return true;
}


void RA::alloc_group()
{
	TMAP_ITER<UINT, LTG*> iter;
	LTG * ltg;
	for (UINT id = m_gltm.m_ltgmgr.get_first(iter, &ltg);
		 id != 0; id = m_gltm.m_ltgmgr.get_next(iter, &ltg)) {
		IR * ir = m_ru->get_ir(id);
		IS_TRUE0(ir && is_range_call(ir, m_gltm.m_dm) && ltg);
		assign_ltg(ltg, ir);
	}
}


void RA::build_local_ig()
{
	for (INT i = 0; i <= m_gltm.m_bb2ltmgr.get_last_idx(); i++) {
		LTM * ltm = m_gltm.m_bb2ltmgr.get(i);
		if (ltm == NULL) { continue; }
		ltm->get_ig()->build();
	}
}


/*
Verify live point for global lt.
Verify occurrence for local lt.
Verify each pr should correspond to individual lt.
*/
bool RA::verify_lt_occ()
{
	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL) { continue; }

		DBITSETC * bbs = GLT_bbs(g);
		if (bbs == NULL) { continue; }

		SC<SEG*> * sc;
		UINT prno = GLT_prno(g);
		for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
			SBITSETC * livein = m_prdf.get_livein(j);
			SBITSETC * liveout = m_prdf.get_liveout(j);
			IS_TRUE0(livein && liveout);
			IS_TRUE0(livein->is_contain(prno) || liveout->is_contain(prno));
		}
	}

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.get_ltm(IR_BB_id(bb));
		if (ltm == NULL) { continue; }
		SVECTOR<LT*> * ltvec = ltm->get_lt_vec();
		for (INT i = 1; i <= ltvec->get_last_idx(); i++) {
			LT * l = ltvec->get(i);
			if (l == NULL) { continue; }

			BITSET * occ = LT_occ(l);
			if (occ == NULL) { continue; }

			UINT prno = LT_prno(l);
			for (INT j = occ->get_first(); j >= 0; j = occ->get_next(j)) {
				IR * ir = ltm->get_ir(j);
				IS_TRUE0(ir);
				if (l->is_def(j)) {
					IR * pr = ir->get_res_pr(prno);
					IS_TRUE0(pr);
				} else {
					IR * pr = ir->get_opnd_pr(prno);
					IS_TRUE0(pr);
				}
			}
		}
	}

	CIR_ITER ii;
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.get_ltm(IR_BB_id(bb));
		if (ltm == NULL) { continue; }

		for (IR const* ir = IR_BB_first_ir(bb);
			 ir != NULL; ir = IR_BB_next_ir(bb)) {
			ii.clean();
			for (IR const* k = ir_iter_init_c(ir, ii);
				 k != NULL; k = ir_iter_next_c(ii)) {
				if (!k->is_pr() || !k->is_stpr()) { continue; }

				LT * l = ltm->map_pr2lt(k->get_prno());
				IS_TRUE0(l);
				IS_TRUE0(LT_prno(l) == k->get_prno());
			}
		}
	}
	return true;
}


//Verify global and local lt has been assigned conflict phy.
bool RA::verify_interf()
{
	LIST<UINT> nis;
	SVECTOR<GLT*> * gltv = m_gltm.get_gltvec();
	BITSET nisregs;
	for (INT i = 1; i <= gltv->get_last_idx(); i++) {
		GLT * g = gltv->get(i);
		if (g == NULL || !g->has_allocated()) { continue; }

		nis.clean();
		bool on = m_ig.get_neighbor_list(nis, GLT_id(g));
		IS_TRUE0(on);
		UINT n = nis.get_elem_count();
		for (UINT j = nis.get_head(); n > 0; j = nis.get_next(), n--) {
			GLT * ni = m_gltm.get_glt(j);
			IS_TRUE0(ni);
			if (!ni->has_allocated()) { continue; }
			nisregs.clean();
			nisregs.bunion(GLT_phy(ni));
			if (GLT_rg_sz(ni) > 1) {
				IS_TRUE0(GLT_rg_sz(ni) == RG_PAIR_SZ);
				nisregs.bunion(GLT_phy(ni) + 1);
			}

			IS_TRUE(!nisregs.is_contain(GLT_phy(g)),
					("conflict with neighbour"));
			if (GLT_rg_sz(g) > 1) {
				IS_TRUE0(GLT_rg_sz(g) == RG_PAIR_SZ);
				IS_TRUE(!nisregs.is_contain(GLT_phy(g) + 1),
						("conflict with neighbour"));
			}
		}
	}

	IR_BB_LIST * bbl = m_ru->get_bb_list();
	for (IR_BB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
		LTM * ltm = m_gltm.map_bb2ltm(bb);
		if (ltm == NULL) { continue; }
		SVECTOR<LT*> * lvec = ltm->get_lt_vec();
		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
			LT * l = lvec->get(i);
			if (l == NULL || !l->has_allocated()) { continue; }

			IG * ig = ltm->get_ig();
			IS_TRUE0(ig);
			nis.clean();
			bool on = ig->get_neighbor_list(nis, LT_uid(l));
			IS_TRUE0(on);
			UINT n = nis.get_elem_count();
			for (UINT j = nis.get_head(); n > 0; j = nis.get_next(), n--) {
				LT * ni = ltm->get_lt(j);
				IS_TRUE0(ni);
				if (!ni->has_allocated()) { continue; }
				nisregs.clean();
				nisregs.bunion(LT_phy(ni));
				if (LT_rg_sz(ni) > 1) {
					IS_TRUE0(LT_rg_sz(ni) == RG_PAIR_SZ);
					nisregs.bunion(LT_phy(ni) + 1);
				}

				IS_TRUE(!nisregs.is_contain(LT_phy(l)),
						("conflict with neighbour"));
				if (LT_rg_sz(l) > 1) {
					IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
					IS_TRUE(!nisregs.is_contain(LT_phy(l) + 1),
							("conflict with neighbour"));
				}
			}
		}
	}
	return true;
}


//Verify the phy of lt in group are continuous.
bool RA::verify_ltg()
{
	TMAP_ITER<UINT, LTG*> iter;
	LTG * ltg;
	for (UINT id = m_gltm.m_ltgmgr.get_first(iter, &ltg);
		 id != 0; id = m_gltm.m_ltgmgr.get_next(iter, &ltg)) {
		IR * ir = m_ru->get_ir(id);
		IS_TRUE0(ir && is_range_call(ir, m_gltm.m_dm) && ltg);

		IR * arg = NULL;
		for (arg = CALL_param_list(ir); arg != NULL; arg = IR_next(arg)) {
			if (arg->is_pr()) { break; }
		}

		IS_TRUE(arg, ("No PR parameter exist."));

		UINT phy = 0;
		for (INT i = 0; i <= ltg->get_last_idx(); i++, arg = IR_next(arg)) {
			IS_TRUE(arg, ("%dth parameter not exist.", i));
			IS_TRUE(arg->is_pr(), ("%dth parameter must be PR.", i));
			LT * l = ltg->get(i);
			IS_TRUE0(l && l->has_allocated());
			if (i == 0) {
				phy = LT_phy(l);
			} else {
				IS_TRUE(LT_phy(l) == phy, ("phy of ltg must be continuous"));
			}

			phy += LT_rg_sz(l);

			IS_TRUE0(arg);
			UINT bsz = arg->get_dt_size(m_dm);
			if (LT_rg_sz(l) == 1) {
				IS_TRUE0(bsz <= BYTE_PER_INT);
			} else if (LT_rg_sz(l) == 2) {
				IS_TRUE(bsz == BYTE_PER_LONGLONG, ("lt should not be pair"));
			} else {
				IS_TRUE0(0);
			}
		}
	}
	return true;
}


//'nis': for tmp used.
void RA::alloc_global(LIST<UINT> & nis, LIST<UINT> & nis2)
{
	LIST<GLT*> prios;
	LIST<GLT*> unalloc; //Record uncolored life times.
	while (true) {
		prios.clean();
		build_prio_list(prios);

		unalloc.clean();
		alloc_prio_list(prios, unalloc, nis, nis2);
		if (unalloc.get_elem_count() == 0) {
			return;
		}
		solve_conflict(unalloc, nis);
	}
}


bool RA::perform(OPT_CTX & oc)
{
	bool omit_constrain = true;
	char const* name = m_ru->get_ru_name();
	//Antutu4.4.1
	//if (strcmp(name, "Lorg/a/a/h;::a")==0) {
	//if (strcmp(name, "Landroid/support/v4/view/ViewPager;::a")==0) {
	//if (strcmp(name, "Landroid/support/v4/app/aa;::onItemClick") == 0) {
	//if (strcmp(name, "Lcom/antutu/benchmark/view/CircleProgress;::onDraw") == 0) {
	//if (strcmp(name, "Lcom/xiaomi/network/HostManager;::generateHostStats") == 0) {
	//if (strcmp(name, "Lorg/a/a/h;::a") == 0) { //count == 3

	//Antutu5.1
	//if (strcmp(name, "Lcom/antutu/benchmark/f/v;::a") == 0) { //hot
	//if (strcmp(name, "Lcom/antutu/benchmark/f/v;::b") == 0) { //hot
	//if (strcmp(name, "Lcom/a/a/aq;::handleMessage") == 0) { //maxvar must be
	//if (strcmp(name, "La/d;::a") == 0) { //verify_interf failed

	//Linpack.
	//if (strcmp(name, "Lcom/adwhirl/AdWhirlManager;::fetchConfig") == 0) { //paired bug
	//if (strcmp(name, "Lcom/greenecomputing/linpack/Linpack;::dgesl") == 0) { //paired bug
	///if (strcmp(name, "Lcom/flurry/android/q;::a") == 0) { //bug, fixed
	//if (strcmp(name, "Lcom/greenecomputing/linpack/Linpack;::dgefa") == 0) { //bug, fixed
	//if (strcmp(name, "Lcom/millennialmedia/android/VideoPlayer$VideoServer;::run") == 0) { //bug, fixed
	//if (strcmp(name, "Lcom/adwhirl/adapters/CustomAdapter;::displayCustom") == 0) { //insert too many move.
	//if (strcmp(name, "Lcom/google/gson/JsonArrayDeserializationVisitor;::<init>") == 0) { //bug, fixed
	//if (strcmp(name, "Lcom/flurry/android/CatalogActivity;::onCreate") == 0) { //insert too many move.
	//if (strcmp(name, "Lcom/admob/android/ads/AdContainer;::drawText") == 0) { //phy exceed usable set.
	if (m_ru->is_ru_name_equ("Lcom/admob/android/ads/AdRequester;::requestAd")) { //glt interference.
		gcount++;
		//if (gcount==1)
		{
			gdebug();
		}
	}
	get_glt(10); //for debug symbol
	get_lt(1,0); //for debug symbol
	dump_bbs(m_ru->get_bb_list());
	m_cfg->dump_vcg();
	m_pr2v->dump();
	START_TIMER("GRA");

	IS_TRUE0(m_var2pr);
	m_prdf.set_var2pr(m_var2pr);
	m_prdf.perform(oc);
	m_cfg->compute_entry_and_exit(false, true);
	m_gltm.set_consider_local_interf(true);
	m_gltm.build(false);

	//The new info collected by glt should reflect back to local part.
	update_local();
	IS_TRUE0(verify_glt(false) && verify_lt_occ());

	m_rsc.perform(omit_constrain);
	IS_TRUE0(verify_usable());

	m_ig.set_consider_local_interf(true);
	m_ig.build();
	build_local_ig();

	alloc_param();
	alloc_group();
	IS_TRUE0(verify_ltg() && verify_usable());
	IS_TRUE0(verify_reg(!omit_constrain, false));

	LIST<UINT> nis, nis2;
	if (!omit_constrain) {
		alloc_local_spec(nis);
	}
	alloc_global(nis, nis2);
	update_local(); //TODO: remove it.
	IS_TRUE0(verify_reg(!omit_constrain, false));
	IS_TRUE0(verify_glt(true));
	IS_TRUE0(verify_lt_occ());

	alloc_local(nis, omit_constrain);
	IS_TRUE0(verify_reg(!omit_constrain, true));
	IS_TRUE0(m_gltm.verify());
	IS_TRUE0(verify_glt(true) && verify_ltg());
	if (!omit_constrain) {
		IS_TRUE0(verify_rsc());
	}
	IS_TRUE0(verify_lt_occ());

	m_gltm.rename_local();
	IS_TRUE0(m_gltm.verify());

	IS_TRUE0(verify_lt_occ());
	if (omit_constrain) {
		rotate_reg();
		IS_TRUE0(verify_glt(true) && verify_ltg());
		revise_rsc();
		IS_TRUE0(verify_reg(false, true)); //Check usable reg is dispensable here.
	} else {
		revise_param();
		IS_TRUE0(verify_reg(true, true));
	}
	IS_TRUE0(m_gltm.verify());
	IS_TRUE0(verify_glt(true) && verify_ltg() && verify_rsc());
	IS_TRUE0(verify_interf());
	m_gltm.free_gltbs();

	END_TIMER();
	return true;
}
//END RA
