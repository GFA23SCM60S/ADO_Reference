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

extern void initStorageManager (void) {
	pageFile = NULL;
}

RC createPageFile(char *fileName) {
    // Check if file already exists
    FILE *fExists = fopen(fileName, "r");
    if (fExists != NULL) {
        fclose(fExists);
        return RC_FILE_ALREADY_EXISTS;
    }

    // Create new file
    pageFile = fopen(fileName, "w");
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Allocate and initialize buffer
    SM_PageHandle buffer = (SM_PageHandle)malloc(PAGE_SIZE * sizeof(char));
    if (buffer == NULL) {
        fclose(pageFile);
        return RC_ERROR;
    }
    memset(buffer, 0, PAGE_SIZE);

    // Write buffer to file
    size_t bytesWritten = fwrite(buffer, sizeof(char), PAGE_SIZE, pageFile);
    free(buffer);
    fclose(pageFile);

    if (bytesWritten < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    // Verify file contents
    fExists = fopen(fileName, "r");
    if (fExists == NULL) {
        return RC_FILE_NOT_FOUND;
    }

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

    return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    // Open file
    pageFile = fopen(fileName, "r");
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Set file handle properties
    fHandle->fileName = fileName;
    fHandle->curPagePos = 0;

    // Get file size and calculate total pages
    struct stat fileInfo;
    if (fstat(fileno(pageFile), &fileInfo) < 0) {
        fclose(pageFile);
        return RC_ERROR;
    }
    fHandle->totalNumPages = fileInfo.st_size / PAGE_SIZE;

    fclose(pageFile);
    return RC_OK;
}


RC closePageFile (SM_FileHandle *fHandle) {
	// Checking if file pointer or the storage manager is intialised. If initialised, then close.
	if(pageFile != NULL)
		pageFile = NULL;
	return RC_OK;
}

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


RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Open file in read mode
    pageFile = fopen(fHandle->fileName, "r");
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Seek to the required page
    long offset = (long)pageNum * PAGE_SIZE;
    if (fseek(pageFile, offset, SEEK_SET) != 0) {
        fclose(pageFile);
        return (errno == EFBIG) ? RC_READ_NON_EXISTING_PAGE : RC_ERROR;
    }

    // Read the page content
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, pageFile);
    fclose(pageFile);

    if (bytesRead != PAGE_SIZE) {
        return RC_READING_FAILED;
    }

    fHandle->curPagePos = (pageNum + 1) * PAGE_SIZE;
    return RC_OK;
}


int getBlockPos(SM_FileHandle *fHandle) {
    return fHandle->curPagePos;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int curPageNum = fHandle->curPagePos / PAGE_SIZE;
    return readBlock(curPageNum - 1, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int curPageNum = fHandle->curPagePos / PAGE_SIZE;
    return readBlock(curPageNum, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int curPageNum = fHandle->curPagePos / PAGE_SIZE;
    return readBlock(curPageNum + 1, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Validate input parameters
    if (pageNum < 0 || fHandle == NULL || memPage == NULL) {
        return RC_WRITE_FAILED;
    }

    // Check if page number is within bounds
    if (pageNum > fHandle->totalNumPages) {
        return RC_WRITE_FAILED;
    }

    // Open file for reading and writing
    FILE *pageFile = fopen(fHandle->fileName, "r+");
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    long startPosition = pageNum * PAGE_SIZE;
    if (pageNum == 0) {
        // Writing data to the first page
        fseek(pageFile, startPosition, SEEK_SET);

        for (int i = 0; i < PAGE_SIZE; i++) {
            if (feof(pageFile)) {
                // Append an empty block if end of file is reached
                appendEmptyBlock(fHandle);
            }
            fputc(memPage[i], pageFile);
        }

        fHandle->curPagePos = ftell(pageFile);
    } else {
        // Writing data to non-first page
        fHandle->curPagePos = startPosition;
        fclose(pageFile);
        return writeCurrentBlock(fHandle, memPage);
    }

    fclose(pageFile);
    return RC_OK;
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || memPage == NULL) {
        return RC_WRITE_FAILED;
    }

    FILE *pageFile = fopen(fHandle->fileName, "r+");
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Append an empty block to make space for new content
    RC appendResult = appendEmptyBlock(fHandle);
    if (appendResult != RC_OK) {
        fclose(pageFile);
        return appendResult;
    }

    // Set file pointer to current page position
    fseek(pageFile, fHandle->curPagePos, SEEK_SET);

    // Write memPage contents to file
    size_t bytesWritten = fwrite(memPage, sizeof(char), strlen(memPage), pageFile);
    if (bytesWritten != strlen(memPage)) {
        fclose(pageFile);
        return RC_WRITE_FAILED;
    }

    // Update current page position
    fHandle->curPagePos = ftell(pageFile);

    fclose(pageFile);
    return RC_OK;
}

RC appendEmptyBlock(SM_FileHandle *fHandle) {
    // Create a new empty page of size PAGE_SIZE bytes
    SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    if (emptyBlock == NULL) {
        return RC_ERROR;
    }

    // Move the cursor to the end of the file
    if (fseek(pageFile, 0, SEEK_END) != 0) {
        free(emptyBlock);
        return RC_WRITE_FAILED;
    }

    // Append the empty page to the file
    size_t bytesWritten = fwrite(emptyBlock, sizeof(char), PAGE_SIZE, pageFile);
    free(emptyBlock);

    if (bytesWritten != PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    // Increment the total number of pages
    fHandle->totalNumPages++;
    return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Open file in append mode
    pageFile = fopen(fHandle->fileName, "a");
    if (pageFile == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Add empty pages until the desired capacity is reached
    while (numberOfPages > fHandle->totalNumPages) {
        RC result = appendEmptyBlock(fHandle);
        if (result != RC_OK) {
            fclose(pageFile);
            return result;
        }
    }

    // Close the file
     fclose(pageFile);
        return RC_OK;
}

    void freePh(SM_PageHandle fHandle) {
        if (fHandle != NULL) {
            free(fHandle);
        }
    }