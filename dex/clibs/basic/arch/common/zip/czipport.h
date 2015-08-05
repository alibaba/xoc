/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_ZIP_PORT_H
#define CLIBS_ZIP_PORT_H

#include "std/cstd.h"

EXTERN_C UInt32 cZipCompFlateInit();

EXTERN_C void cZipCompFlateEnd(UInt32 id);

EXTERN_C Int32 cZipDeflate(UInt32 compID, BYTE* in, UInt32 inSize, BYTE* out, UInt32 bufSize, UInt32* outSize);

EXTERN_C Int32 cZipDeflateFinish(UInt32 compID, BYTE* out, UInt32 bufSize, UInt32* outSize);

EXTERN_C Int32 cZipInflate(UInt32 compID, const BYTE* in, UInt32 inSize, BYTE* out, UInt32 bufSize, UInt32* outSize);

#endif
