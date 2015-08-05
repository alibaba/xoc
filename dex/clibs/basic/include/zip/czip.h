/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_ZIP_H
#define CLIBS_ZIP_H

#include "std/cstd.h"

typedef unsigned long ZipHandle;
typedef unsigned long ZipFile;

typedef bool (*ZipTraversalFunc)(ZipHandle zip, ZipFile zfile, void* args);

/** create a zip struct.
 *  @param bufSize: size of compress data buffer.
 */
EXTERN_C ZipHandle cZipCreate(UInt32 bufSize);

/** open a zip file, create zip struct from file data */
EXTERN_C ZipHandle cZipOpen(const char* fname, UInt32 bufSize);

#define Zip_Comp_None                                            0
#define Zip_Comp_Deflated                                        8
/** for a zip, only one alive added file allow at same time, next call of 'cZipAddFile', previous added ZipFile will be close. */
EXTERN_C bool cZipAddFile(ZipHandle zip, const char* name, UInt16 nameLen, UInt16 compMode);

EXTERN_C bool cZipAddFileData(ZipHandle zip, BYTE* data, UInt32 size);

EXTERN_C void cZipAddFileEnd(ZipHandle zip);

EXTERN_C bool cZipAddDirectory(ZipHandle zip, const char* path, UInt16 pathLen);

/** remove a file or directory from zip.
 *  if 'pathName' is '/' terminate, means remove directory. '/' can't be begin of pathname.
 */
EXTERN_C bool cZipRemove(ZipHandle zip, const char* pathName, UInt16 nameLen);

/** write data to zip file */
EXTERN_C bool cZipWrite(ZipHandle zip, const char* name);

EXTERN_C ZipFile cZipGetFile(ZipHandle zip, const char* name, UInt16 nameLen);

EXTERN_C UInt32 cZipGetFileSize(ZipHandle zip, ZipFile file);

EXTERN_C const char* cZipGetFileName(ZipHandle zip, ZipFile zfile, UInt16* nameLen);

/** @return: actual bytes number got by this function call.
 *  @param 'totalOutSize': total bytes number of this file be got.
 */
EXTERN_C UInt32 cZipGetFileData(ZipHandle zip, ZipFile file, BYTE* buf, UInt32 bufSize, UInt32* totalOutSize);

EXTERN_C bool cZipHave(ZipHandle zip, const char* name, UInt16 nameLen);

EXTERN_C void cZipDestroy(ZipHandle zip);

EXTERN_C ZipFile cZipGetRoot(ZipHandle zip);

EXTERN_C ZipFile cZipGetChild(ZipHandle zip, ZipFile zfile);

EXTERN_C ZipFile cZipGetNext(ZipHandle zip, ZipFile zfile);

EXTERN_C ZipFile cZipGetParent(ZipHandle zip, ZipFile zfile);

EXTERN_C bool cZipIsDirectory(ZipHandle zip, ZipFile zfile);

EXTERN_C bool cZipTraversal(ZipHandle zip, ZipTraversalFunc func, void* args);

EXTERN_C UInt32 cZipFileNumber(ZipHandle zip);

EXTERN_C UInt32 cZipDirNumber(ZipHandle zip);

EXTERN_C bool cZipWriteByfd(ZipHandle zip, Int32 fd);

EXTERN_C ZipHandle cZipCreateByMemory(UInt32 bufSize,void* bufferAddr);

EXTERN_C void cZipDestroyByMemory(ZipHandle zip);

#endif
