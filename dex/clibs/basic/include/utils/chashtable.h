/*Copyright 2011 Alibaba Group*/
#ifndef CLIBS_HASHTABLE_H
#define CLIBS_HASHTABLE_H

#include "std/cstd.h"

typedef ULong HashTableHandle;
typedef UInt32 (*HashCodeFunc)(const void* data, UInt32 len);
typedef bool (*TraversalActionFunc)(void* key, UInt32 keySize, void* value, UInt32 size, void* arg);
typedef void* (*HashMallocFunc)(UInt32 size);
typedef void (*HashFreeFunc)(void* ptr);

#define CHASH_MODE_STORE_NONE                        0
#define CHASH_MODE_STORE_KEY                        1
#define CHASH_MODE_STORE_ALL                        2
EXTERN_C HashTableHandle cHashCreate(HashCodeFunc hashFunc, UInt32 initCapacity, Float32 loadFactor, UInt32 mode);

EXTERN_C HashTableHandle cHashCreateEx(HashCodeFunc hashFunc, HashMallocFunc mallocFunc, HashFreeFunc freeFunc,
        UInt32 initCapacity, Float32 loadFactor, UInt32 mode);

EXTERN_C bool cHashInsert(HashTableHandle hth, const void* key, UInt32 keySize, const void* data, UInt32 dataSize);

EXTERN_C void* cHashGet(HashTableHandle hth, const void* key, UInt32 keySize, UInt32* dataSize);

EXTERN_C void cHashRemove(HashTableHandle hth, const void* key, UInt32 keySize);

EXTERN_C void cHashTraversal(HashTableHandle hth, TraversalActionFunc action, void* arg);

EXTERN_C void cHashFree(HashTableHandle hth);

EXTERN_C UInt32 cHashCount(HashTableHandle hth);

#endif
