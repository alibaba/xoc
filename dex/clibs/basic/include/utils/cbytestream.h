/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_BYTESTREAM_H
#define CLIBS_BYTESTREAM_H

#include "std/cstd.h"

typedef ULong CBSHandle;

EXTERN_C CBSHandle cbsInitialize(UInt32 size);

EXTERN_C void cbsDestroy(CBSHandle bsh);

EXTERN_C Int32 cbsWrite8(CBSHandle bsh, UInt8 d);

EXTERN_C Int32 cbsWrite16(CBSHandle bsh, UInt16 d);

EXTERN_C Int32 cbsWrite32(CBSHandle bsh, UInt32 d);

EXTERN_C Int32 cbsWrite64(CBSHandle bsh, UInt64 d);

EXTERN_C Int32 cbsWrite(CBSHandle bsh, const void* data, UInt32 size);

EXTERN_C Int32 cbsSeek(CBSHandle bsh, UInt32 offset);

EXTERN_C UInt32 cbsGetSize(CBSHandle bsh);
EXTERN_C UInt32 cbsGetPos(CBSHandle bsh);

EXTERN_C BYTE* cbsGetData(CBSHandle bsh);

EXTERN_C void cbsCopyData(CBSHandle bsh, void* data, UInt32 bufSize);

EXTERN_C void cbsReset(CBSHandle bsh);

EXTERN_C bool cbsSetCapacity(CBSHandle bsh, UInt32 size);

#endif
