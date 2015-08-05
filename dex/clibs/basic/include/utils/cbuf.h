/*Copyright 2011 Alibaba Group*/
#ifndef MADA_CBUFFER_H
#define MADA_CBUFFER_H

#include "std/cstd.h"

typedef struct STBufStruct {
    char *buffer;
    UInt32 usedLen;
    UInt32 size;
} CBufferT;

EXTERN_C bool     cBufExpand(CBufferT *mem, int size);
EXTERN_C Int32     cBufFormatString(CBufferT *mem, const char* format, ...);
EXTERN_C void     cBufClear(CBufferT *mem);
EXTERN_C void     cBufFree(CBufferT *mem);
EXTERN_C bool     cBufAppend(CBufferT *mem, const void *data, UInt32 len);
EXTERN_C bool     cBufAppendString(CBufferT *mem, const char* str);
EXTERN_C bool     cBufSetValue(CBufferT *mem, const void *data, UInt32 size);


#endif
