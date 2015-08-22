#if 0
#ifdef COMPILE_DEX2LEX

#include "utils/cbytestream.h"
#include "utils/clbe.h"
#include "xassert.h"
#include "drAlloc.h"
#include "anainterface.h"
#include "mem/cmem.h"
#include "dir.h"
#include "lircomm.h"
#include "liropcode.h"
#include "d2lcode.h"

#define lc_mov4_object lc_mov4
#define lc_getfield_object lc_getfield
#define lc_putfield_object lc_putfield
#define lc_getstatic_object lc_getstatic
#define lc_putstatic_object lc_putstatic
#define lc_mov_object lc_mov
#define lc_mov_result_object lc_mov_result32
#define lc_return_object lc_return32
#define lc_mov_16_object lc_mov_16
#define lc_aget_object lc_aget

typedef struct LexInstrStruct{
    BYTE* instrData;
    UInt32 instrSize;
    UInt32 instrOffset;
}LexInstr;

typedef struct LexInstrListStruct{
    UInt32 instrCount;
    LexInstr* instrList;
}LexInstrList;

typedef struct TargetInsnStruct{
    UInt32 num;
    UInt32 offset;
    UInt32 targetNum;
    UInt32 targetOffset;
    UInt32 size;
    UInt32 lastfixSize;
}TargetInsn;

#define WRITE_16(instrData,num) \
        *((UInt16*)instrData) = num;\
        instrData += 2;

#define WRITE_32(instrData,num) \
        WRITE_16(instrData,num&0xffff) \
        WRITE_16(instrData,(num >>16)&0xffff)

#define WRITE_FORMATAA(lexInstr,data) \
        {\
        lexInstr->instrSize = 2;\
        BYTE* instrData = (BYTE*)LIRMALLOC(2);\
        lexInstr->instrData = instrData; \
        ASSERT(unsignedFitsIn8(data));\
        WRITE_16(instrData,((UInt8)(data) << 8)|((UInt8)lexOpcode))\
        }

#define WRITE_FORMATAABBBB(lexInstr,dataA,dataB) \
        lexInstr->instrSize = 4;\
        BYTE* instrData = (BYTE*)LIRMALLOC(4);\
        lexInstr->instrData = instrData; \
        ASSERT(unsignedFitsIn8(dataA));\
        WRITE_16(instrData,((UInt8)(dataA) << 8)|((UInt8)lexOpcode))\
        WRITE_16(instrData, (UInt16)dataB);

#define WRITE_FORMATAABBBBBBBB(lexInstr,dataA,dataB) \
        lexInstr->instrSize = 6;\
        BYTE* instrData = (BYTE*)LIRMALLOC(6);\
        lexInstr->instrData = instrData; \
        ASSERT(unsignedFitsIn8(dataA));\
        WRITE_16(instrData,((UInt8)(dataA) << 8)|((UInt8)lexOpcode))\
        WRITE_32(instrData, (UInt32)dataB)

#define WRITE_FORMATAABBBBBBBBBBBBBBBB(lexInstr,dataA,dataB) \
        lexInstr->instrSize = 10; \
        BYTE* instrData = (BYTE*)LIRMALLOC(10);\
        lexInstr->instrData = instrData; \
        ASSERT(unsignedFitsIn8(dataA));\
        WRITE_16(instrData, ((UInt8)(dataA) << 8)|((UInt8)lexOpcode))\
        memcpy(instrData,&dataB,8);

#define WRITE_FORMATAABBCC(lexInstr,dataA,dataB,dataC) \
        lexInstr->instrSize = 4;\
        BYTE* instrData = (BYTE*)LIRMALLOC(4);\
        lexInstr->instrData = instrData; \
        ASSERT(unsignedFitsIn8(dataA));\
        ASSERT(unsignedFitsIn8(dataB));\
        ASSERT(unsignedFitsIn8(dataC));\
        WRITE_16(instrData,((UInt8)(dataA) << 8)|((UInt8)lexOpcode))\
        WRITE_16(instrData,((UInt8)(dataC) << 8)|((UInt8)dataB & 0xff))

#define WRITE_FORMATAABBCCLIT(lexInstr,dataA,dataB,dataC) \
        lexInstr->instrSize = 4;\
        BYTE* instrData = (BYTE*)LIRMALLOC(4);\
        lexInstr->instrData = instrData; \
        ASSERT(unsignedFitsIn8(dataA));\
        ASSERT(unsignedFitsIn8(dataB));\
        ASSERT(signedFitsIn8(dataC));\
        WRITE_16(instrData,((UInt8)(dataA) << 8)|((UInt8)lexOpcode))\
        WRITE_16(instrData,((UInt8)(dataC) << 8)|((UInt8)dataB & 0xff))

#define WRITE_FORMATAABBBBCCCC(lexInstr,dataA,dataB,dataC) \
        lexInstr->instrSize = 6;\
        BYTE* instrData = (BYTE*)LIRMALLOC(6);\
        lexInstr->instrData = instrData; \
        ASSERT(unsignedFitsIn8(dataA));\
        WRITE_16(instrData,((UInt8)(dataA) << 8)|((UInt8)lexOpcode))\
        WRITE_16(instrData, (UInt16)dataB);\
        WRITE_16(instrData, (UInt16)dataC);

#define WRITE_FORMAT00AAAABBBB(lexInstr,dataA,dataB) \
        WRITE_FORMATAABBBBCCCC(lexInstr, 0, dataA, dataB)

#define WRITE_FORMATABCCCCDDDD(lexInstr,dataA,dataB,dataC,dataD) \
        ASSERT(unsignedFitsIn4(dataA));\
        ASSERT(unsignedFitsIn4(dataB));\
        UInt8 va = (UInt8)dataA; \
        UInt8 vb = (UInt8)dataB; \
        WRITE_FORMATAABBBBCCCC(lexInstr, (vb << 4) | (va & 0x0f),dataC,dataD)

#define WRITE_FORMATAB(lexInstr,dataA,dataB) \
        ASSERT(unsignedFitsIn4(dataA));\
        ASSERT(unsignedFitsIn4(dataB));\
        UInt8 va = (UInt8)dataA; \
        UInt8 vb = (UInt8)dataB; \
        WRITE_FORMATAA(lexInstr, (vb << 4) | (va & 0x0f))

#define WRITE_FORMATAALIT(lexInstr,data) \
        {\
        lexInstr->instrSize = 2;\
        BYTE* instrData = (BYTE*)LIRMALLOC(2);\
        lexInstr->instrData = instrData; \
        WRITE_16(instrData,((UInt8)(data) << 8)|((UInt8)lexOpcode))\
        }

#define WRITE_FORMATABLIT(lexInstr,dataA,dataB) \
        ASSERT(unsignedFitsIn4(dataA));\
        ASSERT(signedFitsIn4(dataB));\
        UInt8 va = (UInt8)dataA; \
        UInt8 vb = (UInt8)dataB; \
        WRITE_FORMATAALIT(lexInstr, (vb << 4) | (va & 0x0f))

#define WRITE_FORMATABCCCC(lexInstr,dataA,dataB,dataC) \
        ASSERT(unsignedFitsIn4(dataA));\
        ASSERT(unsignedFitsIn4(dataB));\
         UInt8 va = (UInt8)dataA;\
        UInt8 vb = (UInt8)dataB;\
        WRITE_FORMATAABBBB(lexInstr, (vb << 4) | (va & 0x0f),dataC)

static inline UInt8 getlirOpcode(UInt32 codePtr){
    BYTE* data = (BYTE*)codePtr;
     return data[0];
}

static inline bool signedFitsIn16(Int32 value){
    return (Int16) value == value;
}

static inline bool unsignedFitsIn16(UInt32 value) {
    return value == (value & 0xffff);
}

static inline bool signedFitsIn8(Int32 value) {
    return (Int8) value == value;
}

static inline bool unsignedFitsIn8(UInt32 value) {
    return value == (value & 0xff);
}

static inline bool signedFitsIn4(Int32 value) {
    return (value >= -8) && (value <= 7);
}

static inline bool unsignedFitsIn4(UInt32 value) {
    return value == (value & 0xf);
}

static inline bool signedFitsInWide16(Int64 value){
    return (Int64)((Int16) value) == value;
}

static inline bool signedFitsInWide32(Int64 value){
    return (Int64)((Int32) value) == value;
}

static Int32 processLIRAOp(LexInstr* lexInstr,LIRAOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;

    switch(lir->opcode){
    case LOP_RETURN:{
        switch(lir->flags){
        case LIR_JDT_void:
            lexOpcode = lc_return;
            WRITE_FORMATAA(lexInstr,0)
            break;
        case 0:
            lexOpcode = lc_return32;
            WRITE_FORMATAA(lexInstr,lir->vA)
            break;
        case LIR_JDT_object:
             lexOpcode = lc_return_object;
             WRITE_FORMATAA(lexInstr,lir->vA)
             break;
        case LIR_JDT_wide:
             lexOpcode = lc_return64;
             WRITE_FORMATAA(lexInstr,lir->vA)
             break;
        }
        break;
    }
    case LOP_THROW:{
        lexOpcode = lc_throw;
        WRITE_FORMATAA(lexInstr,lir->vA)
        break;
    }
    case LOP_MONITOR_ENTER:{
        lexOpcode = lc_monitorenter;
        WRITE_FORMATAA(lexInstr,lir->vA)
        break;
    }
    case LOP_MONITOR_EXIT:{
        lexOpcode = lc_monitorexit;
        WRITE_FORMATAA(lexInstr,lir->vA)
        break;
    }
    case LOP_MOVE_RESULT:{
        switch(lir->flags)
        {
        case 0: lexOpcode = lc_mov_result32; break;
        case LIR_JDT_wide: lexOpcode = lc_mov_result64; break;
        case LIR_JDT_object: lexOpcode = lc_mov_result_object; break;
        }
        WRITE_FORMATAA(lexInstr,lir->vA)
        break;
    }
    case LOP_MOVE_EXCEPTION:{
        lexOpcode = lc_mov_exception;
        WRITE_FORMATAA(lexInstr,lir->vA)
        break;
    }
    }

    return err;
}

static Int32 genMov(LexInstr* lexInstr,LIRABOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;
    int destReg = lir->vA;
    int srcReg = lir->vB;

    switch(lir->flags){
        case 0:
            if ((srcReg | destReg) < 16) {
                lexOpcode = lc_mov4;
                WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
            } else if (destReg < 256) {
                lexOpcode = lc_mov;
                WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
            } else {
                lexOpcode = lc_mov_16;
                WRITE_FORMAT00AAAABBBB(lexInstr,lir->vA,lir->vB)
            }
            break;
        case LIR_JDT_wide:
            if ((srcReg | destReg) < 16) {
                lexOpcode = lc_mov4_w;
                WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
            } else if (destReg < 256) {
                lexOpcode = lc_mov_w;
                WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
            } else {
                lexOpcode = lc_mov_w_16;
                WRITE_FORMAT00AAAABBBB(lexInstr,lir->vA,lir->vB)
            }
            break;
        case LIR_JDT_object:
            if ((srcReg | destReg) < 16) {
                lexOpcode = lc_mov4_object;
                WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
            } else if (destReg < 256) {
                lexOpcode = lc_mov_object;
                WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
            } else {
                lexOpcode = lc_mov_16_object;
                WRITE_FORMAT00AAAABBBB(lexInstr,lir->vA,lir->vB)
            }
            break;
    }
    return err;
}

static Int32 processLIRABOp(LexInstr* lexInstr,LIRABOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;

    switch(lir->opcode){
    case LOP_MOVE:{
        genMov(lexInstr,lir);
        break;
    }
    case LOP_NEG:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ineg; break;
        case LIR_JDT_long: lexOpcode = lc_lneg; break;
        case LIR_JDT_float: lexOpcode = lc_fneg; break;
        case LIR_JDT_double: lexOpcode = lc_dneg; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_NOT:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_not_int; break;
        case LIR_JDT_long: lexOpcode = lc_not_long; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_CONVERT:{
        switch(lir->flags){
        case LIR_convert_i2l: lexOpcode = lc_i2l; break;
        case LIR_convert_i2f: lexOpcode = lc_i2f; break;
        case LIR_convert_i2d: lexOpcode = lc_i2d; break;
        case LIR_convert_l2i: lexOpcode = lc_l2i; break;
        case LIR_convert_l2f: lexOpcode = lc_l2f; break;
        case LIR_convert_l2d: lexOpcode = lc_l2d; break;
        case LIR_convert_f2i: lexOpcode = lc_f2i; break;
        case LIR_convert_f2l: lexOpcode = lc_f2l; break;
        case LIR_convert_f2d: lexOpcode = lc_f2d; break;
        case LIR_convert_d2i: lexOpcode = lc_d2i; break;
        case LIR_convert_d2l: lexOpcode = lc_d2l; break;
        case LIR_convert_d2f: lexOpcode = lc_d2f; break;
        case LIR_convert_i2b: lexOpcode = lc_i2b; break;
        case LIR_convert_i2c: lexOpcode = lc_i2c; break;
        case LIR_convert_i2s: lexOpcode = lc_i2s; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_ADD_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_iadd_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_ladd_2addr; break;
        case LIR_JDT_float: lexOpcode = lc_fadd_2addr; break;
        case LIR_JDT_double: lexOpcode = lc_dadd_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_SUB_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_isub_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lsub_2addr; break;
        case LIR_JDT_float: lexOpcode = lc_fsub_2addr; break;
        case LIR_JDT_double: lexOpcode = lc_dsub_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_MUL_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_imul_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lmul_2addr; break;
        case LIR_JDT_float: lexOpcode = lc_fmul_2addr; break;
        case LIR_JDT_double: lexOpcode = lc_dmul_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_DIV_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_idiv_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_ldiv_2addr; break;
        case LIR_JDT_float: lexOpcode = lc_fdiv_2addr; break;
        case LIR_JDT_double: lexOpcode = lc_ddiv_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_REM_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_irem_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lrem_2addr; break;
        case LIR_JDT_float: lexOpcode = lc_frem_2addr; break;
        case LIR_JDT_double: lexOpcode = lc_drem_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_AND_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_iand_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_land_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_OR_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ior_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lor_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_XOR_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ixor_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lxor_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_SHL_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ishl_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lshl_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_SHR_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ishr_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lshr_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_USHR_ASSIGN:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_iushr_2addr; break;
        case LIR_JDT_long: lexOpcode = lc_lushr_2addr; break;
        }
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_ARRAY_LENGTH:{
        lexOpcode = lc_arraylength;
        WRITE_FORMATAB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_NEW_INSTANCE:{
        lexOpcode = lc_new;
        WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_CONST_STRING:{

        if(unsignedFitsIn16(lir->vB)){
            lexOpcode = lc_const_str_short;
            WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
        }else{
            lexOpcode = lc_const_str;
            WRITE_FORMATAABBBBBBBB(lexInstr,lir->vA,lir->vB)
        }
        break;
    }
    case LOP_CONST_CLASS:{
        lexOpcode = lc_const_cls;
        WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_SGET:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_getstatic; break;
        case LIR_JDT_wide: lexOpcode = lc_getstatic_w; break;
        case LIR_JDT_object: lexOpcode = lc_getstatic_object; break;
        case LIR_JDT_boolean: lexOpcode = lc_getstatic; break;
        case LIR_JDT_byte: lexOpcode = lc_getstatic; break;
        case LIR_JDT_char: lexOpcode = lc_getstatic; break;
        case LIR_JDT_short: lexOpcode = lc_getstatic; break;
        }
        WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_CHECK_CAST:{
        lexOpcode = lc_checkcast;
        WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
        break;
    }
    case LOP_SPUT:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_putstatic; break;
        case LIR_JDT_wide: lexOpcode = lc_putstatic_w; break;
        case LIR_JDT_object: lexOpcode = lc_putstatic_object; break;
        case LIR_JDT_boolean: lexOpcode = lc_putstatic; break;
        case LIR_JDT_byte: lexOpcode = lc_putstatic; break;
        case LIR_JDT_char: lexOpcode = lc_putstatic; break;
        case LIR_JDT_short: lexOpcode = lc_putstatic; break;
        }
        WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
        break;
    }
    }

    return err;
}

static Int32 processLIRABCOp(LexInstr* lexInstr,LIRABCOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;

    switch(lir->opcode){
    case LOP_APUT:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_aput; break;
        case LIR_JDT_wide: lexOpcode = lc_aput_w; break;
        case LIR_JDT_object: lexOpcode = lc_aput_object; break;
        case LIR_JDT_boolean: lexOpcode = lc_aput_byte; break;
        case LIR_JDT_byte: lexOpcode = lc_aput_byte; break;
        case LIR_JDT_char: lexOpcode = lc_aput_char; break;
        case LIR_JDT_short: lexOpcode = lc_aput_short; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_AGET:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_aget; break;
        case LIR_JDT_wide: lexOpcode = lc_aget_w; break;
        case LIR_JDT_object: lexOpcode = lc_aget_object; break;
        case LIR_JDT_boolean: lexOpcode = lc_aget_byte; break;
        case LIR_JDT_byte: lexOpcode = lc_aget_byte; break;
        case LIR_JDT_char: lexOpcode = lc_aget_char; break;
        case LIR_JDT_short: lexOpcode = lc_aget_short; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_CMPL:{
        switch(lir->flags){
        case LIR_CMP_float: lexOpcode = lc_fcmpl; break;
        //case LIR_JDT_float: lexOpcode = lc_fcmpg; break;
        case LIR_CMP_double: lexOpcode = lc_dcmpl; break;
        //case LIR_JDT_double: lexOpcode = lc_dcmpg; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }

        /*add by gk*/
    case LOP_CMPG:{
        switch(lir->flags){
        case LIR_CMP_float: lexOpcode = lc_fcmpg; break;
        //case LIR_JDT_float: lexOpcode = lc_fcmpg; break;
        case LIR_CMP_double: lexOpcode = lc_dcmpg; break;
        //case LIR_JDT_double: lexOpcode = lc_dcmpg; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }

    case LOP_CMP_LONG:{
        lexOpcode = lc_lcmp;
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_ADD:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_iadd; break;
        case LIR_JDT_long: lexOpcode = lc_ladd; break;
        case LIR_JDT_float: lexOpcode = lc_fadd; break;
        case LIR_JDT_double: lexOpcode = lc_dadd; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_SUB:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_isub; break;
        case LIR_JDT_long: lexOpcode = lc_lsub; break;
        case LIR_JDT_float: lexOpcode = lc_fsub; break;
        case LIR_JDT_double: lexOpcode = lc_dsub; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_MUL:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_imul; break;
        case LIR_JDT_long: lexOpcode = lc_lmul; break;
        case LIR_JDT_float: lexOpcode = lc_fmul; break;
        case LIR_JDT_double: lexOpcode = lc_dmul; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_DIV:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_idiv; break;
        case LIR_JDT_long: lexOpcode = lc_ldiv; break;
        case LIR_JDT_float: lexOpcode = lc_fdiv; break;
        case LIR_JDT_double: lexOpcode = lc_ddiv; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_REM:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_irem; break;
        case LIR_JDT_long: lexOpcode = lc_lrem; break;
        case LIR_JDT_float: lexOpcode = lc_frem; break;
        case LIR_JDT_double: lexOpcode = lc_drem; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_AND:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_iand; break;
        case LIR_JDT_long: lexOpcode = lc_land; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_OR:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ior; break;
        case LIR_JDT_long: lexOpcode = lc_lor; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_XOR:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ixor; break;
        case LIR_JDT_long: lexOpcode = lc_lxor; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_SHL:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ishl; break;
        case LIR_JDT_long: lexOpcode = lc_lshl; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_SHR:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_ishr; break;
        case LIR_JDT_long: lexOpcode = lc_lshr; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_USHR:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_iushr; break;
        case LIR_JDT_long: lexOpcode = lc_lushr; break;
        }
        WRITE_FORMATAABBCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_ADD_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_add_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_add_int_lit16;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_SUB_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_rsub_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_rsub_int;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_MUL_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_mul_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_mul_int_lit16;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_DIV_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_div_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_div_int_lit16;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_REM_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_rem_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_rem_int_lit16;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_AND_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_and_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_and_int_lit16;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_OR_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_or_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_or_int_lit16;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_XOR_LIT:{
        if(signedFitsIn8(lir->vC)){
            lexOpcode = lc_xor_int_lit8;
            WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        }else{
            lexOpcode = lc_xor_int_lit16;
            WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        }
        break;
    }
    case LOP_SHL_LIT:{
        lexOpcode = lc_shl_int_lit8;
        WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_SHR_LIT:{
        lexOpcode = lc_shr_int_lit8;
        WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_USHR_LIT:{
        lexOpcode = lc_ushr_int_lit8;
        WRITE_FORMATAABBCCLIT(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_IPUT:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_putfield; break;
        case LIR_JDT_wide: lexOpcode = lc_putfield_w; break;
        case LIR_JDT_object: lexOpcode = lc_putfield_object; break;
        case LIR_JDT_boolean: lexOpcode = lc_putfield; break;
        case LIR_JDT_byte: lexOpcode = lc_putfield; break;
        case LIR_JDT_char: lexOpcode = lc_putfield; break;
        case LIR_JDT_short: lexOpcode = lc_putfield; break;
        }
        WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_IGET:{
        switch(lir->flags){
        case LIR_JDT_int: lexOpcode = lc_getfield; break;
        case LIR_JDT_wide: lexOpcode = lc_getfield_w; break;
        case LIR_JDT_object: lexOpcode = lc_getfield_object; break;
        case LIR_JDT_boolean: lexOpcode = lc_getfield; break;
        case LIR_JDT_byte: lexOpcode = lc_getfield; break;
        case LIR_JDT_char: lexOpcode = lc_getfield; break;
        case LIR_JDT_short: lexOpcode = lc_getfield; break;
        }
        WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_INSTANCE_OF:{
        lexOpcode = lc_instanceof;
        WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    case LOP_NEW_ARRAY:{
        lexOpcode = lc_newarray;
        WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)
        break;
    }
    }

    return err;
}

static Int32 processLIRSwitchOp(LexInstr* lexInstr,CBSHandle dataHandler,LIRSwitchOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode=0;
    UInt32 offset = 0;
    UInt16 signature=0;
    UInt16* data;
    UInt32 dataSize=0;

    data = lir->data;
    switch(lir->opcode){
    case LOP_TABLE_SWITCH:{
        lexOpcode = lc_tableswitch;
        signature = KVM_kPackedSwitchSignature;
        dataSize = 8 + data[1] * 4;
        break;
    }
    case LOP_LOOKUP_SWITCH:{
        lexOpcode = lc_lookupswitch;
        signature = KVM_kSparseSwitchSignature;
        dataSize = 4 + data[1] * 8;
        break;
    }
    case LOP_FILL_ARRAY_DATA:{
        lexOpcode = lc_fill_array_data;
        signature = KVM_kArrayDataSignature;
        UInt16 elemWidth = data[1];
        UInt32 len = data[2] | (((UInt32)data[3]) << 16);
        dataSize = (4 + (elemWidth * len + 1)/2) * 2;
        break;
    }
    }

    offset = cbsGetSize(dataHandler);
    err += cbsWrite16(dataHandler, signature);
    err += cbsWrite(dataHandler, data + 1, dataSize - 2);
    //if(((dataSize + 2) & 3) != 0){
    //    cbsWrite16(dataHandler, 0);
    //}


    WRITE_FORMATAABBBBBBBB(lexInstr,lir->value,offset);
    return err;
}

static Int32 processLIRInvokeOp(LexInstr* lexInstr,LIRInvokeOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;
    UInt8 flag = lir->flags;

   //clear high 4bit
    UInt8 flag1 = flag & 0x0f;
    //clear low 4bit
    UInt8 flag2  = flag & 0xf0;

    /*is not rang but a inline*/
    if((flag & LIR_invoke_inline) != 0 && (((flag2) & LIR_Range) == 0))
    {
        lexOpcode = lc_execute_inline;
        lir->ref = lir->exeRef;
        goto END;
    }

    flag2 &= ~(LIR_invoke_inline);

    switch(flag1){
    case LIR_invoke_virtual:{
        switch(flag2)
        {
        case 0: lexOpcode = lc_invokevirtual; break;
        case LIR_Range: lexOpcode = lc_invokevirtual_range; break;
        }
        break;
    }
    case LIR_invoke_super:{
        switch(flag2)
        {
        case 0: lexOpcode = lc_invokesuper; break;
        case LIR_Range: lexOpcode = lc_invokesuper_range; break;
        }
        break;
    }
    case LIR_invoke_direct:{
        switch(flag2)
        {
        case 0: lexOpcode = lc_invokedirect; break;
        case LIR_Range: lexOpcode = lc_invokedirect_range; break;
        }
        break;
    }
    case LIR_invoke_static:{
        switch(flag2)
        {
        case 0: lexOpcode = lc_invokestatic; break;
        case LIR_Range: lexOpcode = lc_invokestatic_range; break;
        }
        break;
    }
    case LIR_invoke_interface:{
        switch(flag2)
        {
        case 0: lexOpcode = lc_invokeinterface; break;
        case LIR_Range: lexOpcode = lc_invokeinterface_range; break;
        }
        break;
    }
    case 0:{
        switch(flag2)
        {
        case 0: lexOpcode = lc_filled_new_array; break;
        case LIR_Range: lexOpcode = lc_filled_new_array_range; break;
        }
        break;
    }
    }

END:
    if(flag2 != LIR_Range){
        UInt32 i,count;
        UInt8 args5 = 0;
        UInt16 regList = 0;
        UInt16 methodIdx;

        methodIdx = lir->ref;
        count = lir->argc;

        if(count == 5){
            args5 = lir->args[4];
            count --;
        }
        for(i = 0; i < count; i++){
            regList |= lir->args[count - i - 1];
            if( i != count - 1)
                regList<<= 4;
        }
        WRITE_FORMATABCCCCDDDD(lexInstr,args5,lir->argc,methodIdx,regList)
    }else{
        WRITE_FORMATAABBBBCCCC(lexInstr,lir->argc,lir->ref,lir->args[0])
    }
    return err;
}

static Int32 processLIRConstOp(LexInstr* lexInstr,LIRConstOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;
    UInt32 destReg = lir->vA;
    //Int32 data = lir->vB;

    switch(lir->flags){
    case 0: {
        //lexOpcode = lc_const_high16; break;
        Int32 data = (UInt32)lir->vB;
        if(signedFitsIn4(data) && destReg < 16){
            lexOpcode = lc_const_4;
            WRITE_FORMATABLIT(lexInstr,lir->vA,data)
        }else if(signedFitsIn16(data)){
            lexOpcode = lc_const16;
            WRITE_FORMATAABBBB(lexInstr,lir->vA,(Int16)data)
        }else{
            lexOpcode = lc_const32;
            WRITE_FORMATAABBBBBBBB(lexInstr,lir->vA,data)
        }
        break;
    }
    case LIR_JDT_wide:{
        //lexOpcode = lc_const_wide_high16;
        Int64 data = (Int64)lir->vB;
        if(signedFitsInWide16(data)){
            lexOpcode = lc_const16_w;
            WRITE_FORMATAABBBB(lexInstr,lir->vA,data)
        }else if(signedFitsInWide32(data)){
            lexOpcode = lc_const32_w;
            WRITE_FORMATAABBBBBBBB(lexInstr,lir->vA,lir->vB)
        }else{
            lexOpcode = lc_const64;
            WRITE_FORMATAABBBBBBBBBBBBBBBB(lexInstr,lir->vA,lir->vB)
        }
        break;
    }
    /*case LIR_JDT_high:
    {
        lexOpcode = lc_const_high16;
        WRITE_FORMATAABBBB(lexInstr,lir->vA,(UInt16)lir->vB)
        break;
    }
    case LIR_JDT_high_w:
    {
        lexOpcode = lc_const_wide_high16;
        WRITE_FORMATAABBBB(lexInstr,lir->vA,(UInt16)lir->vB)
        break;
    }*/
    }
    return err;
}

static Int32 processJumpLIRABOp(LexInstr* lexInstr,LIRABOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;

    switch(lir->flags){
        case LIR_cond_EQ: lexOpcode = lc_ifeq; break;
        case LIR_cond_NE: lexOpcode = lc_ifne; break;
        case LIR_cond_LT: lexOpcode = lc_iflt; break;
        case LIR_cond_GE: lexOpcode = lc_ifge; break;
        case LIR_cond_GT: lexOpcode = lc_ifgt; break;
        case LIR_cond_LE: lexOpcode = lc_ifle; break;
    }

    WRITE_FORMATAABBBB(lexInstr,lir->vA,lir->vB)
    return err;
}

static Int32 processJumpLIRABCOp(LexInstr* lexInstr,LIRABCOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;

    switch(lir->flags){
    case LIR_cond_EQ: lexOpcode = lc_if_icmpeq; break;
    case LIR_cond_NE: lexOpcode = lc_if_icmpne; break;
    case LIR_cond_LT: lexOpcode = lc_if_icmplt; break;
    case LIR_cond_GE: lexOpcode = lc_if_icmpge; break;
    case LIR_cond_GT: lexOpcode = lc_if_icmpgt; break;
    case LIR_cond_LE: lexOpcode = lc_if_icmple; break;
    }

    WRITE_FORMATABCCCC(lexInstr,lir->vA,lir->vB,lir->vC)

    return err;
}

static Int32 processLIRGOTOOp(LexInstr* lexInstr,LIRGOTOOp* lir){
    Int32 err = 0;
    UInt8 lexOpcode = 0;

    UInt32 targetNum = (UInt32)lir->target;
    lexOpcode = lc_goto;
    WRITE_FORMATAABBBBBBBB(lexInstr,0,targetNum)

    return err;
}

static UInt32 checkGOTOFits(TargetInsn* insn){
    UInt32 oldSize;
    UInt32 newSize;
    UInt32 targetOffset = (Int32)(insn->targetOffset - insn->offset)/2;

    oldSize = insn->size;
    switch(oldSize){
    case 2:
        insn->lastfixSize = 0;
        break;
    case 4:
        if(signedFitsIn8(targetOffset)){
            insn->size = 2;
            insn->lastfixSize = 2;
        }else{
            insn->lastfixSize = 0;
        }
        break;
    case 6:
        if(signedFitsIn8(targetOffset)){
            insn->size = 2;
            insn->lastfixSize = 4;
        }else if(signedFitsIn16(targetOffset)){
            insn->size = 4;
            insn->lastfixSize = 2;
        }else{
            insn->lastfixSize = 0;
        }
        break;
    }

    return     insn->lastfixSize;
}

static inline UInt32 getLexOffset(LexInstr* instrList,UInt32 num){
    LexInstr* lexInstr =  instrList + num;
    return lexInstr->instrOffset;
}

static void genGOTO(LexInstrList* lexinstrList,UInt32 numGoto){
    UInt32 instrCount = lexinstrList->instrCount;
    LexInstr* instrList = lexinstrList->instrList;
    UInt32 i,j;
    UInt32 curTarget = 0;

    TargetInsn targetList[numGoto];

    //1. gen target list
    for(i = 0; i < instrCount; i++){
        LexInstr* lexInstr =  instrList + i;
        BYTE* ip = lexInstr->instrData;

        if(ip[0] == lc_goto){
            UInt16 targetInstrNum = cReadLE32(ip + 2);
            TargetInsn* insn = targetList + curTarget;

            insn->targetNum = targetInstrNum;
            insn->num = i;
            insn->size = 6;
            insn->offset = lexInstr->instrOffset;

            insn->targetOffset = getLexOffset(instrList,targetInstrNum);
            curTarget ++;
        }
    }

    //2.fixOffset until right
    //UInt32 fixOffset = 0;
    while(true){
        UInt32 curFixOffset = 0;
        for(i = 0; i < curTarget; i++){
            TargetInsn* insn = targetList + i;

            insn->offset -= curFixOffset;
            if(insn->targetNum > insn->num){
                insn->targetOffset -= curFixOffset;
            }else{
                UInt32 tmpFixOffset = 0;
                for(j = 0; j < i; j++){
                        TargetInsn* tinsn = targetList + j;
                        if(tinsn->num > insn->targetNum)
                            break;
                        tmpFixOffset += tinsn->lastfixSize;
                }
                insn->targetOffset -= tmpFixOffset;
            }

            curFixOffset += checkGOTOFits(insn);
        }

        if(curFixOffset != 0){
            //fixOffset += curFixOffset;
            curFixOffset = 0;
        }else{
            break;
        }
    }

    //UInt32 adjectOffset = 0;
    //3.rewrite lex instr offset
    for(i = 0; i < curTarget; i++){
        TargetInsn* insn = targetList + i;

        LexInstr* lexInstr =  instrList + insn->num;
        lexInstr->instrSize = insn->size;
        //adjectOffset += 6 - insn->size;
    }

    //if(adjectOffset != fixOffset){
    //    abort();
    //}
}

static Int32 witeByteCodeAndFixOffset(CBSHandle regIns,LexInstrList* lexInstrList){
    UInt32 instrCount = lexInstrList->instrCount;
    LexInstr* instrList = lexInstrList->instrList;
    UInt32 i;
    Int32 err = 0;

    UInt32 fixOffset = 0;
    for(i = 0; i < instrCount; i++){
        LexInstr* lexInstr =  instrList + i;
        BYTE* ip = lexInstr->instrData;

        switch(ip[0]){
        case lc_goto:
        {
            //sub first
            lexInstr->instrOffset -= fixOffset;

            UInt32 instrNum = cReadLE32(ip + 2);
            if(lexInstr->instrSize == 2){
                fixOffset += 4;
                //cWriteLE32 = instrNum;
                err += cbsWrite8(regIns, (UInt8)lc_goto8);
                err += cbsWrite8(regIns, (Int8)0);
            }else if(lexInstr->instrSize == 4){
                fixOffset += 2;
                err += cbsWrite8(regIns, (UInt8)lc_goto16);
                err += cbsWrite8(regIns, 0);
                err += cbsWrite16(regIns, (Int16)instrNum);
            }else{
                err += cbsWrite8(regIns, (UInt8)lc_goto);
                err += cbsWrite8(regIns, 0);
                err += cbsWrite32(regIns, (UInt32)instrNum);
            }

            break;
        }
        default:{
            err += cbsWrite(regIns, lexInstr->instrData, lexInstr->instrSize);
            lexInstr->instrOffset -= fixOffset;
        }
        }
    }

    return err;
}

static Int32 fixJmupOffset(CBSHandle regIns,CBSHandle dataHandler,LexInstrList* lexInstrList)
{
    UInt32 instrCount = lexInstrList->instrCount;
    LexInstr* instrList = lexInstrList->instrList;
    UInt8 opcode;
    UInt32 i;
    UInt32 codeSize;
    Int32 err = 0;

    codeSize = cbsGetSize(regIns);
    //4 bytes algin
    if((codeSize & 3) != 0){
        cbsWrite16(regIns, 0);
        codeSize = (codeSize + 3) & ~3;
    }

    if(dataHandler != 0){
        BYTE* data = cbsGetData(dataHandler);
        UInt32 dataSize = cbsGetSize(dataHandler);
        err += cbsWrite(regIns, data, dataSize);

        cbsDestroy(dataHandler);
    }

    BYTE* ip = cbsGetData(regIns);
    BYTE* switchBase = ip + codeSize;
    //2.fix jump offset
    for(i = 0; i < instrCount; i++){
        LexInstr* lexInstr =  instrList + i;

        switch(ip[0]){
        case lc_ifeq:
        case lc_ifne:
        case lc_iflt:
        case lc_ifge:
        case lc_ifgt:
        case lc_ifle:
        case lc_if_icmpeq:
        case lc_if_icmpne:
        case lc_if_icmplt:
        case lc_if_icmpge:
        case lc_if_icmpgt:
        case lc_if_icmple:
        {
            UInt16 instrNum = *((UInt16*)(ip + 2));
            UInt32 targetInstrOffset = getLexOffset(instrList,instrNum);
            Int32  targetOffset = (Int32)(targetInstrOffset - lexInstr->instrOffset)/2;
            if(!signedFitsIn16(targetOffset)){
                ABORT();
            }
            *((Int16*)(ip + 2)) = (Int16)targetOffset;
            break;
        }
        case lc_goto16:
        {
            UInt32 instrNum = cReadLE32(lexInstr->instrData + 2);
            UInt32 targetInstrOffset = getLexOffset(instrList,instrNum);
            Int32  targetOffset = (Int32)(targetInstrOffset - lexInstr->instrOffset)/2;
            if(!signedFitsIn16(targetOffset)){
                ABORT();
            }
            *((Int16*)(ip + 2)) = (Int16)targetOffset;
            break;
        }
        case lc_goto8:
        {
            UInt32 instrNum = cReadLE32(lexInstr->instrData + 2);
            UInt32 targetInstrOffset = getLexOffset(instrList,instrNum);
            Int32  targetOffset = (Int32)(targetInstrOffset - lexInstr->instrOffset)/2;
            if(!signedFitsIn8(targetOffset)){
                abort();
            }
            *(ip + 1) = (Int8)targetOffset;
            break;
        }
        case lc_goto:
        {
            UInt32 instrNum = cReadLE32(lexInstr->instrData + 2);
            UInt32 targetInstrOffset = getLexOffset(instrList,instrNum);
            Int32  targetOffset = (Int32)(targetInstrOffset - lexInstr->instrOffset)/2;
            if(targetOffset == 0)
                abort();
            cWriteLE32(ip + 2, targetOffset);
            break;
        }
        case lc_tableswitch:
        {
            UInt32 j;
            UInt32 targetOffset;
            UInt32 lexInstrOffset;
            UInt32* target;
            UInt32 size;
            UInt16* data;

            targetOffset = (UInt32)cReadLE32(ip + 2);
            data = (UInt16*)(switchBase + targetOffset);
            size =  data[1];
            target = (UInt32*)(((BYTE*)data) + 8);

            lexInstrOffset = lexInstr->instrOffset;
            for(j = 0; j < size; j++){
                UInt32 targetInstrOffset;
                UInt32 instrNum;

                instrNum = (UInt32)target[j];

                targetInstrOffset = getLexOffset(instrList,instrNum);
                target[j] = (UInt32)((Int32)(targetInstrOffset - lexInstrOffset)/2);
            }

            targetOffset = (codeSize + targetOffset - lexInstrOffset)/2;
            cWriteLE32(ip + 2, targetOffset);
            break;
        }
        case lc_lookupswitch:
        {
            UInt32 j;
            UInt32 targetOffset;
            UInt32 lexInstrOffset;
            UInt32* target;
            UInt32 size;
            UInt16* data;

            targetOffset = (UInt32)cReadLE32(ip + 2);
            data = (UInt16*)(switchBase + targetOffset);
            size =  data[1];
            target = (UInt32*)(((BYTE*)data) + 4 + 4*size);
            lexInstrOffset = lexInstr->instrOffset;

            for(j = 0; j < size; j++){
                UInt32 targetInstrOffset;
                UInt32 instrNum;

                instrNum = (UInt32)target[j];
                targetInstrOffset = getLexOffset(instrList,instrNum);
                target[j] = (UInt32)((Int32)(targetInstrOffset - lexInstrOffset)/2);
            }

            targetOffset = (codeSize + targetOffset - lexInstrOffset)/2;
            cWriteLE32(ip + 2, targetOffset);
            break;
        }
        case lc_fill_array_data:
        {
            UInt32 targetOffset = (UInt32)cReadLE32(ip + 2);
            targetOffset = (codeSize + targetOffset - lexInstr->instrOffset)/2;
            cWriteLE32(ip + 2, targetOffset);
        }
        break;
        }

        ip += lexInstr->instrSize;
    }

    return err;
}

static UInt32 fixTryCatches(CBSHandle regIns, LIRCode* list,
        LexInstr* instrList, UInt32 instrLength, LCatchException** ppCatch)
{
    LIROpcodeTry* trys = list->trys;
    UInt32 i,j;
    Int32 err = 0;
    UInt32 numException = 0;
    CBSHandle hCatchBuff = 0;
    UInt32 triesSize = list->triesSize;

    hCatchBuff = cbsInitialize(triesSize * sizeof(LCatchException));

    if(!hCatchBuff)
        abort();

    for(i = 0; i< list->triesSize; i++){
        UInt32 end;

        LIROpcodeTry* ltry = trys + i;
        UInt32 start = getLexOffset(instrList,ltry->start_pc);

        if(ltry->end_pc + 1 == (UInt32)list->instrCount){
            end = instrLength;
        }else{
            end = getLexOffset(instrList,ltry->end_pc);
        }

        for(j = 0; j < ltry->catchSize; j++){
            UInt32 catch_type;

            LIROpcodeCatch* lcatch = ltry->catches + j;
            UInt32 handler = getLexOffset(instrList,lcatch->handler_pc);
            err += cbsWrite32(hCatchBuff, start);
            err += cbsWrite32(hCatchBuff, end);
            err += cbsWrite32(hCatchBuff, handler);

            catch_type = lcatch->catch_type;
            err += cbsWrite32(hCatchBuff, catch_type);

            numException ++;
            ASSERT(cbsGetSize(hCatchBuff) == (numException * sizeof(LCatchException)));
        }
    }

    if(err !=0)
        abort();

    *ppCatch = (LCatchException*)cMalloc(cbsGetSize(hCatchBuff));
     cbsCopyData(hCatchBuff, *ppCatch, numException*sizeof(LCatchException));

done:
     if(hCatchBuff)
         cbsDestroy(hCatchBuff);

    return numException;
}

Int32 lir2lexOpcode(CBSHandle regIns, LIRCode* list, UInt32* numException,
        UInt32* instrLength, UInt32* codeLength, LCatchException** pCatch) {

    Int32 i;
    UInt32* lirList = (UInt32*)list->lirList;
    Int32 instrCount = list->instrCount;
    CBSHandle dataHandler = 0;
    Int32 err = 0;
    LexInstrList lexInstrList;
    UInt32 instrOffset = 0;
    UInt32 numGoto = 0;

    lexInstrList.instrCount = instrCount;
    lexInstrList.instrList = (LexInstr*)malloc(instrCount * sizeof(LexInstr));

    for(i = 0; i < instrCount; i++){

        LexInstr* lexInstr =  lexInstrList.instrList + i;
        LIRBaseOp* codePtr = (LIRBaseOp*)lirList[i];

        UInt8 lirOpcode = getlirOpcode((UInt32)codePtr);
        UInt32 formats = gLIROpcodeInfo.formats[lirOpcode];

        switch(formats){
        case lirFmtV:
        {
            LIRBaseOp* lir =  codePtr;
            UInt8 lexOpcode = lc_nop;
            WRITE_FORMATAA(lexInstr,0)
            break;
        }
        case lirFmtA:
        case lirFmtR:
        {
            LIRAOp* lir = (LIRAOp*)codePtr;
            err += processLIRAOp(lexInstr,lir);
            break;
        }
        case lirFmtT:
        {
            LIRGOTOOp* lir = (LIRGOTOOp*)codePtr;
            err += processLIRGOTOOp(lexInstr,lir);
            numGoto ++;
            break;
        }
        case lirFmtRA:
        {
            LIRABOp* lir = (LIRABOp*)codePtr;
            err += processLIRABOp(lexInstr,lir);
            break;
        }
        case lirFmtRS:
        case lirFmtAS:
        {
            LIRABOp* lir = (LIRABOp*)codePtr;
            err += processLIRABOp(lexInstr,lir);
            break;
        }
        case lirFmtAT:
        {
            LIRABOp* lir = (LIRABOp*)codePtr;
            err += processJumpLIRABOp(lexInstr,lir);
            break;
        }
        case lirFmtRL:
        {
            LIRConstOp* lir = (LIRConstOp*)codePtr;
            err += processLIRConstOp(lexInstr,lir);
            break;
        }
        case lirFmtABC:
        case lirFmtRAB:
        case lirFmtRAL:
        {
            LIRABCOp* lir = (LIRABCOp*)codePtr;
            err += processLIRABCOp(lexInstr,lir);
            break;
        }
        case lirFmtABS:
        case lirFmtRAS:
        {
            LIRABCOp* lir = (LIRABCOp*)codePtr;
             err += processLIRABCOp(lexInstr,lir);
            break;
        }
        case lirFmtABT:
        {
            LIRABCOp* lir = (LIRABCOp*)codePtr;
            err += processJumpLIRABCOp(lexInstr,lir);
            break;
        }
        case lirFmtSWITCH:
        {
            LIRSwitchOp* lir = (LIRSwitchOp*)codePtr;
            if(dataHandler == 0){
                dataHandler =  cbsInitialize(0);
            }
            err += processLIRSwitchOp(lexInstr,dataHandler,lir);
            break;
        }
        case lirFmtINVOKE:
        {
            LIRInvokeOp* lir = (LIRInvokeOp*)codePtr;
            err += processLIRInvokeOp(lexInstr, lir);
            break;
        }
        }
        lexInstr->instrOffset = instrOffset;
        instrOffset +=  lexInstr->instrSize;
    }

    if(numGoto != 0){
        genGOTO(&lexInstrList,numGoto);
    }
    err += witeByteCodeAndFixOffset(regIns,&lexInstrList);

    *instrLength = cbsGetSize(regIns);

    err += fixJmupOffset(regIns,dataHandler,&lexInstrList);

    *codeLength =  cbsGetSize(regIns);

    *numException = fixTryCatches(regIns, list, lexInstrList.instrList, *instrLength, pCatch);

    free(lexInstrList.instrList);
    return err;
}

static UInt32 lir2lexCode(LIRCode* list, LCodeData* pLCode)
{
    UInt32 err = 0;

    CBSHandle regIns = cbsInitialize(0);
    LCatchException* pCatch = NULL;

    UInt32 numExceptions = 0;
    UInt32 instrLength = 0;
    UInt32 codeLength = 0;
    UInt32 codeSize;
    BYTE*  codeData;

    err += lir2lexOpcode(regIns, list, &numExceptions, &instrLength, &codeLength, &pCatch);

    codeSize = cbsGetSize(regIns);
    codeData = cbsGetData(regIns);

    pLCode->maxVars = list->maxVars;
    pLCode->numExceptions = numExceptions;
    pLCode->debugOff = 0;
    pLCode->codeLength = codeLength;
    pLCode->instrLength = instrLength;
    pLCode->exceptions = pCatch;
    pLCode->code  = (BYTE*)cMalloc(pLCode->codeLength);

    if(pLCode->code == NULL)
    {
        err += 1;
        goto exit;
    }

    cMemset(pLCode->code, 0, pLCode->codeLength);
    cbsCopyData(regIns,  pLCode->code, codeLength);

exit:
    cbsDestroy(regIns);
    return  err;
}

//static inline LIRCode* anaDoAnalyse(LIRCode* code)
//{
//    LIRCode* newCode;
//    newCode = anaEntry(code);
//    return newCode;
//}

UInt32 lir2lexTransform(LIRCode* code, LCodeData* codeData)
{
    UInt32 err = 0;

    //code = anaDoAnalyse(code);
    ASSERT(0);//obsolete code.

    err = lir2lexCode(code, codeData);
    return err;
}
#endif
#endif
