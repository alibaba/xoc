/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_CONFIG_MTK_H
#define CLIBS_CONFIG_MTK_H


#define C_BYTE_ORDER C_LITTLE_ENDIAN


#ifdef WIN32

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp stricmp
#define strncasecmp    strnicmp

#else

#ifdef __cplusplus
extern "C" {
#endif

char* strdup(const char*);
int strcasecmp(const char *str1, const char *str2);
int strncasecmp(const char *str1, const char *str2, int count);

#ifdef __cplusplus
}
#endif

#endif

#if 1
#define C_MAX_TRACE_SIZE     (10*1024)
#endif

#endif
