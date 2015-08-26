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
#include "drAlloc.h"
#include "ltype.h"
#include "xassert.h"

#define PIG_SIZE (4096)
#define DEFAULT_ALLOC_SIZE (PIG_SIZE*32)
#define DEFAULT_BLOCK_SIZE (DEFAULT_ALLOC_SIZE - sizeof(DRMBlock))

static bool linearInited = false;
static DRMBlock *blockHead,*currentBlock;
static int numBlocks;

inline bool drLinearIsInited(void){
    return linearInited;
}

bool drLinearInit(void){
    blockHead = (DRMBlock*)malloc(DEFAULT_ALLOC_SIZE+10);
    if(blockHead == NULL){
        LOGE("No memory left to create block memory");
        ABORT();
        return false;
    }
    currentBlock = blockHead;
    blockHead->totalSize = DEFAULT_BLOCK_SIZE;
    blockHead->usedSize = 0;
    blockHead->next = NULL;
    numBlocks = 1;
    UInt32 iii;
    iii = DEFAULT_BLOCK_SIZE;
    *(UInt32*)(currentBlock->ptr+iii) = 0xdeaddaad;

    linearInited = true;
    return true;
}

void* drLinearAlloc(size_t size){
    size = (size + 3) & ~3;
retry:
    if(size + currentBlock->usedSize <= currentBlock->totalSize){
        void* ptr;
        ptr = &currentBlock->ptr[currentBlock->usedSize];
        currentBlock->usedSize += size;
        return ptr;
    }else{
        if (currentBlock->next) {
            currentBlock = currentBlock->next;
            goto retry;
        }

        DRMBlock *newBlock = (DRMBlock *)malloc(DEFAULT_ALLOC_SIZE+10);
        if (newBlock == NULL) {
            LOGE("Block allocation failure, total page %d\n",numBlocks*2);
            ABORT();
        }
        newBlock->totalSize = DEFAULT_BLOCK_SIZE;
        newBlock->usedSize = 0;
        newBlock->next = NULL;
        currentBlock->next = newBlock;
        currentBlock = newBlock;
        *(UInt32*)(currentBlock->ptr+DEFAULT_BLOCK_SIZE) = 0xdeaddaad;
        numBlocks++;
        if (numBlocks > 100){
            LOGI("Total block pages for AOT: %d\n", numBlocks);
        }
        goto retry;
    }
    ABORT();
}

void drLinearReset(void)
{
    DRMBlock *block;

    for (block = blockHead; block; block = block->next) {
        block->usedSize = 0;
    }
    currentBlock = blockHead;
}

void drLinearFree(void)
{
    DRMBlock *block;
    DRMBlock *nextBlock;

    block = blockHead;
    while(true){
        if(block == NULL)
            break;
        nextBlock = block->next;
        ASSERT0(*(UInt32*)(block->ptr+DEFAULT_BLOCK_SIZE) == 0xdeaddaad);
        free(block);
        block = nextBlock;
    }
    currentBlock = blockHead = NULL;
    linearInited = false;
}

