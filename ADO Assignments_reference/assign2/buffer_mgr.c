#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

typedef struct bufferPageInfo
{
	
	SM_PageHandle pgHandlerdata;
	int cliCount;
	int ref;
	int inflNum;
	PageNumber numberOfPages;
	bool checkdirty;
	
}bufferPageInfo;

bool val = false;
int inflag = 0; 
int totalIOWrtCnt = 0; 
int trackPtr = 0;
int sizeBufferPool = 0;
int readNumPages = 0;
 

void createNewPage(bufferPageInfo *bufferPage, const bufferPageInfo *bpage, int bpfIndex) {
    if (bufferPage == NULL || bpage == NULL) {
        puts("Error : Null pointer provided to createNewPage.");
        return;
    }else {
		bufferPage[bpfIndex].inflNum = bpage->inflNum;	
		if (!val) {
			bufferPage[bpfIndex].checkdirty = bpage->checkdirty;	
			bufferPage[bpfIndex].cliCount = bpage->cliCount;
		}	
		
		puts("Copying number and Handler data to the given file");
		bufferPage[bpfIndex].numberOfPages = bpage->numberOfPages;
		if(bufferPage != NULL){
		bufferPage[bpfIndex].pgHandlerdata = bpage->pgHandlerdata;
		}
		
	}
}

bool numPages(BM_BufferPool *const bufferPoolManager, BM_PageHandle *const page,const PageNumber numberOfPages){
	
	puts("NumPage functionality called in pinPage");
	SM_FileHandle handleptr;
	int c = 0;
	readNumPages = 0;
	page->pageNum = numberOfPages;
	printf("Accessing the buffer page frame array");
	bufferPageInfo *bufferPg = (bufferPageInfo *)bufferPoolManager->mgmtData;
	bufferPg[c].cliCount++;
	
	bufferPg[c].pgHandlerdata = (SM_PageHandle)malloc(PAGE_SIZE);
	openPageFile(bufferPoolManager->pageFile, &handleptr);
	if(&handleptr != NULL ){
	readBlock(numberOfPages, &handleptr, bufferPg[c].pgHandlerdata);
	}
	if (!val) {
		ensureCapacity(numberOfPages, &handleptr);
	}
	
	while (c == 0) {
		bufferPg[c].numberOfPages = numberOfPages;
		bufferPg[c].ref = 0;
		bufferPg[c].inflNum = inflag;
		break;
	}
	
	page->data = bufferPg[0].pgHandlerdata;
}

void writeBlockToDisk(BM_BufferPool *const bufferPoolManager, bufferPageInfo *bufferPage, int bpfIndex) {
    if (bufferPoolManager == NULL || bufferPage == NULL) {
        puts("Invalid buffer manager or page frame.");
        return; 
    }
    SM_FileHandle fileHandle;
    if (openPageFile(bufferPoolManager->pageFile, &fileHandle) != RC_OK) { 
        printf("Failed in openning page file: %s", bufferPoolManager->pageFile);
        return; 
    }
    int writeStatus = writeBlock(bufferPage[bpfIndex].numberOfPages, &fileHandle, bufferPage[bpfIndex].pgHandlerdata);
    if (writeStatus != RC_OK) {
        puts("Write Block failed");
    } else {
        totalIOWrtCnt++; 
	}
}


void CLOCK_Strategy(BM_BufferPool *const bufferPoolManager, bufferPageInfo *bpage) {
    if (bufferPoolManager == NULL || bpage == NULL) {
        puts("Invalid buffer manager / page frame.");
        return;
    }
    bufferPageInfo *bufferPage = (bufferPageInfo *)bufferPoolManager->mgmtData;
    bool pgReplace = false;
    int bufferPoolSize = bufferPoolManager->numPages; 

    while (!pgReplace) {
        for (int i = 0; i < bufferPoolSize; ++i) {
            int index = (trackPtr + i) % bufferPoolSize; 
            if (bufferPage[index].inflNum == 0 && bufferPage[index].cliCount == 0) {
                if (bufferPage[index].checkdirty) {
                    writeBlockToDisk(bufferPoolManager, bufferPage, index); 
                }
                createNewPage(bufferPage, bpage, index); 
                pgReplace = true;
                trackPtr = (index + 1) % bufferPoolSize; 
                break; 
            }
        }
        if (!pgReplace) {
            trackPtr = (trackPtr + 1) % bufferPoolSize;
        }
    }
    for (int i = 0; i < bufferPoolSize; i++) {
        bufferPage[i].inflNum = 0;
    }
    puts("checking through the beffer pool");
}

void LRU_Strategy(BM_BufferPool *const bufferPoolManager, bufferPageInfo *nPage) {
    bufferPageInfo *bufferPagePtr = (bufferPageInfo *)bufferPoolManager->mgmtData;
    int lIndex = 0;
    puts("Assigning the first page struct with least frequent index");

    int lowestfNum = bufferPagePtr[0].inflNum;
    for (int i = 1; i < sizeBufferPool; i++) {
        if (bufferPagePtr[i].inflNum < lowestfNum) {
            lowestfNum = bufferPagePtr[i].inflNum;
            lIndex = i;
        }
    }
	if (bufferPagePtr[lIndex].checkdirty) {
        puts("Memory saved into the disk.");
        writeBlockToDisk(bufferPoolManager, bufferPagePtr, lIndex);
    }
	createNewPage(bufferPagePtr, nPage, lIndex);
}

void FIFO_Strategy(BM_BufferPool *const bufferPoolManager, bufferPageInfo *nPage) {
 
	int pageCount = sizeBufferPool;
	int curIndex = readNumPages % pageCount;
    int attempts = 0;
	bufferPageInfo *bufferPagePtr = (bufferPageInfo *)bufferPoolManager->mgmtData;

	puts("Loading data to buffer pool");
    while(attempts < pageCount) {  
        if (bufferPagePtr[curIndex].cliCount == 0) {
            if (bufferPagePtr[curIndex].checkdirty == true) {
                writeBlockToDisk(bufferPoolManager, bufferPagePtr, curIndex);
            }
			puts("Saving buffer page in the pool");
           	createNewPage(bufferPagePtr, nPage, curIndex);
            break;
        } else {
            curIndex = (curIndex + 1) % pageCount;
        }
        attempts++;
    }
}

RC initBufferPool(BM_BufferPool* const bufferPoolManager, const char* const namePgFile, const int pageCount, ReplacementStrategy rStrategy, void* stratData) {

	puts("Buffer Page is assigned to memory");
	puts("Buffer Pool is initialized");
    bufferPageInfo* bpage = calloc(pageCount, sizeof(bufferPageInfo));
    if (bpage == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
	sizeBufferPool = pageCount;

    for(int i = 0; i < pageCount; i++) {
		bpage[i].cliCount = 0;
        bpage[i].pgHandlerdata = NULL;
        bpage[i].numberOfPages = -1;
        bpage[i].inflNum = 0;
        bpage[i].ref = 0;
        bpage[i].checkdirty = false;
    }

	totalIOWrtCnt = 0;
    trackPtr = 0;
    bufferPoolManager->mgmtData = bpage;
    bufferPoolManager->numPages = pageCount;
    bufferPoolManager->strategy = rStrategy;
    bufferPoolManager->pageFile = (char*)namePgFile;

    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool* const bufferPoolManager) {

    int itr = 0;
	int pageCount = sizeBufferPool;
    bufferPageInfo* bufferPagePtr = (bufferPageInfo*)bufferPoolManager->mgmtData;
    while (itr < pageCount) {
        if (bufferPagePtr[itr].cliCount != 0) {
            return RC_BUFFER_PIN_PG;
        }
        itr++;
    }
    puts("Saving verified dirty and updated pages in disk");
    forceFlushPool(bufferPoolManager);

    puts("Free up memory");
    free(bufferPagePtr);
    bufferPoolManager->mgmtData = NULL;
    return RC_OK;
}


RC forceFlushPool(BM_BufferPool* const bufferPoolManager) {
    SM_FileHandle fHandler;
    int pageCount = sizeBufferPool;
    bufferPageInfo* bufferPagePtr = (bufferPageInfo*)bufferPoolManager->mgmtData;

    for (int i = 0; i < pageCount; i++) {
        bool dirty = bufferPagePtr[i].checkdirty;
        int cliCount = bufferPagePtr[i].cliCount;

        if (cliCount == 0 && dirty == true) {
            openPageFile(bufferPoolManager->pageFile, &fHandler);
			if(&fHandler != NULL){ //check if file opened successfully
            writeBlock(bufferPagePtr[i].numberOfPages, &fHandler, bufferPagePtr[i].pgHandlerdata);
			}
			totalIOWrtCnt++;
            bufferPagePtr[i].checkdirty = false;
        }
    }
    return RC_OK;
}

RC markDirty(BM_BufferPool* const bufferPoolManager, BM_PageHandle* const bpage) {
    bufferPageInfo* bufferPg = (bufferPageInfo*)bufferPoolManager->mgmtData;
	bool cDirty = false; 
	
    for (int j = 0; j < sizeBufferPool; j++) {
		  
		int frameNumPg = bufferPg[j].numberOfPages; 
        int numPg = bpage->pageNum;
      
        if (numPg == frameNumPg) {
            bufferPg[j].checkdirty = true;
			cDirty = true;
        }
    }
	return (cDirty ? RC_OK : RC_BUFFER_ERROR);
}


RC unpinPage(BM_BufferPool* const bufferPoolManager, BM_PageHandle* const bpage) {

	int numPage = bpage->pageNum;   
    int itr = 0;
	bufferPageInfo* bufferPg = (bufferPageInfo*)bufferPoolManager->mgmtData;
    
    while (itr < sizeBufferPool) {
        int frameNumPg = bufferPg[itr].numberOfPages;      
        if (numPage == frameNumPg) {
			puts("Checking if number of pages are same");
        	bufferPg[itr].cliCount = (bufferPg[itr].cliCount > 0) ? bufferPg[itr].cliCount - 1 : 0;
            break;
        }         
        itr++;
    }   
  return RC_OK;
}

RC forcePage(BM_BufferPool* const bufferPoolManager, BM_PageHandle* const page) {

    bool cliCount = false; 
    int numPg = page->pageNum;
    bufferPageInfo* bufferPg = (bufferPageInfo*)bufferPoolManager->mgmtData;
    
	puts("Writing to the disk");
    for (int i = 0; i < sizeBufferPool; i++) {
        int frameNumPg = bufferPg[i].numberOfPages;
        
        if (numPg == frameNumPg) {
            writeBlockToDisk(bufferPoolManager, bufferPg, i);
            cliCount = true; 
        }
    } 
    return (cliCount ? RC_OK : RC_PAGE_NOT_FOUND);
}

RC pinPage(BM_BufferPool *const bufferPoolManager, BM_PageHandle *const page,const PageNumber numberOfPages) {   
	
    bool checkbuffer = true;
	int initial = 0;
	bufferPageInfo *bufferPg = (bufferPageInfo *)bufferPoolManager->mgmtData;
	
    if(bufferPg[initial].numberOfPages == -1) {
        numPages(bufferPoolManager, page, numberOfPages);
        return RC_OK;
    }

    int pgCount = bufferPg[initial].numberOfPages;
  	puts("Traversing the page frames.");

	for (int itr = 0; itr < sizeBufferPool; itr++) {   
		if (bufferPg[itr].numberOfPages == numberOfPages) {
			
			inflag++;
			bufferPg[itr].cliCount++;
			checkbuffer = false;
            if (bufferPoolManager->strategy == RS_LRU) {
				bufferPg[itr].inflNum = inflag;
				puts("LRU is Replacement strategy");
			}
			page->pageNum = numberOfPages;
			page->data = bufferPg[itr].pgHandlerdata;
            if (bufferPoolManager->strategy == RS_CLOCK) {
				bufferPg[itr].inflNum = 1;
				puts("Clock is Replacement strategy");
			}
			trackPtr++;
			puts("ending once the pointer is incremented");
			break;
		}
		
		if (pgCount == -1) {
			puts("Inside the check condition of page frame empty");
			SM_FileHandle fileptr;
			puts("file open");
			openPageFile(bufferPoolManager->pageFile, &fileptr);
			bufferPg[itr].pgHandlerdata = (SM_PageHandle)malloc(PAGE_SIZE);
			if (bufferPg[itr].pgHandlerdata == NULL){
				puts("Memory allocation failed.");
				return RC_CALLOC_FAILED;
			}
			
			readNumPages = readNumPages+1;
			printf("flag value after updatition",inflag++);
			readBlock(numberOfPages, &fileptr, bufferPg[itr].pgHandlerdata);
			bufferPg[itr].cliCount = 1;
			bufferPg[itr].ref = 0;
            bufferPg[itr].numberOfPages = numberOfPages;
			
            
            if(bufferPoolManager->strategy == RS_LRU) {
                bufferPg[itr].inflNum = inflag;
            }
			
			page->pageNum = numberOfPages;
			page->data = bufferPg[itr].pgHandlerdata;
            
            if (bufferPoolManager->strategy == RS_CLOCK) {
                bufferPg[itr].inflNum = 1;
            }
			
			checkbuffer = false;
			puts("ending this loop once the buffer is set to false.");
			break;
			
		}
	}
	if (checkbuffer == true) {
		bufferPageInfo *newPg = (bufferPageInfo *)malloc(sizeof(bufferPageInfo));
		if (newPg == NULL){
				puts("Memory allocation failed.");
				return RC_CALLOC_FAILED;
			}
		readNumPages++;
		SM_FileHandle fileptr;
		openPageFile(bufferPoolManager->pageFile, &fileptr);
		if(&fileptr == NULL){
			return RC_FILE_NOT_FOUND;
		}
		newPg->numberOfPages = numberOfPages;
		newPg->ref = 0;
		inflag = inflag + 1;
		newPg->pgHandlerdata = (SM_PageHandle)malloc(PAGE_SIZE);
		if (newPg->pgHandlerdata == NULL){
				puts("Memory allocation failed.");
				return RC_CALLOC_FAILED;
			}
		readBlock(numberOfPages, &fileptr, newPg->pgHandlerdata);
		newPg->cliCount = 1;
		newPg->checkdirty = false;
		
		
        while (bufferPoolManager->strategy==RS_LRU) {
            newPg->inflNum = inflag;
            break;
        }
        while (bufferPoolManager->strategy!=RS_LRU && bufferPoolManager->strategy==RS_CLOCK) {
            newPg->inflNum = 1;
            break;
        }

		page->pageNum = numberOfPages;
		page->data = newPg->pgHandlerdata;
		ReplacementStrategy strategy = bufferPoolManager->strategy;

        switch (strategy) {
            case RS_FIFO:
                puts("Calling FIFO on the RS_FIFO");
                FIFO_Strategy(bufferPoolManager, newPg);
                break;
            case RS_LRU:
                puts("Calling LRU_strategy on the RS_LRU");
                LRU_Strategy(bufferPoolManager, newPg);
                break;
            case RS_CLOCK:
                puts("Calling clock_strategy on the RS_Clock");
                CLOCK_Strategy(bufferPoolManager, newPg);
                break;
            default:
                puts("didn't match to any case");
        }

		
	puts("Replace the page with new page.");
	}
	puts("Pin page success");
	return RC_OK;
}


PageNumber* getFrameContents(BM_BufferPool* const bufferPoolManager) {
    bufferPageInfo* bufferPg = (bufferPageInfo*)bufferPoolManager->mgmtData;
	PageNumber* conPgNum = malloc(sizeof(PageNumber) * bufferPoolManager->numPages);
    if (conPgNum == NULL) {
        return RC_CALLOC_FAILED;
    }   
    for (int i = 0; i < bufferPoolManager->numPages; i++) {
        conPgNum[i] = (bufferPg[i].numberOfPages == -1) ? NO_PAGE : bufferPg[i].numberOfPages;
    }

    return conPgNum;
}


bool* getDirtyFlags(BM_BufferPool* const bufferPoolManager) {
    bufferPageInfo* bufferPg = (bufferPageInfo*)bufferPoolManager->mgmtData;
    bool* cliCount = malloc(sizeof(bool) * sizeBufferPool);
    if (cliCount == NULL) {
        return RC_CALLOC_FAILED;
    }
    for (int i = 0; i < sizeBufferPool; i++) {
        cliCount[i] = bufferPg[i].checkdirty;
    }
    return cliCount;
}


int* getFixCounts(BM_BufferPool* const bufferPoolManager) {
    bufferPageInfo* bufferPg = (bufferPageInfo*)bufferPoolManager->mgmtData;
    int* fixCounts = malloc(sizeof(int) * bufferPoolManager->numPages);   
    if (fixCounts == NULL) {
        return RC_CALLOC_FAILED;
    }
    for (int i = 0; i < bufferPoolManager->numPages; i++) {
        // Assuming 'false' is meant to be 0 and 'true' to be 1
        fixCounts[i] = (bufferPg[i].cliCount == -1) ? 0 : bufferPg[i].cliCount;
    }
    return fixCounts;
}


int getNumReadIO(BM_BufferPool *const bufferPoolManager)
{
	int pgcount = ++readNumPages;
    return pgcount;
}


int getNumWriteIO(BM_BufferPool *const bufferPoolManager)
{ 
	if (totalIOWrtCnt != NULL){
			return totalIOWrtCnt;
	}
}