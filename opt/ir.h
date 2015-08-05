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
#ifndef __IR_H__
#define __IR_H__

class SIMP_CTX;
class IR_BB;
class DU;
class SSAINFO;

typedef LIST<IR_BB*> IR_BB_LIST;
typedef LIST<IR const*> CIR_ITER;
typedef LIST<IR *> IR_ITER;

//Map IR to its Holder during instrument operation.
typedef TMAP<IR*, C<IR*>*> IR2HOLDER;
typedef ELIST<IR*, IR2HOLDER> IR_LIST;

//IR type
typedef enum {
	IR_UNDEF =		0,
	IR_CONST =		1,  //Constant value: include integer, float, string.
	IR_ID =			2,  //Identifier.
	IR_LD =			3,  //Load.
	IR_ILD =		4,  //Indirect load.
	IR_PR = 		5,  //Temporary Pseudo Register which can NOT be
						//taken address, and can be look like both
						//register and memory.
	IR_ARRAY =		6,  //Array operation, include base and ofst.
	IR_ST = 		7,	//Store to ID.
	IR_STPR =		8,	//Store to PR.
	IR_IST =		9,	//Indirect store.
	IR_SETEPR =		10, //Set element of PR, where PR is memory chunk or vector.
	IR_GETEPR =		11, //Get element of PR, where PR is memory chunk or vector.
	IR_CALL = 		12, //Direct call.
	IR_ICALL = 		13, //Indirect call.
	IR_LDA =		14,	//Move variable's address to a register.
	IR_ADD = 		15, //Addition.
	IR_SUB = 		16, //Substraction.
	IR_MUL = 		17, //Multiplication.
	IR_DIV = 		18, //Division.
	IR_REM = 		19, //Remainder operation
	IR_MOD = 		20, //Modulus operation
	IR_LAND = 		21, //Logical AND, &&
	IR_LOR = 		22, //Logical OR, ||
	IR_BAND = 		23, //Bitwise AND, &
	IR_BOR = 		24, //Bitwise OR, |
	IR_XOR = 		25, //Exclusive OR.
	IR_ASR =		26, //Arithmetic shift right
	IR_LSR =		27, //Logical shift right
	IR_LSL =		28, //Logical shift left
	IR_LT = 		29, //Less than.
	IR_LE = 		30, //Less than or equal to.
	IR_GT = 		31, //Greater than.
	IR_GE = 		32, //Greater than or equal to.
	IR_EQ = 		33, //Equal to.
	IR_NE = 		34, //Not equal to.
	IR_BNOT = 		35, //Bitwise not, e.g BNOT(0x0001) = 0xFFFE
	IR_LNOT = 		36, //Boolean logical not e.g LNOT(0x0001) = 0x0000
	IR_NEG = 		37, //Negative operation.
	IR_CVT =		38, //Data-type convert
	IR_GOTO = 		39, //Goto definitely label.
	IR_IGOTO = 		40, //Indirect Goto a list of definitely label.
	IR_DO_WHILE = 	41, //Do-While loop struct.
	IR_WHILE_DO = 	42,	//While-Do loop struct.
	IR_DO_LOOP = 	43, //A kind of loop with plainly definition of
					    //INIT(low bound), HIGH bound, LOOP-BODY and STEP
					    //of IV.

	IR_IF = 		44, //High level IF clasuse, include det,
					    //truebody, and false body

	IR_LABEL = 		45, //Describe internal and customer defined label.
	IR_SWITCH = 	46, //Switch clause, include determinant expression, a
					    //list of case, and body.

	IR_CASE = 		47, //CASE VALUE, this is used only within SWITCH clause.
	IR_TRUEBR = 	48, //Branch if determinant express is true.
	IR_FALSEBR = 	49, //Branch if determinant express is false.
	IR_RETURN = 	50, //Return Statement.

	IR_SELECT = 	51, //Conditional select true-exp or false-exp , formalized as :
						//determinant expression ? true-exp : false-exp

	IR_BREAK = 		52, //Terminate current loop end switch execution, which
						//include do-loop, do-while, while-do, and switch stmt.

	IR_CONTINUE = 	53, //Re-execute loop, which
						//include do-loop, do-while, while-do.

	IR_PHI = 		54, //Phi statement.
	IR_REGION = 	55, //REGION statement.

	//DO NOT ADD NEW IR TYPE AFTER THIS ONE.
	IR_TYPE_NUM = 	56  //The last IR type, the number of IR type.

	//////////////////////////////////////////////////////////////////
	//NOTE: Extend IR::ir_type bit length if type value large than 63.
	//////////////////////////////////////////////////////////////////
} IR_TYPE;


//Describe miscellaneous information for IR.
#define IRT_IS_STMT			0x1 //statement.
#define IRT_IS_BIN			0x2 //binary operation.
#define IRT_IS_UNA			0x4 //unary operation.

//Memory reference operation. memory reference indicate all
//operation which write or load memory object.
#define IRT_IS_MEM_REF		0x8

//Memory operand indicate all operation which only load memory object.
#define IRT_IS_MEM_OPND		0x10
#define IRT_IS_ASSOCIATIVE	0x20
#define IRT_IS_COMMUTATIVE	0x40
#define IRT_IS_RELATION		0x80
#define IRT_IS_LOGICAL		0x100
#define IRT_IS_LEAF			0x200
#define IRT_HAS_RESULT		0x400

#define IRDES_type(m)			((m).type)
#define IRDES_name(m)			((m).name)
#define IRDES_kid_map(m)		((m).kid_map)
#define IRDES_kid_num(m)		((m).kid_num)
#define IRDES_is_stmt(m)		(HAVE_FLAG(((m).attr), IRT_IS_STMT))
#define IRDES_is_bin(m)			(HAVE_FLAG(((m).attr), IRT_IS_BIN))
#define IRDES_is_una(m)			(HAVE_FLAG(((m).attr), IRT_IS_UNA))
#define IRDES_is_mem_ref(m)		(HAVE_FLAG(((m).attr), IRT_IS_MEM_REF))
#define IRDES_is_mem_opnd(m)	(HAVE_FLAG(((m).attr), IRT_IS_MEM_OPND))
#define IRDES_is_associative(m)	(HAVE_FLAG(((m).attr), IRT_IS_ASSOCIATIVE))
#define IRDES_is_commutative(m)	(HAVE_FLAG(((m).attr), IRT_IS_COMMUTATIVE))
#define IRDES_is_relation(m)	(HAVE_FLAG(((m).attr), IRT_IS_RELATION))
#define IRDES_is_logical(m)		(HAVE_FLAG(((m).attr), IRT_IS_LOGICAL))
#define IRDES_is_leaf(m)		(HAVE_FLAG(((m).attr), IRT_IS_LEAF))
#define IRDES_has_result(m)		(HAVE_FLAG(((m).attr), IRT_HAS_RESULT))
#define IRDES_size(m)			((m).size)
class IR_DESC {
public:
	IR_TYPE type;
	CHAR const* name;
	BYTE kid_map;
	BYTE kid_num;
	UINT attr;
	BYTE size;
};


typedef SHASH<IR*> IR_ADDR_HASH;

#ifdef _DEBUG_
INT ck_kid_num_valid(IR const* ir, UINT n, CHAR const* file, INT lineno);
INT ck_kid_num_valid_call(IR const* ir, UINT n, CHAR const* filename, INT line);
INT ck_kid_num_valid_loop(IR const* ir, UINT n, CHAR const* filename, INT line);
INT ck_kid_num_valid_branch(IR const* ir, UINT n, CHAR const* filename, INT line);
INT ck_kid_num_valid_binary(IR const* ir, UINT n, CHAR const* filename, INT line);
INT ck_kid_num_valid_unary(IR const* ir, UINT n, CHAR const* filename, INT line);
INT ck_kid_num_irty(IR const* ir, UINT n, IR_TYPE irty,
					CHAR const* filename, INT line);
IR const* ck_irt(IR const* ir, IR_TYPE irt);
IR const* ck_irt_br(IR const* ir);
IR const* ck_irt_call(IR const* ir);
IR const* ck_irt_only_call(IR const* ir);
IR const* ck_irt_only_icall(IR const* ir);
UINT ck_dim(IR const* ir, UINT n);
#endif


//Exported Variables.
extern IR_DESC const g_ir_desc[];

#ifdef _DEBUG_
#define CK_KID_NUM(ir, n, f, l)				(ck_kid_num_valid(ir, n, f, l))
#define CK_KID_NUM_IRTY(ir, n, irty, f, l)	(ck_kid_num_irty(ir, n, irty, f, l))
#define CK_KID_NUM_UNA(ir, n, f, l)			(ck_kid_num_valid_unary(ir, n, f, l))
#define CK_KID_NUM_BIN(ir, n, f, l)			(ck_kid_num_valid_binary(ir, n, f, l))
#define CK_KID_NUM_BR(ir, n, f, l)			(ck_kid_num_valid_branch(ir, n, f, l))
#define CK_KID_NUM_LOOP(ir, n, f, l)		(ck_kid_num_valid_loop(ir, n, f, l))
#define CK_KID_NUM_CALL(ir, n, f, l)		(ck_kid_num_valid_call(ir, n, f, l))
#define CK_IRT(ir, irt)						(ck_irt(ir, irt))
#define CK_IRT_BR(ir)						(ck_irt_br(ir))
#define CK_IRT_CALL(ir)						(ck_irt_call(ir))
#define CK_IRT_ONLY_CALL(ir)				(ck_irt_only_call(ir))
#define CK_IRT_ONLY_ICALL(ir)				(ck_irt_only_icall(ir))
#define CK_DIM(ir, n)						(ck_dim(ir, n))
#else
#define CK_KID_NUM(ir, n, f, l)				(n)
#define CK_KID_NUM_IRTY(ir, n, irty, f, l)	(n)
#define CK_KID_NUM_UNA(ir, n, f, l)			(n)
#define CK_KID_NUM_BIN(ir, n, f, l)			(n)
#define CK_KID_NUM_BR(ir, n, f, l)			(n)
#define CK_KID_NUM_LOOP(ir, n, f, l)		(n)
#define CK_KID_NUM_CALL(ir, n, f, l)		(n)
#define CK_IRT(ir, irt)						(ir)
#define CK_IRT_BR(ir)						(ir)
#define CK_IRT_CALL(ir)						(ir)
#define CK_IRT_ONLY_CALL(ir)				(ir)
#define CK_IRT_ONLY_ICALL(ir)				(ir)
#define CK_DIM(ir, n)						(n)
#endif

#define CKID_TY(ir, irty, n)	CK_KID_NUM_IRTY(ir, n, irty, __FILE__, __LINE__)
#define CKID_BR(ir, n)			CK_KID_NUM_BR(ir, n, __FILE__, __LINE__)
#define CKID_LOOP(ir, n)		CK_KID_NUM_LOOP(ir, n, __FILE__, __LINE__)
#define CKID_UNA(ir, n)			CK_KID_NUM_UNA(ir, n, __FILE__, __LINE__)
#define CKID_BIN(ir, n)			CK_KID_NUM_BIN(ir, n, __FILE__, __LINE__)
#define CKID_CALL(ir, n)		CK_KID_NUM_CALL(ir, n, __FILE__, __LINE__)


//Used by all IR.
#define IRNAME(ir)				(IRDES_name(g_ir_desc[IR_type(ir)]))
#define IRTNAME(irt)			(IRDES_name(g_ir_desc[irt]))
#define IRTSIZE(irt)			(IRDES_size(g_ir_desc[irt]))

#define IR_MAX_KID_NUM(ir)		(IRDES_kid_num(g_ir_desc[IR_type(ir)]))

//Each IR at same REGION has it own unique id.
#define IR_id(ir)				((ir)->id)

//Record result data type.
#define IR_dt(ir)				((ir)->result_data_type_index)

//Record if ir might throw exception.
#define IR_may_throw(ir)		((ir)->may_throw_exception)

//Indicate IR will terminate current control flow.
//If this flag is true, the code that followed subsequently is unreachable.
#define IR_is_termiate(ir)		((ir)->is_terminate_control_flow)

//Record IR type.
#define IR_type(ir)				((ir)->type)
#define IR_get_type(ir)			((IR_TYPE)((ir)->type))

//Access parent IR.
#define IR_parent(ir)			((ir)->parent)

//Access next IR.
#define IR_next(ir)				((ir)->next)

//Access prev IR.
#define IR_prev(ir)				((ir)->prev)

//Record attached info.
#define IR_ai(ir)				((ir)->ai)

//True if current operation is atomic. If ir is atomic load, write or
//read-modify-write.
#define IR_is_atomic(ir)		((ir)->is_atomic)

/* True if current operation is atomic read-modify-write.
For given variable, RMW operation read the old value, then compare
with new value, then write the new value to the variable. The write
operation may be failed.
If the variable is volatile, one should not change the order of this
operation with other volatile operations. */
#define IR_is_read_mod_write(ir)	((ir)->is_read_mod_write)

//True if ir has sideeffect. This flag often be used to prevent user
//perform incorrect optimization.
#define IR_has_sideeffect(ir)		((ir)->has_sideeffect)

// Define this marco if we try to searh
//ir in free_ir_tab while invoking new_ir().
//#define CONST_IRT_SZ

#ifdef CONST_IRT_SZ
#define IR_irt_size(ir)			((ir)->irt_size)
#endif
class IR {
public:
	UINT id; //Each IR has unique id.

	#ifdef _DEBUG_
	IR_TYPE type;
	#else
	UINT type:6;
	#endif
	UINT result_data_type_index:10; //allows 1024 kinds of type-id.
	UINT may_throw_exception:1;
	UINT is_atomic:1;
	UINT is_read_mod_write:1;
	UINT is_terminate_control_flow:1;
	UINT has_sideeffect:1;

	#ifdef CONST_IRT_SZ
	UINT irt_size:6;
	#endif

	//Both of 'next' and 'prev' used by the process of
	//complicated tree level IR construction.
	IR * next;
	IR * prev;

	//Used in all processs at all level IR.
	//This field should be NULL if IR is the top level of stmt.
	IR * parent;
	IR_AI * ai; //Each IR has unique attach info.

	bool calc_array_ofst(TMWORD * ofst, DT_MGR * dm) const;
	inline DU * clean_du();
	void clean_ref_md()
	{
		DU * du = get_du();
		if (du == NULL) { return; }
		DU_md(du) = NULL;
	}

	void clean_ref_mds()
	{
		DU * du = get_du();
		if (du == NULL) { return; }
		DU_mds(du) = NULL;
	}

	void clean_ref()
	{
		DU * du = get_du();
		if (du == NULL) { return; }
		DU_mds(du) = NULL;
		DU_md(du) = NULL;
	}

	//Copy memory reference only for current ir node.
	//'src': copy MD reference from 'src', it may be different to current ir.
	void copy_ref(IR const* src, REGION * ru)
	{
		IS_TRUE0(src && ru);
		IS_TRUE(is_memory_ref(), ("not memory reference"));
		set_ref_md(src->get_ref_md(), ru);
		set_ref_mds(src->get_ref_mds(), ru);
	}

	/* Copy each memory reference for whole ir tree.
	'src': copy MD reference from 'src', it must be equal to current ir tree.
	'copy_kid_ref': copy MD reference for kid recursively. */
	void copy_ref_for_tree(IR const* src, REGION * ru)
	{
		IS_TRUE0(src && is_ir_equal(src, true) && ru);
		IS_TRUE0(src != this);
		if (is_memory_ref()) {
			set_ref_md(src->get_ref_md(), ru);
			set_ref_mds(src->get_ref_mds(), ru);
		}

		for (INT i = 0; i < IR_MAX_KID_NUM(this); i++) {
			IR * kid = get_kid(i);
			if (kid == NULL) { continue; }

			IR * srckid = src->get_kid(i);
			IS_TRUE0(srckid);
			for (; kid != NULL; kid = IR_next(kid), srckid = IR_next(srckid)) {
				IS_TRUE0(srckid);
				kid->copy_ref_for_tree(srckid, ru);
			}
		}
	}

	//Clean all DU-Chain and Defined/Used-MD reference info.
	inline void free_duset(SDBITSET_MGR & sbs_mgr)
	{
		DU * du = get_du();
		if (du == NULL || DU_duset(du) == NULL) { return; }

		//Free DU_SET back to SEG_MGR, or it will
		//complain and make an assertion.
		sbs_mgr.free_sbitsetc(DU_duset(du));
		DU_duset(du) = NULL;
	}

	inline IR * get_kid(UINT idx) const;
	inline IR_BB * get_bb() const;
	inline DU * get_du() const;
	inline UINT get_ofst() const;

	/* Return STMT for given kid.
	e.g:  st(i32 a)
		     ld(i32 b)
	If given expression is ld, this function return st stmt. */
	inline IR * get_stmt() const
	{
		IS_TRUE0(IR_type(this) != IR_UNDEF);
		IS_TRUE(!is_stmt(), ("IR already be stmt, it is performance bug."));
		IR const* ir = this;
		while (IR_parent(ir) != NULL) { ir = IR_parent(ir); }
		IS_TRUE(ir->is_stmt(), ("ir is orphan"));
		return (IR*)ir;
	}

	/* Return nearest STMT for given kid.
	e.g: if (det)
		   st(i32 a)
		     ld(i32 b)
		 endif
	If given expression is ld, this function return st. */
	inline IR * get_nearest_stmt() const
	{
		//Return STMT for given kid.
		IS_TRUE0(IR_type(this) != IR_UNDEF);
		IS_TRUE(!is_stmt(), ("IR already be stmt, it is performance bug."));
		IR const* ir = this;
		while (IR_parent(ir) != NULL) {
			ir = IR_parent(ir);
			if (ir->is_stmt()) { break; }
		}
		IS_TRUE(ir->is_stmt(), ("ir is orphan"));
		return (IR*)ir;
	}

	inline LABEL_INFO * get_label() const;
	inline UINT get_array_elem_dt_size(DT_MGR const* dm) const;
	inline IR * get_pr_results();

	//Return byte size of ir data type.
	inline UINT get_dt_size(DT_MGR const* dm) const
	{ return dm->get_dtd_bytesize(dm->get_dtd(IR_dt(this))); }

	inline DATA_TYPE get_rty(DT_MGR const* dm) const
	{ return DTD_rty(dm->get_dtd(IR_dt(this))); }

	//Return data type descriptor.
	inline DTD const* get_dtd(DT_MGR const* dm) const
	{ return dm->get_dtd(IR_dt(this)); }

	//Return rhs if exist. Some stmt has rhs,
	//such as IR_ST, IR_STPR and IR_IST.
	inline IR * get_rhs() const;

	//Return the PR no if exist.
	inline UINT get_prno() const;
	inline SSAINFO * get_ssainfo() const;
	IR * get_res_pr(UINT prno);
	IR * get_opnd_pr_list(UINT prno);
	IR * get_opnd_pr(UINT prno, IR_ITER & ii); //Nonrecursively.
	IR * get_opnd_pr(UINT prno);

	//Get the MD that IR referrenced.
	MD const* get_ref_md() const
	{
		DU * du = get_du();
		if (du == NULL) { return NULL; }
		return DU_md(du);
	}

	//Get the MD that IR referrenced. This function is readonly.
	DU_SET const* get_duset_c() const
	{
		DU * du = get_du();
		if (du == NULL) { return NULL; }
		return DU_duset(du);
	}

	//Get the MD that IR referrenced.
	DU_SET * get_duset() const
	{
		DU * du = get_du();
		if (du == NULL) { return NULL; }
		return DU_duset(du);
	}

	//Get the MD_SET that IR referrenced.
	MD_SET const* get_ref_mds() const
	{
		DU * du = get_du();
		if (du == NULL) { return NULL; }
		return DU_mds(du);
	}

	//Return exact MD if ir defined.
	inline MD const* get_exact_ref() const
	{
		MD const* md = get_ref_md();
		if (md == NULL || !md->is_exact()) { return NULL; }
		return md;
	}

	inline MD const* get_effect_ref() const
	{
		MD const* md = get_ref_md();
		IS_TRUE0(md == NULL || !MD_is_may(md));
		return md;
	}

	//Return true if ir may compute at least one result.
	bool has_result() const
	{ return IRDES_has_result(g_ir_desc[IR_type(this)]); }

	//Return true if ir is call and does have a return value.
	inline bool has_return_val() const;

	//Return true if current IR tree may contain memory reference.
	bool is_contain_mem_ref() const
	{
		switch (IR_type(this)) {
		case IR_GOTO:
		case IR_LABEL:
		case IR_CASE:
		case IR_BREAK:
		case IR_CONTINUE:
			return false;
		default:;
		}
		return true;
	}

	//Return true if ir's data type is vector.
	inline bool is_vec(DT_MGR const* dm) const
	{ return DTD_is_vec(get_dtd(dm)); }

	//Return true if ir's data type is pointer.
	inline bool is_ptr(DT_MGR const* dm) const
	{ return DTD_is_ptr(get_dtd(dm)); }

	//Return true if ir's data type is string.
	inline bool is_str(DT_MGR const* dm) const
	{ return dm->is_str(IR_dt(this)); }

	//Return true if ir's data type is memory chunk.
	inline bool is_mc(DT_MGR const* dm) const { return DTD_is_mc(get_dtd(dm)); }

	//Return true if ir is label.
	inline bool is_lab() const { return IR_type(this) == IR_LABEL; }

	//Return true if current ir equal to src.
	bool is_ir_equal(IR const* src, bool is_cmp_kid = true) const;

	//Return true if current ir is both PR and equal to src.
	inline bool is_pr_equal(IR const* src) const;

	//Return true if ir-list are equivalent.
	bool is_irs_equal(IR const* irs, bool is_cmp_kid = true) const;

	//Return true if current ir's data type is equal to 'src'.
	bool is_type_equal(IR const* src) const
	{ return IR_dt(this) == IR_dt(src); }

	//Return true if current ir is memory store operation.
	bool is_store() const
	{
		return IR_type(this) == IR_ST || IR_type(this) == IR_STPR ||
			   IR_type(this) == IR_IST;
	}

	//Return true if current ir is valid type to be phi operand.
	inline bool is_phi_opnd() const { return is_pr() || is_const(); }

	//Return true if current ir is stmt.
	//Only statement can be chained.
	bool is_stmt() const { return IRDES_is_stmt(g_ir_desc[IR_type(this)]); }

	//Return true if current ir is expression.
	bool is_exp() const { return !is_stmt(); }

	//Return true if current ir is binary operation.
	bool is_bin_op() const { return IRDES_is_bin(g_ir_desc[IR_type(this)]); }

	//Return true if current ir is unary operation.
	bool is_unary_op() const { return IRDES_is_una(g_ir_desc[IR_type(this)]); }

	//Return true if k is kid node of right-hand-side of current ir.
	bool is_rhs(IR const* k) const { return !is_lhs(k) && k != this; }

	//Return true if k is the lhs of current ir.
	inline bool is_lhs(IR const* k) const;

	//Return true if ir is return stmt.
	bool is_return() const { return IR_type(this) == IR_RETURN; }

	//Return true if ir terminates the control flow.
	bool is_terminate() const { return IR_is_termiate(this); }

	//Return true if ir data type is signed, and the type
	//may be integer or float.
	inline bool is_signed(DT_MGR const* dm) const
	{ return dm->is_signed(IR_dt(this)); }

	//Return true if ir data type is signed integer.
	inline bool is_sint(DT_MGR const* dm) const
	{ return dm->is_sint(IR_dt(this)); }

	//Return true if ir data type is unsgined integer.
	bool is_uint(DT_MGR const* dm) const
	{
		return (dm->is_uint(IR_dt(this)) ||
			    dm->is_bool(IR_dt(this)));
	}

	//Return true if ir data type is integer.
	bool is_int(DT_MGR const* dm) const
	{
		return (dm->is_sint(IR_dt(this)) ||
			    dm->is_uint(IR_dt(this)) ||
			    dm->is_bool(IR_dt(this)));
	}

	//Return true if ir data type is float.
	bool is_fp(DT_MGR const* dm) const
	{ return dm->is_fp(IR_dt(this)); }

	//Return true if ir data type is boolean.
	bool is_bool(DT_MGR const* dm) const
	{ return dm->is_bool(IR_dt(this)); }

	//Return true if ir is volatile.
	inline bool is_volatile() const;

	//Return true if ir is constant expression.
	inline bool is_const_exp() const;

	/* Return true if ir is readonly expression.
	This attribute indicate that the expression does not modify any
	memory. Note it can only refer to expression. */
	inline bool is_readonly_exp() const;

	/* Return true if ir is readonly function call.
	This attribute indicate that, on expression, it does not modify
	any memory; on function, it does not modify any global memory or
	any memory object that pass through pointer arguments. */
	inline bool is_readonly_call() const;

	bool is_truebr() const { return IR_type(this) == IR_TRUEBR; }
	bool is_falsebr() const { return IR_type(this) == IR_FALSEBR; }
	bool is_cond_br() const
	{ return IR_type(this) == IR_TRUEBR || IR_type(this) == IR_FALSEBR; }

	//Return true if ir may jump to multiple target.
	bool is_multicond_br() const { return IR_type(this) == IR_SWITCH; }

	bool is_uncond_br() const
	{ return IR_type(this) == IR_GOTO || IR_type(this) == IR_IGOTO; }

	//Return true if ir is indirect jump to multiple target.
	bool is_indirect_br() const { return IR_type(this) == IR_IGOTO; }

	bool is_call() const
	{ return IR_type(this) == IR_CALL || IR_type(this) == IR_ICALL; }

	//Return true if ir is a call and has a return value.
	inline bool is_call_has_retval() const
	{ return is_call() && has_return_val(); }

	//Return true if ir is PR.
	bool is_pr() const { return IR_type(this) == IR_PR; }

	//Return true if current stmt exactly modifies a PR.
	//CALL/ICALL may modify PR if it has a return value.
	inline bool is_write_pr() const
	{ return is_stpr() || is_phi() || is_setepr(); }

	//Return true if current stmt read value from PR.
	inline bool is_read_pr() const
	{ return is_pr() || is_getepr(); }

	//Return true if current ir operation reference memory.
	//These kinds of operation always define or use MD.
	bool is_memory_ref() const
	{ return IRDES_is_mem_ref(g_ir_desc[IR_type(this)]); }

	//Return true if current ir operation reference memory, and
	//it is the rhs of stmt.
	//These kinds of operation always use MD.
	bool is_memory_opnd() const
	{ return IRDES_is_mem_opnd(g_ir_desc[IR_type(this)]); }

	//True if store to pseudo register.
	bool is_stpr() const { return IR_type(this) == IR_STPR; }

	//True if store to specified element of pseduo register.
	//The pseduo register must be D_MC or vector type.
	bool is_setepr() const { return IR_type(this) == IR_SETEPR; }

	//True if picking up specified element of givne PR and store the value
	//to a new PR. The base PR must be D_MC or vector type.
	//And the result PR must be element type of base PR.
	bool is_getepr() const { return IR_type(this) == IR_GETEPR; }

	//True if ir is phi.
	bool is_phi() const { return IR_type(this) == IR_PHI; }

	//True if store to identifier.
	bool is_stid() const { return IR_type(this) == IR_ST; }

	bool is_const() const { return IR_type(this) == IR_CONST; }

	//True if ir is atomic read-modify-write.
	inline bool is_rmw() const;

	//True if ir is atomic operation.
	bool is_atom() const { return IR_is_atomic(this); }

	//True if memory operation is store to array.
	inline bool is_st_array() const;
	bool is_judge() const
	{ return is_relation() || is_logical(); }

	bool is_logical() const
	{ return IRDES_is_logical(g_ir_desc[IR_type(this)]); }

	bool is_relation() const
	{ return IRDES_is_relation(g_ir_desc[IR_type(this)]); }

	//IR meet commutative, e.g: a+b = b+a
	bool is_commutative() const
	{ return IRDES_is_commutative(g_ir_desc[IR_type(this)]); }

	//IR meet associative, e.g: (a+b)+c = a+(b+c)
	bool is_associative() const
	{ return IRDES_is_associative(g_ir_desc[IR_type(this)]); }

	//Return true if current ir is leaf node at IR tree.
	//Leaf node must be expression node and it does not have any kids.
	bool is_leaf() const
	{ return IRDES_is_leaf(g_ir_desc[IR_type(this)]); }

	//Return true if kid is the kid node of current ir.
	inline bool is_kids(IR const* exp) const;

	//Return true if access array via id. In contrast to direct array reference,
	//one can access array via pointer, that is indirect array access.
	inline bool is_direct_array_ref() const;

	//This function invert the operation accroding to it semantics.
	inline void invert_ir_type(REGION * ru)
	{
		switch (IR_type(this)) {
		case IR_LT: IR_type(this) = IR_GE; break;
		case IR_LE: IR_type(this) = IR_GT; break;
		case IR_GT: IR_type(this) = IR_LE; break;
		case IR_GE: IR_type(this) = IR_LT; break;
		case IR_EQ: IR_type(this) = IR_NE; break;
		case IR_NE: IR_type(this) = IR_EQ; break;
		case IR_TRUEBR: IR_type(this) = IR_FALSEBR; break;
		case IR_FALSEBR: IR_type(this) = IR_TRUEBR; break;
		case IR_LOR:
			invert_lor(ru);
			break;
		case IR_LAND:
			invert_land(ru);
			break;
		default: IS_TRUE(0, ("unsupport"));
		}
	}

	void invert_land(REGION * ru);
	void invert_lor(REGION * ru);

	//Return true if current ir can be placed in BB.
	bool is_stmt_in_bb() const
	{
		switch (IR_type(this)) {
		case IR_ST:
		case IR_STPR:
		case IR_SETEPR:
		case IR_GETEPR:
		case IR_IST:
		case IR_CALL:
		case IR_ICALL:
		case IR_GOTO:
		case IR_IGOTO:
	    case IR_TRUEBR:
		case IR_FALSEBR:
		case IR_SWITCH:
		case IR_RETURN:
		case IR_REGION:
		case IR_PHI:
			return true;
		default:;
		}
		return false;
	}

	//Return true if current stmt must modify 'md'.
	inline bool is_exact_def(MD const* md) const;
	inline bool is_exact_def(MD const* md, MD_SET const* mds) const;

	inline void set_rhs(IR * rhs);
	inline void set_prno(UINT prno);
	inline void set_ssainfo(SSAINFO * ssa);
	inline void set_label(LABEL_INFO * li);
	inline void set_ofst(UINT ofst);
	inline void set_du(DU * du);
	inline void set_bb(IR_BB * bb);

	//Set 'kid' to be 'idx'th child of current ir.
	inline void set_kid(UINT idx, IR * kid);

	//Set the relationship between parent and its kid.
	void set_parent_pointer(bool recur = true);

	//The current ir is set to kid's parent.
	void set_parent(IR * kid)
	{
		IS_TRUE0(kid && is_kids(kid));
		for (IR * k = kid; k != NULL; k = IR_next(k)) {
			IR_parent(k) = this;
		}
	}

	//The current ir is set to pointer type.
	inline void set_pointer_type(UINT pointer_base_size, DT_MGR * dm)
	{
		IS_TRUE0(pointer_base_size != 0);
		DTD d;
		DTD_rty(&d) = D_PTR;
		DTD_ptr_base_sz(&d) = pointer_base_size;
		IR_dt(this) = dm->register_dtd(&d);
	}
	void set_ref_md(MD const* md, REGION * ru);
	void set_ref_mds(MD_SET const* mds, REGION * ru);

	/* Find and substitute 'newk' for 'oldk'.
	Return true if replaced the 'oldk'.
	'recur': set to true if function recusively perform replacement for 'oldk'. */
	bool replace_kid(IR * oldk, IR * newk, bool recur)
	{
		for (INT i = 0; i < IR_MAX_KID_NUM(this); i++) {
			IR * kid = get_kid(i);
			if (kid == NULL) { continue; }
			for (IR * x = kid; x != NULL; x = IR_next(x)) {
				if (x == oldk) {
					::replace(&kid, oldk, newk);
					if (IR_prev(newk) == NULL) {
						//oldk is the header, and update the kid i.
						set_kid(i, kid);
					} else {
						IR_parent(newk) = IR_parent(oldk);
					}
					return true;
				}
				if (recur && x->replace_kid(oldk, newk, true)) {
					return true;
				}
			}
		}
		return false;
	}

	bool verify(DT_MGR const* dm) const;
	bool verify_kids() const;
};


//Used by CONST, integer constant value
#define IR_is_const(ir)			(IR_type(ir) == IR_CONST)

//Record float point.
#define CONST_fp_val(ir)		(((CCONST*)CK_IRT(ir, IR_CONST))->u1.fp_const_value)

//Record integer.
#define CONST_int_val(ir)		(((CCONST*)CK_IRT(ir, IR_CONST))->u1.int_const_value)

//Record string.
#define CONST_str_val(ir)		(((CCONST*)CK_IRT(ir, IR_CONST))->u1.str_value)
class CCONST : public IR {
public:
	union {
		//record string const if ir is IR_CONST.
		SYM * str_value;

		//Record integer const with length at host machine.
		HOST_INT int_const_value;

		//Record float point value with length at host machine.
		HOST_FP fp_const_value;
	} u1;
};


class VAR_PROP {
public:
	//Record VAR if ir is IR_LD|IR_ID.
	VAR * id_info;
};


#define DU_PROP_du(ir)		(((DU_PROP*)ir)->du)
class DU_PROP : public IR {
public:
	DU * du;
};


#define ID_info(ir)			(((CID*)CK_IRT(ir, IR_ID))->id_info)
#define ID_du(ir)			(((CID*)CK_IRT(ir, IR_ID))->du)
class CID : public DU_PROP, public VAR_PROP {
public:
};


class OFST_PROP {
public:
	/* Record accessing field. result-type-idx should be D_MC.
	ir is used by IR_LD|IR_ST|IR_ILD|IR_IST|IR_LDA|IR_ARRAY

	Usage:
		LD<ofst:3>('x') 				=> pr=*(&x + 3)
		ILD<ofst:3>(LD('x'))			=> pr=*(x + 3)
		ST<ofst:3>('x', IMM:0x100)		=> *(&x + 3)=0x100
		IST<ofst:3>(LD('x'), IMM:0x100) => *(x + 3)=0x100
		LDA<ofst:3>('x')				=> pr = &x + 3
		ARRAY<ofst:3>(LDA('x'), OFST:5) => *(&x[5] + 3) = pr or
											pr = *(&x[5] + 3)
	*/
	UINT field_offset;
};


/* This class represent memory load operation.
LD_ofst descibe the byte offset that is the addend to variable base address.

usage: ld(i32, ofst:10, s) with LD_ofst = 10 means:
	Assum a pointer p, it point to the address of variable s.
	The ld operation loads i32 value from the address (p + 10) */
#define LD_ofst(ir) 		(((CLD*)CK_IRT(ir, IR_LD))->field_offset)
#define LD_idinfo(ir)		(((CLD*)CK_IRT(ir, IR_LD))->id_info)
#define LD_du(ir)			(((CLD*)CK_IRT(ir, IR_LD))->du)
class CLD : public DU_PROP, public VAR_PROP, public OFST_PROP {
public:
};


/* This class represent indirect memory load operation.
ILD_ofst descibe the byte offset that is the addend to address.
If ILD_ofst is not 0, the base memory address must add the offset.

usage: ild p, where p is ILD_base, it must be pointer.
	1. res = ild (p), if ILD_ofst is 0.
	2. res = ild (p + ILD_ofst) if ILD_ofst is not 0. */
#define ILD_ofst(ir)		(((CILD*)CK_IRT(ir, IR_ILD))->field_offset)
#define ILD_du(ir)			(((CILD*)CK_IRT(ir, IR_ILD))->du)
#define ILD_base(ir)		(((CILD*)ir)->opnd[CKID_TY(ir, IR_ILD, 0)])
#define ILD_kid(ir, idx)	(((CILD*)ir)->opnd[CKID_TY(ir, IR_ILD, idx)])
class CILD : public DU_PROP, public OFST_PROP {
public:
	IR * opnd[1];
};


//This class represent properties of stmt.
class STMT_PROP {
public:
	IR_BB * bb;
};


/* This class represent memory store operation.
ST_ofst descibe the byte offset that is the addend to address.
ST_idinfo describe the memory variable.
If ST_ofst is not 0, the base memory address must add the offset.

usage: st(lhs, rhs), p = &lhs, where p is the memory address of lhs.
	1. [p] = rhs, if ST_ofst is 0.
	2. [p + ST_ofst] = rhs if ST_ofst is not 0. */
#define ST_bb(ir)			(((CST*)CK_IRT(ir, IR_ST))->bb)
#define ST_idinfo(ir)		(((CST*)CK_IRT(ir, IR_ST))->id_info)
#define ST_ofst(ir)			(((CST*)CK_IRT(ir, IR_ST))->field_offset)
#define ST_du(ir)			(((CST*)CK_IRT(ir, IR_ST))->du)
#define ST_rhs(ir)			(((CST*)ir)->opnd[CKID_TY(ir, IR_ST, 0)])
#define ST_kid(ir, idx)		(((CST*)ir)->opnd[CKID_TY(ir, IR_ST, idx)])
class CST: public CLD, public STMT_PROP {
public:
	IR * opnd[1];
};


/* This class represent temporary memory store operation.
The temporary memory named pseudo register.

usage: stpr(prno:1, val), will store val to PR1. */
#define STPR_bb(ir)			(((CSTPR*)CK_IRT(ir, IR_STPR))->bb)
#define STPR_no(ir)			(((CSTPR*)CK_IRT(ir, IR_STPR))->prno)
#define STPR_ssainfo(ir)	(((CSTPR*)CK_IRT(ir, IR_STPR))->ssainfo)
#define STPR_du(ir)			(((CSTPR*)CK_IRT(ir, IR_STPR))->du)
#define STPR_rhs(ir)		(((CSTPR*)ir)->opnd[CKID_TY(ir, IR_STPR, 0)])
#define STPR_kid(ir, idx)	(((CSTPR*)ir)->opnd[CKID_TY(ir, IR_STPR, idx)])
class CSTPR: public DU_PROP, public STMT_PROP {
public:
	UINT prno; //PR number.
	SSAINFO * ssainfo; //Present ssa def and use set.
	IR * opnd[1];
};


/* This class represent an operation that store value to one of element of
a temporary location named pseudo register.

SETEPR_ofst descibe the byte offset that is the addend to address.

If SETEPR_ofst is not 0, the base memory address must add the offset.
The the number of byte of SETEPR_base must be an integer multiple of
the number of byte of SETEPR_rhs.

usage: setepr(prno:1, val, ofst), where PR1 must be memory chunk or vector.
This operation will store val to the memory which offset to the memory chunk
or vector's base address. */
#define SETEPR_bb(ir)		(((CSETEPR*)CK_IRT(ir, IR_SETEPR))->bb)
#define SETEPR_no(ir)		(((CSETEPR*)CK_IRT(ir, IR_SETEPR))->prno)
#define SETEPR_ssainfo(ir)	(((CSETEPR*)CK_IRT(ir, IR_SETEPR))->ssainfo)
#define SETEPR_du(ir)		(((CSETEPR*)CK_IRT(ir, IR_SETEPR))->du)
#define SETEPR_rhs(ir)		(((CSETEPR*)ir)->opnd[CKID_TY(ir, IR_SETEPR, 0)])
#define SETEPR_ofst(ir)		(((CSETEPR*)ir)->opnd[CKID_TY(ir, IR_SETEPR, 1)])
#define SETEPR_kid(ir, idx)	(((CSETEPR*)ir)->opnd[CKID_TY(ir, IR_SETEPR, idx)])
class CSETEPR: public DU_PROP, public STMT_PROP {
public:
	UINT prno; //PR number.
	SSAINFO * ssainfo; //Present ssa def and use set.
	IR * opnd[2];
};


/* This class represent an operation that get an element from a temporary
location named pseudo register. The the number of byte of GETEPR_base must be
an integer multiple of the number of byte of current ir.

usage: gete $pr1(i32) $pr2(vec<4*i32>), 4.
	Get value of the second element of pr2 to pr1.

NOTE:
	1. PR can not be taken address.
	2. PR is always allocate on stack. */
#define GETEPR_bb(ir)		(((CGETEPR*)CK_IRT(ir, IR_GETEPR))->bb)
#define GETEPR_no(ir)		(((CGETEPR*)CK_IRT(ir, IR_GETEPR))->prno)
#define GETEPR_ssainfo(ir)	(((CGETEPR*)CK_IRT(ir, IR_GETEPR))->ssainfo)
#define GETEPR_du(ir)		(((CGETEPR*)CK_IRT(ir, IR_GETEPR))->du)
#define GETEPR_base(ir)		(((CGETEPR*)ir)->opnd[CKID_TY(ir, IR_GETEPR, 0)])
#define GETEPR_ofst(ir)		(((CGETEPR*)ir)->opnd[CKID_TY(ir, IR_GETEPR, 1)])
#define GETEPR_kid(ir, idx)	(((CGETEPR*)ir)->opnd[CKID_TY(ir, IR_GETEPR, idx)])
class CGETEPR : public DU_PROP, public STMT_PROP {
public:
	UINT prno; //PR number.

	//versioned presentation or ssa def and use list in ssa mode.
	//Note this field only avaiable if SSA information is maintained.
	SSAINFO * ssainfo;

	IR * opnd[2];
};


/* This class represent indirect memory store operation.
IST_ofst descibe the byte offset that is the addend to address.

If IST_ofst is not 0, the base memory address must add the offset.

usage: ist = ld p, rhs, where the value of p is the base memory address
to be stored. The followed code exhibits the behaivor of such usage.
	1. [p] = rhs, if IST_ofst is 0.
	2. [p + IST_ofst] = rhs, if IST_ofst is not 0. */
#define IST_bb(ir)			(((CIST*)CK_IRT(ir, IR_IST))->bb)
#define IST_ofst(ir)		(((CIST*)CK_IRT(ir, IR_IST))->field_offset)
#define IST_du(ir)			(((CIST*)CK_IRT(ir, IR_IST))->du)
#define IST_base(ir)		(((CIST*)ir)->opnd[CKID_TY(ir, IR_IST, 0)])
#define IST_rhs(ir)			(((CIST*)ir)->opnd[CKID_TY(ir, IR_IST, 1)])
#define IST_kid(ir, idx)	(((CIST*)ir)->opnd[CKID_TY(ir, IR_IST, idx)])
class CIST : public DU_PROP, public OFST_PROP, public STMT_PROP {
public:
	IR * opnd[2];
};


/* This class represent the operation to load memory variable address.
The base of LDA may be ID, LABEL, STRING.

NOTE: LDA_ofst describe the byte offset that is the addend to the address.

usage: lda(s) with LDA_ofst = 10 means:
	pointer p = lda(s)
	p = p + 10
	return p */
#define LDA_ofst(ir)		(((CLDA*)CK_IRT(ir, IR_LDA))->field_offset)
#define LDA_base(ir)		(((CLDA*)ir)->opnd[CKID_TY(ir, IR_LDA, 0)])
#define LDA_kid(ir, idx)	(((CLDA*)ir)->opnd[CKID_TY(ir, IR_LDA, idx)])
class CLDA : public IR, public OFST_PROP {
public:
	//record expression which will to be taken address.
	//It can be ID, string const, LABEL.
	IR * opnd[1];
};


/* This class uses bits to describe attributes.
Represents a direct function call.
NOTE: 'opnd' must be the last member. */
#define CALL_bb(ir)				(((CCALL*)CK_IRT_CALL(ir))->bb)
#define CALL_idinfo(ir)			(((CCALL*)CK_IRT_ONLY_CALL(ir))->id_info)

//Returned result PR no.
#define CALL_prno(ir)			(((CCALL*)CK_IRT_CALL(ir))->prno)

//SSA info of result PR.
#define CALL_ssainfo(ir)		(((CCALL*)CK_IRT_CALL(ir))->ssainfo)

//True if this call is intrinsic function.
#define CALL_is_intrinsic(ir)	(((CCALL*)CK_IRT_CALL(ir))->is_intrinsic)

//True if this call does not modify any memory.
#define CALL_is_readonly(ir)	(VAR_is_readonly(CALL_idinfo((CCALL*)CK_IRT_CALL(ir))))

//True if this call do allocate memory from heap. It always the function
//like malloc or new.
#define CALL_is_alloc_heap(ir)	(((CCALL*)CK_IRT_CALL(ir))->is_alloc_heap)

#define CALL_du(ir)				(((CCALL*)CK_IRT_CALL(ir))->du)

//Parameter list of call.
#define CALL_param_list(ir)		(((CCALL*)ir)->opnd[CKID_CALL(ir, 0)])
#define CALL_kid(ir, idx)		(((CCALL*)ir)->opnd[CKID_CALL(ir, idx)])
class CCALL : public DU_PROP, public VAR_PROP, public STMT_PROP {
public:
	//True if current call is intrinsic call.
	BYTE is_intrinsic:1;

	//True if this call do allocate memory from heap. It always the function
	//like malloc or new.
	BYTE is_alloc_heap:1;

	UINT prno; //PR number.

	SSAINFO * ssainfo; //Present ssa def and use set.

	//NOTE: 'opnd' must be the last member.
	IR * opnd[1];
};


/* Represents a indirect function call.
This class uses macro operations of CCALL.
Expression to compute the target function address.
NOTE: 'opnd_pad' must be the first member. */

//Indicate the callee function pointer.
#define ICALL_callee(ir)		(((CICALL*)ir)->opnd[CKID_TY(ir, IR_ICALL, 1)])

//True if current call is readonly.
#define ICALL_is_readonly(ir)	(((CICALL*)CK_IRT_ONLY_ICALL(ir))->is_readonly)
#define ICALL_kid(ir, idx)	(((CICALL*)ir)->opnd[CKID_TY(ir, IR_ICALL, idx)])
class CICALL : public CCALL {
public:
	//NOTE: 'opnd_pad' must be the first member.
	IR * opnd_pad[1];

	//True if current call is readonly.
	BYTE is_readonly:1;
};


//Binary Operations, include add, sub, mul, div, rem, mod,
//land, lor, band, bor, xor, lt, le, gt, ge, eq, ne, asr, lsr, lsl.
#define BIN_opnd0(ir)		(((CBIN*)ir)->opnd[CKID_BIN(ir, 0)])
#define BIN_opnd1(ir)		(((CBIN*)ir)->opnd[CKID_BIN(ir, 1)])
#define BIN_kid(ir, idx)	(((CBIN*)ir)->opnd[CKID_BIN(ir, idx)])
class CBIN : public IR {
public:
	IR * opnd[2];
};


//Unary Operations, include neg, bnot, lnot.
#define UNA_opnd0(ir)		(((CUNA*)ir)->opnd[CKID_UNA(ir, 0)])
#define UNA_kid(ir, idx)	(((CUNA*)ir)->opnd[CKID_UNA(ir, idx)])
class CUNA : public IR {
public:
	IR * opnd[1];
};


//This class represent goto operation, unconditional jump to target label.
#define GOTO_bb(ir)			(((CGOTO*)CK_IRT(ir, IR_GOTO))->bb)
#define GOTO_lab(ir)		(((CGOTO*)CK_IRT(ir, IR_GOTO))->jump_target_lab)
class CGOTO : public IR, public STMT_PROP {
public:
	LABEL_INFO * jump_target_lab;
};


/* This class represent indirect goto operation,
the control flow will unconditional jump to one target label of a list of
label which determined by value-exp.

usage: igoto (value-exp) case_list */
#define IGOTO_bb(ir)			(((CIGOTO*)CK_IRT(ir, IR_IGOTO))->bb)

//Value expression.
#define IGOTO_vexp(ir)			(((CIGOTO*)ir)->opnd[CKID_TY(ir, IR_IGOTO, 0)])

//Record a list pair of <case-value, jump label>.
#define IGOTO_case_list(ir)		(((CIGOTO*)ir)->opnd[CKID_TY(ir, IR_IGOTO, 1)])

#define IGOTO_kid(ir, idx)		(((CIGOTO*)ir)->opnd[CKID_TY(ir, IR_IGOTO, idx)])
class CIGOTO : public IR, public STMT_PROP {
public:
	IR * opnd[2];
};


/* High level control loop operation.
usage:
	while (det)
	  body
	endwhile
NOTE:
	* The member layout should be same with do_while.
	* 'opnd' must be the last member of CWHILE_DO. */
//Determinate expression.
#define LOOP_det(ir)		(((CWHILE_DO*)ir)->opnd[CKID_LOOP(ir, 0)])

//Loop body.
#define LOOP_body(ir)		(((CWHILE_DO*)ir)->opnd[CKID_LOOP(ir, 1)])
#define LOOP_kid(ir, idx)	(((CWHILE_DO*)ir)->opnd[CKID_LOOP(ir, idx)])
class CWHILE_DO : public IR {
public:
	//NOTE: 'opnd' must be the last member of CWHILE_DO.
	IR * opnd[2];
};


/* High level control loop operation.
usage:
	do
	  body
	while (det) */
class CDO_WHILE : public CWHILE_DO {
public:
};


/* High level control loop operation.
This structure represent a kind of loop with
plainly definition of INIT(low bound), DET(HIGH bound),
LOOP-BODY and STEP(Increment or Dcrement) of induction variable.
usage:
	do
	  init: i = 0
	  det: i <= 10
	  body
	  step: i = i+1
	enddo
This class uses LOOP_det, LOOP_body to access its determinate
expression, and loop body.

NOTE: 'opnd_pad' must be the first member of CDO_LOOP. */

//Record the stmt that init iv.
#define LOOP_init(ir)		(((CDO_LOOP*)ir)->opnd[CKID_TY(ir, IR_DO_LOOP, 2)])

//Record the stmt that update iv.
#define LOOP_step(ir)		(((CDO_LOOP*)ir)->opnd[CKID_TY(ir, IR_DO_LOOP, 3)])
#define DOLOOP_kid(ir, idx)	(((CDO_LOOP*)ir)->opnd[CKID_TY(ir, IR_DO_LOOP, idx)])
class CDO_LOOP : public CWHILE_DO {
public:
	//NOTE: 'opnd_pad' must be the first member of CDO_LOOP.
	IR * opnd_pad[2];
};


/* This class represent high level control IF operation.
usage:
	if (det)
	  truebody
	  falsebody
	endif */
#define IF_det(ir)			(((CIF*)ir)->opnd[CKID_TY(ir, IR_IF, 0)])
#define IF_truebody(ir)		(((CIF*)ir)->opnd[CKID_TY(ir, IR_IF, 1)])
#define IF_falsebody(ir)	(((CIF*)ir)->opnd[CKID_TY(ir, IR_IF, 2)])
#define IF_kid(ir, idx)		(((CIF*)ir)->opnd[CKID_TY(ir, IR_IF, idx)])
class CIF : public IR {
public:
	IR * opnd[3];
};


//This class represent internal and customer defined label.
#define LAB_lab(ir)		(((CLAB*)CK_IRT(ir, IR_LABEL))->label_info)
class CLAB : public IR {
public:
	LABEL_INFO * label_info;
};


/* This class represent high and middle level control flow switch operation.
usage:
	switch (value-exp)
	case_list
	body
	endswitch */
#define SWITCH_bb(ir)			(((CSWITCH*)CK_IRT(ir, IR_SWITCH))->bb)

/* Default label.
This is a label repesent the default jump target of IR_SWITCH.
The label is optional.
If there are not any cases matched, the control flow will jump to
the default label. */
#define SWITCH_deflab(ir)		(((CSWITCH*)CK_IRT(ir, IR_SWITCH))->default_label)

//Value expression.
#define SWITCH_vexp(ir)			(((CSWITCH*)ir)->opnd[CKID_TY(ir, IR_SWITCH, 0)])

//Switch body.
#define SWITCH_body(ir)			(((CSWITCH*)ir)->opnd[CKID_TY(ir, IR_SWITCH, 1)])

//Record a list pair of <case-value, jump label>.
#define SWITCH_case_list(ir)	(((CSWITCH*)ir)->opnd[CKID_TY(ir, IR_SWITCH, 2)])

#define SWITCH_kid(ir, idx)		(((CSWITCH*)ir)->opnd[CKID_TY(ir, IR_SWITCH, idx)])
class CSWITCH : public IR, public STMT_PROP {
public:
	IR * opnd[3];
	LABEL_INFO * default_label;
};


//This class represent the case value expression and its jump target label.
//NOTE: this class is used only by SWITCH and IGOTO.
#define CASE_lab(ir)		(((CCASE*)CK_IRT(ir, IR_CASE))->jump_target_label)

//Value expression.
#define CASE_vexp(ir)		(((CCASE*)ir)->opnd[CKID_TY(ir, IR_CASE, 0)])

#define CASE_kid(ir, idx)	(((CCASE*)ir)->opnd[CKID_TY(ir, IR_CASE, idx)])
class CCASE : public IR {
public:
	IR * opnd[1]; //case-value
	LABEL_INFO * jump_target_label; //jump lable for case.
};


/* This class represent array operation.
Base of array can be LDA, or other computational expression.
This operation do not perform any array bound diagnositc.

* If array base is LDA, it denotes that the array's base is variable with
array type,
	e.g: char p[N]; (&p)[i] = ...

* If array base is computational expression, it denotes that the array's
base is pointer, and the pointer point to an array.
	e.g: char * p; (p+1)[i] = ...

'elem_ty' represent the type of array element.
Moreover, element may be array as well.

'elem_num' represent the number of array element in current dimension.

If 'elem_tyid' is vector, ARR_ofst refers the referrenced element byte offset.
*/
#define ARR_ofst(ir)			(((CARRAY*)CK_IRT(ir, IR_ARRAY))->field_offset)
#define ARR_du(ir)				(((CARRAY*)CK_IRT(ir, IR_ARRAY))->du)
#define ARR_elem_ty(ir)			(((CARRAY*)CK_IRT(ir, IR_ARRAY))->elem_ty)

/* Get the number of element of each dimension.
e.g: Given array D_I32 A[10][20], the 0th dimension has 20 elements,
each element has type D_I32, the 1th dimension has 10 elements,
each element has type [D_I32*20].
If we have an array accessing, such as A[i][j], the sublist will be
ld(j)->ld(i), and elem_num list will be 20->10.
the fininal access address will be (j + 20 * i) * sizeof(D_I32) + lda(A). */
#define ARR_elem_num(ir, dim)	(((CARRAY*)CK_IRT(ir, IR_ARRAY))->elem_num[CK_DIM(ir, dim)])
#define ARR_elem_num_buf(ir)	(((CARRAY*)CK_IRT(ir, IR_ARRAY))->elem_num)

//Array base.
#define ARR_base(ir)		(((CARRAY*)ir)->opnd[CKID_TY(ir, IR_ARRAY, 0)])

//Array subscript expression.
#define ARR_sub_list(ir)	(((CARRAY*)ir)->opnd[CKID_TY(ir, IR_ARRAY, 1)])
#define ARR_kid(ir, idx)	(((CARRAY*)ir)->opnd[CKID_TY(ir, IR_ARRAY, idx)])
class CARRAY : public DU_PROP, public OFST_PROP {
public:
	IR * opnd[2];

	/* IR_dt describe the data-type of ARRAY operation + ARR_ofst.
	elem_ty describe array element type.
	e.g: struct {int a, b; } s[100];
	     ... = s[2].b;
	data-type of array operation is D_I32, because ARR_ofst is 4,
	that means we take the value of second field of struct, meanwhile
	data-type of array element is D_MC, size is 8, (struct {int a, b;}). */
	UINT elem_ty; //record data-type of array element.

	//Record the number of array element for each dimension.
	//Note that the elem_num buffer can NOT be modified
	//after it is created.
	TMWORD const* elem_num;

	//Return the number of dimension.
	TMWORD get_dimn() const
	{
		IS_TRUE0(IR_type(this) == IR_ARRAY);
		TMWORD dim = 0;
		for (IR const* s = ARR_sub_list(this); s != NULL; s = IR_next(s)) {
			dim++;
		}
		return dim;
	}

	//Return true if exp is array base.
	bool is_base(IR const* exp) const
	{ return exp == ARR_base(this); }

	//Return true if exp is array subscript expression list.
	bool is_in_sub_list(IR const* exp) const
	{
		for (IR const* s = ARR_sub_list(this); s != NULL; s = IR_next(s)) {
			if (s == exp || s->is_kids(exp)) { return true; }
		}
	}
};


//This class represent data-type convertion.
//Record the expression to be converted.
#define CVT_exp(ir)			(((CCVT*)ir)->opnd[CKID_TY(ir, IR_CVT, 0)])
#define CVT_kid(ir, idx)	(((CCVT*)ir)->opnd[CKID_TY(ir, IR_CVT, idx)])
class CCVT : public IR {
public:
	IR * opnd[1]; //expression to be converted.
};


/* This class represent temporary memory location which named pseudo register.
It can be used to indicate the REGION live-in register. In this case, a PR may not
have a definition.
NOTE:
	1.PR can not be taken address.
	2.PR is always allocate on stack. */
#define PR_no(ir)			(((CPR*)CK_IRT(ir, IR_PR))->prno)
#define PR_ssainfo(ir)		(((CPR*)CK_IRT(ir, IR_PR))->ssainfo)
#define PR_du(ir)			(((CPR*)CK_IRT(ir, IR_PR))->du)
class CPR : public DU_PROP {
public:
	UINT prno; //PR number.

	//versioned presentation or ssa def and use list in ssa mode.
	//Note this field only avaiable if SSA information is maintained.
	SSAINFO * ssainfo;
};


//This class represent true branch operation.
//Branch if determinant express is true, otherwise control flow does not change.

//NOTE: the lay out of truebr should same with falsebr.
#define BR_bb(ir)			(((CTRUEBR*)CK_IRT_BR(ir))->bb)
#define BR_lab(ir)			(((CTRUEBR*)CK_IRT_BR(ir))->jump_target_lab)

//Determinant expression.
#define BR_det(ir)			(((CTRUEBR*)ir)->opnd[CKID_BR(ir, 0)])
#define BR_kid(ir, idx)		(((CTRUEBR*)ir)->opnd[CKID_BR(ir, idx)])
class CTRUEBR : public IR, public STMT_PROP {
public:
	IR * opnd[1];
	LABEL_INFO * jump_target_lab; //jump target label.
};


//This class represent false branch operation.
//Branch if determinant express is false, otherwise control flow does not change.
//Also use BR_det, BR_lab access.
//NOTE: the lay out of truebr should same with falsebr.
class CFALSEBR : public CTRUEBR {
public:
};


//This class represent function return operation.
//Return value expressions list.
//usage: return a, b, c;  a, b, c are return value expressions.
#define RET_bb(ir)			(((CRET*)CK_IRT(ir, IR_RETURN))->bb)
#define RET_exp(ir)			(((CRET*)ir)->opnd[CKID_TY(ir, IR_RETURN, 0)])
#define RET_kid(ir, idx)	(((CRET*)ir)->opnd[CKID_TY(ir, IR_RETURN, idx)])
class CRET : public IR, public STMT_PROP {
public:
	IR * opnd[1];
};


/* This class represent conditional select operation.
usage: res = select(a > b), (10), (20)
	means:
	if (a > b) res = 10;
	else res = 20;

This operation compute the value accroding to the result of
determinate expression, if the result value is true, return
SELECT_trueexp, otherwise return SELECT_falseexp. */
//Determinate expression.
#define SELECT_det(ir)			(((CSELECT*)ir)->opnd[CKID_TY(ir, IR_SELECT, 0)])

//True part
#define SELECT_trueexp(ir)		(((CSELECT*)ir)->opnd[CKID_TY(ir, IR_SELECT, 1)])
#define SELECT_falseexp(ir)	(((CSELECT*)ir)->opnd[CKID_TY(ir, IR_SELECT, 2)])
#define SELECT_kid(ir, idx)	(((CSELECT*)ir)->opnd[CKID_TY(ir, IR_SELECT, idx)])
class CSELECT : public IR {
public:
	IR * opnd[3];
};


//This class represent high level control structure, that
//terminate current loop execution immediately without any
//other operations.
//This operation is used by do-loop, do-while, while-do.
class CBREAK : public IR {};


//This class represent high level control structure, that
//re-execute current loop immediately without any
//other operations.
//This operation is used by do-loop, do-while, while-do.
class CCONTINUE : public IR {};


//This class represent phi operation.
#define PHI_bb(ir)			(((CPHI*)CK_IRT(ir, IR_PHI))->bb)
#define PHI_prno(ir)		(((CPHI*)CK_IRT(ir, IR_PHI))->prno)
#define PHI_ssainfo(ir)		(((CPHI*)CK_IRT(ir, IR_PHI))->ssainfo)
#define PHI_opnd_list(ir)	(((CPHI*)ir)->opnd[CKID_TY(ir, IR_PHI, 0)])
#define PHI_kid(ir, idx)	(((CPHI*)ir)->opnd[CKID_TY(ir, IR_PHI, idx)])
class CPHI : public DU_PROP, public STMT_PROP {
public:
	UINT prno; //PR number.
	SSAINFO * ssainfo; //Present ssa def and use set.
	IR * opnd[1];
};


//This class represent region operation.
//NOTE: If region is in BB, it must be single entry, single exit, since
//it might be reduced from reducible graph.
#define REGION_bb(ir)			(((CREGION*)CK_IRT(ir, IR_REGION))->bb)
#define REGION_ru(ir)			(((CREGION*)CK_IRT(ir, IR_REGION))->ru)
class CREGION : public IR, public STMT_PROP {
public:
	REGION * ru;
};


//Exported Functions

//
//START IR
//
IR * IR::get_rhs() const
{
	switch (IR_type(this)) {
	case IR_ST: return ST_rhs(this);
	case IR_STPR: return STPR_rhs(this);
	case IR_IST: return IST_rhs(this);
	default: IS_TRUE(0, ("not store operation."));
	}
	return NULL;
}


UINT IR::get_prno() const
{
	switch (IR_type(this)) {
	case IR_PR: return PR_no(this);
	case IR_STPR: return STPR_no(this);
	case IR_GETEPR: return GETEPR_no(this);
	case IR_SETEPR: return SETEPR_no(this);
	case IR_CALL:
	case IR_ICALL: return CALL_prno(this);
	case IR_PHI: return PHI_prno(this);
	default: IS_TRUE0(0);
	}
	return 0;
}


SSAINFO * IR::get_ssainfo() const
{
	switch (IR_type(this)) {
	case IR_PR: return PR_ssainfo(this);
	case IR_STPR: return STPR_ssainfo(this);
	case IR_PHI: return PHI_ssainfo(this);
	case IR_GETEPR: return GETEPR_ssainfo(this);
	case IR_SETEPR: return SETEPR_ssainfo(this);
	case IR_CALL:
	case IR_ICALL: return CALL_ssainfo(this);
	default: IS_TRUE0(0);
	}
	return NULL;
}


IR * IR::get_kid(UINT idx) const
{
	switch (IR_type(this)) {
	case IR_UNDEF: IS_TRUE(0, ("ir should not be undef")); break;
	case IR_CONST:
	case IR_ID:
	case IR_LD: return NULL;
	case IR_ST: return ST_kid(this, idx);
	case IR_STPR: return STPR_kid(this, idx);
	case IR_SETEPR: return SETEPR_kid(this, idx);
	case IR_GETEPR: return GETEPR_kid(this, idx);
	case IR_ILD: return ILD_kid(this, idx);
	case IR_IST: return IST_kid(this, idx);
	case IR_LDA: return LDA_kid(this, idx);
	case IR_CALL: return CALL_kid(this, idx);
	case IR_ICALL: return ICALL_kid(this, idx);
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
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
	case IR_BAND:
	case IR_BOR: return BIN_kid(this, idx);
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG: return UNA_kid(this, idx);
	case IR_GOTO: return NULL;
	case IR_IGOTO: return IGOTO_kid(this, idx);
	case IR_DO_WHILE:
	case IR_WHILE_DO: return LOOP_kid(this, idx);
	case IR_DO_LOOP: return DOLOOP_kid(this, idx);
	case IR_IF: return IF_kid(this, idx);
	case IR_LABEL: return NULL;
	case IR_SWITCH: return SWITCH_kid(this, idx);
	case IR_CASE: return CASE_kid(this, idx);
	case IR_ARRAY: return ARR_kid(this, idx);
	case IR_CVT: return CVT_kid(this, idx);
	case IR_PR: return NULL;
	case IR_TRUEBR:
	case IR_FALSEBR: return BR_kid(this, idx);
	case IR_RETURN: return RET_kid(this, idx);
	case IR_SELECT: return SELECT_kid(this, idx);
	case IR_BREAK:
	case IR_CONTINUE: return NULL;
	case IR_PHI: return PHI_kid(this, idx);
	case IR_REGION: return NULL;
	default: IS_TRUE(0, ("Unknown IR type"));
	}
	return NULL;
}


IR_BB * IR::get_bb() const
{
	switch (IR_type(this)) {
	case IR_ST: return ST_bb(this);
	case IR_STPR: return STPR_bb(this);
	case IR_SETEPR: return SETEPR_bb(this);
	case IR_GETEPR: return GETEPR_bb(this);
	case IR_IST: return IST_bb(this);
	case IR_CALL:
	case IR_ICALL: return CALL_bb(this);
	case IR_GOTO: return GOTO_bb(this);
	case IR_IGOTO: return IGOTO_bb(this);
	case IR_SWITCH: return SWITCH_bb(this);
	case IR_TRUEBR:
	case IR_FALSEBR: return BR_bb(this);
	case IR_RETURN: return RET_bb(this);
	case IR_PHI: return PHI_bb(this);
	case IR_REGION: return REGION_bb(this);
	default: IS_TRUE(0, ("Not stmt type"));
	}
	return NULL;
}


DU * IR::get_du() const
{
	switch (IR_type(this)) {
	case IR_ID:
	case IR_LD:
	case IR_ILD:
	case IR_PR:
	case IR_ARRAY:
	case IR_ST:
	case IR_STPR:
	case IR_SETEPR:
	case IR_GETEPR:
	case IR_IST:
	case IR_CALL:
	case IR_ICALL:
	case IR_PHI:
		return DU_PROP_du(this);
	default:;
	}
	return NULL;
}


void IR::set_du(DU * du)
{
	#ifdef _DEBUG_
	switch (IR_type(this)) {
	case IR_ID:
	case IR_LD:
	case IR_ILD:
	case IR_PR:
	case IR_ARRAY:
	case IR_ST:
	case IR_STPR:
	case IR_SETEPR:
	case IR_GETEPR:
	case IR_IST:
	case IR_CALL:
	case IR_ICALL:
	case IR_PHI:
		break;
	default: IS_TRUE0(0);
	}
	#endif
	DU_PROP_du(this) = du;
}


UINT IR::get_ofst() const
{
	switch (IR_type(this)) {
	case IR_LD: return LD_ofst(this);
	case IR_ILD: return ILD_ofst(this);
	case IR_ARRAY: return ARR_ofst(this);
	case IR_ST: return ST_ofst(this);
	case IR_IST: return IST_ofst(this);
	case IR_LDA: return LDA_ofst(this);
	default: return 0;
	}
	return 0;
}


//Return label or NULL.
LABEL_INFO * IR::get_label() const
{
	switch (IR_type(this)) {
	case IR_TRUEBR:
	case IR_FALSEBR: return BR_lab(this);
	case IR_GOTO: return GOTO_lab(this);
	case IR_IGOTO:
		IS_TRUE(0, ("must specify the specific target label"));
		return NULL;
	case IR_LABEL: return LAB_lab(this);
	case IR_CASE: return CASE_lab(this);
	case IR_SWITCH: return SWITCH_deflab(this);
	default: IS_TRUE(0, ("%s has not label", IRTNAME(IR_type(this))));
	}
	return NULL;
}


UINT IR::get_array_elem_dt_size(DT_MGR const* dm) const
{
	IS_TRUE0(IR_type(this) == IR_ARRAY);
	return dm->get_dtd_bytesize(dm->get_dtd(ARR_elem_ty(this)));
}


bool IR::is_const_exp() const
{
	if (IR_type(this) == IR_CONST) { return true; }
	if (IR_type(this) == IR_CVT) { return CVT_exp(this)->is_const_exp(); }
	return false;
}


bool IR::is_readonly_exp() const
{
	IS_TRUE0(is_exp());
	switch (IR_type(this)) {
	case IR_CONST: return true;
	case IR_CVT: return CVT_exp(this)->is_readonly_exp();
	case IR_LD:
		if (VAR_is_readonly(LD_idinfo(this)) &&
			!VAR_is_volatile(LD_idinfo(this))) {
			return true;
		}
		return false;
	case IR_LDA: return true;
	default:;
	}
	return false;
}


bool IR::is_readonly_call() const
{
	switch (IR_type(this)) {
	case IR_CALL: return CALL_is_readonly(this);
	case IR_ICALL: return ICALL_is_readonly(this);
	default: IS_TRUE(0, ("not a call"));
	}
	return false;
}


bool IR::is_volatile() const
{
	//Describing if IR's address has been taken.
	if (IR_type(this) == IR_ID) {
		VAR * id_info = ID_info(this);
		IS_TRUE0(id_info != NULL);
		return VAR_is_volatile(id_info);
	}
	return false;
}


//True if memory operation is store to array.
bool IR::is_st_array() const
{
	return IR_type(this) == IR_IST && IR_type(IST_base(this)) == IR_ARRAY;
}


bool IR::is_direct_array_ref() const
{
	return IR_type(this) == IR_ARRAY &&
		   IR_type(ARR_base(this)) == IR_LDA &&
		   IR_type(LDA_base(ARR_base(this))) != IR_ID;
}


void IR::set_bb(IR_BB * bb)
{
	switch (IR_type(this)) {
	case IR_ST: ST_bb(this) = bb; return;
	case IR_STPR: STPR_bb(this) = bb; return;
	case IR_SETEPR: SETEPR_bb(this) = bb; return;
	case IR_GETEPR: GETEPR_bb(this) = bb; return;
	case IR_IST: IST_bb(this) = bb; return;
	case IR_CALL:
	case IR_ICALL: CALL_bb(this) = bb; return;
	case IR_GOTO: GOTO_bb(this) = bb; return;
	case IR_IGOTO: IGOTO_bb(this) = bb; return;
	case IR_SWITCH: SWITCH_bb(this) = bb; return;
	case IR_TRUEBR:
	case IR_FALSEBR: BR_bb(this) = bb; return;
	case IR_RETURN: RET_bb(this) = bb; return;
	case IR_PHI: PHI_bb(this) = bb; return;
	case IR_REGION: REGION_bb(this) = bb; return;
	default:
		//Do not assert to facilitate coding. Just return.
		//IS_TRUE(0, ("Not stmt type"));
		return;
	}
}


void IR::set_rhs(IR * rhs)
{
	switch (IR_type(this)) {
	case IR_ST: ST_rhs(this) = rhs; return;
	case IR_STPR: STPR_rhs(this) = rhs; return;
	case IR_SETEPR: SETEPR_rhs(this) = rhs; return;
	case IR_IST: IST_rhs(this) = rhs; return;
	default: IS_TRUE(0, ("not store operation."));
	}
}


void IR::set_ofst(UINT ofst)
{
	switch (IR_type(this)) {
	case IR_LD: LD_ofst(this) = ofst; return;
	case IR_ST: ST_ofst(this) = ofst; return;
	case IR_ILD: ILD_ofst(this) = ofst; return;
	case IR_IST: IST_ofst(this) = ofst; return;
	case IR_LDA: LDA_ofst(this) = ofst; return;
	case IR_ARRAY: ARR_ofst(this) = ofst; return;
	default: return;
	}
}


void IR::set_ssainfo(SSAINFO * ssa)
{
	switch (IR_type(this)) {
	case IR_PR: PR_ssainfo(this) = ssa; return;
	case IR_STPR: STPR_ssainfo(this) = ssa; return;
	case IR_SETEPR: SETEPR_ssainfo(this) = ssa; return;
	case IR_GETEPR: GETEPR_ssainfo(this) = ssa; return;
	case IR_PHI: PHI_ssainfo(this) = ssa; return;
	default: IS_TRUE(0, ("unsupport"));
	}
}


void IR::set_prno(UINT prno)
{
	switch (IR_type(this)) {
	case IR_PR: PR_no(this) = prno; return;
	case IR_STPR: STPR_no(this) = prno; return;
	case IR_SETEPR: SETEPR_no(this) = prno; return;
	case IR_GETEPR: GETEPR_no(this) = prno; return;
	case IR_CALL: CALL_prno(this) = prno; return;
	case IR_ICALL: CALL_prno(this) = prno; return;
	case IR_PHI: PHI_prno(this) = prno; return;
	default: IS_TRUE(0, ("unsupport"));
	}
}


//Return label or NULL.
void IR::set_label(LABEL_INFO * li)
{
	switch (IR_type(this)) {
	case IR_TRUEBR:
	case IR_FALSEBR: BR_lab(this) = li; return;
	case IR_GOTO: GOTO_lab(this) = li; return;
	case IR_IGOTO:
		IS_TRUE(0, ("must specify the specific target label."));
		return;
	case IR_LABEL: LAB_lab(this) = li; return;
	case IR_CASE: CASE_lab(this) = li; return;
	case IR_SWITCH: SWITCH_deflab(this) = li; return;
	default: IS_TRUE(0, ("%s has not label", IRTNAME(IR_type(this))));
	}
}


//Set the No.idx child to be 'kid', and update the IR_parent of kid.
void IR::set_kid(UINT idx, IR * kid)
{
	switch (IR_type(this)) {
	case IR_UNDEF: IS_TRUE(0, ("ir should not be undef")); return;
	case IR_CONST:
	case IR_ID:
	case IR_LD: return;
	case IR_ST: ST_kid(this, idx) = kid; break;
	case IR_STPR: STPR_kid(this, idx) = kid; break;
	case IR_SETEPR: SETEPR_kid(this, idx) = kid; break;
	case IR_GETEPR: GETEPR_kid(this, idx) = kid; break;
	case IR_ILD: ILD_kid(this, idx) = kid; break;
	case IR_IST: IST_kid(this, idx) = kid; break;
	case IR_LDA: LDA_kid(this, idx) = kid; break;
	case IR_CALL: CALL_kid(this, idx) = kid; break;
	case IR_ICALL: ICALL_kid(this, idx) = kid; break;
	case IR_ADD:
	case IR_SUB:
	case IR_MUL:
	case IR_DIV:
	case IR_REM:
	case IR_MOD:
	case IR_LAND:
	case IR_LOR:
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
	case IR_BAND:
	case IR_BOR: BIN_kid(this, idx) = kid; break;
	case IR_BNOT:
	case IR_LNOT:
	case IR_NEG: UNA_kid(this, idx) = kid; break;
	case IR_GOTO: return;
	case IR_IGOTO: IGOTO_kid(this, idx) = kid; break;
	case IR_DO_WHILE:
	case IR_WHILE_DO: LOOP_kid(this, idx) = kid; break;
	case IR_DO_LOOP: DOLOOP_kid(this, idx) = kid; break;
	case IR_IF: IF_kid(this, idx) = kid; break;
	case IR_LABEL: return;
	case IR_SWITCH: SWITCH_kid(this, idx) = kid; break;
	case IR_CASE: CASE_kid(this, idx) = kid; break;
	case IR_ARRAY: ARR_kid(this, idx) = kid; break;
	case IR_CVT: CVT_kid(this, idx) = kid; break;
	case IR_PR: return;
	case IR_TRUEBR:
	case IR_FALSEBR: BR_kid(this, idx) = kid; break;
	case IR_RETURN: RET_kid(this, idx) = kid; break;
	case IR_SELECT: SELECT_kid(this, idx) = kid; break;
	case IR_BREAK:
	case IR_CONTINUE: return;
	case IR_PHI: PHI_kid(this, idx) = kid; break;
	case IR_REGION: return;
	default: IS_TRUE(0, ("Unknown IR type"));
	}
	for (IR * k = kid; k != NULL; k = IR_next(k)) {
		IR_parent(k) = this;
	}
}


bool IR::is_pr_equal(IR const* src) const
{
	IS_TRUE0(is_write_pr() && src->is_read_pr());
	return IR_dt(this) == IR_dt(src) && get_prno() == src->get_prno();
}


bool IR::is_rmw() const
{
	if (IR_is_read_mod_write(this)) {
		IS_TRUE0(IR_type(this) == IR_CALL &&
		 		 CALL_param_list(this) != NULL &&
				 IR_type(CALL_param_list(this)) == IR_ID &&
				 IR_next(CALL_param_list(this)) != NULL &&
				 (IR_type(IR_next(CALL_param_list(this))) == IR_LD ||
				  IR_next(CALL_param_list(this))->is_pr() ||
				  IR_next(CALL_param_list(this))->is_const()) &&
				 CALL_prno(this) != 0);
		return true;
	}
	return false;
}


/* Check if 'exp' is child or grandchildren of current ir.
Here we only compare equality of two IR pointer to determine and apply
the DFS searching in tree. */
bool IR::is_kids(IR const* exp) const
{
	if (exp == NULL) return false;
	IR * tmp;
	for (INT i = 0; i < IR_MAX_KID_NUM(this); i++) {
		tmp = get_kid(i);
		while (tmp != NULL) {
			if (exp == tmp) {
				return true;
			}
			if (tmp->is_kids(exp)) {
				return true;
			}
			tmp = IR_next(tmp);
		} //end while
	} //end for
	return false;
}


//Return true if k is the kid node of current ir.
bool IR::is_lhs(IR const* k) const
{
	IS_TRUE0(is_stmt());
	switch (IR_type(this)) {
	case IR_ST:
	case IR_STPR:
	case IR_SETEPR:
	case IR_GETEPR:
	case IR_CALL:
	case IR_ICALL:
		return k == this;
	case IR_IST:
		if (IR_type(k) == IR_ARRAY && k == IST_base(this)) {
			return true;
		}
		return false;
	case IR_GOTO:
	case IR_IGOTO:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_CASE:
	case IR_SWITCH:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_RETURN:
	case IR_BREAK:
	case IR_CONTINUE:
		return false;
	case IR_PHI:
		return k == this;
	case IR_REGION:
		return false;
	default: IS_TRUE0(0);
	} //end switch
	return false;
}


//Return true if ir exactly modified 'md' or element in MD_SET 'mds'.
bool IR::is_exact_def(MD const* md, MD_SET const* mds) const
{
	IS_TRUE0(is_stmt());

	MD const* cur_ir_defined_md = get_ref_md();

	if (cur_ir_defined_md != NULL && cur_ir_defined_md->is_exact()) {
		if (cur_ir_defined_md == md || cur_ir_defined_md->is_overlap(md)) {
			return true;
		}

		if (mds != NULL && mds->is_contain_pure(MD_id(cur_ir_defined_md))) {
			return true;
		}
	}

	//We can not determine whether current ir is
	//exactly modified md or mds.
	return false;
}


bool IR::is_exact_def(MD const* md) const
{
	IS_TRUE0(is_stmt() && md);
	if (!md->is_exact()) { return false; }

	MD const* cur_ir_defined_md = get_ref_md();
	if (cur_ir_defined_md != NULL &&
		cur_ir_defined_md->is_exact() &&
		(cur_ir_defined_md == md || cur_ir_defined_md->is_overlap(md))) {
		return true;
	}
	return false;
}


//Set ir DU to be NULL, return the DU pointer.
DU * IR::clean_du()
{
	switch (IR_type(this)) {
	case IR_ID:
	case IR_LD:
	case IR_ILD:
	case IR_PR:
	case IR_ARRAY:
	case IR_ST:
	case IR_STPR:
	case IR_SETEPR:
	case IR_GETEPR:
	case IR_IST:
	case IR_CALL:
	case IR_ICALL:
		{
			DU * du = DU_PROP_du(this);
			DU_PROP_du(this) = NULL;
			return du;
		}
	default:;
	}
	return NULL;
}


//Return pr or a list or pr if ir's result are PRs.
IR * IR::get_pr_results()
{
	IS_TRUE0(is_stmt());
	switch (IR_type(this)) {
	case IR_ST: return NULL;
	case IR_STPR:
	case IR_SETEPR:
	case IR_GETEPR:
	case IR_CALL:
	case IR_ICALL:
		return this;
	case IR_IST: return NULL;
	case IR_GOTO:
	case IR_IGOTO:
	case IR_DO_WHILE:
	case IR_WHILE_DO:
	case IR_DO_LOOP:
	case IR_IF:
	case IR_LABEL:
	case IR_SWITCH:
	case IR_TRUEBR:
	case IR_FALSEBR:
	case IR_RETURN:
	case IR_BREAK:
	case IR_CONTINUE:
		return NULL;
	case IR_PHI:
		return this;
	case IR_REGION:
		return NULL;
	default: IS_TRUE0(0);
	}
	return NULL;
}


//Return true if ir is call and does have a return value.
bool IR::has_return_val() const
{
	return CALL_prno(this) != 0;
}
//END IR


//Exported Functions.
void dump_ir(IR const* ir, DT_MGR const* dm, CHAR * attr = NULL,
			 bool dump_kid = true, bool dump_src_line = true,
			 bool dump_addr = false);
void dump_irs_h(IR * ir_list , DT_MGR const* dm);
void dump_irs(IR * ir_list, DT_MGR const* dm, CHAR * attr = NULL);
void dump_irs(IR_LIST & ir_list, DT_MGR const* dm);
void dump_irs(LIST<IR*> & ir_list, DT_MGR const* dm);
bool verify_irs(IR * ir, IR_ADDR_HASH * irh, DT_MGR const* dm);
bool verify_ir_and_bb(IR_BB_LIST * ir_bb_list, DT_MGR const* dm);
bool verify_simp(IR * ir, SIMP_CTX & simp);
bool verify_lowest_form(IR_BB_LIST * ir_bb_list, REGION * ru);


/* Iterative access ir tree. This funtion initialize the iterator.
'ir': the root ir of the tree.
'ii': iterator.
Readonly function. */
inline IR const* ir_iter_init_c(IR const* ir, OUT CIR_ITER & ii)
{
	if (ir == NULL) { return NULL; }
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid == NULL) { continue; }
		ii.append_tail(kid);
	}
	if (IR_next(ir) != NULL) {
		ii.append_tail(IR_next(ir));
	}
	return ir;
}


/* Iterative access ir tree.
This function return the next IR node accroding to 'ii'.
'ii': iterator.
Readonly function. */
inline IR const* ir_iter_next_c(IN OUT CIR_ITER & ii)
{
	IR const* ir = ii.remove_head();
	if (ir == NULL) { return NULL; }
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid == NULL) { continue; }
		ii.append_tail(kid);
	}
	if (IR_next(ir) != NULL) {
		ii.append_tail(IR_next(ir));
	}
	return ir;
}


/* Iterative access the right-hand-side expression of stmt.
This funtion initialize the iterator.
'ir': the root ir of the tree, it must be stmt.
'ii': iterator.

This function is a readonly function.

Use ir_iter_rhs_next_c to iter next IR. */
inline IR const* ir_iter_rhs_init_c(IR const* ir, OUT CIR_ITER & ii)
{
	if (ir == NULL) { return NULL; }
	IS_TRUE0(ir->is_stmt());
	if (ir->is_call()) {
		if (CALL_param_list(ir) != NULL) {
			ii.append_tail(CALL_param_list(ir));
		}
		if (IR_type(ir) == IR_ICALL) {
			ii.append_tail(ICALL_callee(ir));
		}
	} else if (ir->is_st_array()) {
		IR * arr = IST_base(ir);
		for (IR * sub = ARR_sub_list(arr); sub != NULL; sub = IR_next(sub)) {
			ii.append_tail(sub);
		}
		IS_TRUE(ARR_base(arr), ("array must have base"));
		ii.append_tail(ARR_base(arr));
		ii.append_tail(IST_rhs(ir));
	} else {
		//Other stmt.
		for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
			IR * kid = ir->get_kid(i);
			if (kid == NULL) { continue; }
			ii.append_tail(kid);
		}
	}

	IR const* x = ii.remove_head();
	if (x == NULL) { return NULL; }
	for (UINT i = 0; i < IR_MAX_KID_NUM(x); i++) {
		IR * kid = x->get_kid(i);
		if (kid == NULL) { continue; }
		ii.append_tail(kid);
	}
	if (IR_next(x) != NULL) {
		ii.append_tail(IR_next(x));
	}
	return x;
}


/* Iterative access the expression.
This funtion initialize the iterator.
'ir': the root ir of the tree, it must be expression.
'ii': iterator.
Readonly function.
Use ir_iter_rhs_next_c to iter next IR. */
inline IR const* ir_iter_exp_init_c(IR const* ir, OUT CIR_ITER & ii)
{
	if (ir == NULL) { return NULL; }
	IS_TRUE0(ir->is_exp());
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid == NULL) { continue; }
		ii.append_tail(kid);
	}
	if (IR_next(ir) != NULL) {
		IS_TRUE(!IR_next(ir)->is_stmt(), ("ir can not be stmt list"));
		ii.append_tail(IR_next(ir));
	}
	return ir;
}


/* Iterative access the right-hand-side expression of stmt.
This function return the next IR node accroding to 'ii'.
'ii': iterator.
Readonly function. */
inline IR const* ir_iter_rhs_next_c(IN OUT CIR_ITER & ii)
{
	return ir_iter_next_c(ii);
}


/* Iterative access the ir tree that start with 'ir'.
This funtion initialize the iterator.
'ir': the root ir of the tree, it may be either stmt or expression.
'ii': iterator.
Note this function is NOT readonly, the returnd IR may be modified. */
inline IR * ir_iter_init(IN IR * ir, OUT IR_ITER & ii)
{
	if (ir == NULL) { return NULL; }
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid == NULL) { continue; }
		ii.append_tail(kid);
	}
	if (IR_next(ir) != NULL) {
		ii.append_tail(IR_next(ir));
	}
	return ir;
}


/* Iterative access the ir tree.
This funtion return the next IR node accroding to 'ii'.
'ii': iterator.
Note this function is NOT readonly, the returnd IR may be modified. */
inline IR * ir_iter_next(IN OUT IR_ITER & ii)
{
	IR * ir = ii.remove_head();
	if (ir == NULL) { return NULL; }
	for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
		IR * kid = ir->get_kid(i);
		if (kid == NULL) { continue; }
		ii.append_tail(kid);
	}
	if (IR_next(ir) != NULL) {
		ii.append_tail(IR_next(ir));
	}
	return ir;
}


/* Iterative access the right-hand-side expression of stmt.
This funtion initialize the iterator.
'ir': the root ir of the tree, it must be stmt.
'ii': iterator.

Use ir_iter_rhs_next_c to iter next IR. */
inline IR * ir_iter_rhs_init(IR * ir, OUT IR_ITER & ii)
{
	if (ir == NULL) { return NULL; }
	IS_TRUE0(ir->is_stmt());
	if (ir->is_call()) {
		if (CALL_param_list(ir) != NULL) {
			ii.append_tail(CALL_param_list(ir));
		}
		if (IR_type(ir) == IR_ICALL) {
			ii.append_tail(ICALL_callee(ir));
		}
	} else if (ir->is_st_array()) {
		IR * arr = IST_base(ir);
		IS_TRUE0(ARR_sub_list(arr));
		ii.append_tail(ARR_sub_list(arr));
		IS_TRUE(ARR_base(arr), ("array must have base"));
		ii.append_tail(ARR_base(arr));
		ii.append_tail(IST_rhs(ir));
	} else {
		//Other stmt.
		for (UINT i = 0; i < IR_MAX_KID_NUM(ir); i++) {
			IR * kid = ir->get_kid(i);
			if (kid == NULL) { continue; }
			ii.append_tail(kid);
		}
	}

	IR * x = ii.remove_head();
	if (x == NULL) { return NULL; }
	for (UINT i = 0; i < IR_MAX_KID_NUM(x); i++) {
		IR * kid = x->get_kid(i);
		if (kid == NULL) { continue; }
		ii.append_tail(kid);
	}
	if (IR_next(x) != NULL) {
		ii.append_tail(IR_next(x));
	}
	return x;
}


/* Iterative access the right-hand-side expression of stmt.
This function return the next IR node accroding to 'ii'.
'ii': iterator.
Readonly function. */
inline IR * ir_iter_rhs_next(IN OUT IR_ITER & ii)
{
	return ir_iter_next(ii);
}


inline bool is_commutative(IR_TYPE irt)
{ return IRDES_is_commutative(g_ir_desc[irt]); }

inline bool is_bin_irt(IR_TYPE irt)
{ return IRDES_is_bin(g_ir_desc[irt]); }

inline bool is_una_irt(IR_TYPE irt)
{ return IRDES_is_una(g_ir_desc[irt]); }

void irs_set_parent_pointer(IR * ir_list);
UINT get_arth_precedence(IR_TYPE ty);
#endif
