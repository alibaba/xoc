/*Copyright 2012 Alibaba Group*/
#ifndef CLIBS_TRACE_H
#define CLIBS_TRACE_H

#include "clibconfig.h"
#include "std/cstd.h"


typedef enum {
    TRACE_LEVEL_UNKNOWN = 0,
    TRACE_LEVEL_VERBOSE,
    TRACE_LEVEL_DEBUG,
    TRACE_LEVEL_INFO,
    TRACE_LEVEL_WARN,
    TRACE_LEVEL_ERROR,
    TRACE_LEVEL_NONE,
} TraceLevelT;

EXTERN_C bool cTraceOut(TraceLevelT level, const char* fmt, ...);

#if defined(ANDROID) && !HOST_LEMUR
//TODO: define LOG_TAG in Android.mk
#include "cutils/log.h"
#ifndef LOGD
#define LOGV ALOGV
#define LOGD ALOGD
#define LOGI ALOGI
#define LOGW ALOGW
#define LOGE ALOGE
#endif

#else

#define LOGV printf
#define LOGD printf
#define LOGI printf
#define LOGW printf
#define LOGE printf

#endif

#endif
