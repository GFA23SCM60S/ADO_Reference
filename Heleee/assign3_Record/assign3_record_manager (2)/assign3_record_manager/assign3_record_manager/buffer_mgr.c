#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

typedef struct Page
{
	SM_PageHandle data;
	PageNumber pageNum;
	int dirtyBit;
	int fixCount;
	int hitNum;
	int refNum;
} PageFrame;

int bufferSize = 0;
int rearIndex = 0;
int writeCount = 0;
int hit = 0;
int clockPointer = 0;
int lfuPointer = 0;

void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
    printf("FIFO Started");
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int frontIndex = rearIndex % bm->numPages;

    while (1) {
        if (pageFrame[frontIndex].fixCount == 0) {

            if (pageFrame[frontIndex].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[frontIndex].pageNum, &fh, pageFrame[frontIndex].data);
                writeCount++;
            }

            pageFrame[frontIndex].data = page->data;
            pageFrame[frontIndex].pageNum = page->pageNum;
            pageFrame[frontIndex].dirtyBit = page->dirtyBit;
            pageFrame[frontIndex].fixCount = page->fixCount;
            break;
        } else {
            frontIndex++;
            frontIndex = (frontIndex % bm->numPages == 0) ? 0 : frontIndex;
        }
    }
}

void LFU(BM_BufferPool *const bm, PageFrame *page) {

    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    int bufferSize = bm->numPages;
    int leastFreqIndex = -1;
    int leastFreqCount = -1;

    for (int i = 0; i < bufferSize; i++) {
        if (pageFrames[i].fixCount == 0) {
            if (leastFreqIndex == -1 || pageFrames[i].refNum < leastFreqCount) {
                leastFreqIndex = i;
                leastFreqCount = pageFrames[i].refNum;
            }
        }
    }

    if (leastFreqIndex == -1) {
        return;
    }

    if (pageFrames[leastFreqIndex].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrames[leastFreqIndex].pageNum, &fh, pageFrames[leastFreqIndex].data);

        writeCount++;
    }

    pageFrames[leastFreqIndex].data = page->data;
    pageFrames[leastFreqIndex].pageNum = page->pageNum;
    pageFrames[leastFreqIndex].dirtyBit = page->dirtyBit;
    pageFrames[leastFreqIndex].fixCount = page->fixCount;
    pageFrames[leastFreqIndex].refNum = 1;  

    for (int i = 0; i < bufferSize; i++) {
        if (i != leastFreqIndex && pageFrames[i].fixCount == 0) {
            pageFrames[i].refNum++;
        }
    }
}

extern void LRU(BM_BufferPool *const bm, PageFrame *page)
{	
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	int i, leastHitIndex, leastHitNum;

	for(i = 0; i < bufferSize; i++)
	{

		if(pageFrame[i].fixCount == 0)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitNum;
			break;
		}
	}	

	for(i = leastHitIndex + 1; i < bufferSize; i++)
	{
		if(pageFrame[i].hitNum < leastHitNum)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitNum;
		}
	}

	if(pageFrame[leastHitIndex].dirtyBit == 1)
	{
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		writeBlock(pageFrame[leastHitIndex].pageNum, &fh, pageFrame[leastHitIndex].data);

		writeCount++;
	}

	pageFrame[leastHitIndex].data = page->data;
	pageFrame[leastHitIndex].pageNum = page->pageNum;
	pageFrame[leastHitIndex].dirtyBit = page->dirtyBit;
	pageFrame[leastHitIndex].fixCount = page->fixCount;
	pageFrame[leastHitIndex].hitNum = page->hitNum;
}

void CLOCK(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    int bufferSize = bm->numPages;

    while (1) {
        clockPointer = (clockPointer % bufferSize == 0) ? 0 : clockPointer;

        if (pageFrames[clockPointer].hitNum == 0) {

            if (pageFrames[clockPointer].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrames[clockPointer].pageNum, &fh, pageFrames[clockPointer].data);

                writeCount++;
            }

            pageFrames[clockPointer].data = page->data;
            pageFrames[clockPointer].pageNum = page->pageNum;
            pageFrames[clockPointer].dirtyBit = page->dirtyBit;
            pageFrames[clockPointer].fixCount = page->fixCount;
            pageFrames[clockPointer].hitNum = page->hitNum;
            clockPointer++;
            break;
        } else {

            pageFrames[clockPointer].hitNum = 0;
            clockPointer++;
        }
    }
}

extern RC initBufferPool(
    BM_BufferPool *const bm, const char *const pageFileName,
                         const int numPages, ReplacementStrategy strategy,
                         void *stratData) {
    bm->pageFile = (char *)pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    PageFrame *pageFrames = (PageFrame *)malloc(sizeof(PageFrame) * numPages);

    for (int i = 0; i < numPages; i++) {
        pageFrames[i].data = NULL;
        pageFrames[i].pageNum = -1;
        pageFrames[i].dirtyBit = 0;
        pageFrames[i].fixCount = 0;
        pageFrames[i].hitNum = 0;
        pageFrames[i].refNum = 0;
    }

    bm->mgmtData = pageFrames;
    bufferSize = numPages;
    writeCount = clockPointer = lfuPointer = 0;

    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

    RC rc = forceFlushPool(bm);

    if (rc != RC_OK) {
        return rc; 
    }

    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrames[i].fixCount != 0) {
            return RC_PINNED_PAGES_IN_BUFFER;
        }
    }

    free(pageFrames);
    bm->mgmtData = NULL;

    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT; 
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].fixCount == 0 && pageFrame[i].dirtyBit == 1) {
            SM_FileHandle fh;
            openPageFile(bm->pageFile, &fh); 
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data); 
            pageFrame[i].dirtyBit = 0; 
            writeCount++; 
        }
    }

    return RC_OK;
}

extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{

    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT;
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].pageNum == page->pageNum) {
            pageFrame[i].dirtyBit = 1; 
            return RC_OK;
        }
    }

    return RC_PAGE_NOT_FOUND;
}

extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{

    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT; 
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].pageNum == page->pageNum) {
            if (pageFrame[i].fixCount > 0) {
                pageFrame[i].fixCount--; 
            } else {
                return RC_PAGE_NOT_PINNED; 
            }
            return RC_OK;
        }
    }

    return RC_PAGE_NOT_FOUND; 
}

extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{

    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT; 
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].pageNum == page->pageNum) {
            if (pageFrame[i].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);

                pageFrame[i].dirtyBit = 0;
                return RC_OK;
            } else {
                return RC_PAGE_NOT_DIRTY; 
            }
        }
    }

    return RC_PAGE_NOT_FOUND; 
}

extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	if(pageFrame[0].pageNum == -1)
	{

		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		pageFrame[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
		ensureCapacity(pageNum,&fh);
		readBlock(pageNum, &fh, pageFrame[0].data);
		pageFrame[0].pageNum = pageNum;
		pageFrame[0].fixCount++;
		rearIndex = hit = 0;
		pageFrame[0].hitNum = hit;	
		pageFrame[0].refNum = 0;
		page->pageNum = pageNum;
		page->data = pageFrame[0].data;

		return RC_OK;		
	}
	else
	{	
		int i;
		bool isBufferFull = true;

		for(i = 0; i < bufferSize; i++)
		{
			if(pageFrame[i].pageNum != -1)
			{	

				if(pageFrame[i].pageNum == pageNum)
				{

					pageFrame[i].fixCount++;
					isBufferFull = false;
					hit++; 

					if(bm->strategy == RS_LRU)

						pageFrame[i].hitNum = hit;
					else if(bm->strategy == RS_CLOCK)

						pageFrame[i].hitNum = 1;
					else if(bm->strategy == RS_LFU)

						pageFrame[i].refNum++;

					page->pageNum = pageNum;
					page->data = pageFrame[i].data;

					clockPointer++;
					break;
				}				
			} else {
				SM_FileHandle fh;
				openPageFile(bm->pageFile, &fh);
				pageFrame[i].data = (SM_PageHandle) malloc(PAGE_SIZE);
				readBlock(pageNum, &fh, pageFrame[i].data);
				pageFrame[i].pageNum = pageNum;
				pageFrame[i].fixCount = 1;
				pageFrame[i].refNum = 0;
				rearIndex++;	
				hit++; 

				if(bm->strategy == RS_LRU)

					pageFrame[i].hitNum = hit;				
				else if(bm->strategy == RS_CLOCK)

					pageFrame[i].hitNum = 1;

				page->pageNum = pageNum;
				page->data = pageFrame[i].data;

				isBufferFull = false;
				break;
			}
		}

		if(isBufferFull == true)
		{

			PageFrame *newPage = (PageFrame *) malloc(sizeof(PageFrame));		

			SM_FileHandle fh;
			openPageFile(bm->pageFile, &fh);
			newPage->data = (SM_PageHandle) malloc(PAGE_SIZE);
			readBlock(pageNum, &fh, newPage->data);
			newPage->pageNum = pageNum;
			newPage->dirtyBit = 0;		
			newPage->fixCount = 1;
			newPage->refNum = 0;
			rearIndex++;
			hit++;

			if(bm->strategy == RS_LRU)

				newPage->hitNum = hit;				
			else if(bm->strategy == RS_CLOCK)

				newPage->hitNum = 1;

			page->pageNum = pageNum;
			page->data = newPage->data;			

			switch(bm->strategy)
			{			
				case RS_FIFO: 
					FIFO(bm, newPage);
					break;

				case RS_LRU: 
					LRU(bm, newPage);
					break;

				case RS_CLOCK: 
					CLOCK(bm, newPage);
					break;

				case RS_LFU: 
					LFU(bm, newPage);
					break;

				case RS_LRU_K:
					printf("\n LRU-k algorithm not implemented");
					break;

				default:
					printf("\nAlgorithm Not Implemented\n");
					break;
			}

		}		
		return RC_OK;
	}	
}

extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL; 
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    PageNumber *frameContents = (PageNumber *)malloc(sizeof(PageNumber) * bm->numPages);

    if (frameContents == NULL) {
        return NULL; 
    }

    for (int i = 0; i < bm->numPages; i++) {
        frameContents[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
    }

    return frameContents;
}

extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL; 
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    bool *dirtyFlags = (bool *)malloc(sizeof(bool) * bm->numPages);

    if (dirtyFlags == NULL) {
        return NULL; 
    }

    for (int i = 0; i < bm->numPages; i++) {
        dirtyFlags[i] = (pageFrame[i].dirtyBit == 1) ? true : false;
    }

    return dirtyFlags;
}

extern int *getFixCounts(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL; 
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int *fixCounts = (int *)malloc(sizeof(int) * bm->numPages);

    if (fixCounts == NULL) {
        return NULL; 
    }

    for (int i = 0; i < bm->numPages; i++) {
        fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0;
    }

    return fixCounts;
}

extern int getNumReadIO(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return -1; 
    }

    return (rearIndex + 1);
}

extern int getNumWriteIO(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return -1; 
    }

    return writeCount;
}