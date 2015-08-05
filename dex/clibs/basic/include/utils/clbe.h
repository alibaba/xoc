/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_LITTLE_BIG_ENDIAN_H
#define CLIBS_LITTLE_BIG_ENDIAN_H

#include "std/cstd.h"

#define cReadLE32(p) ((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))
#define cReadLE16(p) ((p)[0]|((p)[1]<<8))
EXTERN_C UInt64 cReadLE64(BYTE* p);
EXTERN_C void cWriteLE32(BYTE* dst, UInt32 num);
EXTERN_C void cWriteLE16(BYTE* dst, UInt16 num);
EXTERN_C void cWriteLE64(BYTE* dst, UInt64 num);

#define cReadBE32(p) (((p)[0]<<24) | ((p)[1]<<16) | ((p)[2]<<8) | (p)[3])
#define cReadBE16(p) (((p)[0]<<8) | (p)[1])
EXTERN_C UInt64 cReadBE64(BYTE* p);
EXTERN_C void cWriteBE32(BYTE* dst, UInt32 num);
EXTERN_C void cWriteBE16(BYTE* dst, UInt16 num);
EXTERN_C void cWriteBE64(BYTE* dst, UInt64 num);

#endif
