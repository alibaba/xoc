/*Copyright 2011 Alibaba Group*/
/*
 *  cmemport.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/19/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */
#ifndef CLIBS_MEM_PORT_H
#define CLIBS_MEM_PORT_H

#include "mem/cmem.h"

EXTERN_C bool     cMemInitPort();

EXTERN_C void     cMemFinalPort();

EXTERN_C void*     cMallocPort(UInt32 size);

EXTERN_C void     cFreePort(void* p);

EXTERN_C UInt32     cMemTotalSizePort(void);

EXTERN_C UInt32     cMemUsedSizePort(void);


//** added by raymond
void *cMmapPort(Int32 fd, Int32 alignment, Int32 size, Int32 mode);
void cMunmapPort(void *address, Int32 length);
//** end
#endif
