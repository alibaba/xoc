/*Copyright 2011 Alibaba Group*/
/*
 *  csys.h
 *  madagascar
 *
 *  Created by Aaron Wang on 4/15/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_SYS_H
#define CLIBS_SYS_H

#include "std/cstd.h"

struct Property {
    const char *key;
    const char *value;
};

typedef struct STPlatformInfo {
    const char* os;
    const char* osVersion;
    Int64 CPUFrequency;
    bool isSupportCamera;
    bool isSupportGPS;
    bool isSupportGravity;
} CPlatformInfoT;

EXTERN_C bool cSysInit(void);

EXTERN_C void cSysFinal(void);

EXTERN_C void cSysGetPlatform(CPlatformInfoT* info);

EXTERN_C void cSysSetRootPath(const char* path);

EXTERN_C const char* cSysGetRootPath();

EXTERN_C Int32 cSysProcessorNumber();

/* Retrieves the environment variables for the current process.
 * @param[out]: blockSize size of the environment block
 * @return: If ok, return the environment block. If failed, return NULL.
 * remark:
 * the format of the environment block is
 *  Var1=Value1\0
 *  Var2=Value2\0
 *  Var3=Value3\0
 *  ...
 *  VarN=ValueN\0\0
 */
EXTERN_C const char** cSysGetEnvBlock(UInt32 *blockSize);

/* Retrieves the contents of the specified variable from the environment block of the calling process.
 * @param name[in]: The name of the environment variable.
 * @return: the value of name if ok, or NULL.
 */
EXTERN_C const char* cSysGetEnvValue(const char *name);

EXTERN_C void cSysExit(Int32 status);

EXTERN_C const struct Property *cSystemInitPlatformProperties(int *num);

#endif
