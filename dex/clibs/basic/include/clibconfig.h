/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_CONFIG_H
#define CLIBS_CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config/clibplatform.h"

/* Add any platform that doesn't build using the configure system */
#if defined(ANDROID)
#include "config/clibconfig_android.h"
#elif defined(__UNIX_LIKE__)
#include "config/clibconfig_unix.h"
#elif defined(WINCE)
#include "config/clibconfig_wince.h"
#elif defined(__SYMBIAN32__)
#include "config/clibconfig_symbian.h"
#elif defined(__MTK__)
#include "config/clibconfig_mtk.h"
#elif defined(__BREW__)
#include "config/clibconfig_brew.h" //phone platforms(wince/symbian/mtk) must before win32 for pc simulator
#elif defined(__WIN32__)
#include "config/clibconfig_win32.h" //phone platforms(wince/symbian/mtk) must before win32 for pc simulator
#elif defined(__IPHONE__)
#include "config/clibconfig_iphone.h"
#else
#error "unkown platform"
#endif /* platform config */

#endif
