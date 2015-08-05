/*Copyright 2011 Alibaba Group*/
/*
 *  cstd.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/19/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_STD_H
#define CLIBS_STD_H

#include "clibconfig.h"

/** define C_BYTE_ORDER in clibconfig_xxx.h */
#define C_LITTLE_ENDIAN 1234
#define C_BIG_ENDIAN 4321
#define C_IS_LITTLE_ENDIAN (C_BYTE_ORDER == C_LITTLE_ENDIAN)
#define C_IS_BIG_ENDIAN (C_BYTE_ORDER == C_BIG_ENDIAN)


/* Define NULL pointer value */

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

# include <stdbool.h>   /* C99 */

#if 0
#ifndef __cplusplus
/**define bool for C*/
#undef bool
#undef true
#undef false
#define bool char
#define true (!0)
#define false 0
#endif
#endif

typedef signed int Int32;
#ifdef __IPHONEOS__
#include <MacTypes.h>
#else
typedef unsigned int UInt32;
typedef unsigned long ULong;
typedef signed long Long;
#endif
typedef signed short Int16;
typedef unsigned short UInt16;
typedef signed char Int8;
typedef unsigned char UInt8;
typedef unsigned char BYTE;
typedef float Float32;
typedef double Float64;
#ifdef _MSC_VER
//for VC6, it's a must
typedef __int64 Int64;
typedef unsigned __int64 UInt64;
#define LLValue(v) v##L
#elif defined(__SYMBIAN32__)
#ifdef __WINSCW__
typedef long long Int64;
#endif
typedef unsigned long long UInt64;
#define LLValue(v) v##LL
#else
typedef long long Int64;
typedef unsigned long long UInt64;
#define LLValue(v) v##LL
#endif

typedef void* HandleT;

#define FIXED_FRACTION_BITS 16
typedef Int32 SFixed32;
#define Int32ToSFixed32(_SINT_X) ((_SINT_X)<<FIXED_FRACTION_BITS)
#define SFixed32ToInt32(_SFIXED_X) ((_SFIXED_X)>>FIXED_FRACTION_BITS)

#ifndef ABS
#define ABS(x) ((x)<0?-(x):(x))
#endif
#if 0
#ifndef MIN
#define MIN(x, y) ((x)<(y) ? (x):(y))
#endif

#ifndef MAX
#define MAX(x, y) ((x)>(y) ? (x):(y))
#endif
#endif
//carefully use it! wrong when x==y
#ifndef SWAP
#define SWAP(x,y) {(x)^=(y);(y)^=(x);(x)^=(y);}
#endif


#endif
