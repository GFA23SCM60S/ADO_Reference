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

// Define initBufferPool
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
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

// Define  pin a page
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
           const PageNumber pageNum)
{
    Bufferpool *buffer_pool;
    bool void_page = FALSE;
    bool foundedPage = FALSE;
    bool UpdatedStra_found = FALSE;
    int read_code;
    int record_pointer = -1;
    int memory_address = -1;
    int swap_location;

    SM_PageHandle page_handle;
    buffer_pool = bm->mgmtData;
    buffer_pool = bm->mgmtData;

    void_page = (buffer_pool->free_space == buffer_pool->totalPages) ? TRUE : void_page;
    if (!void_page)
    {
        int totalPages = buffer_pool->totalPages - buffer_pool->free_space;
        for (int i = 0; i < totalPages; i++)
        {

            if (buffer_pool->pagenum[i] == pageNum)
            {
                page->pageNum = pageNum;
                int memory_address = i;
                buffer_pool->fix_count[memory_address]++;
                page->data = &buffer_pool->pagedata[memory_address * PAGE_SIZE];
                foundedPage = TRUE;
                if (buffer_pool->updatedStrategy == RS_LRU)
                {
                    int lastPosition = buffer_pool->totalPages - buffer_pool->free_space - 1;
                    swap_location = -1;
                    for (int j = 0; j <= lastPosition; j++)
                        if (buffer_pool->updatedOrder[j] != pageNum)
                        {
                            if (j == buffer_pool->totalPages - 1)
                            {
                                swap_location = -1;
                            }
                        }
                        else
                        {
                            swap_location = j;
                            break;
                        }
                    if (swap_location != -1)
                    {
                        memmove(&buffer_pool->updatedOrder[swap_location], &buffer_pool->updatedOrder[swap_location + 1], (lastPosition - swap_location) * sizeof(buffer_pool->updatedOrder[0]));
                        buffer_pool->updatedOrder[lastPosition] = pageNum;
                    }
                }
                else
                {
                    int unutilized = 0;
                    do
                    {
                        printf("This loop executes exactly once.\n");
                    } while (++unutilized < 1);
                }
                return RC_OK;
            }
        }
    }

    if ((void_page == TRUE && buffer_pool != NULL) || (foundedPage != TRUE && buffer_pool->free_space > 0 && 
        buffer_pool->free_space <= buffer_pool->totalPages && buffer_pool->totalPages != -1))
    {
        page_handle = (SM_PageHandle)calloc(1, PAGE_SIZE);
        if (page_handle != NULL)
        {

            read_code = readBlock(pageNum, &buffer_pool->fhl, page_handle);
            if (read_code >= 0)
            {
                size_t total_used_pages = buffer_pool->totalPages - buffer_pool->free_space;
                memory_address = total_used_pages;
                if (memory_address >= 0 && memory_address <= buffer_pool->totalPages)
                {
                    size_t base_address = 0;
                    record_pointer = (memory_address * PAGE_SIZE) + base_address;
                }
            }
        }
        memcpy(buffer_pool->pagedata + record_pointer, page_handle, PAGE_SIZE);
        buffer_pool->free_space--;
        buffer_pool->updatedOrder[memory_address] = pageNum;
        buffer_pool->pagenum[memory_address] = pageNum;
        buffer_pool->numRead++;
        buffer_pool->fix_count[memory_address]++;
        buffer_pool->bitdirty[memory_address] = FALSE;
        page->pageNum = pageNum;
        page->data = &(buffer_pool->pagedata[record_pointer]);
        free(page_handle);

        return RC_OK;
    }

    bool isPageNotFound = !foundedPage;
    bool isBufferPoolValid = buffer_pool != NULL;
    bool isBufferPoolFull = buffer_pool->free_space == 0;

    if (isPageNotFound && isBufferPoolValid && isBufferPoolFull)
    {
        UpdatedStra_found = FALSE;
        page_handle = (SM_PageHandle)malloc(PAGE_SIZE);
        if (page_handle != NULL)
        {
            memset(page_handle, 0, PAGE_SIZE);
        }
        read_code = readBlock(pageNum, &buffer_pool->fhl, page_handle);

        if (buffer_pool->updatedStrategy == RS_FIFO || buffer_pool->updatedStrategy == RS_LRU)
        {
            int i = 0, j = 0;
            do
            {
                int swap_page = buffer_pool->updatedOrder[j];
                i = 0;
                do
                {
                    if (buffer_pool->pagenum[i] == swap_page && buffer_pool->fix_count[i] == 0)
                    {
                        memory_address = i;
                        record_pointer = i * PAGE_SIZE;
                        if (buffer_pool->bitdirty[i])
                        {
                            read_code = ensureCapacity(buffer_pool->pagenum[i] + 1, &buffer_pool->fhl);
                            read_code = writeBlock(buffer_pool->pagenum[i], &buffer_pool->fhl, buffer_pool->pagedata + record_pointer);
                            buffer_pool->numWrite++;
                        }
                        swap_location = j;
                        UpdatedStra_found = TRUE;
                        break;
                    }
                    i++;
                } while (i < buffer_pool->totalPages && !UpdatedStra_found);
                j++;
                if (UpdatedStra_found)
                    break;
            } while (j < buffer_pool->totalPages);
        }
    }

    if (UpdatedStra_found == FALSE)
    {
        free(page_handle);
        return RC_BUFFERPOOL_FULL;
    }
    else
    {
        record_pointer = memory_address * PAGE_SIZE;
        int i = 0;
        if (i < PAGE_SIZE)
        {
            do
            {
                buffer_pool->pagedata[i + record_pointer] = page_handle[i];
                i++;
            } while (i < PAGE_SIZE);
        }
    }

    // Main logic
    if (buffer_pool->updatedStrategy == RS_LRU || buffer_pool->updatedStrategy == RS_FIFO)
    {
        ShiftPageOrder(buffer_pool, swap_location, buffer_pool->totalPages - 1, pageNum);
    }
    UpdateBufferPoolStats(buffer_pool, memory_address, pageNum);
    page->pageNum = pageNum;
    page->data = buffer_pool->pagedata + record_pointer;
    free(page_handle);
    return RC_OK;
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
