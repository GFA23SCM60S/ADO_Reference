#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>
#include<errno.h>
#include "storage_mgr.h"

FILE *pageFile;


/*
	- description : Initialize the storage manager program
	- param : void
	- return : void
*/
void initStorageManager (void) {
	// Initialising file pointer i.e. storage manager.
	pageFile = NULL;
}


/*
	- description : Create a new page file with size of one page intially filled with '\0' bytes. 
	- param : 
		1. fileName - name of the file
	- return : RC code
*/
RC createPageFile (char *fileName) {
	
	// Opening file stream in read mode. 
	FILE *fExists = fopen(fileName, "r");

	// It should be NULL
	// First check if it exists
	if (fExists != NULL) {
		fclose(fExists);
		return RC_FILE_ALREADY_EXISTS;
	}
	
	// Opening in w mode opens an empty file for writting in text mode.
	pageFile = fopen(fileName, "w");

	if(pageFile == NULL) {
		// ENOENT is error code for fopen when not finding the file
		if (errno == ENOENT) return RC_FILE_NOT_FOUND;
		// If no error code, just return generic errorreturn RC_FILE_NOT_FOUND 
		return RC_FILE_NOT_FOUND; 
	}
	// Allocates memory to a buffer and fills it with PAGE_SIZE null characters
	SM_PageHandle buffer = (SM_PageHandle)malloc(PAGE_SIZE * sizeof(char));
	for (int i=0; i<PAGE_SIZE; i++) {
		buffer[i] = '\0';
	}

	// write the buffer to the file ('\0' 4096 times)
	int nc = fwrite(buffer, sizeof(char), PAGE_SIZE, pageFile); 

	// Checks if it wrote the full PAGE_SIZE characters
	if(nc < PAGE_SIZE) {
		fclose(pageFile);
		return RC_WRITE_FAILED;
	}
	
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

/*
	- description : Open a page file and initialize the page file fields with the updated information. 
	- param : 
		1. filename -  name of the file
		2. fHandle -  File handler which represents the open page file
	- return : RC code
*/
RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
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
		*/
		
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


/*
	- description : Closes an open page file. 
	- param : 
		1. fHandle -  File handler which represents the open page file
	- return : RC code
*/
RC closePageFile (SM_FileHandle *fHandle) {
	// Checking if file pointer or the storage manager is intialised. If initialised, then close.
	if(pageFile != NULL)
		pageFile = NULL;	
	return RC_OK; 
}


/*
	- description : Destroys or deletes a page file 
	- param : 
		1. fileName -  name of the file
	- return : RC code
*/
RC destroyPageFile (char *fileName) {
	// Checks if fileName file exists. If it is NULL, the destroying fails
	pageFile = fopen(fileName, "r");
	if(pageFile == NULL) {
		return RC_ERROR_DELETING; 
	}
	fclose(pageFile);
	if(access(fileName, F_OK) == 0){
		if (remove(fileName) != 0) {
			return RC_ERROR_DELETING;
		}
	}
	return RC_OK;
}

/*
	- description : Reads the block at specific position from a file and stores its content in the memory pointed to the page handle.
	- param :
		1. pageNum -  page number of the file
		2. fHandle -  file handler which represents the open page file
		3. memPage -  pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
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
	//memPage = (char*)malloc(sizeof(char)*PAGE_SIZE);
	int readCode = fread(memPage, sizeof(char), PAGE_SIZE, pageFile);
	if (readCode != PAGE_SIZE) {
		return RC_READING_FAILED;
	}
	fHandle->curPagePos = (pageNum + 1)* PAGE_SIZE;
	fclose(pageFile);

	// if everything is ok, return RC_OK
    return RC_OK;
}


/*
    - description : Returns the current page position of a file
	- param : 
		1. fHandle -  file handler which represents the open page file
	- return : current page position
*/
int getBlockPos (SM_FileHandle *fHandle) {
	// Returns the curPagePos from the fHandle struct
	return fHandle->curPagePos;
}


/*
    - description : Reads the first page/block in the file
	- param : 
		1. fHandle - file handler which represents the open page file
		2. memPage - pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// readFirstBlock is just a wrapper for readBlock, but reading the page 0	
	return readBlock(0, fHandle, memPage);
}

/*
    - description : Reads the previous page/block relative to the current Page Position in the file
	- param : 
		1. fHandle - file handler which represents the open page file
		2. memPage - pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// readPrevBlock is just a wrapper for readBlock, but reading the page curPageNum-1
	int curPageNum = fHandle->curPagePos / PAGE_SIZE;
	return readBlock(curPageNum - 1, fHandle, memPage);
}

/*
    - description : Reads the current page/block in the file
	- param : 
		1. fHandle - file handler which represents the open page file
		2. memPage - pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// readCurrentBlock is just a wrapper for readBlock, but reading the page curPageNum
	int curPageNum = fHandle->curPagePos / PAGE_SIZE;
	return readBlock(curPageNum, fHandle, memPage);
}

/*
    - description : Reads the next page/block relative to the current Page Position in the file
	- param : 
		1. fHandle - file handler which represents the open page file
		2. memPage - pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	// readNextBlock is just a wrapper for readBlock, but reading the page curPageNum+1
	int curPageNum = fHandle->curPagePos / PAGE_SIZE;
	return readBlock(curPageNum + 1, fHandle, memPage);
}

/*
    - description : Reads the last page/block in the file
	- param : 
		1. fHandle - file handler which represents the open page file
		2. memPage - pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	// readLastBlock is just a wrapper for readBlock, but reading the page totalNumPages-1
	return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}


/*
    - description : Writes a page to disk sing the absolute position
	- param : 
		1. pageNum - page number of the file
		2. fHandle - file handler which represents the open page file
		3. memPage - pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

	// Throw error if condition is not satisfied
	int write_failed = RC_WRITE_FAILED;
	int file_not_found = RC_FILE_NOT_FOUND;
	int success = RC_OK;

	if(pageNum < 0) return write_failed;
	if(pageNum > fHandle->totalNumPages) return write_failed;

	pageFile = fopen(fHandle->fileName, "r+");
	
	// Throw error if pageFile is NULL
	if(pageFile == NULL) return file_not_found;
	else{
		if(fseek(pageFile, (PAGE_SIZE * pageNum), SEEK_SET) == 0) {
			// Writing the contents of memPage to pageFile with length of memPage length
			fwrite(memPage, sizeof(char), PAGE_SIZE, pageFile);
			
			// The current page position is assigned with bytes of pageFile
			fHandle->curPagePos = ftell(pageFile);

			// Closing the pageFile
			fclose(pageFile);
		}else{
			return write_failed;
		}
	}

	// If no error is thrown then return success code
	return success;
}

/*
    - description : Writes a page to disk using the current position
	- param : 
		1. fHandle - file handler which represents the open page file
		2. memPage - pointer to a memory location where a page's data are stored
	- return : RC code
*/
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	// Current page number 
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;

	// increase total number of pages by 1
	fHandle->totalNumPages += 1;

	// return the Write Block with current page number
	return writeBlock(currentPageNumber, fHandle, memPage);
}

/*
    - description : Increase the number of pages in the file by one with '\0' bytes in the new page
	- param : 
		1. fHandle - file handler which represents the open page file
	- return : RC code
*/
RC appendEmptyBlock (SM_FileHandle *fHandle) {

	// Creating an new empty page of size PAGE_SIZE bytes
	SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
	
	// Moving the cursor (pointer) position to the begining of the file stream.
	// if fseek() return 0, then seek is success 
	if( fseek(pageFile, 0, SEEK_END) == 0 ) {
		// Appending that empty page to the file
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, pageFile);
	} else {
		// If write operation failed by any reason, de-allocate the memory and return write failed status code
		return RC_WRITE_FAILED;
	}
	
	// De-allocating the memory previously allocated to 'emptyPage'.
	// This is optional but always better to do for proper memory management.
	free(emptyBlock);
	
	// Incrementing the total number of pages since we added an empty black.
	fHandle->totalNumPages++;
	return RC_OK;
}

/*
	- description : Make sure if the page-file capacity is equal to given number of pages
	- param :
		1. numberOfPages - number of pages required
		2. fHandle - file handler representing an open page file
	- return : RC code
*/
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {

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

/*
	- description : free the additional memory allocated 
	- param :
		1. fHandle - file handler representing an open page file
	- return : void
*/
void freePh(SM_PageHandle fHandle){
	free(fHandle);
}
