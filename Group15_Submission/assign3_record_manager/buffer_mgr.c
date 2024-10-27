#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

// This structure represents one page frame in buffer pool (memory).
typedef struct Page
{
	SM_PageHandle data; // Actual data of the page
	PageNumber pageNum; // An identification integer given to each page
	int dirtyBit; // Used to indicate whether the contents of the page has been modified by the client
	int fixCount; // Used to indicate the number of clients using that page at a given instance
	int hitNum;   // Used by LRU algorithm to get the least recently used page
	int refNum;   // Used by LFU algorithm to get the least frequently used page
} PageFrame;

int bufferSize = 0;
int rearIndex = 0;
int writeCount = 0;
int hit = 0;
int clockPointer = 0;
int lfuPointer = 0;


extern void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int frontIndex = rearIndex % bufferSize;

    // Iterate through all page frames in the buffer pool
    for (int i = 0; i < bufferSize; i++)
    {
        if (pageFrame[frontIndex].fixCount == 0)
        {
            // If the page in memory has been modified, write it to disk
            if (pageFrame[frontIndex].dirtyBit == 1)
            {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[frontIndex].pageNum, &fh, pageFrame[frontIndex].data);
                writeCount++; // Increment write count
            }

            // Replace the page frame's content with the new page's content
            pageFrame[frontIndex].data = page->data;
            pageFrame[frontIndex].pageNum = page->pageNum;
            pageFrame[frontIndex].dirtyBit = page->dirtyBit;
            pageFrame[frontIndex].fixCount = page->fixCount;
            break;
        }
        else
        {
            // Move to the next location if the current page frame is in use
            frontIndex = (frontIndex + 1) % bufferSize;
        }
    }
}

extern void LFU(BM_BufferPool *const bm, PageFrame *page)
{
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int leastFreqIndex = lfuPointer;
    int leastFreqRef;

    // Find the first unpinned page
    for (int i = 0; i < bufferSize; i++)
    {
        int index = (leastFreqIndex + i) % bufferSize;
        if (pageFrame[index].fixCount == 0)
        {
            leastFreqIndex = index;
            leastFreqRef = pageFrame[index].refNum;
            break;
        }
    }

    // Find the least frequently used unpinned page
    for (int i = 0; i < bufferSize; i++)
    {
        int index = (leastFreqIndex + i + 1) % bufferSize;
        if (pageFrame[index].fixCount == 0 && pageFrame[index].refNum < leastFreqRef)
        {
            leastFreqIndex = index;
            leastFreqRef = pageFrame[index].refNum;
        }
    }

    // If the page in memory has been modified, write it to disk
    if (pageFrame[leastFreqIndex].dirtyBit == 1)
    {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[leastFreqIndex].pageNum, &fh, pageFrame[leastFreqIndex].data);
        writeCount++; // Increment write count
    }

    // Replace the page frame's content with the new page's content
    pageFrame[leastFreqIndex].data = page->data;
    pageFrame[leastFreqIndex].pageNum = page->pageNum;
    pageFrame[leastFreqIndex].dirtyBit = page->dirtyBit;
    pageFrame[leastFreqIndex].fixCount = page->fixCount;
    lfuPointer = (leastFreqIndex + 1) % bufferSize;
}

extern void LRU(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int leastHitIndex = 0, leastHitNum = 0;

    // Find the first unused page frame
    for (int i = 0; i < bufferSize; i++) {
        if (pageFrame[i].fixCount == 0) {
            leastHitIndex = i;
            leastHitNum = pageFrame[i].hitNum;
            break;
        }
    }

    // Find the least recently used page frame among unused frames
    for (int i = leastHitIndex + 1; i < bufferSize; i++) {
        if (pageFrame[i].fixCount == 0 && pageFrame[i].hitNum < leastHitNum) {
            leastHitIndex = i;
            leastHitNum = pageFrame[i].hitNum;
        }
    }

    // Write dirty page to disk if necessary
    if (pageFrame[leastHitIndex].dirtyBit) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[leastHitIndex].pageNum, &fh, pageFrame[leastHitIndex].data);
        writeCount++;
    }

    // Replace the page frame with new content
    pageFrame[leastHitIndex] = *page;
}

extern void CLOCK(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    while (true) {
        // Reset clock pointer if it completes a full round
        if (clockPointer % bufferSize == 0) {
            clockPointer = 0;
        }

        // If the current frame is not recently used, replace it
        if (pageFrame[clockPointer].hitNum == 0) {
            // Write to disk if the page is dirty
            if (pageFrame[clockPointer].dirtyBit) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[clockPointer].pageNum, &fh, pageFrame[clockPointer].data);
                writeCount++;
            }

            // Replace the page frame with new content
            pageFrame[clockPointer] = *page;

            // Move the clock hand
            clockPointer++;
            break;
        } else {
            // Give the page a second chance and move to the next frame
            pageFrame[clockPointer].hitNum = 0;
            clockPointer++;
        }
    }
}

extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                         const int numPages, ReplacementStrategy strategy,
                         void *stratData)
{
    // Initialize buffer pool properties
    bm->strategy = strategy;
    bm->numPages = numPages;
    bm->pageFile = (char *)pageFileName;

    // Allocate memory for page frames
    PageFrame *pageFrames = malloc(sizeof(PageFrame) * numPages);
    if (pageFrames == NULL) {
        return RC_ERROR;
    }

    // Set global buffer size
    bufferSize = numPages;

    // Initialize each page frame
    for (int i = 0; i < bufferSize; i++) {
        pageFrames[i].data = NULL;
        pageFrames[i].pageNum = -1;
        pageFrames[i].dirtyBit = 0;
        pageFrames[i].fixCount = 0;
        pageFrames[i].hitNum = 0;
        pageFrames[i].refNum = 0;
    }

    // Set management data and initialize global counters
    bm->mgmtData = pageFrames;
    lfuPointer = writeCount = clockPointer = 0;

    return RC_OK;
}

extern RC shutdownBufferPool(BM_BufferPool *const bm) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

    // Flush dirty pages to disk
    RC flushStatus = forceFlushPool(bm);
    if (flushStatus != RC_OK) {
        return flushStatus;
    }

    // Check for pinned pages
    for (int i = 0; i < bufferSize; i++) {
        if (pageFrames[i].fixCount != 0) {
            return RC_PINNED_PAGES_IN_BUFFER;
        }
    }

    // Free allocated memory and reset management data
    free(pageFrames);
    bm->mgmtData = NULL;

    return RC_OK;
}

extern RC forceFlushPool(BM_BufferPool *const bm) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    SM_FileHandle fileHandle;

    // Open the page file
    RC openStatus = openPageFile(bm->pageFile, &fileHandle);
    if (openStatus != RC_OK) {
        return openStatus;
    }

    // Flush dirty pages to disk
    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrames[i].fixCount == 0 && pageFrames[i].dirtyBit == 1) {
            RC writeStatus = writeBlock(pageFrames[i].pageNum, &fileHandle, pageFrames[i].data);
            if (writeStatus != RC_OK) {
                closePageFile(&fileHandle);
                return writeStatus;
            }
            pageFrames[i].dirtyBit = 0;
            writeCount++;
        }
    }

    // Close the page file
    return closePageFile(&fileHandle);
}


extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int i = 0;
	// Iterating through all the pages in the buffer pool
	while (i < bufferSize)
	{
		// If the current page is the page to be marked dirty, then set dirtyBit = 1 (page has been modified) for that page
		if (pageFrame[i].pageNum == page->pageNum){
			pageFrame[i].dirtyBit = 1;
			return RC_OK;
		}

		i++;
	}

	return RC_ERROR;
}


extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrame *frameOfPage = (PageFrame *)bm->mgmtData;

    int j = 0;
    // Repeat this process for each page in the buffer pool.
    while (j < bufferSize)
    {
        // Check if the current page is the page to be unpinned
        if (frameOfPage[j].pageNum == page->pageNum)
        {
            frameOfPage[j].fixCount--;
            break; // Verify that the page being unpinned is the current page.
        }
        j++;
    }

    return RC_OK;
}


extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int i = 0;
	// Iterating through all the pages in the buffer pool
	while (i < bufferSize)
	{
		// If the current page = page to be written to disk, then right the page to the disk using the storage manager functions
		if (pageFrame[i].pageNum == page->pageNum)
		{
			SM_FileHandle fh;
			openPageFile(bm->pageFile, &fh);
			writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);

			// Mark page as undirty because the modified page has been written to disk
			pageFrame[i].dirtyBit = 0;

			// Increase the writeCount which records the number of writes done by the buffer manager.
			writeCount++;

			break;
		}
		i++;
	}

	return RC_OK;
}


bool isPageFrameEmpty(const PageFrame *frame){
		return frame->pageNum == -1;
	}


extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
	    const PageNumber pageNum)
{
	PageFrame *frameOfPage = (PageFrame *)bm->mgmtData;


	// Ascertaining that this is the first page to be pinned and that the buffer pool is empty
	if(isPageFrameEmpty(&frameOfPage[0]))
	{
		// Reading a page from the disk and starting the buffer pool with the contents of the page frame
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);

		SM_PageHandle pageData = (SM_PageHandle) malloc(PAGE_SIZE);
		frameOfPage[0].data = pageData;

		ensureCapacity(pageNum,&fh);
		readBlock(pageNum, &fh, frameOfPage[0].data);

		frameOfPage[0].pageNum = pageNum;
		rearIndex = hit = 0;
		frameOfPage[0].hitNum = hit;
		frameOfPage[0].fixCount++;
		frameOfPage[0].refNum = 0;

		page->pageNum = pageNum;
		page->data = frameOfPage[0].data;

		return RC_OK;
	}
	else
	{
		int j;
		bool bufferFull = true;

		for(j = 0; j < bufferSize; j++)
		{
			if(frameOfPage[j].pageNum != -1)
			{
				// Verifying that the page is in memory
				if(frameOfPage[j].pageNum == pageNum)
				{
					// Updating fixCount i.e., a new client has just accessed this page.
					bufferFull = false;
					frameOfPage[j].fixCount++;
					hit++; // Increasing the hit (the LRU method uses the hit to find the least recently used page).


					if(bm->strategy == RS_CLOCK)
						// hitNum is set to 1 to signify that this was the final page frame checked before adding it to the buffer pool.
						frameOfPage[j].hitNum = 1;
					else if(bm->strategy == RS_LFU)
						// Increasing referenceThe number represents the addition of one more usages to the page usage count (referenced).
						frameOfPage[j].refNum++;
					else if(bm->strategy == RS_LRU)
						// The least recently used page is determined by the LRU algorithm using the hit value.
						frameOfPage[j].hitNum = hit;

					clockPointer++;
					page->data = frameOfPage[j].data;
					page->pageNum = pageNum;

					break;
				}
			}
			else {
				SM_FileHandle fileh;
				openPageFile(bm->pageFile, &fileh);
				frameOfPage[j].data = (SM_PageHandle) malloc(PAGE_SIZE);
				readBlock(pageNum, &fileh, frameOfPage[j].data);
				frameOfPage[j].refNum = 0;
				frameOfPage[j].pageNum = pageNum;
				frameOfPage[j].fixCount = 1;

				rearIndex++;
				hit++; // Increasing the hit (the LRU algorithm uses the hit to find the least recently used page)

				if(bm->strategy == RS_LRU)
					// The least recently used page is determined by the LRU algorithm using the hit value.
					frameOfPage[j].hitNum = hit;
				else if(bm->strategy == RS_CLOCK)
					// hitNum is set to 1 to signify that this was the final page frame checked before adding it to the buffer.
					frameOfPage[j].hitNum = 1;

				page->pageNum = pageNum;
				page->data = frameOfPage[j].data;

				bufferFull = false;
				break;
			}
		}

		//If bufferFull = true, then the buffer is full and we must use the page replacement approach to replace an existing page.
		if(bufferFull == true)
		{
			// Create a new page to store data read from the file.
			PageFrame *newPage;
			newPage = (PageFrame *)malloc(sizeof(PageFrame));

			// Reading a page from disk and starting the buffer pool with the contents of the page frame
			SM_FileHandle fileH;
			openPageFile(bm->pageFile, &fileH);
			newPage->data = (SM_PageHandle) malloc(PAGE_SIZE);
			readBlock(pageNum, &fileH, newPage->data);
			newPage->pageNum = pageNum;
			newPage->dirtyBit = 0;
			newPage->refNum = 0;
			newPage->fixCount = 1;

			hit++;
			rearIndex++;



			if(bm->strategy == RS_CLOCK)
				// hitNum is set to 1 to signify that this was the final page frame checked before adding it to the buffer.
				newPage->hitNum = 1;

			else if(bm->strategy == RS_LRU)
				// The least recently used page is determined by the LRU algorithm using the hit value.
				newPage->hitNum = hit;

			page->pageNum = pageNum;
			page->data = newPage->data;

			//depending on the chosen page replacement technique, call the relevant algorithm's function (provided through arguments).
if (bm->strategy == RS_FIFO) {
    FIFO(bm, newPage);
} else if (bm->strategy == RS_LRU) {
    LRU(bm, newPage);
} else if (bm->strategy == RS_CLOCK) {
    CLOCK(bm, newPage);
} else if (bm->strategy == RS_LFU) {
    LFU(bm, newPage);
} else if (bm->strategy == RS_LRU_K) {
    printf("\nLRU-k algorithm is not used.\n");
} else {
    printf("\nNo algorithm has been used.\n");
}
			free(newPage);
		}
		return RC_OK;
	}
}


extern PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	PageNumber *frameContents = malloc(sizeof(PageNumber) * bufferSize);
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;

	// Iterating through all the pages in the buffer pool and setting frameContents' value to pageNum of the page
	for(int i = 0; i < bufferSize; i++){
		if(pageFrame[i].pageNum != -1){
			frameContents[i] = pageFrame[i].pageNum;
		}else{
			frameContents[i] = NO_PAGE;
		}
	}

	return frameContents;
}

extern bool *getDirtyFlags (BM_BufferPool *const bm)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	// Allocate memory of bool type and bufferSize
	bool *dirtyFlags = malloc(sizeof(bool) * bufferSize);

	int i = 0;

	// Iterating through all the pages in the buffer pool and setting dirtyFlags' value to TRUE if page is dirty else FALSE
	while(i < bufferSize)
	{
		if(pageFrame[i].dirtyBit == 1){
			dirtyFlags[i] = true;
		}else{
			dirtyFlags[i] = false;
		}

		i++;
	}

	return dirtyFlags;
}

extern int *getFixCounts (BM_BufferPool *const bm)
{

	PageFrame *pageFrame= (PageFrame *)bm->mgmtData;

	// Allocate memory of int type and bufferSize
	int *fixCounts = malloc(sizeof(int) * bufferSize);


	// Iterating through all the pages in the buffer pool and setting fixCounts' value to page's fixCount
	for(int i = 0; i < bufferSize; i++){
		if(pageFrame[i].fixCount != -1){
			fixCounts[i] = pageFrame[i].fixCount;
		}else{
			fixCounts[i] = 0;
		}
	}

	return fixCounts;
}

extern int getNumReadIO (BM_BufferPool *const bm)
{
	// rearIndex starts with 0.
	return (rearIndex + 1);
}

extern int getNumWriteIO (BM_BufferPool *const bm)
{
	return writeCount;
}