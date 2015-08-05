/*Copyright 2011 Alibaba Group*/
#ifndef C_ZIP_UTILS_H
#define C_ZIP_UTILS_H

#include "zip/czip.h"

EXTERN_C bool cZipUnpack(ZipHandle zip, const char* path);

EXTERN_C bool cZipUnpackFile(const char* zipFname, const char* targetPath);

EXTERN_C bool cZipPackDir(const char* zipFname, const char* aSrcPath);

#endif
