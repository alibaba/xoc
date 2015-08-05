/*Copyright 2011 Alibaba Group*/
/*
 *  cmem.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/19/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_MEM_H
#define CLIBS_MEM_H

#include "std/cstd.h"


typedef void (*CMemAllocFailProc)();

EXTERN_C void        cMemSetAllocFailProc(CMemAllocFailProc proc);

EXTERN_C bool     cMemInit();

EXTERN_C void     cMemFinal();


EXTERN_C void*    cMallocRaw(UInt32 size);
#define cMalloc(size) cMallocRaw(size)

EXTERN_C void     cFree(void* p);

EXTERN_C UInt32     cMemTotalSize(void);

EXTERN_C UInt32     cMemFreeSize(void);

EXTERN_C UInt32     cMemUsedSize(void);

#ifdef __BREW__
#include "aeestdlib.h"
#define cMemchr MEMCHR
#define cMemmove MEMMOVE
#define cMemcmp MEMCMP
#define cMemcpy MEMCPY
#define    cMemset MEMSET

#else
#define cMemchr memchr
#define cMemmove memmove
#define cMemcmp memcmp
#define cMemcpy memcpy
#define    cMemset memset
#endif

//** added by raymond
void *cMmap(Int32 fd, Int32 alignment, Int32 size, Int32 mode);
void cMunmap(void *address, Int32 length);


//** end
#endif
