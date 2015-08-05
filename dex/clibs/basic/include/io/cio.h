/*Copyright 2011 Alibaba Group*/
/*
 *  io.h
 *  madagascar
 *
 *  Created by Misa.Z on 2/22/09.
 *  Copyright 2009 Virtualnology. All rights reserved.
 *
 */

#ifndef CLIBS_IO_H
#define CLIBS_IO_H

#include "std/cstd.h"

#define MADA_MAX_PATH_LEN             512
#define MADA_MAX_STORAGE_PATH_LEN    63

/**
 * the mode  of open file
 *    MADA_OF_READ: open file just for read
 *    MADA_OF_WRITE: open file for read and write
 *    MADA_OF_APPEND: open file for append
 *    MADA_OF_TRUNC: open file for truncate files
 *  MADA_OF_CREATE: create file if the file is not exist
 */
//windows have defined OF_READ and so on
#define MADA_OF_READ        0x1
#define MADA_OF_WRITE        0x2
#define MADA_OF_APPEND    0x4
#define MADA_OF_TRUNC        0x8
#define MADA_OF_CREATE    0x10


/**
*  file/folder info flag
*  MDFS_ATTR_READ: file is read
*  MDFS_ATTR_WRITE: file is write
*  MDFS_ATTR_HIDDEN: file is hide
*  MDFS_ATTR_SYSTEM: file is system file
*  MDFS_ATTR_VOLUME: file is
*  MDFS_ATTR_DIR: file is folder
*  MDFS_ATTR_ARCHIVE: file is archive file
*  MDFS_LONGNAME_ATTR: file is longname file
*/
#define MDFS_ATTR_READ               0x01
#define MDFS_ATTR_WRITE               0x02
#define MDFS_ATTR_HIDDEN           0x04
#define MDFS_ATTR_SYSTEM           0x08
#define MDFS_ATTR_VOLUME           0x10
#define MDFS_ATTR_DIR              0x20
#define MDFS_ATTR_ARCHIVE          0x40
#define MDFS_LONGNAME_ATTR         0x0F

/*<2010.01.25 add by jonezheng for complile lexgen.*/
#define MDFS_COPYDIR_FOR_LEXGEN_ATTR         0x1234
/*>2010.01.25 add by jonezheng for complile lexgen.*/

/**
 *  file seek flag
 *  SK_SET: offset is relative the start of the file
 *  SK_CUR: offset is relative to the current position
 *  SK_END: offset is relative to the end of the file
 */
#define SK_SET    0x1
#define SK_CUR    0x2
#define SK_END    0x3

typedef struct STFileInfo {
    UInt32 fileAttributes;
    UInt32 lastModified;
    Int32 size;
} FileInfoT;

typedef ULong DirHandleT;
//typedef UInt32 DirHandleT;

/**
*  file/folder Attributes
*  fileAttributes: file Attributes
*  fileSize: file size
*  creationTime: file create time
*  lastAccessTime: last access time
*  lastWriteTime: last modify time
*  fileName: file name, use ut8 format
*/
typedef struct STDirent {
  UInt32 fileAttributes;
  UInt32 fileSize;
  UInt32 creationTime;
  UInt32 lastAccessTime;//not support now
  UInt32 lastWriteTime;//last modified time in seconds
  char fileName[512];    // FIXME size size
} DirentT;

#define DIRENT_isDirectory(d) ((d).fileAttributes & MDFS_ATTR_DIR)
#define DIRENT_isRegular(d) ((d).fileAttributes & MDFS_ATTR_ARCHIVE)
#define DIRENT_name(d) ((d).fileName)


//aaron modify 2009-04-14


//TODO: relative path ../ and ./

//file only operations

EXTERN_C Int32 cIOOpen(const char* fname, UInt32 flags);

EXTERN_C void cIOClose(Int32 fd);

EXTERN_C Int32 cIORead(Int32 fd, void* buf, UInt32 bufSize);

EXTERN_C Int32 cIOReadLine(Int32 fd, BYTE* buf, UInt32 size);

EXTERN_C Int32 cIOWrite(Int32 fd, const void* buf, UInt32 bufSize);

EXTERN_C Int32 cIOSeek(Int32 fd, Int32 offset, UInt8 flags);

EXTERN_C Int32 cIOPosition(Int32 fd);

EXTERN_C bool cIORemove(const char* fname);

EXTERN_C bool cIOGetFileInfo(const char* fname, FileInfoT* info);

EXTERN_C bool cIOGetFDInfo(Int32 fd, FileInfoT* info);

EXTERN_C bool cIOSetFileAttributes(const char* fname, UInt32 attrs);

EXTERN_C Int32 cIOSize(Int32 fd);

EXTERN_C bool cIOTruncate(Int32 fd, UInt32 size);

EXTERN_C bool cIOCopy(const char *aDstFile, const char *aSrcFile);

EXTERN_C bool cIOMove(const char *aDstFile, const char *aSrcFile);

//dir only operations
/**
* @param matchPattern NULL for default, that means match *.*<br>
*             don't support matchPattern yet
* @return 0 if failed
*/
EXTERN_C DirHandleT cIOOpenDir(const char* path, const char* matchPattern);

EXTERN_C bool cIOReadDir(DirHandleT dir, DirentT* info);

EXTERN_C void cIORewindDir(DirHandleT dir);

EXTERN_C void cIOCloseDir(DirHandleT dir);

EXTERN_C bool cIOMakeDir(const char* path);

EXTERN_C bool cIORemoveDir(const char* path);

//folder and file operations
EXTERN_C bool cIOExists(const char* fname);

EXTERN_C bool cIORename(const char*oldName,const char* newName);

EXTERN_C const char* cIORawPath(const char* path);

EXTERN_C const char* cIOSafeName(const char* path);

//aaron modify end

typedef bool (*DirTraversalFuncT)(const char *aRootDirName, const char *aSubName, bool isDir, void* aArgs);

EXTERN_C bool cIOTraversalDir(const char *aRootDirName, UInt32 aAttr, DirTraversalFuncT aCallBack, void* aArgs);

//** added by raymond
EXTERN_C Int32 cIOFsync(Int32 fd);
EXTERN_C Int32 cIOGetGranularity();
EXTERN_C Int32 cIOIsCaseSensitive();
EXTERN_C void cIOGetCanon(char *);
EXTERN_C bool cIOSync(Int32 fd);
//** end

/**
* Sets the time this file was last modified, measured in milliseconds since
* January 1st, 1970, midnight.
*/
EXTERN_C bool cIOSetLastModified(const char *filePath,UInt64 time);

EXTERN_C char cIOSeparatorChar();

EXTERN_C char cIOPathSeparatorChar();


#endif
