/*Copyright 2011 Alibaba Group*/
/**
 * clibplatform.h
 *
 *  Created on: 2009-7-24
 *    Author: aaron
 */
#ifndef CLIBS_PLATFORM_H
#define CLIBS_PLATFORM_H

#if defined(_AIX)
#    undef __AIX__
#    define __AIX__        1
#endif

#if defined(__BEOS__)
#    undef __BEOS__
#    define __BEOS__    1
#endif

#if defined(bsdi) || defined(__bsdi) || defined(__bsdi__)
#undef __BSDI__
#define __BSDI__    1
#endif

#if defined(_arch_dreamcast)
#undef __DREAMCAST__
#define __DREAMCAST__    1
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
#undef __FREEBSD__
#define __FREEBSD__    1
#define __UNIX_LIKE__    1
#endif

#if defined(hpux) || defined(__hpux) || defined(__hpux__)
#undef __HPUX__
#define __HPUX__    1
#define __UNIX_LIKE__    1
#endif

#if defined(sgi) || defined(__sgi) || defined(__sgi__) || defined(_SGI_SOURCE)
#undef __IRIX__
#define __IRIX__    1
#endif

#if defined(linux) || defined(__linux) || defined(__linux__)
#undef __LINUX__
#define __LINUX__    1
#define __UNIX_LIKE__    1
#endif

#if defined(__APPLE__)
#undef __MACOSX__
#define __MACOSX__    1
#define __IPHONE__    1
#elif defined(macintosh)
#undef __MACOS__
#define __MACOS__    1
#define __IPHONE__    1
#endif

#if defined(__NetBSD__)
#undef __NETBSD__
#define __NETBSD__    1
#define __UNIX_LIKE__    1
#endif

#if defined(__OpenBSD__)
#undef __OPENBSD__
#define __OPENBSD__    1
#define __UNIX_LIKE__    1
#endif

#if defined(__OS2__)
#undef __OS2__
#define __OS2__        1
#endif

#if defined(osf) || defined(__osf) || defined(__osf__) || defined(_OSF_SOURCE)
#undef __OSF__
#define __OSF__        1
#endif

#if defined(__QNXNTO__)
#undef __QNXNTO__
#define __QNXNTO__    1
#endif

#if defined(riscos) || defined(__riscos) || defined(__riscos__)
#undef __RISCOS__
#define __RISCOS__    1
#endif
#if defined(__SVR4)
#undef __SOLARIS__
#define __SOLARIS__    1
#endif

#if defined(MTK_KAL) || defined(__MTK_TARGET__)
#undef __MTK__
#define __MTK__        1
#endif

#if defined(WINCE)

#endif

#if defined(WIN32) || defined(_WIN32)
#undef __WIN32__
#define __WIN32__    1
#endif

#endif
