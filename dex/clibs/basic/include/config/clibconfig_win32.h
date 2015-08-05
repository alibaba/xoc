/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_CONFIG_WIN32_H
#define CLIBS_CONFIG_WIN32_H

#define C_BYTE_ORDER C_LITTLE_ENDIAN

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp stricmp
#define strncasecmp    strnicmp

#ifdef _DEBUG
#define MADA_DEBUG
#define C_MAX_TRACE_SIZE     (10*1024)
#endif

#define HAVE_NO_SYSTEM_NEXTAFTER
#define HAVE_NO_SYSTEM_NEXTAFTERF
#define HAVE_NO_SYSTEM_RINT
#define HAVE_NO_SYSTEM_REMAINDER
#define HAVE_NO_SYSTEM_CBRT
#define HAVE_NO_SYSTEM_EXPM1
#define HAVE_NO_SYSTEM_LOG1P

#endif
