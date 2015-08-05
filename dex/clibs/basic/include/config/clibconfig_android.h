/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_CONFIG_ANDROID_H
#define CLIBS_CONFIG_ANDROID_H

#define C_BYTE_ORDER C_LITTLE_ENDIAN

#ifdef _DEBUG
#define MADA_DEBUG
#define C_MAX_TRACE_SIZE     (10*1024)
#endif

#define MADA_LINE_SEPERATOR "\n"

#define CONFIG_PLATFORM_FILENAME "platform_unix.config"

#endif
