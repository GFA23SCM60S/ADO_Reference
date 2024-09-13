#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include "const.h"
#include "dberror.h"
//#include "data_structures.h"
#include "storage_mgr.h"

// Include bool DT
#include "dt.h"

// 1250 * 8KB(PAGE_SIZE is 8196) = 10MB, assuming we will need 10MB for every pool, 1259 is the closest prime number to 1250
#define HASH_LEN 1259


typedef struct Node {
  void *data; // general pointer to be used with different data types
  struct Node *next;
  struct Node *previous;
} Node;

// hashmap
typedef struct HM {
  Node *tbl[HASH_LEN]; // table of linked list to solve hashmap collision
} HM;

// Replacement Strategies
typedef enum ReplacementStrategy {
  RS_FIFO = 0,
  RS_LRU = 1,
  RS_CLOCK = 2,
  RS_LFU = 3,
  RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

typedef struct Statistics {
  int *pageNumbers;
  int *fixCounts;
  bool *dirtyFlags;
  int readIO; // disk read count
  int writeIO; // disk write count

} Statistics;

typedef struct PoolMgmtInfo {
  HM *hm; // hashmap for direct access

  // head and tail for linked list
  Node *head;
  Node *tail;

  Statistics *statistics;

  SM_FileHandle *fHandle;
} PoolMgmtInfo;

typedef struct BM_BufferPool {
  char *pageFile;
  int numPages;
  ReplacementStrategy strategy;
  void *mgmtData; // use this one to store the bookkeeping info your buffer
                  // manager needs for a buffer pool
} BM_BufferPool;

typedef struct PageFrameInfo {
  int statisticsPosition;
  PageNumber pageNum;
  int fixCount;
  bool dirty;
  char *pageData;
} PageFrameInfo;

typedef struct BM_PageHandle {
  PageNumber pageNum;
  char *data;
} BM_PageHandle;

// convenience macros
#define MAKE_POOL()					\
  ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
  ((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		  const int numPages, ReplacementStrategy strategy,
		  void *stratData);
RC shutdownBufferPool(BM_BufferPool *const bm);
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
	    const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
bool *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);

#endif
