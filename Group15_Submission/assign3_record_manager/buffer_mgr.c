#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

// Represents a single page frame in the buffer pool (memory)
typedef struct PageFrame
{
    SM_PageHandle data;    
    PageNumber pageNum;   
    int dirtyBit;        
    int fixCount;          
    int lastAccessedTime;  
    int accessFrequency;  
	int hitNum;  
    int refNum;  
} PageFrame;

#define RC_MEM_ALLOC_FAIL 1
#define RC_READ_FAILED 2

int pagesReadCount = 0;
int hit = 0;
int lfuPosition = 0;

int writeCount = 0;
int clockPointer = 0;
int bufferSize = 0;

// Represents the size of the buffer pool, i.e., the maximum number of page frames it can hold
int bufferPoolSize = 0;


// Counts the total number of I/O writes to the disk, indicating how many pages have been written back to disk
int pageWriteCount = 0;

// Tracks the number of page hits, i.e., the number of times a page frame is added to the buffer pool
int pageHitCount = 0;

// Pointer used by the CLOCK algorithm to indicate the position of the last added page in the buffer pool
int clockHand = 0;




/*
	- Description: Implements the First In First Out (FIFO) page replacement strategy
	- Parameters: 
		1. bm - pointer to the buffer pool
		2. page - pointer to the new PageFrame to be added
	- Return: void
*/
extern void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
	// Get the array of page frames from the buffer pool's management data
	PageFrame *pageFrames = (PageFrame *) bm->mgmtData;

	// Calculate the index of the front page frame using FIFO strategy
	int frontIndex = pagesReadCount % bufferPoolSize;

	// Iterate through the buffer pool to find an unpinned (unused) page frame
	for (int i = 0; i < bufferPoolSize; i++)
	{
		// Check if the page frame is not being used (fixCount == 0)
		if (pageFrames[frontIndex].fixCount == 0)
		{
			// If the page frame has been modified (dirtyBit == 1), write it to disk
			if (pageFrames[frontIndex].dirtyBit == 1)
			{
				SM_FileHandle fileHandle;
				openPageFile(bm->pageFile, &fileHandle);
				writeBlock(pageFrames[frontIndex].pageNum, &fileHandle, pageFrames[frontIndex].data);
				
				// Increment the write counter to track disk writes
				pageWriteCount++;
			}

			// Replace the contents of the page frame with the new page's data
			pageFrames[frontIndex].data = page->data;
			pageFrames[frontIndex].pageNum = page->pageNum;
			pageFrames[frontIndex].dirtyBit = page->dirtyBit;
			pageFrames[frontIndex].fixCount = page->fixCount;

			// Exit after replacing the page
			break;
		}

		// Move to the next page frame (FIFO circular queue logic)
		frontIndex = (frontIndex + 1) % bufferPoolSize;
	}
}



/*
	- Description: Implements the Least Frequently Used (LFU) page replacement strategy
	- Parameters: 
		1. bm - pointer to the buffer pool
		2. page - pointer to the new PageFrame to be added
	- Return: void
*/
extern void LFU(BM_BufferPool *const bm, PageFrame *page)
{
	// Get the array of page frames from the buffer pool's management data
	PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
	
	int leastFreqIndex = lfuPosition; // Start from the last known LFU position
	int leastFreqRef = pageFrames[leastFreqIndex].refNum; // Reference count for LFU
	
	// Iterate through all pages in the buffer pool to find the least frequently used page
	for (int i = 0; i < bufferPoolSize; i++)
	{
		int currentIndex = (leastFreqIndex + i) % bufferPoolSize; // Circularly traverse the buffer pool

		// Check if the page is not in use (fixCount == 0) and has a lower reference count
		if (pageFrames[currentIndex].fixCount == 0 && pageFrames[currentIndex].refNum < leastFreqRef)
		{
			leastFreqIndex = currentIndex; // Update the index of the least frequently used page
			leastFreqRef = pageFrames[currentIndex].refNum; // Update the least frequency reference value
		}
	}

	// If the least frequently used page has been modified, write it back to disk
	if (pageFrames[leastFreqIndex].dirtyBit == 1)
	{
		SM_FileHandle fileHandle;
		openPageFile(bm->pageFile, &fileHandle);
		writeBlock(pageFrames[leastFreqIndex].pageNum, &fileHandle, pageFrames[leastFreqIndex].data);
		
		// Increment the write counter to track disk writes
		pageWriteCount++;
	}

	// Replace the least frequently used page with the new page's data
	pageFrames[leastFreqIndex].data = page->data;
	pageFrames[leastFreqIndex].pageNum = page->pageNum;
	pageFrames[leastFreqIndex].dirtyBit = page->dirtyBit;
	pageFrames[leastFreqIndex].fixCount = page->fixCount;

	// Update the LFU pointer to point to the next position for future use
	lfuPosition = (leastFreqIndex + 1) % bufferPoolSize;
}


/*
	- Description: Implements the Least Recently Used (LRU) page replacement strategy
	- Parameters: 
		1. bm - pointer to the buffer pool
		2. page - pointer to the new PageFrame to be added
	- Return: void
*/
extern void LRU(BM_BufferPool *const bm, PageFrame *page) {
	// Get the array of page frames from the buffer pool's management data
	PageFrame *pageFrames = (PageFrame *) bm->mgmtData;

	int leastHitIndex = -1;  // Index of the least recently used page
	int leastHitNum = -1;    // Hit number for least recently used page

	// Find the first unused page frame (fixCount == 0) and use it as the starting point for comparison
	for (int i = 0; i < bufferPoolSize; i++) {
		if (pageFrames[i].fixCount == 0) { 
			leastHitIndex = i;
			leastHitNum = pageFrames[i].lastAccessedTime;
			printf("Found an unused page frame at index %d with hitNum %d\n", leastHitIndex, leastHitNum);
			break;
		}
	}

	// Now find the least recently used page frame by comparing hit numbers
	for (int i = leastHitIndex + 1; i < bufferPoolSize; i++) {
		if (pageFrames[i].fixCount == 0 && pageFrames[i].hitNum < leastHitNum) {
			leastHitIndex = i;
			leastHitNum = pageFrames[i].hitNum;
			printf("Updating least recently used page frame to index %d with hitNum %d\n", leastHitIndex, leastHitNum);
		}
	}

	// If the page to be replaced is dirty (modified), write it back to disk
	if (pageFrames[leastHitIndex].dirtyBit == 1) {
		SM_FileHandle fileHandle;
		openPageFile(bm->pageFile, &fileHandle); // Open the file to write
		writeBlock(pageFrames[leastHitIndex].pageNum, &fileHandle, pageFrames[leastHitIndex].data);
		writeCount++; // Increment the write count
		printf("Page frame at index %d was dirty, wrote it back to disk. Total writes: %d\n", leastHitIndex, writeCount);
	}

	// Replace the least recently used page with the new page's data
	pageFrames[leastHitIndex].data = page->data;
	pageFrames[leastHitIndex].pageNum = page->pageNum;
	pageFrames[leastHitIndex].dirtyBit = page->dirtyBit;
	pageFrames[leastHitIndex].fixCount = page->fixCount;
	pageFrames[leastHitIndex].hitNum = page->hitNum;

	// Print the details of the replacement
	printf("Replaced page frame at index %d with new page: pageNum = %d, dirtyBit = %d, fixCount = %d, hitNum = %d\n",
		   leastHitIndex, page->pageNum, page->dirtyBit, page->fixCount, page->hitNum);
}


/*
	- Description: Implements the CLOCK page replacement strategy
	- Parameters: 
		1. bm - pointer to the buffer pool
		2. page - pointer to the new PageFrame to be added
	- Return: void
*/
extern void CLOCK(BM_BufferPool *const bm, PageFrame *page) {	
	// Get the array of page frames from the buffer pool's management data
	PageFrame *pageFrames = (PageFrame *) bm->mgmtData;

	while (true) {
		// Restart the index if a full round has been completed
		if (clockPointer % bm->numPages  == 0) {
			printf("Completed a full round, resetting clockPointer to 0\n");
			clockPointer = 0;
		}
		
		// Check the current page frame
		PageFrame *currentFrame = &pageFrames[clockPointer];

		// If the page frame is not in use (hitNum == 0), replace it
		if (currentFrame->accessFrequency == 0) {
			printf("Replacing page frame at index %d\n", clockPointer);

			// If the page is dirty, write it to disk
			if (currentFrame->dirtyBit == 1) {
				SM_FileHandle fileHandle;
				openPageFile(bm->pageFile, &fileHandle); // Open the disk file
				writeBlock(currentFrame->pageNum, &fileHandle, currentFrame->data); // Write the data
				writeCount++; // Increment the write count
				printf("Wrote dirty page frame at index %d to disk. Total writes: %d\n", clockPointer, writeCount);
			}
			
			// Replace the contents of the page frame with the new page
			currentFrame->data = page->data;
			currentFrame->pageNum = page->pageNum;
			currentFrame->dirtyBit = page->dirtyBit;
			currentFrame->fixCount = page->fixCount;
			currentFrame->hitNum = page->hitNum;

			// Move the clock hand to the next page frame
			clockPointer++;
			printf("Replaced page frame at index %d with new page: pageNum = %d, dirtyBit = %d, fixCount = %d, hitNum = %d\n",
				clockPointer - 1, page->pageNum, page->dirtyBit, page->fixCount, page->hitNum);
			break;	
		} else {
			// If the page frame is in use, set its hitNum to 0 and move to the next frame
			printf("Page frame at index %d is in use, setting hitNum to 0 and moving to the next frame\n", clockPointer);
			currentFrame->hitNum = 0;
			clockPointer++; // Move the clock pointer to the next page frame
		}
	}
}



/*
	- Description: Creates and initializes a buffer pool with page frames (numPages).
	- Parameters: 
		1. bm - pointer to the buffer pool
		2. pageFileName - stores the name of the page file which is cached in memory.
		3. numPages - number of page frames to allocate in the buffer pool
		4. strategy - the page replacement strategy (FIFO, LRU, LFU, CLOCK)
		5. stratData - parameters specific to the page replacement strategy
	- Return: RC code
*/
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData)
{
	// Set the buffer pool's parameters
	bm->strategy = strategy;
	bm->numPages = numPages;
	bm->pageFile = (char *)pageFileName;
	
	// Allocate memory for the page frames in the buffer pool
	PageFrame *myPageFrames = malloc(sizeof(PageFrame) * numPages);
	if (myPageFrames == NULL) {
		printf("Error: Memory allocation for page frames failed\n");
		return RC_MEM_ALLOC_FAIL; // Return error if memory allocation fails
	}

	// Set the global buffer size for later reference
	bufferSize = numPages;	
	printf("Buffer pool initialized with %d page frames\n", bufferSize);

	// Initialize every page frame in the buffer pool
	for (int k = 0; k < bufferSize; k++) {
		myPageFrames[k].data = NULL;       // Initialize data pointer to NULL
		myPageFrames[k].pageNum = -1;      // Set page number to -1 (indicating no page)
		myPageFrames[k].dirtyBit = 0;      // Mark the page as clean
		myPageFrames[k].fixCount = 0;      // No clients are using this page
		myPageFrames[k].hitNum = 0;        // Hit count for LRU or CLOCK
		myPageFrames[k].refNum = 0;        // Reference count for LFU

		printf("Initialized page frame %d: pageNum = %d, dirtyBit = %d, fixCount = %d\n",
		       k, myPageFrames[k].pageNum, myPageFrames[k].dirtyBit, myPageFrames[k].fixCount);
	}

	// Assign the allocated page frames to the buffer pool's management data
	bm->mgmtData = myPageFrames;

	// Initialize global pointers for LFU and CLOCK strategies
	//lfuPointer = 0;
	writeCount = 0;
	clockPointer = 0;
	int lfuPosition = 0;
	int hit = 0;

	printf("Buffer pool initialization complete.\n");
	return RC_OK;
}



/*
	- Description: Destroys the buffer pool by freeing all allocated memory.
	- Parameters:
		1. bm - pointer to the buffer pool
	- Return: RC code indicating success or failure
*/
extern RC shutdownBufferPool(BM_BufferPool *const bm) {
	PageFrame *pageFrames = (PageFrame *) bm->mgmtData;

	// Check if there are pinned pages in the buffer pool
	for (int i = 0; i < bufferSize; i++) {
		if (pageFrames[i].fixCount != 0) {
			printf("Error: Cannot shutdown buffer pool. There are pinned pages in buffer.\n");
			return RC_PINNED_PAGES_IN_BUFFER; // Cannot shutdown if there are pinned pages
		}
	}

	// Write any dirty pages to disk before shutdown
	printf("Flushing dirty pages to disk...\n");
	forceFlushPool(bm);
	
	// Free allocated memory for page frames
	free(pageFrames);
	bm->mgmtData = NULL; // Clear the pointer to the freed memory

	printf("Buffer pool shutdown complete. Memory has been freed.\n");
	return RC_OK;
}

/*
	- Description: Forces the flush of all dirty pages in the buffer pool to disk.
	- Parameters:
		1. bm - pointer to the buffer pool
	- Return: RC code indicating success or failure
*/
extern RC forceFlushPool(BM_BufferPool* const bm) {
	PageFrame *pageFrames = (PageFrame*)bm->mgmtData;
	
	// Check for any dirty pages and write them to disk if no clients are using them
	printf("Flushing dirty pages to disk...\n");
	for (int i = 0; i < bm->numPages; i++) {
		if (pageFrames[i].fixCount == 0) { // Page is not pinned by any client
			if (pageFrames[i].dirtyBit == 1) { // Page is dirty
				SM_FileHandle fh;
				RC rc = openPageFile(bm->pageFile, &fh); // Open the disk file
				if (rc != RC_OK) {
					printf("Error: Unable to open page file for writing.\n");
					return rc; // Return error if file cannot be opened
				}

				rc = writeBlock(pageFrames[i].pageNum, &fh, pageFrames[i].data); // Write the data to disk
				if (rc != RC_OK) {
					printf("Error: Failed to write block %d to disk.\n", pageFrames[i].pageNum);
					return rc; // Return error if write fails
				}

				pageFrames[i].dirtyBit = 0; // Mark the page as clean after writing
				writeCount++; // Increment the write count
				printf("Successfully flushed dirty page %d to disk.\n", pageFrames[i].pageNum);
			}
		}
	}
	
	printf("All dirty pages flushed successfully.\n");
	return RC_OK; // Indicate successful operation
}


/*
	- Description: Marks the specified page as dirty, indicating that its data has been modified.
	- Parameters:
		1. bm - pointer to the buffer pool
		2. page - pointer to the page handle to be marked as dirty
	- Return: RC code indicating success or failure
*/
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
	PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

	// Iterating through all the pages in the buffer pool to find the target page
	for (int i = 0; i < bufferSize; i++) {
		// Check if the current page matches the page to be marked as dirty
		if (pageFrames[i].pageNum == page->pageNum) {
			pageFrames[i].dirtyBit = 1; // Mark the page as dirty
			printf("Page %d has been marked dirty.\n", page->pageNum);
			return RC_OK; // Indicate successful operation
		}
	}

	// If the page is not found, return an error
	printf("Error: Page %d not found in buffer pool.\n", page->pageNum);
	return RC_ERROR; // Indicate failure
}



/*
	- Description: Removes the specified page from memory by decrementing its fix count (unpinning the page).
	- Parameters:
		1. bm - pointer to the buffer pool
		2. page - pointer to the page handle to be unpinned
	- Return: RC code indicating success or failure
*/
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;

    // Iterate through all pages in the buffer pool to find the target page
    for (int j = 0; j < bufferSize; j++) {
        // Check if the current page matches the page to be unpinned
        if (pageFrames[j].pageNum == page->pageNum) {
            // Ensure fixCount doesn't go negative
            if (pageFrames[j].fixCount > 0) {
                pageFrames[j].fixCount--; // Decrement the fix count
                printf("Page %d has been unpinned. Current fix count: %d\n", page->pageNum, pageFrames[j].fixCount);
                return RC_OK; // Indicate successful operation
            } else {
                printf("Error: Attempting to unpin page %d, but it is already unpinned.\n", page->pageNum);
                return RC_ERROR; // Indicate error due to already unpinned page
            }
        }
    }

    // If the page is not found, return an error
    printf("Error: Page %d not found in buffer pool.\n", page->pageNum);
    return RC_ERROR; // Indicate failure
}


/*
	- Description: Writes the current content of the specified page back to the page file on disk.
	- Parameters:
		1. bm - pointer to the buffer pool
		2. page - pointer to the page handle for the page to be written to disk
	- Return: RC code indicating success or failure
*/
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    PageFrame *pageFrames = (PageFrame *)bm->mgmtData;
    
    // Iterate through all the pages in the buffer pool to find the target page
    for (int i = 0; i < bufferSize; i++) {
        // Check if the current page matches the page to be written to disk
        if (pageFrames[i].pageNum == page->pageNum) {
            SM_FileHandle fh;

            // Open the page file for writing
            if (openPageFile(bm->pageFile, &fh) != RC_OK) {
                printf("Error: Unable to open page file %s for writing.\n", bm->pageFile);
                return RC_FILE_NOT_FOUND; // Indicate failure to open file
            }

            // Write the page data to disk
            if (writeBlock(pageFrames[i].pageNum, &fh, pageFrames[i].data) != RC_OK) {
                printf("Error: Unable to write page %d to disk.\n", pageFrames[i].pageNum);
                return RC_WRITE_FAILED; // Indicate failure to write page
            }

            // Mark page as undirty because the modified page has been written to disk
            pageFrames[i].dirtyBit = 0;

            // Increase the write count which records the number of writes done by the buffer manager
            writeCount++;

            printf("Page %d has been successfully written to disk. Current write count: %d\n", pageFrames[i].pageNum, writeCount);
            return RC_OK; // Indicate successful operation
        }
    }

    // If the page is not found, return an error
    printf("Error: Page %d not found in buffer pool.\n", page->pageNum);
    return RC_ERROR; // Indicate failure
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
	- Description: Adds the page with the specified page number to the buffer pool by pinning it.
	When the buffer pool is full, a page in memory is replaced with the newly pinned page using the appropriate page replacement strategy.
	- Parameters:
		1. bm - pointer to the buffer pool
		2. page - pointer to the page handle to store page information
		3. pageNum - the position of the page in the page file
	- Return: RC code indicating success or failure
*/
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    PageFrame *frames = (PageFrame *)bm->mgmtData;

    // Check if the first page frame is empty (initial pin)
    if (isPageFrameEmpty(&frames[0])) {
        SM_FileHandle fh;
        if (openPageFile(bm->pageFile, &fh) != RC_OK) {
            printf("Error: Unable to open page file %s.\n", bm->pageFile);
            return RC_FILE_NOT_FOUND;
        }

        // Allocate memory for page data and read the page from disk
        frames[0].data = (SM_PageHandle)malloc(PAGE_SIZE);
        ensureCapacity(pageNum, &fh);
        if (readBlock(pageNum, &fh, frames[0].data) != RC_OK) {
            printf("Error: Unable to read page %d from disk.\n", pageNum);
            free(frames[0].data); // Free allocated memory
            return RC_READ_FAILED;
        }

        // Initialize the frame properties
        frames[0].pageNum = pageNum;
        frames[0].dirtyBit = 0;
        frames[0].fixCount = 1;
        frames[0].hitNum = 0;
        frames[0].refNum = 0;

        // Set the page handle
        page->pageNum = pageNum;
        page->data = frames[0].data;

        printf("Pinned page %d to frame 0.\n", pageNum);
        return RC_OK;
    } else {
        bool bufferFull = true;

        // Check if the page is already in memory
        for (int j = 0; j < bufferSize; j++) {
            if (frames[j].pageNum != -1) {
                if (frames[j].pageNum == pageNum) {
                    // Page already in memory, increment fixCount
                    frames[j].fixCount++;
                    bufferFull = false;

                    // Update usage statistics based on the replacement strategy
                    hit++;
                    if (bm->strategy == RS_CLOCK) {
                        frames[j].hitNum = 1;
                    } else if (bm->strategy == RS_LFU) {
                        frames[j].refNum++;
                    } else if (bm->strategy == RS_LRU) {
                        frames[j].hitNum = hit;
                    }

                    // Set the page handle
                    page->data = frames[j].data;
                    page->pageNum = pageNum;

                    printf("Page %d already pinned. Fix count: %d\n", pageNum, frames[j].fixCount);
                    return RC_OK;
                }
            } else {
                // Frame is empty, read page into this frame
                SM_FileHandle fileh;
                if (openPageFile(bm->pageFile, &fileh) != RC_OK) {
                    printf("Error: Unable to open page file %s.\n", bm->pageFile);
                    return RC_FILE_NOT_FOUND;
                }

                frames[j].data = (SM_PageHandle)malloc(PAGE_SIZE);
                ensureCapacity(pageNum, &fileh);
                if (readBlock(pageNum, &fileh, frames[j].data) != RC_OK) {
                    printf("Error: Unable to read page %d from disk.\n", pageNum);
                    free(frames[j].data); // Free allocated memory
                    return RC_READ_FAILED;
                }

                frames[j].pageNum = pageNum;
                frames[j].fixCount = 1;
                frames[j].dirtyBit = 0;
                frames[j].refNum = 0;

               pagesReadCount++;
                bufferFull = false;

                // Update hitNum based on strategy
                if (bm->strategy == RS_LRU) {
                    frames[j].hitNum = pagesReadCount;
                } else if (bm->strategy == RS_CLOCK) {
                    frames[j].hitNum = 1;
                }

                // Set the page handle
                page->pageNum = pageNum;
                page->data = frames[j].data;

                printf("Pinned page %d to empty frame %d.\n", pageNum, j);
                return RC_OK;
            }
        }

        // If bufferFull is true, apply page replacement strategy
        if (bufferFull) {
            // Create a new page frame to read the new page
            SM_FileHandle fileH;
            if (openPageFile(bm->pageFile, &fileH) != RC_OK) {
                printf("Error: Unable to open page file %s.\n", bm->pageFile);
                return RC_FILE_NOT_FOUND;
            }

            // Allocate memory for the new page and read it from disk
            PageFrame *newPage = (PageFrame *)malloc(sizeof(PageFrame));
            newPage->data = (SM_PageHandle)malloc(PAGE_SIZE);
            ensureCapacity(pageNum, &fileH);
            if (readBlock(pageNum, &fileH, newPage->data) != RC_OK) {
                printf("Error: Unable to read page %d from disk.\n", pageNum);
                free(newPage->data); // Free allocated memory
                free(newPage);
                return RC_READ_FAILED;
            }

            // Initialize the new page frame
            newPage->pageNum = pageNum;
            newPage->dirtyBit = 0;
            newPage->refNum = 0;
            newPage->fixCount = 1;

            hit++;

            // Update hitNum based on strategy
            if (bm->strategy == RS_CLOCK) {
                newPage->hitNum = 1;
            } else if (bm->strategy == RS_LRU) {
                newPage->hitNum = hit;
            }

            // Handle page replacement using the appropriate strategy
            if (bm->strategy == RS_FIFO) {
                FIFO(bm, newPage);
            } else if (bm->strategy == RS_LRU) {
                LRU(bm, newPage);
            } else if (bm->strategy == RS_CLOCK) {
                CLOCK(bm, newPage);
            } else if (bm->strategy == RS_LFU) {
                LFU(bm, newPage);
            } else if (bm->strategy == RS_LRU_K) {
                printf("LRU-k algorithm is not used.\n");
            } else {
                printf("No algorithm has been used.\n");
            }

            // Set the page handle
            page->pageNum = pageNum;
            page->data = newPage->data;

            // Free the newPage structure, but keep the data
            free(newPage);

            printf("Pinned page %d using page replacement strategy.\n", pageNum);
        }
    }
}


/*
	- Description: Returns an array of page numbers currently in the buffer pool.
	- Parameters:
		1. bm - pointer to the buffer pool
	- Return: Pointer to an array of PageNumbers
*/
extern PageNumber *getFrameContents(BM_BufferPool *const bm) {
    // Allocate memory for the array of page numbers
    PageNumber *frameContents = (PageNumber *)malloc(sizeof(PageNumber) * bufferSize);
    if (frameContents == NULL) {
        printf("Error: Memory allocation failed for frame contents.\n");
        return NULL; // Handle memory allocation failure
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Iterate through all the pages in the buffer pool
    for (int i = 0; i < bufferSize; i++) {
        frameContents[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
    }

    return frameContents;
}

/*
	- Description: Returns an array of booleans representing the dirty flags of the respective pages.
	- Parameters:
		1. bm - pointer to the buffer pool
	- Return: Pointer to an array of booleans
*/
extern bool *getDirtyFlags(BM_BufferPool *const bm) {
    // Allocate memory for the array of dirty flags
    bool *dirtyFlags = (bool *)malloc(sizeof(bool) * bufferSize);
    if (dirtyFlags == NULL) {
        printf("Error: Memory allocation failed for dirty flags.\n");
        return NULL; // Handle memory allocation failure
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Iterate through all the pages in the buffer pool
    for (int i = 0; i < bufferSize; i++) {
        dirtyFlags[i] = (pageFrame[i].dirtyBit == 1);
    }

    return dirtyFlags;
}

/*
	- Description: Returns an array of integers representing the fix counts of the pages stored in the respective page frames.
	- Parameters:
		1. bm - pointer to the buffer pool
	- Return: Pointer to an array of fix counts
*/
extern int *getFixCounts(BM_BufferPool *const bm) {
    // Allocate memory for the array of fix counts
    int *fixCounts = (int *)malloc(sizeof(int) * bufferSize);
    if (fixCounts == NULL) {
        printf("Error: Memory allocation failed for fix counts.\n");
        return NULL; // Handle memory allocation failure
    }

    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    // Iterate through all the pages in the buffer pool
    for (int i = 0; i < bufferSize; i++) {
        fixCounts[i] = pageFrame[i].fixCount; // Directly assign fixCount (no need to check for -1)
    }

    return fixCounts;
}

/*
	- Description: Returns the number of pages that have been read from disk since the buffer pool was initialized.
	- Parameters:
		1. bm - pointer to the buffer pool
	- Return: The number of pages read from disk
*/
extern int getNumReadIO(BM_BufferPool *const bm) {
    return (pagesReadCount);
}

/*
	- Description: Returns the number of pages that have been written to the page file since the buffer pool was initialized.
	- Parameters:
		1. bm - pointer to the buffer pool
	- Return: The number of pages written to the page file
*/
extern int getNumWriteIO(BM_BufferPool *const bm) {
    // Return the total number of write operations
    return writeCount;
}
