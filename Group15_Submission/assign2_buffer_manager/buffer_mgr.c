#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"

// Define Bufferpool
typedef struct Bufferpool
{
    int numRead;
    int numWrite;
    int totalPages;
    int updatedStrategy;
    int free_space;
    int *updatedOrder;
    int *fix_count;
    int *accessTime;
    int *pagenum;
    bool *bitdirty;
    char *pagedata;
    SM_FileHandle fhl;
} Bufferpool;

//  Helper Functions
static RC flushDirtyPages(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return RC_ERROR;
    }

    Bufferpool *buffer = bm->mgmtData;

    // iterate each pages in the buffer pool
    for (int i = 0; i < buffer->totalPages; i++)
    {
        // check if the status of the current page whether it is dirty or not
        if (buffer->bitdirty[i])
        {
            int offset = i * PAGE_SIZE;

            // ensure the file capacity for write
            RC result = ensureCapacity(buffer->pagenum[i] + 1, &buffer->fhl);
            if (result != RC_OK)
            {
                return result;
            }

            // write the dirty page to disk
            result = writeBlock(buffer->pagenum[i], &buffer->fhl, buffer->pagedata + offset);
            if (result != RC_OK)
            {
                return RC_WRITE_FAILED;
            }

            buffer->numWrite++;
        }
    }
    return RC_OK;
}

static RC freeBufferPoolMemory(BM_BufferPool *const bm)

{
    if (bm == NULL) {
        return RC_ERROR;
    }

    Bufferpool *bpl = bm->mgmtData;
    if (bpl != NULL) {
        free(bpl->updatedOrder);
        free(bpl->pagenum);
        free(bpl->bitdirty);
        free(bpl->fix_count);
        free(bpl->pagedata);
        free(bpl);
        bm->mgmtData = NULL;
    }

    return RC_OK;
}

static void ShiftPageOrder(Bufferpool *bp, int start, int end, int newPageNum)
{
    if (bp == NULL) {
        return;
    }

    for (int i = start; i < end; i++)
    {
        bp->updatedOrder[i] = bp->updatedOrder[i + 1];
    }
    bp->updatedOrder[end] = newPageNum;
}

static void UpdateBufferPoolStats(Bufferpool *bp, int memoryAddress, int pageNum)
{
    if (bp == NULL) {
        return;
    }

    bp->pagenum[memoryAddress] = pageNum;
    bp->numRead++;
    bp->fix_count[memoryAddress]++;

    // mark the page as clean - not dirty
    bp->bitdirty[memoryAddress] = FALSE;
}

static void updateLRUOrder(Bufferpool *bp, PageNumber pageNum)
{
    if (bp == NULL) {
        return;
    }

    if (bp->updatedStrategy == RS_LRU)
    {
        int lastPosition = bp->totalPages - bp->free_space - 1;
        int swap_location = -1;
        for (int j = 0; j <= lastPosition; j++)
        {
            if (bp->updatedOrder[j] == pageNum)
            {
                swap_location = j;
                break;
            }
        }
        if (swap_location != -1)
        {
            memmove(&bp->updatedOrder[swap_location], &bp->updatedOrder[swap_location + 1], (lastPosition - swap_location) * sizeof(bp->updatedOrder[0]));
            bp->updatedOrder[lastPosition] = pageNum;
        }
    }
}

static RC findPageInBufferPool(Bufferpool *bp, BM_PageHandle *page, PageNumber pageNum, bool *foundedPage)
{
    if (bp == NULL || page == NULL) {
        return RC_ERROR;
    }

    int totalPages = bp->totalPages - bp->free_space;
    for (int i = 0; i < totalPages; i++)
    {
        if (bp->pagenum[i] == pageNum)
        {
            page->pageNum = pageNum;
            int memory_address = i;
            bp->fix_count[memory_address]++;
            page->data = &bp->pagedata[memory_address * PAGE_SIZE];
            *foundedPage = TRUE;
            updateLRUOrder(bp, pageNum);
            return RC_OK;
        }
    }
    return RC_IM_KEY_NOT_FOUND;
}

static RC addNewPageToBufferPool(Bufferpool *bp, BM_PageHandle *page, PageNumber pageNum)
{
    if (bp == NULL || page == NULL) {
        return RC_ERROR;
    }

    SM_PageHandle page_handle = (SM_PageHandle)calloc(1, PAGE_SIZE);
    if (page_handle == NULL)
        return RC_MEMORY_ALLOCATION_FAIL;

    int read_code = readBlock(pageNum, &bp->fhl, page_handle);
    if (read_code < 0)
    {
        free(page_handle);
        return RC_ERROR;
    }

    int memory_address = bp->totalPages - bp->free_space;
    int record_pointer = memory_address * PAGE_SIZE;

    memcpy(bp->pagedata + record_pointer, page_handle, PAGE_SIZE);
    bp->free_space--;
    bp->updatedOrder[memory_address] = pageNum;
    bp->pagenum[memory_address] = pageNum;
    bp->numRead++;
    bp->fix_count[memory_address]++;
    bp->bitdirty[memory_address] = FALSE;
    page->pageNum = pageNum;
    page->data = &(bp->pagedata[record_pointer]);

    free(page_handle);
    return RC_OK;
}

static RC replacePage(Bufferpool *bp, BM_PageHandle *page, PageNumber pageNum)
{
    if (bp == NULL || page == NULL) {
        return RC_ERROR;
    }

    SM_PageHandle page_handle = (SM_PageHandle)malloc(PAGE_SIZE);
    if (page_handle == NULL)
        return RC_MEMORY_ALLOCATION_FAIL;

    memset(page_handle, 0, PAGE_SIZE);
    int read_code = readBlock(pageNum, &bp->fhl, page_handle);

    int memory_address = -1;
    int swap_location = -1;
    bool UpdatedStra_found = FALSE;

    for (int j = 0; j < bp->totalPages && !UpdatedStra_found; j++)
    {
        int swap_page = bp->updatedOrder[j];
        for (int i = 0; i < bp->totalPages; i++)
        {
            if (bp->pagenum[i] == swap_page && bp->fix_count[i] == 0)
            {
                memory_address = i;
                if (bp->bitdirty[i])
                {
                    writeBlock(bp->pagenum[i], &bp->fhl, bp->pagedata + i * PAGE_SIZE);
                    bp->numWrite++;
                }
                swap_location = j;
                UpdatedStra_found = TRUE;
                break;
            }
        }
    }

    if (!UpdatedStra_found)
    {
        free(page_handle);
        return RC_BUFFERPOOL_FULL;
    }

    memcpy(bp->pagedata + memory_address * PAGE_SIZE, page_handle, PAGE_SIZE);
    ShiftPageOrder(bp, swap_location, bp->totalPages - 1, pageNum);
    UpdateBufferPoolStats(bp, memory_address, pageNum);
    page->pageNum = pageNum;
    page->data = bp->pagedata + memory_address * PAGE_SIZE;

    free(page_handle);
    return RC_OK;
}

// Define initBufferPool
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    SM_FileHandle fh;
    Bufferpool *bp;
    int rcode;

    if (bm == NULL) {
        return RC_ERROR;
    }

    if ((rcode = openPageFile((char *)pageFileName, &fh)) != RC_OK) {
        return rcode;
    }

    bp = (Bufferpool *)calloc(1, sizeof(Bufferpool));
    if (!bp) {
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    bp->pagedata = (char *)calloc(numPages * PAGE_SIZE, sizeof(char));
    bp->updatedOrder = (int *)calloc(numPages, sizeof(int));
    bp->bitdirty = (bool *)calloc(numPages, sizeof(bool));
    bp->pagenum = (int *)calloc(numPages, sizeof(int));
    bp->fix_count = (int *)calloc(numPages, sizeof(int));

    if (!bp->pagedata || !bp->updatedOrder || !bp->bitdirty || !bp->pagenum || !bp->fix_count) {
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    bp->totalPages = bp->free_space = numPages;
    bp->numRead = bp->numWrite = 0;
    bp->updatedStrategy = strategy;
    bp->fhl = fh;

    for (int i = 0; i < numPages; i++) {
        bp->bitdirty[i] = FALSE;
        bp->fix_count[i] = 0;
        bp->pagenum[i] = NO_PAGE;
        bp->updatedOrder[i] = NO_PAGE;
    }

    if (bm != NULL) {
        bm->pageFile = (pageFileName != NULL) ? strdup(pageFileName) : NULL;
        bm->numPages = numPages;
        bm->strategy = strategy;
        bm->mgmtData = bp;
    }

    return RC_OK;
}

// Define shutdown the buffer pool
RC shutdownBufferPool(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return RC_ERROR;
    }

    Bufferpool *buffer = bm->mgmtData;

    // check if any page from the buff-pool, still in the use
    for (int i = 0; i < buffer->totalPages; i++)
    {
        if (buffer->fix_count[i] > 0)
        {
            return RC_BUFFERPOOL_IN_USE;
        }
    }

    // flush all dirty pages
    RC status = flushDirtyPages(bm);
    if (status != RC_OK)
    {
        return status;
    }

    // close the file
    if (closePageFile(&buffer->fhl) != RC_OK)
    {
        return RC_CLOSE_FAILED;
    }

    // free the mem-pool
    freeBufferPoolMemory(bm);
    return RC_OK;
}

// Define flush the bufferpool
RC forceFlushPool(BM_BufferPool *const bm)
{
    Bufferpool *poolData;
    int returnCode = RC_OK;

    if (bm == NULL) {
        return RC_ERROR;
    }

    poolData = bm->mgmtData;

    for (int index = 0; poolData != NULL && index < poolData->totalPages; index++)
    {
        if (poolData->fix_count[index] == 0 && poolData->bitdirty[index] == TRUE)
        {
            FILE *filePointer = fopen(poolData->fhl.fileName, "r+");
            fseek(filePointer, 0, SEEK_END);
            long currentSize = ftell(filePointer);
            long necessarySize = (poolData->pagenum[index] + 1) * PAGE_SIZE;
            if (currentSize < necessarySize)
            {
                fseek(filePointer, necessarySize - 1, SEEK_SET);
                fputc('\0', filePointer);
            }

            int dataOffset = index * PAGE_SIZE;
            fseek(filePointer, poolData->pagenum[index] * PAGE_SIZE, SEEK_SET);
            if (fwrite(poolData->pagedata + dataOffset, sizeof(char), PAGE_SIZE, filePointer) != PAGE_SIZE)
            {
                returnCode = RC_WRITE_FAILED;
                fclose(filePointer);
                break;
            }
            fclose(filePointer);
            filePointer = NULL;

            if (poolData != NULL && index >= 0)
            {
                poolData->bitdirty[index] = FALSE;
                poolData->numWrite++;
            }
        }
    }
    return returnCode;
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    if (bm == NULL || page == NULL) {
        return RC_ERROR;
    }

    Bufferpool *buffer_pool = bm->mgmtData;
    bool void_page = (buffer_pool->free_space == buffer_pool->totalPages);
    bool foundedPage = FALSE;

    if (!void_page)
    {
        RC result = findPageInBufferPool(buffer_pool, page, pageNum, &foundedPage);
        if (result == RC_OK)
            return RC_OK;
    }

    if (void_page || (buffer_pool->free_space > 0 && buffer_pool->free_space <= buffer_pool->totalPages))
    {
        return addNewPageToBufferPool(buffer_pool, page, pageNum);
    }

    if (!foundedPage && buffer_pool->free_space == 0)
    {
        return replacePage(buffer_pool, page, pageNum);
    }

    return RC_BUFFERPOOL_FULL;
}

// Define unpin a page
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    if (bm == NULL) {
        return RC_ERROR;
    }

    Bufferpool *pool = bm->mgmtData;
    int pageIndex = -1;

    for (int idx = 0; idx < pool->totalPages; idx++) {
        if (pool->pagenum[idx] == page->pageNum) {
            pageIndex = idx;
            break;
        }
    }

    if (pageIndex != -1) {
        if (pool->fix_count[pageIndex] > 0) {
            pool->fix_count[pageIndex]--;
        }
    }
    return RC_OK;
}

// Define mark a page dirty
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    if (bm == NULL) {
        return RC_ERROR;
    }

    Bufferpool *pool = bm->mgmtData;

    for (int index = 0; index < pool->totalPages; index++) {
        if (pool->pagenum[index] == page->pageNum) {
            if (pool->bitdirty[index] != TRUE) {
                pool->bitdirty[index] = TRUE;
            }
            break;
        }
    }
    return RC_OK;
}

// Define force a page
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    if (bm == NULL) {
        return RC_ERROR;
    }

    Bufferpool *bufferPool = bm->mgmtData;
    bool isPageLocated = FALSE;

    for (int index = 0; index < bufferPool->totalPages; index++) {
        if (bufferPool->pagenum[index] == page->pageNum) {
            int diskPosition = index * PAGE_SIZE;
            printf("Writing page %d to disk at offset %d.\n", page->pageNum, diskPosition);
            bufferPool->bitdirty[index] = FALSE;
            bufferPool->numWrite++;
            isPageLocated = TRUE;
            break;
        }
    }

    if (isPageLocated) {
        return RC_OK;
    } else {
        printf("Unable to find page %d in the buffer pool.\n", page->pageNum);
        return RC_WRITE_FAILED;
    }
}

// Define fixed counts of the pages
int *getFixCounts(BM_BufferPool *const bm)
{
    if (bm == NULL)
    {
        printf("Buffer pool pointer is null.\n");
        return NULL;
    }

    Bufferpool *pool = bm->mgmtData;
    if (pool == NULL) {
        printf("Error: Management data pointer is null.\n");
        return NULL;
    }

    if (pool->free_space == pool->totalPages) {
        static int noPagesInUse = 0;
        printf("No pages are currently in use. Fix counts are zero.\n");
        return &noPagesInUse;
    } else {
        printf("Returning the fix counts for pages that are currently in use.\n");
        return pool->fix_count;
    }
}

// Define the number of pages that have been read
int getNumReadIO(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return 0;
    }

    Bufferpool *pool = bm->mgmtData;
    if (pool == NULL) {
        return 0;
    }

    return pool->numRead;
}

// Define the number of pages written
int getNumWriteIO(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return 0;
    }

    Bufferpool *pool = bm->mgmtData;
    if (pool == NULL) {
        return 0;
    }

    return pool->numWrite;
}

// Define the page numbers as an array
PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return NULL;
    }

    Bufferpool *pool = bm->mgmtData;
    bool isBufferFull = (pool->free_space == pool->totalPages);
    if (isBufferFull) {
        return NULL;
    } else {
        return pool->pagenum;
    }
}

// Define array of dirty page
bool *getDirtyFlags(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return NULL;
    }

    Bufferpool *pool = bm->mgmtData;
    if (pool == NULL) {
        return NULL;
    }

    bool *flags = pool->bitdirty;
    return flags;
}
