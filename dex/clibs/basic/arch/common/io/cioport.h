/*Copyright 2011 Alibaba Group*/
/*
 *  cioport.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/22/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_IO_PORT_H
#define CLIBS_IO_PORT_H


#include "io/cio.h"

//file only operations

EXTERN_C Int32 cIOOpenPort(const char* fname, UInt32 flags);

EXTERN_C void cIOClosePort(Int32 fd);

EXTERN_C Int32 cIOReadPort(Int32 fd, void* buf, UInt32 bufSize);

EXTERN_C Int32 cIOWritePort(Int32 fd, const void* buf, UInt32 bufSize);

EXTERN_C Int32 cIOSeekPort(Int32 fd, Int32 offset, UInt8 flags);

EXTERN_C Int32 cIOPositionPort(Int32 fd);

EXTERN_C bool cIORemovePort(const char* fname);

EXTERN_C bool cIOGetFileInfoPort(const char* fname, FileInfoT* info);

EXTERN_C bool cIOGetFDInfoPort(Int32 fd, FileInfoT* info);

EXTERN_C bool cIOSetFileAttributesPort(const char* fname, UInt32 attrs);

EXTERN_C Int32 cIOSizePort(Int32 fd);

EXTERN_C bool cIOTruncatePort(Int32 fd, UInt32 size);

//dir only operations

/**
* @param matchPattern NULL for default, that means match *.*
* @return 0 if failed
*/
EXTERN_C DirHandleT cIOOpenDirPort(const char* path, const char* matchPattern);

EXTERN_C bool cIOReadDirPort(DirHandleT dir, DirentT* info);

EXTERN_C bool cIORemoveDirPort(const char *pathName);

EXTERN_C void cIORewindDirPort(DirHandleT dir);

EXTERN_C void cIOCloseDirPort(DirHandleT dir);

EXTERN_C bool cIOMakeDirPort(const char* path);

EXTERN_C bool cIORemoveDirPort(const char* path);

//folder and file operations
EXTERN_C bool cIOExistsPort(const char* fname);

EXTERN_C bool cIORenamePort(const char*oldName,const char* newName);

//** added by raymond
Int32 cIOFsyncPort(Int32 fd);
Int32 cIOGetGranularityPort();
Int32 cIOIsCaseSensitivePort();
void cIOGetCanonPort(char *path);
bool cIOSyncPort(Int32 fd);
//** end

EXTERN_C bool cIOSetLastModifiedPort(const char *filePath,UInt64 time);

EXTERN_C char cIOSeparatorCharPort();

EXTERN_C char cIOPathSeparatorCharPort();

#endif
