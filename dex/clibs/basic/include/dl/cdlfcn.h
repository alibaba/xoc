/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_DL_FUNC_H
#define CLIBS_DL_FUNC_H

#include "std/cstd.h"

EXTERN_C ULong cdlopen(const char* name);

EXTERN_C void cdlclose(ULong handle);

EXTERN_C void* cdlsym(ULong handle, const char* symname);

EXTERN_C const char* cdlerror();

#endif
