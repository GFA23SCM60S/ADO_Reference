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


/*
	- description : Implement the First In First Out (FIFO) page replacement strategy
	- param : 
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
	- return : void
*/
extern void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
	//printf("FIFO Started");
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	
	int frontIndex = rearIndex % bufferSize;

	// Interating through all the page frames in the buffer pool
	for(int i = 0; i < bufferSize; i++)
	{
		if(pageFrame[frontIndex].fixCount == 0)
		{
			// If page in memory has been modified (dirtyBit = 1), then write page to disk
			if(pageFrame[frontIndex].dirtyBit == 1)
			{
				SM_FileHandle fh;
				openPageFile(bm->pageFile, &fh);
				writeBlock(pageFrame[frontIndex].pageNum, &fh, pageFrame[frontIndex].data);
				
				// Increase the writeCount which records the number of writes done by the buffer manager.
				writeCount++;
			}
			
			// Setting page frame's content to new page's content
			pageFrame[frontIndex].data = page->data;
			pageFrame[frontIndex].pageNum = page->pageNum;
			pageFrame[frontIndex].dirtyBit = page->dirtyBit;
			pageFrame[frontIndex].fixCount = page->fixCount;
			break;
		}
		else
		{
			// If the current page frame is being used by some client, we move on to the next location
			frontIndex++;

			bool frontIndexBool = (frontIndex % bufferSize == 0);

			frontIndex = frontIndexBool ? 0 : frontIndex;
		}
	}
}

/*
	- description : Implement the Least Frequently Used(LFU) page replacement strategy
	- param : 
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
	- return : void
*/
extern void LFU(BM_BufferPool *const bm, PageFrame *page)
{
	//printf("LFU Started");
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	
	int leastFreqIndex, leastFreqRef;
	int i = 0;
	leastFreqIndex = lfuPointer;	
	
	// Interating all the pages with the buffer pool
	while(i < bufferSize){
		if(pageFrame[leastFreqIndex].fixCount == 0)
			{
				leastFreqIndex = (leastFreqIndex + i) % bufferSize;
				leastFreqRef = pageFrame[leastFreqIndex].refNum;
				break;
			}
		i++;
	}

	i = (leastFreqIndex + 1) % bufferSize;

	// Finding the page frame having minimum refNum that is least frequent page frame

	int j = 0;

	while(j < bufferSize){
		if(pageFrame[i].refNum < leastFreqRef)
		{
			leastFreqIndex = i;
			leastFreqRef = pageFrame[i].refNum;
		}
		i = (i + 1) % bufferSize;		
	}

	// If page in memory has been modified so update dirtyBit = 1 and write page to disk	
	if(pageFrame[leastFreqIndex].dirtyBit == 1)
	{
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		writeBlock(pageFrame[leastFreqIndex].pageNum, &fh, pageFrame[leastFreqIndex].data);
		
		// Increament the writeCount which records the number of writes done by the buffer manager
		writeCount++;
	}
	
	// Setting page frame's content to new page's content		
	pageFrame[leastFreqIndex].data = page->data;
	pageFrame[leastFreqIndex].pageNum = page->pageNum;
	pageFrame[leastFreqIndex].dirtyBit = page->dirtyBit;
	pageFrame[leastFreqIndex].fixCount = page->fixCount;
	lfuPointer = leastFreqIndex + 1;
}


/*
	- description : Implement the Least Recently Used(LFU) page replacement strategy
	- param : 
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
	- return : void
*/
extern void LRU(BM_BufferPool *const bm, PageFrame *page) {	
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	int i, leastHitIndex = 0, leastHitNum = 0;

	// Find the next unused page frame with the index
	for(i=0; i<bufferSize; i++) {

		// Search for the next unused page frame if it is not used
		if (pageFrame[i].fixCount == 0) { 
			leastHitIndex  = i;
			leastHitNum = pageFrame[i].hitNum;
			break;
		}
	}	

	// Now find the least recently used page frame, after the unusued page
	for(i=leastHitIndex + 1; i<bufferSize; i++) {
		if (pageFrame[i].hitNum < leastHitNum) {  // if it is the new least 
			leastHitIndex  = i;
			leastHitNum = pageFrame[i].hitNum;
		}
	}

	// Check if it is dirty, and write it to disk if yes
	if (pageFrame[leastHitIndex].dirtyBit == 1) {
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh); // Open the disk file
		writeBlock(pageFrame[leastHitIndex].pageNum, &fh, pageFrame[leastHitIndex].data); // and write the data
		writeCount++;
	}
	
	// Copy new content to the frame
	pageFrame[leastHitIndex].data = page->data;
	pageFrame[leastHitIndex].pageNum = page->pageNum;
	pageFrame[leastHitIndex].dirtyBit = page->dirtyBit;
	pageFrame[leastHitIndex].fixCount = page->fixCount;
	pageFrame[leastHitIndex].hitNum = page->hitNum;
}


/*
	- description : Implement the CLOCK page replacement strategy
	- param : 
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
	- return : void
*/
extern void CLOCK(BM_BufferPool *const bm, PageFrame *page) {	
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;

	while(true) {
		// If it give a full round, restart the index
		if (clockPointer % bufferSize == 0) {
			clockPointer = 0;
		}
		
		// If not used, replace it
		if(pageFrame[clockPointer].hitNum == 0) {
			if(pageFrame[clockPointer].dirtyBit == 1) {  // if dirty, write to disk
				SM_FileHandle fh;
				openPageFile(bm->pageFile, &fh); // Open the disk file
				writeBlock(pageFrame[clockPointer].pageNum, &fh, pageFrame[clockPointer].data); // and write the data
				writeCount++;
			}
			
			// Copy new content to the frame
			pageFrame[clockPointer].data = page->data;
			pageFrame[clockPointer].pageNum = page->pageNum;
			pageFrame[clockPointer].dirtyBit = page->dirtyBit;
			pageFrame[clockPointer].fixCount = page->fixCount;
			pageFrame[clockPointer].hitNum = page->hitNum;
			
			// And moves clock hand
			clockPointer++;
			break;	
		} else {
			// Incrementing clockPointer so that we can check the next page frame location.
			// We set hitNum = 0 so that this loop doesn't go into an infinite loop.
			pageFrame[clockPointer++].hitNum = 0;		
		}
	}
}


/*
	- description : Creates and initializes a buffer pool with page frames (numPages).
	- param :
		1. bm - pointer to the buffer pool
		2. pageFileName -  stores the no of page files which are cached in memory.
		3. numPages - no. of the page frames
		4. strategy -  the page replacement strategy (FIFO, LRU, LFU, CLOCK)
		5. stratData -  parameters to the page replacement strategy
	- return : RC code
*/
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData)
{
	bm->strategy = strategy;
	bm->numPages = numPages;
	bm->pageFile = (char *)pageFileName;

	// Reserver memory space is calculated as follows: pages x page size
	PageFrame *mypage = malloc(sizeof(PageFrame) * numPages);
	
	// The total amount of pages in memory or the buffer pool is known as the buffersize.
	bufferSize = numPages;	
	int k;

	// Initializing every page in the buffer pool. The fields (variables) on the page have values of 0 or NULL.
	for(k=0; k < bufferSize; k++)
	{
		mypage[k].data = NULL;
		mypage[k].pageNum = -1;
		mypage[k].dirtyBit = 0;
		mypage[k].fixCount = 0;
		mypage[k].hitNum = 0;	
		mypage[k].refNum = 0;
	
	}

	bm->mgmtData = mypage;
	lfuPointer = writeCount = clockPointer = 0;
	return RC_OK;
		
}


/*
	- description : destroys the buffer pool by freeing all the pointer
	- param :
		1. bm - pointer to the buffer pool
	- return : void
*/
extern RC shutdownBufferPool(BM_BufferPool *const bm) {
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	// If there are pinnedPages, it fails
	
	// If not, writes the dirty pages to disk
	forceFlushPool(bm);
	for(int i=0; i<bufferSize; i++) {
		if(pageFrame[i].fixCount != 0) {
			return RC_PINNED_PAGES_IN_BUFFER;
		}
	}
	
	// Then erases the properties
	free(pageFrame);
	
	bm->mgmtData = NULL;
	// Then exists. When the bm variable is destroyed, it should be freed
	return RC_OK;
}

/*
	- description : destroys the buffer pool by freeing all the pointer
	- param :
		1. bm - pointer to the buffer pool
	- return : void
*/
extern RC forceFlushPool(BM_BufferPool* const bm) {
	PageFrame *pageFrame  = (PageFrame*)bm->mgmtData;
	
	// Checks if the page is dirty and if no one is touching it, save it to disk	
	for(int i = 0; i < bm->numPages; i++) {
		if (pageFrame[i].fixCount == 0 && pageFrame[i].dirtyBit == 1) {
			SM_FileHandle fh;
			openPageFile(bm->pageFile, &fh); // Open the disk file
			writeBlock(pageFrame [i].pageNum, &fh, pageFrame [i].data); // and write the data
			pageFrame [i].dirtyBit = 0;
			writeCount++;
		}
	}
	return RC_OK;
}

/*
	- description : marks the page as dirty i.e page data is modified
	- param :
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
	- return : RC code
*/
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



/*
	- description : removes the page from memory (unpin the page)
	- param :
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
	- return : RC code
*/
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

/*
	- description : writes the current content of the page back to the page file on disk
	- param :
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
	- return : RC code
*/
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


/*
	- description : checks if the page frame is empty
	- param :
		1. frame - pointer to the PageFrame
	- return : boolean value
*/
bool isPageFrameEmpty(const PageFrame *frame){
		return frame->pageNum == -1;
	}

/*
	- description : Adds the page with page number pageNum to the buffer pool by pinning the 
	page with that number. When the buffer pool is full, a page in memory is replaced with the newly pinned 
	page using the proper page replacement approach. 
	- param :
		1. bm - pointer to the buffer pool
		2. page - pointer to the PageFrame
		3. pageNum - position of the page in the page file
	- return : RC code
*/
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
			realloc(newPage, 0);		
		}		
		return RC_OK;
	}	
}

/*
	- description : returns an array of page number
	- param :
		1. bm - pointer to the buffer pool
	- return : PageNumber
*/
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

/*
	- description : returns an array of booleans each representing the dirtyBit of the respective page
	- param :
		1. bm - pointer to the buffer pool
	- return : boolean
*/
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

/*
	- description : returns an array of integers each representing fix count of the page stored in the respective page frame
	- param :
		1. bm - pointer to the buffer pool
	- return : int
*/
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

/*
	- description : returns the no of pages that is read from disk since the buffer pool initialized
	- param :
		1. bm - pointer to the buffer pool
	- return : int
*/
extern int getNumReadIO (BM_BufferPool *const bm)
{
	// rearIndex starts with 0.
	return (rearIndex + 1);
}

/*
	- description : returns the no of pages that is written to the page file since the buffer pool initialized
	- param :
		1. bm - pointer to the buffer pool
	- return : int
*/
extern int getNumWriteIO (BM_BufferPool *const bm)
{
	return writeCount;
}