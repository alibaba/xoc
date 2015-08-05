/*Copyright 2011 Alibaba Group*/
/*
 *  cstring.h
 *  madagascar
 *
 *  Created by Aaron Wang on 4/15/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_STRING_H
#define CLIBS_STRING_H

#include "std/cstd.h"


#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __BREW__
#define cStrcat strcat
#define cStrchr strchr
#define cStrrchr strrchr
#define cStrcmp strcmp
#define cStrcpy strcpy
#define cStrlen strlen
#define cStrncat strncat
#define cStrncmp strncmp
#define cStrncpy strncpy
#define cStrstr strstr
#define cStrcasecmp strcasecmp
#define cStrncasecmp strncasecmp
#define cSnprintf snprintf
#define cVsnprintf vsnprintf
#define cSprintf sprintf
#define cStrdup strdup
#define    cIsspace isspace

#define cAtoi atoi
#define cAtof atof
#else /* !__BREW__ */
#include "AEEStdLib.h"
#define cStrcat STRCAT
#define cStrchr STRCHR
#define cStrrchr STRRCHR
#define cStrcmp STRCMP
#define cStrcpy STRCPY
#define cStrlen STRLEN
#define cStrncat(src,dst,len) STRCAT((src),(dst))
#define cStrncmp STRNCMP
#define cStrncpy STRNCPY
#define cStrstr STRSTR
#define cStrcasecmp STRICMP
#define cStrncasecmp STRNICMP
#define cSnprintf SNPRINTF
#define cVsnprintf VSNPRINTF
#define cSprintf SPRINTF
#define cStrdup STRDUP
#define    cIsspace(c) ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t' || (c) == '\v')

#define cAtoi ATOI
#define cAtof ATOF
#endif /* __BREW__ */

/** replace all 'pat' to 'rep' in 'str' */
void cStrcrep(char* str, char pat, char rep);

Int32 cStrLastIndexOf(const char* str, char c);

#ifdef __cplusplus
}
#endif

#endif
