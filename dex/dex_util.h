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
#ifndef _DEX_UTIL_H_
#define _DEX_UTIL_H_

class Dex2IR;

//
//START DexRegion
//
class DexRegion : public Region {
protected:
	VAR2PR m_var2pr; //map from var id to prno.

protected:
	bool is_64bit(IR const* ir)
	{ return get_dm()->get_bytesize(IR_dt(ir))== 8; }

public:
	DexRegion(REGION_TYPE rt, RegionMgr * rm) : Region(rt, rm) {}
	virtual ~DexRegion() {}

	IR * gen_and_add_sib(IR * ir, UINT prno);
	
	virtual bool HighProcess(OptCTX & oc);

	virtual PassMgr * newPassMgr();
	virtual IR_AA * newAliasAnalysis();

	void updateRAresult(RA & ra, Prno2UINT & prno2v);

	bool verifyRAresult(RA & ra, Prno2UINT & prno2v);

	void process_group_bb(IRBB * bb, List<IR*> & lst);
	void process_group();
	void processSimply(OUT Prno2UINT & prno2v, UINT param_num,
						UINT vregnum, Dex2IR & d2ir, UINT2PR * v2pr,
						IN Prno2UINT * pr2v, TypeIndexRep * tr);
	virtual void process(Prno2UINT & prno2v, UINT param_num, UINT vregnum,
						 UINT2PR * v2pr, Prno2UINT * pr2v, TypeIndexRep * tr);
	virtual void process() { ASSERT0(0); }
};
//END DexRegion


//
//START DexRegionMgr
//
class DexRegionMgr : public RegionMgr {
public:
	DexRegionMgr() : RegionMgr() {}
	virtual ~DexRegionMgr() {}
	virtual Region * newRegion(REGION_TYPE rt)
	{
		Region * ru = new DexRegion(rt, this);
		REGION_id(ru) = m_ru_count++;
		return ru;
	}
};
//END DexRegionMgr

#define OBJ_MC_SIZE			16
#define ARRAY_MC_SIZE		32

#define LIRC_num_of_op(l)	((l)->instrCount)
#define LIRC_op(l, i)		((l)->lirList[i])

#define LIR_opcode(ir)		((ir)->opcode)
#define LIR_name(ir)		(gLIROpcodeInfo.opNames[LIR_opcode(ir)])
#define LIR_dt(ir)			((ir)->flags)

//For CONST
#define LIR_int_imm(ir)		(((LIRConstOp*)ir)->vB)

//For SPUT
#define LIR_sym_ptr(ir)		(((LIRABOp*)ir)->ptr)
#define LIR_val(ir)			(((LIRABOp*)ir)->vA) //store value

//For ADD,SUB,MUL,DIV,REM,AND,OR,SHL,SHR,USHR,
//XOR,CONST,NEW_INSTANCE.
#define LIR_res(ir)				(((LIRABCOp*)ir)->vA)

//For ADD,SUB,MUL,DIV,REM,AND,OR,SHL,SHR,USHR,
//XOR,NEW_INSTANCE.
#define LIR_op0(ir)				(((LIRABCOp*)ir)->vB)

//For ADD,SUB,MUL,DIV,REM,AND,OR,SHL,SHR,USHR,XOR
#define LIR_op1(ir)				(((LIRABCOp*)ir)->vC)


#define LIR_fill_array_data_mark(ir) \
		(((USHORT*)(((LIRSwitchOp*)ir)->data))[0])
#define LIR_switch_kind(ir) \
		(((USHORT*)(((LIRSwitchOp*)ir)->data))[0])
#define LIR_case_num(ir) \
		(((USHORT*)(((LIRSwitchOp*)ir)->data))[1])
#define LIR_packed_switch_base_value(ir) \
		(*((INT*)(&(((LIRSwitchOp*)ir)->data[2]))))
#define LIR_packed_switch_case_entry(ir) \
		((UINT*)(((USHORT*)((LIRSwitchOp*)ir)->data) + 4))
#define LIR_sparse_switch_case_value(ir) \
		((UINT*)(((USHORT*)(((LIRSwitchOp*)ir)->data)) + 2))
#define LIR_sparse_switch_case_entry(ir) \
		(UINT*)&(((BYTE*)(((LIRSwitchOp*)ir)->data))[4 + LIR_case_num(ir) * 4])

typedef LIRBaseOp LIR;

//Backfill data.
class BackFillData {
public:
	IR * ir;
	LIR * lir;
};

//Export Functions.
void dump_lir(LIR * lir, DexFile * df, INT pos);
void dump_lir2(LIR * lir, DexFile * df, INT pos);
void dump_all_lir(LIRCode * fu, DexFile * df, DexMethod const* dm);
void dump_all_class_and_field(DexFile * df);
inline bool is_wide(LIR * lir)
{
	return LIR_dt(lir) == LIR_JDT_wide ||
			LIR_dt(lir) == LIR_JDT_double ||
			LIR_dt(lir) == LIR_JDT_long;
}

CHAR const* get_class_name(DexFile * df, DexMethod const* dm);
CHAR const* get_class_name(DexFile * df, UINT cls_type_idx);
CHAR const* get_class_name(DexFile * df, DexClassDef const* class_info);
CHAR const* get_func_name(DexFile * df, DexMethod const* dm);
CHAR const* get_field_name(DexFile * df, UINT field_id);
CHAR const* get_field_name(DexFile * df, DexField * finfo);
CHAR const* get_field_type_name(DexFile * df, UINT field_id);
CHAR const* get_field_type_name(DexFile * df, DexField * finfo);
CHAR const* get_field_class_name(DexFile * df, UINT field_id);
CHAR const* get_field_class_name(DexFile * df, DexField * finfo);
CHAR const* get_dt_name(LIR * ir);
#endif

