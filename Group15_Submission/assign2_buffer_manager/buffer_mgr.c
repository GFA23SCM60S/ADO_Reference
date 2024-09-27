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
        printf("Error: [flushDirtyPages]: Buffer pool pointer is null.\n");
        return RC_ERROR;
    }

    Bufferpool *buffer = bm->mgmtData;
    int totalPageCount = buffer->totalPages;
    RC result;

    // iterate each pages in the buffer pool
    for (int i = 0; i < totalPageCount; i++)
    {
        // check if the status of the current page whether it is dirty or not
        if (buffer->bitdirty[i])
        {
            int offset = i * PAGE_SIZE;

            // ensure the file capacity for write
            if ((result = ensureCapacity(buffer->pagenum[i] + 1, &buffer->fhl)) != RC_OK)
            {
                return result;
            }

            // write the dirty page to disk
            if ((result = writeBlock(buffer->pagenum[i], &buffer->fhl, buffer->pagedata + offset))!= RC_OK)
            {
                return RC_WRITE_FAILED;
            }

            buffer->numWrite++;
        }
    }
    return RC_OK;
}

static RC freeBufferPool(BM_BufferPool *const bm)

{
    if (bm == NULL) {
        printf("Error: [freeBufferPool]: Buffer pool pointer is null.\n");
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
        printf("Error: [ShiftPageOrder]: Buffer pool pointer is null.\n");
        return;
    }

    int *sequence = bp->updatedOrder;
    for (int i = start; i <= end; i++)
    {
        sequence[i] = sequence[i+1];
    }

    sequence[end] = newPageNum;
}

static void UpdateBufferPoolStats(Bufferpool *bp, int memoryAddress, int pageNum)
{
    if (bp == NULL) {
        printf("Error: [UpdateBufferPoolStats]: Buffer pool pointer is null.\n");
        return;
    }

    bp->fix_count[memoryAddress]++;
    bp->numRead++;
    bp->pagenum[memoryAddress] = pageNum;

    // mark the page as clean - not dirty
    bp->bitdirty[memoryAddress] = FALSE;
}

static void updateLRUOrder(Bufferpool *bp, PageNumber pageNum)
{
    if (bp == NULL) {
        printf("Error: [updateLRUOrder]: Buffer pool pointer is null.\n");
        return;
    }

    // Track where the page is in the buffer pool
    int lastPosition = bp->totalPages - bp->free_space - 1;
    int pageIndex = -1;

    // Find the page in the current order
    for (int i = 0; i <= lastPosition; i++)
    {
        if (bp->updatedOrder[i] == pageNum)
        {
            pageIndex = i;
            break;
        }
    }

    if (pageIndex != -1)
    {
        // Shift all elements after pageIndex up by 1 to fill the gap
        memmove(&bp->updatedOrder[pageIndex], &bp->updatedOrder[pageIndex + 1], (lastPosition - pageIndex) * sizeof(bp->updatedOrder[0]));
        bp->updatedOrder[lastPosition] = pageNum;
    }
    else
    {
        if (bp->free_space == 0)
        {
            int evictedPage = bp->updatedOrder[0];
            memmove(&bp->updatedOrder[0], &bp->updatedOrder[1], lastPosition * sizeof(bp->updatedOrder[0]));
            bp->updatedOrder[lastPosition] = pageNum;
        }
        else
        {
            // if space is there, add the new page to the most recently used position
            bp->updatedOrder[++lastPosition] = pageNum;
            bp->free_space--;
        }
    }
}

static RC findPageInBufferPool(Bufferpool *bp, BM_PageHandle *page, PageNumber pageNum, bool *pageFound)
{
    if (bp == NULL || page == NULL) {
        printf(bp == NULL ? "Error: [findPageInBufferPool]: Buffer pool pointer is null.\n" : "Error: page handle pointer is null.\n");
        return RC_ERROR;
    }

    int memory_address = -1;
    for (int i = 0; i < bp->totalPages - bp->free_space; i++)
    {
        if (bp->pagenum[i] == pageNum)
        {
            page->pageNum = pageNum;
            memory_address = i;
            bp->fix_count[memory_address]++;
            page->data = &bp->pagedata[memory_address * PAGE_SIZE];
            *pageFound = TRUE;
            updateLRUOrder(bp, pageNum);
            return RC_OK;
        }
    }

    return RC_IM_KEY_NOT_FOUND;
}

static RC addNewPageToBufferPool(Bufferpool *bp, BM_PageHandle *page, PageNumber pageNum)
{
    if (bp == NULL || page == NULL) {
        printf(bp == NULL ? "Error: [addNewPageToBufferPool]: Buffer pool pointer is null.\n" : "Error: page handle pointer is null.\n");
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
    bp->numRead++;
    bp->fix_count[memory_address]++;
    bp->bitdirty[memory_address] = FALSE;
    bp->updatedOrder[memory_address] = bp->pagenum[memory_address] = page->pageNum = pageNum;
    page->data = &(bp->pagedata[record_pointer]);

    free(page_handle);
    return RC_OK;
}

static RC replacePage(Bufferpool *bp, BM_PageHandle *page, PageNumber pageNum)
{
    if (bp == NULL || page == NULL) {
        printf(bp == NULL ? "Error: [replacePage]: Buffer pool pointer is null.\n" : "Error: page handle pointer is null.\n");
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
    if (bm == NULL || pageFileName == NULL) {
        printf(bm == NULL ? "Error: [initBufferPool]: Buffer pool pointer is null.\n" : "Error: page filename pointer is null.\n");
        return RC_ERROR;
    }

    SM_FileHandle fh;
    Bufferpool *bp;
    int rcode;

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
        printf("Error: [shutdownBufferPool]: Buffer pool pointer is null.\n");
        return RC_ERROR;
    }

    Bufferpool *buffer = bm->mgmtData;
    RC result;

    // check if any page from the buff-pool, still in the use
    for (int i = 0; i < buffer->totalPages; i++)
    {
        if (buffer->fix_count[i] > 0)
        {
            return RC_BUFFERPOOL_IN_USE;
        }
    }

    // flush all dirty pages
    if ((result = flushDirtyPages(bm))!= RC_OK)
    {
        return result;
    }

    // close the file
    if (closePageFile(&buffer->fhl) != RC_OK)
    {
        return RC_CLOSE_FAILED;
    }

    // free the mem-pool
    freeBufferPool(bm);
    return RC_OK;
}

// Define flush the bufferpool
RC forceFlushPool(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        printf("Error: [forceFlushPool]: Buffer pool pointer is null.\n");
        return RC_ERROR;
    }

    int returnCode = RC_OK;
    Bufferpool *poolData = bm->mgmtData;

    for (int index = 0; poolData != NULL && index < poolData->totalPages; index++)
    {
        if (poolData->fix_count[index] == 0 && poolData->bitdirty[index] == TRUE)
        {
            FILE *filePointer = fopen(poolData->fhl.fileName, "r+");
            long necessarySize = (poolData->pagenum[index] + 1) * PAGE_SIZE;
            long currentSize = ftell(filePointer);
            fseek(filePointer, 0, SEEK_END);
            if (currentSize < necessarySize)
            {
                fseek(filePointer, necessarySize - 1, SEEK_SET);
                fputc('\0', filePointer);
            }

            fseek(filePointer, poolData->pagenum[index] * PAGE_SIZE, SEEK_SET);
            if (fwrite(poolData->pagedata + (index * PAGE_SIZE), sizeof(char), PAGE_SIZE, filePointer) != PAGE_SIZE)
            {
                fclose(filePointer);
                printf("Error: fwrite failed");
                returnCode = RC_WRITE_FAILED;
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
        printf(bm == NULL ? "Error: [pinPage]: Buffer pool pointer is null.\n" : "Error: page handle pointer is null.\n");
        return RC_ERROR;
    }

    Bufferpool *buffer_pool = bm->mgmtData;
    bool void_page = (buffer_pool->free_space == buffer_pool->totalPages);
    bool foundedPage = FALSE;
    int pinnedPages = 0;

    // Check if the page number is valid (non-negative)
    if (pageNum < 0) {
        printf("Error: [pinPage]: Negative page number.\n");
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Count how many pages are pinned (fix_count > 0)
    for (int i = 0; i < buffer_pool->totalPages; i++) {
        if (buffer_pool->fix_count[i] > 0) {
            pinnedPages++;
        }
    }

    if (pinnedPages == buffer_pool->totalPages) {
        printf("All pages are pinned. Cannot pin more pages.\n");
        return RC_BUFFERPOOL_FULL;
    }

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
    if (bm == NULL || page == NULL) {
        printf(bm == NULL ? "Error: [unpinPage]: Buffer pool pointer is null.\n" : "Error: page handle pointer is null.\n");
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

    if (pageIndex != -1 && pool->fix_count[pageIndex] > 0) {
        pool->fix_count[pageIndex]--;
        return RC_OK;
    }

    return RC_ERROR;
}

// Define mark a page dirty
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    if (bm == NULL || page == NULL) {
        printf(bm == NULL ? "Error: [markDirty]: Buffer pool pointer is null.\n" : "Error: page handle pointer is null.\n");
        return RC_ERROR;
    }

    Bufferpool *pool = bm->mgmtData;

    for (int index = 0; index < pool->totalPages; index++) {
        if (pool->pagenum[index] == page->pageNum && pool->bitdirty[index] != TRUE) {
            pool->bitdirty[index] = TRUE;
            break;
        }
    }
    return RC_OK;
}

// Define force a page
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    if (bm == NULL || page == NULL) {
        printf(bm == NULL ? "Error: [forcePage]: Buffer pool pointer is null.\n" : "Error: page handle pointer is null.\n");
        return RC_ERROR;
    }

    Bufferpool *bufferPool = bm->mgmtData;

    for (int index = 0; index < bufferPool->totalPages; index++) {
        if (bufferPool->pagenum[index] == page->pageNum) {
            int diskPosition = index * PAGE_SIZE;
            bufferPool->numWrite++;
            bufferPool->bitdirty[index] = FALSE;
            printf("Writing page %d to disk at offset %d.\n", page->pageNum, diskPosition);
            return RC_OK;  // Return immediately upon finding the page
        }
    }

    printf("Unable to find page %d in the buffer pool.\n", page->pageNum);
    return RC_WRITE_FAILED;
}

// Define fixed counts of the pages
int *getFixCounts(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL)
    {
        printf(bm == NULL ? "Error: [getFixCounts]: Buffer pool pointer is null.\n" : "Error: Management data pointer is null.\n");
        return NULL;
    }

    Bufferpool *mgmtData = (Bufferpool *)bm->mgmtData;
    static int noPagesInUse = -1;
    if (mgmtData->free_space == mgmtData->totalPages) {
        // no pages in use
        noPagesInUse = 0;
        return &noPagesInUse;
    } else {
        // return fix count of pages that are in use
        return mgmtData->fix_count;
    }
}

// Define the number of pages that have been read
int getNumReadIO(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        printf(bm == NULL ? "Error: [getNumReadIO]: Buffer pool pointer is null.\n" : "Error: Management data pointer is null.\n");
        return 0;
    }

    // return the number of read
    return ((Bufferpool *)bm->mgmtData)->numRead;
}

// Define the number of pages written
int getNumWriteIO(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        printf(bm == NULL ? "Error: [getNumWriteIO]: Buffer pool pointer is null.\n" : "Error: Management data pointer is null.\n");
        return 0;
    }

    // return the number of pages written
    return ((Bufferpool *)bm->mgmtData)->numWrite;
}

// Define the page numbers as an array
PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        printf(bm == NULL ? "Error: [getFrameContents]: Buffer pool pointer is null.\n" : "Error: Management data pointer is null.\n");
        return NULL;
    }

    Bufferpool *mgmtData = (Bufferpool *)bm->mgmtData;
    if (mgmtData->free_space == mgmtData->totalPages) {
        return NULL;
    }

    // return the page number
    return mgmtData->pagenum;
}

// Define array of dirty page
bool *getDirtyFlags(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        printf(bm == NULL ? "Error: [getDirtyFlags]: Buffer pool pointer is null.\n" : "Error: Management data pointer is null.\n");
        return NULL;
    }

    // return the dirty flag
    return ((Bufferpool *)bm->mgmtData)->bitdirty;
}
