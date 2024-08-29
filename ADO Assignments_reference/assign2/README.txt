Advanced Database Organization(CS525) Assignment 2: Buffer Manager 

Group Number: cs525-SP24-group#10


/************************************************************************/
SYNOPSIS:
/************************************************************************/
Implementing Storage Manager for the Database management system. 

/************************************************************************/
Step by step installation and execution For Linux:
/************************************************************************/

a. Download the repository from git and unzip it.
b. Open 'assign2' folder in VS code(or any IDE). 
c. Make sure you have gcc and make install on your system. (commands : 'sudo apt install make' and 'sudo apt install gcc') 
d. Open Terminal and run command 'make clean'
e. On successful run, execute the test file using command './test1'

/************************************************************************/
FUNCTIONS USED: 
/************************************************************************/

1. RC initBufferPool(BM_BufferPool* const bufferPoolManager, const char* const namePgFile, const int pageCount, ReplacementStrategy rStrategy, void* stratData):
	initBufferPool sets up a buffer pool to cache pages from a specified file, using a selected page replacement strategy.It configures the buffer pool's size and tracks the page file, with replacement strategy parameters passed via strategyReplace.

2. RC shutdownBufferPool(BM_BufferPool* const bufferPoolManager):
	shutdownBufferPool efficiently deallocates memory and resources tied to the buffer pool, invoking forceFlushPool to write modified pages before removal.It ensures controlled deallocation, raising an error if any page is still in use by a client (error code RC_BUFFER_PIN_PG).

3. RC forceFlushPool(BM_BufferPool* const bufferPoolManager):
	forceFlushPool writes modified pages marked as "badOrNot = True" to the disk, ensuring that changes are saved and synchronized with the storage system for data 	  integrity.It verifies each page frame to confirm it meets the criteria of being both modified and not actively used by clients (countofClient = 0), proceeding to write the page frame to the disk if both conditions are met.

4. RC markDirty(BM_BufferPool* const bufferPoolManager, BM_PageHandle* const bpage):
	The purpose of the "markDirty" function is to indicate a page in the buffer pool as altered by setting a dirty flag. It traverses the page frames to find the specified page, marking it as dirty if found, and subsequently returns RC_OK. In case the page is not found, it returns RC_ERROR.


5. RC unpinPage(BM_BufferPool* const bufferPoolManager, BM_PageHandle* const bpage):
	Here the client count of a page is decremented by 1  in the buffer pool, signaling its reduced usage. Upon successfully locating the page, the function returns RC_OK; otherwise, it returns RC_ERROR.


6. RC unpinPage(BM_BufferPool* const bufferPoolManager, BM_PageHandle* const bpage):
	Here a specified page is moved from the buffer pool to disk, clearing its dirty flag if it exists. It is also used to update the write count and returing RC_OK. And if the page does not exist, then it returns RC_ERROR.


7. RC pinPage(BM_BufferPool *const bufferPoolManager, BM_PageHandle *const page,const PageNumber numberOfPages):
	This function manages the retrieval of a specific page from the buffer pool, adapting to different scenarios based on the page's state. It supervises page pinning, file management, and utilizes various replacement strategies to ensure smooth operation. Upon successful completion, the function returns RC_OK.


/************************************************************************/
STATISTICS FUNCTIONS: 
/************************************************************************/

1. PageNumber* getFrameContents(BM_BufferPool* const bufferPoolManager):
	This function traverses every page within the buffer pool to gather information about the contents of each frame.


2. bool* getDirtyFlags(BM_BufferPool* const bufferPoolManager):
	This function returns the array size that aligns with the number of pages, and the function examines each page frame to determine the status of its dirty flag.

3. int* getFixCounts(BM_BufferPool* const bufferPoolManager):
	An array of integer is returned by this function, and to get the fixed counts all pageframes are traversed.

4. int getNumReadIO(BM_BufferPool *const bufferPoolManager):
	The function furnishes the cumulative count of input/output (IO) reads executed by the buffer pool, reflecting the quantity of disk pages read.

5. int getNumWriteIO(BM_BufferPool *const bufferPoolManager):
	The function offers the overall count of input/output (IO) writes conducted, illustrating the volume of disk pages written

/************************************************************************/
PAGE REPLACEMENT ALGORITHMS: 
/************************************************************************/

void FIFO_Strategy(BM_BufferPool *const bufferPoolManager, bufferPageInfo *nPage):
	For buffer pool management, FIFO page replacement it employs a queue.It gives precedence to replacing the page that was initially loaded into the buffer pool when the need arises.

void LRU_Strategy(BM_BufferPool *const bufferPoolManager, bufferPageInfo *nPage):
	The LRU page replacement algorithm chooses the page frame in the buffer pool that has had the least recent access for replacement. It uses the fileLstIndx field to keep track of access frequency, preferring to replace the page frame with the lowest fileLstIndx value.

void CLOCK_Strategy(BM_BufferPool *const bufferPoolManager, bufferPageInfo *bpage):
	The CLOCK algorithm for page replacement identifies the recently added page frame in the buffer pool to replace. It employs a circular list with a "hand" to indicate the next page frame for replacement.
