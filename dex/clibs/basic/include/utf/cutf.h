/*Copyright 2011 Alibaba Group*/
/*
 *  cutf.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/28/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_UTF_H
#define CLIBS_UTF_H

#include "std/cstd.h"

EXTERN_C UInt32 cUtf8Chars(const char* p, UInt32 size);

EXTERN_C UInt32 cUtf8LengthOfUtf16(const UInt16* p, UInt32 size);

EXTERN_C UInt32 cUtf8ToUtf16(const char* utf8, UInt32 utf8Size, UInt16* utf16, UInt32 utf16Size);

EXTERN_C UInt32 cUtf16ToUtf8(const UInt16* utf16, UInt32 utf16Size, char* utf8, UInt32 utf8Size);

// need free with cFree, len is the number of unichar in result
EXTERN_C UInt16* cUTF16FromUTF8(const char* utf8, UInt32 utf8Size, Int32 *len);

// need free with cFree, len is the size of bytes in result
EXTERN_C char* cUTF8FromUTF16(const UInt16* utf16, UInt32 utf16Size, Int32 *len);

#endif
