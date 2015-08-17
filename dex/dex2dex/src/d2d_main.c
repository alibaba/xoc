/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: GongKai, JinYue
@*/
#include <stdio.h>
#include <malloc.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "trace/ctrace.h"
#include "libdex/DexFile.h"
#include "libdex/DexClass.h"
#include "libdex/CmdUtils.h"
#include "str/cstr.h"
#include "liropcode.h"
#include "d2d_d2l.h"
#include "d2d_main.h"
#include "utils/cbytestream.h"

static void DumpClassName(DexFile * pDexFile)
{
    unsigned int clsN = pDexFile->pHeader->classDefsSize;
    FILE * h = fopen("n.tmp","a+");
    for (unsigned int i = 0; i < clsN; i++) {
       DexClassDef const* pDexClassDef = dexGetClassDef(pDexFile, i);
       char const* name = dexGetClassDescriptor(pDexFile,pDexClassDef);
       fprintf(h, "\n    \"%s\",", name);
    }
    fclose(h);
}

int d2dTest(int argc, const char* argv[])
{
    if(argc < 2)
    {
        LOGE("Need path name!\n");
        return -1;
    }

    const char* dexPath = argv[1];

    int fd = open(dexPath, O_RDWR);

    if (fd < 0)
    {
        LOGE("Open error %s\n", strerror(errno));
        return errno;
    }

    long fileLen;
    if(d2dEntry(fd, &fileLen, false) != 0)
    {
        LOGE("do dex2dex error!\n");
        return errno;
    }

    if (fd) {
        close(fd);
	}
    return 0;
}


//'dxbuf': Buffer for dex file memory mapped.
bool d2dEntryBuf(unsigned char ** dxbuf, unsigned int* dxbuflen, unsigned int* cbsHandler)
{
    bool result = false;
    Int32 error = 0;

    DexFile * pDexFile = dexFileParse((const u1*)*dxbuf, *dxbuflen, kDexParseDefault);
    if (NULL == pDexFile) {
        LOGE("LEMUR:pDexFile is null: DEX parse failed for \n");
        goto END;
    }

    //DumpClassName(pDexFile);
    pDexFile->pClassLookup = dexCreateClassLookup(pDexFile);

    //TODO free buffer
    error = doDex2Dex2(pDexFile, dxbuf, dxbuflen, cbsHandler);
    if (error != 0) {
        goto END;
    }
    result = true;

END:
    if (NULL != pDexFile) {
        dexFileFree(pDexFile);
    }
    return result;
}

void d2dEntryBufFree(unsigned int cbsHandler){
    if(cbsHandler != 0) {
        cbsDestroy(cbsHandler);
    }
}

int d2dEntry(int dexFd, long* fileLen, bool ifOpt)
{
    Int32 error = 0;
    DexFile* pDexFile = NULL;
    MemMapping map;
    bool mapped = false;

    UnzipToFileResult utfr = kUTFRSuccess;
    lseek(dexFd, 0L, SEEK_SET);

    if (sysMapFileInShmemWritableReadOnly(dexFd, &map) != 0) {
        error = -1;
        LOGE("ERROR: Unable to map \n");
        goto END;
    }
    mapped = true;

    if (ifOpt) {
        map.addr = (u1*)(map.addr) + sizeof(DexOptHeader);
        map.length -= sizeof(DexOptHeader);
    }

    pDexFile = dexFileParse((const u1*)(map.addr), map.length, kDexParseDefault);
    if (NULL == pDexFile) {
        LOGE("LEMUR:pDexFile is null: DEX parse failed for \n");
        error = -1;
        goto END;
    }

    pDexFile->pClassLookup = dexCreateClassLookup(pDexFile);
    error = doDex2Dex(pDexFile, dexFd, fileLen, ifOpt);
END:
    if (mapped) {
        sysReleaseShmem(&map);
    }

    if (NULL != pDexFile) {
        dexFileFree(pDexFile);
    }

    return error;
}
