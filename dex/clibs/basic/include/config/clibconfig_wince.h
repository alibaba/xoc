/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_CONFIG_WINCE_H
#define CLIBS_CONFIG_WINCE_H

#define C_BYTE_ORDER C_LITTLE_ENDIAN

#define strdup _strdup
#define stricmp _stricmp
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp stricmp
#define strncasecmp    _strnicmp

#define USE_SYSTEM_CANVAS    1
#define USE_SYSTEM_FONT        1

#ifdef _DEBUG
#define C_MAX_TRACE_SIZE     (10*1024)
#endif

#endif
