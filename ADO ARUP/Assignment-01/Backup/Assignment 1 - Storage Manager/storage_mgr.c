#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "storage_mgr.h"

FILE *pageFile;

extern void initStorageManager (void) {
	// Initialising file pointer i.e. storage manager.
	pageFile = NULL;
}

// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC createPageFile (char *fileName) {
	
	// First check if it exists
	FILE *fExists = fopen(fileName, "r");
	// It should be NULL
	if (fExists != NULL) {
		fclose(fExists);
		return RC_FILE_ALREADY_EXISTS;
	}
	
	// Opening in w mode opens an empty file for writting in text mode.
	pageFile = fopen(fileName, "w");

	if(pageFile == NULL) {
		if (errno == ENOENT) return RC_FILE_NOT_FOUND; // ENOENT is error code for fopen when not finding the file
		return RC_FILE_NOT_FOUND; // If no error code, just return generic errorreturn RC_FILE_NOT_FOUND;
	}
	// Allocates memory to a buffer and fills it with PAGE_SIZE null characters
	SM_PageHandle buffer = (SM_PageHandle)malloc(PAGE_SIZE, sizeof(char));
	for (int i=0; i<PAGE_SIZE; i++) {
		buffer[i] = '\0';
	}

	int nc = fwrite(&buffer, sizeof(char), PAGE_SIZE, pageFile); // write the buffer to the file ('\0' 4096 times)

	// Checks if it wrote the full PAGE_SIZE characters
	if(nc < PAGE_SIZE) {
		fclose(pageFile);
		printf("Error writting");
		return RC_WRITE_FAILED;
	}
	printf("Successfully writted");
	
	// Frees buffer and closes file
	free(buffer);
	fclose(pageFile);
	
	// During class, it was mentioned that the system checks for the file after creation to leverage the head position
	// Check if file was generated correctly
	fExists = fopen(fileName, "r");
	if (fExists == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	// if exists, check content
	for (int i = 0; i < PAGE_SIZE; i++) {
		if (fgetc(fExists) != '\0') {
			fclose(fExists);
        	return RC_WRITE_FAILED;
		}
	}
	
	if (fgetc(fExists) != EOF) {
		fclose(fExists);
		return RC_WRITE_FAILED;
	}
	fclose(fExists);

	// if everything is ok, return RC_OK
	return RC_OK;
	
}

extern RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
	// Opening file stream in read mode. 'r' mode creates an empty file for reading only.
	pageFile = fopen(fileName, "r");

	// Checking if file was successfully opened.
	if(pageFile == NULL) {
		return RC_FILE_NOT_FOUND;
	} else { 
		// Updating file handle's filename and set the current position to the start of the page.
		fHandle->fileName = fileName;
		fHandle->curPagePos = 0;

		/* In order to calculate the total size, we perform following steps -
		   1. Move the position of the file stream to the end of file
		   2. Check the file end position
		   3. Move the position of the file stream to the beginning of file  
		
		fseek(pageFile, 0L, SEEK_END);
		int totalSize = ftell(pageFile);
		fseek(pageFile, 0L, SEEK_SET);
		fHandle->totalNumPages = totalSize/ PAGE_SIZE;  */
		
		/* Using fstat() to get the file total size.
		   fstat() is a system call that is used to determine information about a file based on its file descriptor.
		   'st_size' member variable of the 'stat' structure gives the total size of the file in bytes.
		*/

		struct stat fileInfo;
		if(fstat(fileno(pageFile), &fileInfo) < 0)    
			return RC_ERROR;
		fHandle->totalNumPages = fileInfo.st_size/ PAGE_SIZE;

		// Closing file stream so that all the buffers are flushed. 
		fclose(pageFile);
		return RC_OK;
	}
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC closePageFile (SM_FileHandle *fHandle) {
	// Checks if pageFile has a file and closes it. If it is NULL, the closing fails
	if (pageFile == NULL) {
		return RC_ERROR_CLOSING;
	}
	fclose(pageFile);
	free(fHandle);
	return RC_OK; 
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC destroyPageFile (char *fileName) {
	// Checks if fileName file exists. If it is NULL, the destroying fails
	pageFile = fopen(fileName, "r");
	if(pageFile == NULL) {
		return RC_ERROR_DELETING; 
	}
	remove(fileName);
	return RC_OK;
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
	// Opens file in read mode	
	pageFile = fopen(fHandle->fileName, "r");

	// It should not be NULL
	if(pageFile == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	// Moves the pointer to the required page (pageNumber * pageSize), from
	// from the beginning of the file (SEEK_SET)
	int seekCode = fseek(pageFile, pageNum * PAGE_SIZE, SEEK_SET);
	if (seekCode != 0) {
		if (errno == EBADF ) return RC_FILE_HANDLE_NOT_INIT;
		if (errno == EFBIG) return RC_READ_NON_EXISTING_PAGE;
		return RC_ERROR;
	}
	fHandle->curPagePos = pageNum * PAGE_SIZE;

	// Then just read the information into the memory pointed by memPage
	memPage = (char*)malloc(sizeof(char)*PAGE_SIZE);
	int readCode = fread(memPage, sizeof(char), PAGE_SIZE, pageFile);
	if (readCode != PAGE_SIZE) {
		return RC_READING_FAILED;
	}
	fHandle->curPagePos = (pageNum + 1)* PAGE_SIZE;
	fclose(pageFile);

	// if everything is ok, return RC_OK
    return RC_OK;
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern int getBlockPos (SM_FileHandle *fHandle) {
	// Returns the curPagePos from the fHandle struct
	return fHandle->curPagePos;
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// readFirstBlock is just a wrapper for readBlock, but reading the page 0	
	return readBlock(0, fHandle, memPage);
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// readPrevBlock is just a wrapper for readBlock, but reading the page curPageNum-1
	int curPageNum = fHandle->curPagePos / PAGE_SIZE;
	return readBlock(curPageNum - 1, fHandle, memPage);
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// readCurrentBlock is just a wrapper for readBlock, but reading the page curPageNum
	int curPageNum = fHandle->curPagePos / PAGE_SIZE;
	return readBlock(curPageNum, fHandle, memPage);
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	// readNextBlock is just a wrapper for readBlock, but reading the page curPageNum+1
	int curPageNum = fHandle->curPagePos / PAGE_SIZE;
	return readBlock(curPageNum + 1, fHandle, memPage);
}
// ANDRES (just for tracking which functions are already written's sake) (delete this before submitting HW)
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	// readLastBlock is just a wrapper for readBlock, but reading the page totalNumPages-1
	return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// Checking if the pageNumber parameter is less than Total number of pages and less than 0, then return respective error code
	if (pageNum > fHandle->totalNumPages || pageNum < 0)
        	return RC_WRITE_FAILED;
	
	// Opening file stream in read & write mode. 'r+' mode opens the file for both reading and writing.	
	pageFile = fopen(fHandle->fileName, "r+");
	
	// Checking if file was successfully opened.
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND;

	// Setting the cursor(pointer) position of the file stream. The seek is successfull if fseek() return 0
	int isSeekSuccess = fseek(pageFile, (pageNum * PAGE_SIZE), SEEK_SET);
	if(isSeekSuccess == 0) {
		/*int k;
		for(k = 0; k < PAGE_SIZE; k++) {
			// Check after each iteration, if file is ending.
			if(feof(pageFile)) {
				// If we reached end of file, append an empty block at the end of file.
				appendEmptyBlock(fHandle);
			}
			// Writing content from memPage to pageFile stream
			fputc(memPage[k], pageFile);
		}*/

		// Writing content from memPage to pageFile stream
		fwrite(memPage, sizeof(char), strlen(memPage), pageFile);

		// Setting the current page position to the cursor(pointer) position of the file stream
		fHandle->curPagePos = ftell(pageFile);
		

		// Closing file stream so that all the buffers are flushed.     	
		fclose(pageFile);
	} else {
		return RC_WRITE_FAILED;
	}	
	
	return RC_OK;
}

extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// Calculating current page number by dividing page size by current page position	
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
	
	// Incrementing total number of pages since we are adding this content to a new location as in current empty block.
	fHandle->totalNumPages++;
	return writeBlock(currentPageNumber, fHandle, memPage);
}


extern RC appendEmptyBlock (SM_FileHandle *fHandle) {

	RC returnStatus;

	// Creating an new empty page of size PAGE_SIZE bytes
	SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
	
	// Moving the cursor (pointer) position to the begining of the file stream.
	// if fseek() return 0, then seek is success 
	if( fseek(pageFile, 0, SEEK_END) == 0 ) {
		// Appending that empty page to the file
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, pageFile);
	} else {
		// If write operation failed by any reason, de-allocate the memory and return write failed status code
		free(emptyBlock);
		return RC_WRITE_FAILED;
	}
	
	// De-allocating the memory previously allocated to 'emptyPage'.
	// This is optional but always better to do for proper memory management.
	free(emptyBlock);
	
	// Incrementing the total number of pages since we added an empty black.
	fHandle->totalNumPages++;
	return RC_OK;
}

extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
	// Opening file stream in append mode. 'a' mode opens the file to append the data at the end of file.
	pageFile = fopen(fHandle->fileName, "a");
	
	if(pageFile == NULL)
		return RC_FILE_NOT_FOUND;
	
	// Checking if numberOfPages is greater than totalNumPages.
	// If that is the case, then add empty pages till numberofPages = totalNumPages
	while(numberOfPages > fHandle->totalNumPages)
		appendEmptyBlock(fHandle);
	
	// Closing file stream so that all the buffers are flushed. 
	fclose(pageFile);
	return RC_OK;
}
