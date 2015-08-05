/*Copyright 2011 Alibaba Group*/
/*
 *  csysport.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/19/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_SYS_PORT_H
#define CLIBS_SYS_PORT_H

#include "sys/csys.h"

EXTERN_C bool cSysInitPort(void);

EXTERN_C void cSysFinalPort(void);

EXTERN_C Int32 cSysProcessorNumberPort();

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
EXTERN_C const char** cSysGetEnvBlockPort(UInt32 *blockSize);

/* Retrieves the contents of the specified variable from the environment block of the calling process.
 * @param name[in]: The name of the environment variable.
 * @return: the value of name if ok, or NULL.
 */
EXTERN_C const char* cSysGetEnvValuePort(const char *name);

EXTERN_C void cSysExitPort(Int32 status);

EXTERN_C const struct Property *cSystemInitPlatformPropertiesPort(int *num);
#endif
