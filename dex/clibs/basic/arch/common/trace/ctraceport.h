/*Copyright 2011 Alibaba Group*/
/*
 *  ctraceport.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/19/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_TRACE_PORT_H
#define CLIBS_TRACE_PORT_H

#include "trace/ctrace.h"

#ifdef MADA_DEBUG

EXTERN_C void rawErrorPrint(const char* msg);
EXTERN_C void rawTracePrint(const char* msg);

#endif

#endif
