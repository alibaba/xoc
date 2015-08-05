/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_CONFIG_SYMBIAN_H
#define CLIBS_CONFIG_SYMBIAN_H

#define C_BYTE_ORDER C_LITTLE_ENDIAN

#define USE_SYSTEM_BLT        0
#define USE_SYSTEM_CANVAS    1
#define USE_SYSTEM_FONT        1

//temploray mark this #ifdef, when release, should open this macro
//#ifdef _DEBUG
#define C_MAX_TRACE_SIZE     (10*1024)
//#endif

#ifndef __WINSCW__
#include <stddef.h>
//symbain, gcce or armv5

#ifdef __cplusplus
extern "C" {
#endif
int snprintf(char *buf, size_t n, const char *format, ...);
int vsnprintf(char* buf, size_t n, const char *format, va_list list);

#ifdef __cplusplus
}
#endif

#endif

#endif
