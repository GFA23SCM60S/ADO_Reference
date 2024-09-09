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

// "bufferSize" represents the size of the buffer pool i.e. maximum number of page frames that can be kept into the buffer pool
int bufferSize = 0;

// "rearIndex" basically stores the count of number of pages read from the disk.
// "rearIndex" is also used by FIFO function to calculate the frontIndex i.e.
int rearIndex = 0;

// "writeCount" counts the number of I/O write to the disk i.e. number of pages writen to the disk
int writeCount = 0;

// "hit" a general count which is incremented whenever a page frame is added into the buffer pool.
// "hit" is used by LRU to determine least recently added page into the buffer pool.
int hit = 0;

// "clockPointer" is used by CLOCK algorithm to point to the last added page in the buffer pool.
int clockPointer = 0;

// "lfuPointer" is used by LFU algorithm to store the least frequently used page frame's position. It speeds up operation  from 2nd replacement onwards.
int lfuPointer = 0;

// Defining FIFO (First In First Out) function
// extern void FIFO(BM_BufferPool *const bm, PageFrame *page)
// {
// 	printf("FIFO Started");
// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
// 	int frontIndex = rearIndex % bufferSize;

// 	// Interating through all the page frames in the buffer pool
// 	for(int i = 0; i < bufferSize; i++)
// 	{
// 		if(pageFrame[frontIndex].fixCount == 0)
// 		{
// 			// If page in memory has been modified (dirtyBit = 1), then write page to disk
// 			if(pageFrame[frontIndex].dirtyBit == 1)
// 			{
// 				SM_FileHandle fh;
// 				openPageFile(bm->pageFile, &fh);
// 				writeBlock(pageFrame[frontIndex].pageNum, &fh, pageFrame[frontIndex].data);
// 				writeCount++;
// 			}
			
// 			// Setting page frame's content to new page's content
// 			pageFrame[frontIndex].data = page->data;
// 			pageFrame[frontIndex].pageNum = page->pageNum;
// 			pageFrame[frontIndex].dirtyBit = page->dirtyBit;
// 			pageFrame[frontIndex].fixCount = page->fixCount;
// 			break;
// 		}
// 		else
// 		{
// 			frontIndex++;
// 			frontIndex = (frontIndex % bufferSize == 0) ? 0 : frontIndex;
// 		}
// 	}
// }

void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
    printf("FIFO Started");
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int frontIndex = rearIndex % bm->numPages;

    // Iterate through all the page frames in the buffer pool
    while (1) {
        if (pageFrame[frontIndex].fixCount == 0) {
            // If the page in memory has been modified (dirtyBit = 1), then write it to disk
            if (pageFrame[frontIndex].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[frontIndex].pageNum, &fh, pageFrame[frontIndex].data);
                writeCount++;
            }

            // Set the page frame's content to the new page's content
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


// Defining LFU (Least Frequently Used) function
// extern void LFU(BM_BufferPool *const bm, PageFrame *page)
// {
// 	printf("LFU Started");
// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	
// 	int i, j, leastFreqIndex, leastFreqRef;
// 	leastFreqIndex = lfuPointer;	
	
// 	// Interating through all the page frames in the buffer pool
// 	for(i = 0; i < bufferSize; i++)
// 	{
// 		if(pageFrame[leastFreqIndex].fixCount == 0)
// 		{
// 			leastFreqIndex = (leastFreqIndex + i) % bufferSize;
// 			leastFreqRef = pageFrame[leastFreqIndex].refNum;
// 			break;
// 		}
// 	}

// 	i = (leastFreqIndex + 1) % bufferSize;

// 	// Finding the page frame having minimum refNum (i.e. it is used the least frequent) page frame
// 	for(j = 0; j < bufferSize; j++)
// 	{
// 		if(pageFrame[i].refNum < leastFreqRef)
// 		{
// 			leastFreqIndex = i;
// 			leastFreqRef = pageFrame[i].refNum;
// 		}
// 		i = (i + 1) % bufferSize;
// 	}
		
// 	// If page in memory has been modified (dirtyBit = 1), then write page to disk	
// 	if(pageFrame[leastFreqIndex].dirtyBit == 1)
// 	{
// 		SM_FileHandle fh;
// 		openPageFile(bm->pageFile, &fh);
// 		writeBlock(pageFrame[leastFreqIndex].pageNum, &fh, pageFrame[leastFreqIndex].data);
		
// 		// Increase the writeCount which records the number of writes done by the buffer manager.
// 		writeCount++;
// 	}
	
// 	// Setting page frame's content to new page's content		
// 	pageFrame[leastFreqIndex].data = page->data;
// 	pageFrame[leastFreqIndex].pageNum = page->pageNum;
// 	pageFrame[leastFreqIndex].dirtyBit = page->dirtyBit;
// 	pageFrame[leastFreqIndex].fixCount = page->fixCount;
// 	lfuPointer = leastFreqIndex + 1;
// }

void LFU(BM_BufferPool *const bm, PageFrame *page) {
    // Initialization
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    int bufferSize = bm->numPages;
    int leastFreqIndex = -1;
    int leastFreqCount = -1;

    // Find the page frame with the least reference count (LFU)
    for (int i = 0; i < bufferSize; i++) {
        if (pageFrames[i].fixCount == 0) {
            if (leastFreqIndex == -1 || pageFrames[i].refNum < leastFreqCount) {
                leastFreqIndex = i;
                leastFreqCount = pageFrames[i].refNum;
            }
        }
    }

    // If all frames are currently in use (fixCount > 0), return
    if (leastFreqIndex == -1) {
        return;
    }

    // If the selected page frame is dirty, write it back to disk
    if (pageFrames[leastFreqIndex].dirtyBit == 1) {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrames[leastFreqIndex].pageNum, &fh, pageFrames[leastFreqIndex].data);

        // Increase the writeCount to record the write operation
        writeCount++;
    }

    // Replace the content of the LFU page frame with the new page
    pageFrames[leastFreqIndex].data = page->data;
    pageFrames[leastFreqIndex].pageNum = page->pageNum;
    pageFrames[leastFreqIndex].dirtyBit = page->dirtyBit;
    pageFrames[leastFreqIndex].fixCount = page->fixCount;
    pageFrames[leastFreqIndex].refNum = 1;  // Reset the reference count

    // Update LFU statistics for other pages
    for (int i = 0; i < bufferSize; i++) {
        if (i != leastFreqIndex && pageFrames[i].fixCount == 0) {
            pageFrames[i].refNum++;
        }
    }
}


// Defining LRU (Least Recently Used) function
extern void LRU(BM_BufferPool *const bm, PageFrame *page)
{	
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	int i, leastHitIndex, leastHitNum;

	// Interating through all the page frames in the buffer pool.
	for(i = 0; i < bufferSize; i++)
	{
		// Finding page frame whose fixCount = 0 i.e. no client is using that page frame.
		if(pageFrame[i].fixCount == 0)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitNum;
			break;
		}
	}	

	// Finding the page frame having minimum hitNum (i.e. it is the least recently used) page frame
	for(i = leastHitIndex + 1; i < bufferSize; i++)
	{
		if(pageFrame[i].hitNum < leastHitNum)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitNum;
		}
	}

	// If page in memory has been modified (dirtyBit = 1), then write page to disk
	if(pageFrame[leastHitIndex].dirtyBit == 1)
	{
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		writeBlock(pageFrame[leastHitIndex].pageNum, &fh, pageFrame[leastHitIndex].data);
		
		// Increase the writeCount which records the number of writes done by the buffer manager.
		writeCount++;
	}
	
	// Setting page frame's content to new page's content
	pageFrame[leastHitIndex].data = page->data;
	pageFrame[leastHitIndex].pageNum = page->pageNum;
	pageFrame[leastHitIndex].dirtyBit = page->dirtyBit;
	pageFrame[leastHitIndex].fixCount = page->fixCount;
	pageFrame[leastHitIndex].hitNum = page->hitNum;
}

// Defining CLOCK function
void CLOCK(BM_BufferPool *const bm, PageFrame *page) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    int bufferSize = bm->numPages;

    // Find the page to replace using the CLOCK algorithm
    while (1) {
        clockPointer = (clockPointer % bufferSize == 0) ? 0 : clockPointer;

        if (pageFrames[clockPointer].hitNum == 0) {
            // If page in memory has been modified (dirtyBit = 1), then write page to disk
            if (pageFrames[clockPointer].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrames[clockPointer].pageNum, &fh, pageFrames[clockPointer].data);

                // Increase the writeCount to record the write operation
                writeCount++;
            }

            // Replace the content of the CLOCK page frame with the new page
            pageFrames[clockPointer].data = page->data;
            pageFrames[clockPointer].pageNum = page->pageNum;
            pageFrames[clockPointer].dirtyBit = page->dirtyBit;
            pageFrames[clockPointer].fixCount = page->fixCount;
            pageFrames[clockPointer].hitNum = page->hitNum;
            clockPointer++;
            break;
        } else {
            // Reset hitNum to 0 and move to the next page frame
            pageFrames[clockPointer].hitNum = 0;
            clockPointer++;
        }
    }
}

// Buffer Pool Function
//Initialize buffer pool(initBufferPool)
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData)
{
	bm->pageFile = (char *)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;

	// Allocate memory for page frames
	PageFrame *page = malloc(sizeof(PageFrame) * numPages);
	
	// Buffersize is the total number of pages in memory or the buffer pool.
	   bufferSize = numPages;	
	// Initialize buffer pool pages
	for(int i = 0; i < bufferSize; i++)
	{
		page[i].data = NULL;
		page[i].pageNum = -1;
		page[i].dirtyBit = 0;
		page[i].fixCount = 0;
		page[i].hitNum = 0;	
		page[i].refNum = 0;
	}
	//set buffer pool managment data
	bm->mgmtData = page;
	// Initialize writeCount 
	writeCount = clockPointer = lfuPointer = 0;
	return RC_OK;
		
}

// Shutdown function(shutdownBufferPool)
RC shutdownBufferPool(BM_BufferPool *const bm) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    
    // Write all dirty pages (modified pages) back to disk
    RC rc = forceFlushPool(bm);

    if (rc != RC_OK) {
        return rc; // Propagate the forceFlushPool error
    }

    // Check if there are any pinned pages in the buffer
    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrames[i].fixCount != 0) {
            return RC_PINNED_PAGES_IN_BUFFER;
        }
    }

    // Free the memory occupied by the page frames
    free(pageFrames);
    bm->mgmtData = NULL;

    return RC_OK;
}

//ForceFlush Pool (forceFulshPool)
// extern RC forceFlushPool(BM_BufferPool *const bm)
// {
// 	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
		
// 	for(int i = 0; i < bufferSize; i++)
// 	{
// 		if(pageFrame[i].fixCount == 0 && pageFrame[i].dirtyBit == 1)
// 		{
// 			SM_FileHandle fh; 
// 			openPageFile(bm->pageFile, &fh); // Opening page file
// 			writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);// Writing block
// 			pageFrame[i].dirtyBit = 0;// Mark the page not dirty. 
// 			writeCount++;// Increase the writeCount
// 		}
// 	}	
// 	return RC_OK;
// }

RC forceFlushPool(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT; // Handle uninitialized buffer pool appropriately
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].fixCount == 0 && pageFrame[i].dirtyBit == 1) {
            SM_FileHandle fh;
            openPageFile(bm->pageFile, &fh); // Opening page file
            writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data); // Writing block
            pageFrame[i].dirtyBit = 0; // Mark the page as not dirty.
            writeCount++; // Increase the writeCount
        }
    }

    return RC_OK;
}



// Page Management Function

// Make Dirty: makes a page dirty
// extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
// {
// 	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
// 	int i;
// 	// Iterating through all the pages in the buffer pool
// 	for(i = 0; i < bufferSize; i++)
// 	{
// 		// If the current page is the page to be marked dirty, then set dirtyBit = 1 (page has been modified) for that page
// 		if(pageFrame[i].pageNum == page->pageNum)
// 		{
// 			pageFrame[i].dirtyBit = 1;
// 			return RC_OK;		
// 		}			
// 	}		
// 	return RC_ERROR;
// }

extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // Check if the buffer pool is initialized
    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT;
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Search for the page in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].pageNum == page->pageNum) {
            pageFrame[i].dirtyBit = 1; // Mark the page as dirty
            return RC_OK;
        }
    }

    // Page not found in the buffer pool
    return RC_PAGE_NOT_FOUND;
}

// unpins a page 
// extern RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
// {	
// 	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
// 	int i;
// 	// Iterating through all the pages in the buffer pool
// 	for(i = 0; i < bufferSize; i++)
// 	{
// 		// If the current page is the page to be unpinned, then decrease fixCount (which means client has completed work on that page) and exit loop
// 		if(pageFrame[i].pageNum == page->pageNum)
// 		{
// 			pageFrame[i].fixCount--;
// 			break;		
// 		}		
// 	}
// 	return RC_OK;
// }

extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // Check if the buffer pool is initialized
    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT; // Define this error code appropriately
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Search for the page in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].pageNum == page->pageNum) {
            if (pageFrame[i].fixCount > 0) {
                pageFrame[i].fixCount--; // Decrease the fixCount if it's greater than 0
            } else {
                return RC_PAGE_NOT_PINNED; // Define this error code appropriately
            }
            return RC_OK;
        }
    }

    // Page not found in the buffer pool
    return RC_PAGE_NOT_FOUND; // Define this error code appropriately
}

// Force Page 
// extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
// {
// 	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
// 	int i;
// 	// Iterating through all the pages in the buffer pool
// 	for(i = 0; i < bufferSize; i++)
// 	{
// 		// If the current page = page to be written to disk, then right the page to the disk using the storage manager functions
// 		if(pageFrame[i].pageNum == page->pageNum)
// 		{		
// 			SM_FileHandle fh;
// 			openPageFile(bm->pageFile, &fh);
// 			writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);
		
// 			// Mark page as undirty because the modified page has been written to disk
// 			pageFrame[i].dirtyBit = 0;
			
// 			// Increase the writeCount which records the number of writes done by the buffer manager.
// 			writeCount++;
// 		}
// 	}	
// 	return RC_OK;
// }

extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // Check if the buffer pool is initialized
    if (bm == NULL || bm->mgmtData == NULL) {
        return RC_BUFFER_POOL_NOT_INIT; //define erroe code
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Search for the page in the buffer pool
    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrame[i].pageNum == page->pageNum) {
            if (pageFrame[i].dirtyBit == 1) {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[i].pageNum, &fh, pageFrame[i].data);

                // Mark page as clean because the modified page has been written to disk
                pageFrame[i].dirtyBit = 0;
                return RC_OK;
            } else {
                return RC_PAGE_NOT_DIRTY; // Define this error code appropriately
            }
        }
    }

    // Page not found in the buffer pool
    return RC_PAGE_NOT_FOUND; // Define this error code appropriately
}


// This function pins a page with page number pageNum i.e. adds the page with page number pageNum to the buffer pool.
// If the buffer pool is full, then it uses appropriate page replacement strategy to replace a page in memory with the new page being pinned. 
extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
	// Checking if buffer pool is empty and this is the first page to be pinned
	if(pageFrame[0].pageNum == -1)
	{
		// Reading page from disk and initializing page frame's content in the buffer pool
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
				// Checking if page is in memory
				if(pageFrame[i].pageNum == pageNum)
				{
					// Increasing fixCount i.e. now there is one more client accessing this page
					pageFrame[i].fixCount++;
					isBufferFull = false;
					hit++; // Incrementing hit (hit is used by LRU algorithm to determine the least recently used page)

					if(bm->strategy == RS_LRU)
						// LRU algorithm uses the value of hit to determine the least recently used page	
						pageFrame[i].hitNum = hit;
					else if(bm->strategy == RS_CLOCK)
						// hitNum = 1 to indicate that this was the last page frame examined (added to the buffer pool)
						pageFrame[i].hitNum = 1;
					else if(bm->strategy == RS_LFU)
						// Incrementing refNum to add one more to the count of number of times the page is used (referenced)
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
				hit++; // Incrementing hit (hit is used by LRU algorithm to determine the least recently used page)

				if(bm->strategy == RS_LRU)
					// LRU algorithm uses the value of hit to determine the least recently used page
					pageFrame[i].hitNum = hit;				
				else if(bm->strategy == RS_CLOCK)
					// hitNum = 1 to indicate that this was the last page frame examined (added to the buffer pool)
					pageFrame[i].hitNum = 1;
						
				page->pageNum = pageNum;
				page->data = pageFrame[i].data;
				
				isBufferFull = false;
				break;
			}
		}
		
		// If isBufferFull = true, then it means that the buffer is full and we must replace an existing page using page replacement strategy
		if(isBufferFull == true)
		{
			// Create a new page to store data read from the file.
			PageFrame *newPage = (PageFrame *) malloc(sizeof(PageFrame));		
			
			// Reading page from disk and initializing page frame's content in the buffer pool
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
				// LRU algorithm uses the value of hit to determine the least recently used page
				newPage->hitNum = hit;				
			else if(bm->strategy == RS_CLOCK)
				// hitNum = 1 to indicate that this was the last page frame examined (added to the buffer pool)
				newPage->hitNum = 1;

			page->pageNum = pageNum;
			page->data = newPage->data;			

			// Call appropriate algorithm's function depending on the page replacement strategy selected (passed through parameters)
			switch(bm->strategy)
			{			
				case RS_FIFO: // Using FIFO algorithm
					FIFO(bm, newPage);
					break;
				
				case RS_LRU: // Using LRU algorithm
					LRU(bm, newPage);
					break;
				
				case RS_CLOCK: // Using CLOCK algorithm
					CLOCK(bm, newPage);
					break;
  				
				case RS_LFU: // Using LFU algorithm
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


// ***** STATISTICS FUNCTIONS ***** //

// This function returns an array of page numbers.
// extern PageNumber *getFrameContents (BM_BufferPool *const bm)
// {
// 	PageNumber *frameContents = malloc(sizeof(PageNumber) * bufferSize);
// 	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	
// 	int i = 0;
// 	// Iterating through all the pages in the buffer pool and setting frameContents' value to pageNum of the page
// 	while(i < bufferSize) {
// 		frameContents[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
// 		i++;
// 	}
// 	return frameContents;
// }

extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL; // Handle uninitialized buffer pool appropriately
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    PageNumber *frameContents = (PageNumber *)malloc(sizeof(PageNumber) * bm->numPages);

    if (frameContents == NULL) {
        return NULL; // Handle memory allocation failure
    }

    for (int i = 0; i < bm->numPages; i++) {
        frameContents[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
    }

    return frameContents;
}


// This function returns an array of bools, each element represents the dirtyBit of the respective page.
// extern bool *getDirtyFlags (BM_BufferPool *const bm)
// {
// 	bool *dirtyFlags = malloc(sizeof(bool) * bufferSize);
// 	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	
// 	int i;
// 	// Iterating through all the pages in the buffer pool and setting dirtyFlags' value to TRUE if page is dirty else FALSE
// 	for(i = 0; i < bufferSize; i++)
// 	{
// 		dirtyFlags[i] = (pageFrame[i].dirtyBit == 1) ? true : false ;
// 	}	
// 	return dirtyFlags;
// }

extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL; // Handle uninitialized buffer pool appropriately
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    bool *dirtyFlags = (bool *)malloc(sizeof(bool) * bm->numPages);

    if (dirtyFlags == NULL) {
        return NULL; // Handle memory allocation failure
    }

    for (int i = 0; i < bm->numPages; i++) {
        dirtyFlags[i] = (pageFrame[i].dirtyBit == 1) ? true : false;
    }

    return dirtyFlags;
}

// This function returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame.
// extern int *getFixCounts (BM_BufferPool *const bm)
// {
// 	int *fixCounts = malloc(sizeof(int) * bufferSize);
// 	PageFrame *pageFrame= (PageFrame *)bm->mgmtData;
	
// 	int i = 0;
// 	// Iterating through all the pages in the buffer pool and setting fixCounts' value to page's fixCount
// 	while(i < bufferSize)
// 	{
// 		fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0;
// 		i++;
// 	}	
// 	return fixCounts;
// }

extern int *getFixCounts(BM_BufferPool *const bm)
{
    if (bm == NULL || bm->mgmtData == NULL) {
        return NULL; // Handle uninitialized buffer pool appropriately
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int *fixCounts = (int *)malloc(sizeof(int) * bm->numPages);

    if (fixCounts == NULL) {
        return NULL; // Handle memory allocation failure
    }

    for (int i = 0; i < bm->numPages; i++) {
        fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0;
    }

    return fixCounts;
}


// This function returns the number of pages that have been read from disk since a buffer pool has been initialized.
// extern int getNumReadIO (BM_BufferPool *const bm)
// {
// 	// Adding one because with start rearIndex with 0.
// 	return (rearIndex + 1);
// }

extern int getNumReadIO(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return -1; // Handle invalid buffer pool appropriately
    }

    // Calculate the number of pages read based on rearIndex
    return (rearIndex + 1);
}
// This function returns the number of pages written to the page file since the buffer pool has been initialized.
// extern int getNumWriteIO (BM_BufferPool *const bm)
// {
// 	return writeCount;
// }

extern int getNumWriteIO(BM_BufferPool *const bm)
{
    if (bm == NULL) {
        return -1; // Handle invalid buffer pool appropriately
    }

    // Return the recorded write count from the buffer pool
    return writeCount;
}