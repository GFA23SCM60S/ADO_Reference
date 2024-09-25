#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initStorageManager(void) {
    printf("Storage Manager initialized.\n");
}
RC createPageFile(char *fileName) {
    FILE *file = fopen(fileName, "w+");
    if (file == NULL) {
        return RC_FILE_NOT_FOUND; 
    }

    SM_PageHandle emptyPage = calloc(PAGE_SIZE, 1);
    if (emptyPage == NULL) {
        fclose(file);
        return RC_WRITE_FAILED;
    }

    size_t writeSize = fwrite(emptyPage, sizeof(char), PAGE_SIZE, file);
    free(emptyPage);
    fclose(file);

    if (writeSize < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }
    return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fileHandle) {
    fileHandle->mgmtInfo = fopen(fileName, "r+");
    if (fileHandle->mgmtInfo == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    fileHandle->fileName = strdup(fileName);
    fileHandle->curPagePos = 0;
    
    fseek(fileHandle->mgmtInfo, 0, SEEK_END);
    long fileSize = ftell(fileHandle->mgmtInfo);
    fileHandle->totalNumPages = fileSize / PAGE_SIZE;
    fseek(fileHandle->mgmtInfo, 0, SEEK_SET);

    return RC_OK;
}


RC closePageFile(SM_FileHandle *fileHandle) {
    if (fileHandle == NULL || fileHandle->mgmtInfo == NULL) {
        printf("Error: File handle or file pointer is NULL\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if (fclose(fileHandle->mgmtInfo) == 0) {
        printf("File closed successfully.\n");
        fileHandle->mgmtInfo = NULL; 
        return RC_OK;
    } else {
        perror("Error closing file");
        return RC_FILE_NOT_FOUND;
    }
}


RC destroyPageFile(char *fileName) {
    if (remove(fileName) != 0) {
        return RC_FILE_NOT_FOUND; 
    }
    return RC_OK;
}

/* Read a block from a page file */
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Check if the file handle and memory page are valid

    if (fHandle == NULL || memPage == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    // Check if the page number is within range
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Seek to the page position
    fseek(fHandle->mgmtInfo, pageNum * PAGE_SIZE, SEEK_SET);
    // Read the page into the memory page buffer
    size_t bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
    // Check if the read was successful
    if (bytesRead < PAGE_SIZE) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Update the file handle's current page position
    fHandle->curPagePos = pageNum;

    return RC_OK;
}

/* Get the current block position */
int getBlockPos(SM_FileHandle *fHandle) {
    return fHandle->curPagePos;
}

/* Read the first block in a page file */
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

/* Read the previous block in a page file */
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int currentPos = getBlockPos(fHandle);
    if (currentPos <= 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(currentPos - 1, fHandle, memPage);
}

/* Read the current block in a page file */
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int currentPos = getBlockPos(fHandle);
    return readBlock(currentPos, fHandle, memPage);
}

/* Read the next block in a page file */
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    int currentPos = getBlockPos(fHandle);
    if (currentPos >= fHandle->totalNumPages - 1) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(currentPos + 1, fHandle, memPage);
}

/* Read the last block in a page file */
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

//Function: writeBlock
/* 
 * Function: writeBlock
 * Description: Writes a block of data to a specific page in a file.
 * Pseudocode:
 * 1. Validate the input parameters (file handle, page buffer, page number).
 * 2. Verify if the page number is within valid range.
 * 3. Move the file pointer to the appropriate location based on the page number.
 * 4. Check the success of the seek operation.
 * 5. Confirm the file pointer is at a valid position.
 * 6. Write the data to the file.
 * 7. Verify if the data was written successfully.
 * 8. Perform additional checks to ensure the file remains in a valid state.
 * 9. Return success if no issues are encountered.
 */
RC writeBlock(int pageNumber, SM_FileHandle *fileHandle, SM_PageHandle pageBuffer) {
    // Step 1: Validate input
    // Ensure the file handle and page buffer are not NULL
    if (fileHandle == NULL || pageBuffer == NULL) {
        return RC_WRITE_FAILED;  // Error: Invalid inputs
    }

    // Step 2: Validate page number
    // If page number is negative, it's invalid
    if (pageNumber < 0) {
        return RC_WRITE_FAILED;  // Error: Page number out of bounds
    }

    // Step 3: Check if page number exceeds total pages
    // Prevent writing beyond the file's current size
    if (pageNumber >= fileHandle->totalNumPages) {
        return RC_WRITE_FAILED;  // Error: Page number exceeds file limits
    }

    // Step 4: Move file pointer to the required page
    int seekResult = fseek(fileHandle->mgmtInfo, pageNumber * PAGE_SIZE, SEEK_SET);

    // Step 5: Validate seek result
    // If fseek failed, return an error
    if (seekResult != 0) {
        if (fileHandle->mgmtInfo == NULL) {
            return RC_WRITE_FAILED;  // Error: Invalid file pointer
        }
        if (pageNumber > 0) {
            return RC_WRITE_FAILED;  // Error: Seek operation failed
        }
    }

    // Step 6: Verify file pointer position
    // Ensure the file pointer is correctly positioned
    long filePos = ftell(fileHandle->mgmtInfo);
    if (filePos < 0) {
        return RC_WRITE_FAILED;  // Error: File position invalid
    }

    // Step 7: Write data to the file
    size_t bytesWritten = fwrite(pageBuffer, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo);

    // Step 8: Verify write operation
    if (bytesWritten != PAGE_SIZE) {
        if (bytesWritten == 0) {
            return RC_WRITE_FAILED;  // Error: No bytes written
        }
        return RC_WRITE_FAILED;  // Error: Partial write
    }

    // Step 9: Check file state after writing
    if (ferror(fileHandle->mgmtInfo)) {
        return RC_WRITE_FAILED;  // Error: File error after writing
    }

    // Step 10: Return success if all checks passed
    return RC_OK;
}

//Function: writeCurrentBlock
/* 
 * Function: writeCurrentBlock
 * Description: Write data to the current block of a page file. 
 * Pseudocode:
 * 1. Validate the input parameters (file handle and memory page).
 * 2. Determine the current page number based on the file's current position.
 * 3. Check if the current page number is within the valid range.
 * 4. Call the writeBlock function to write data to the file.
 * 5. If successful, return success status, otherwise handle failure.
 */
RC writeCurrentBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage) {
    // Step 1: Validate the input parameters
    // Check if the file handle or memory page is NULL
    if (fileHandle == NULL || memoryPage == NULL) {
        return RC_WRITE_FAILED;  // Return an error if inputs are invalid
    }

    // Step 2: Calculate the current page number
    int currentPageNumber = fileHandle->curPagePos / PAGE_SIZE;

    // Step 3: Ensure the current page number is within valid range
    // If the page number is negative or beyond total pages, it's out of range
    if (currentPageNumber < 0 || currentPageNumber >= fileHandle->totalNumPages) {
        return RC_WRITE_FAILED;  // Return an error for invalid page position
    }

    // Step 4: Attempt to write the data to the current block
    // Call the writeBlock function and pass the calculated current page number
    RC writeStatus = writeBlock(currentPageNumber, fileHandle, memoryPage);

    // Step 5: Handle the result of the write operation
    if (writeStatus != RC_OK) {
        // If the write failed, return the error code
        return RC_WRITE_FAILED;  // Return failure
    }

    // Step 6: Update the current position after a successful write
    fileHandle->curPagePos += PAGE_SIZE;

    // Step 7: Return success status if everything went fine
    return RC_OK;
}


//Function: appendEmptyBlock
/*
 * Function: appendEmptyBlock
 * Description: Adds an empty block (page) to the end of a file.
 * Pseudocode:
 * 1. Allocate a block of memory to represent an empty page.
 * 2. Ensure the memory allocation was successful.
 * 3. Move the file pointer to the end of the file.
 * 4. Write the empty page buffer to the file.
 * 5. Free the allocated memory to avoid memory leaks.
 * 6. Check if the write operation completed successfully.
 * 7. Update the file handle's total page count.
 * 8. Return success or failure based on the outcome.
 */
RC appendEmptyBlock(SM_FileHandle *fileHandle) {
    // Step 1: Allocate memory for an empty page (block)
    // Initialize an empty page with the size of one block (PAGE_SIZE)
    SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));

    // Step 2: Validate memory allocation
    // Check if memory allocation failed (emptyPage is NULL)
    if (emptyPage == NULL) {
        return RC_WRITE_FAILED;  // Error: Memory allocation failed
    }

    // Step 3: Move the file pointer to the end
    // Use fseek to position the pointer at the end of the file
    int seekStatus = fseek(fileHandle->mgmtInfo, 0, SEEK_END);

    // Step 4: Validate the seek operation
    if (seekStatus != 0) {
        free(emptyPage);  // Clean up before returning
        return RC_WRITE_FAILED;  // Error: Seeking failed
    }

    // Step 5: Write the empty page to the file
    // Write the empty block to the file from the buffer
    size_t bytesWritten = fwrite(emptyPage, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo);

    // Step 6: Release allocated memory to avoid memory leaks
    free(emptyPage);  // Clean up the empty page buffer

    // Step 7: Validate the write operation
    // Check if the number of bytes written is less than the size of one page
    if (bytesWritten != PAGE_SIZE) {
        return RC_WRITE_FAILED;  // Error: Write operation failed
    }

    // Step 8: Update the file's total number of pages
    fileHandle->totalNumPages++;  // Increment the page count after a successful write

    // Step 9: Return success status
    return RC_OK;  // Success: Block appended
}


//Function: ensureCapacity
/*
 * Function: ensureCapacity
 * Description: Ensures that the file has at least the specified number of pages.
 * Pseudocode:
 * 1. Check if the file handle and its internal management info are valid.
 * 2. Retrieve the current total number of pages in the file.
 * 3. Compare the requested number of pages to the current number of pages.
 * 4. If more pages are required, calculate how many extra pages are needed.
 * 5. Loop to append empty blocks for the required number of pages.
 * 6. If appending blocks fails, return an error.
 * 7. Return success if the file now has the required capacity.
 */
RC ensureCapacity(int numberOfPages, SM_FileHandle *fileHandle) {
    // Step 1: Validate the file handle and internal management information
    // If the file handle or file management information is NULL, return an error
    if (fileHandle == NULL || fileHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;  // Error: File handle not initialized
    }

    // Step 2: Get the current number of pages in the file
    int currentPages = fileHandle->totalNumPages;

    // Step 3: Compare the requested number of pages to the current page count
    // If the desired number of pages is greater than the current number of pages
    if (numberOfPages > currentPages) {
        // Step 4: Calculate the number of additional pages required
        int pagesToAdd = numberOfPages - currentPages;

        // Step 5: Loop through and append empty blocks for each page required
        for (int i = 0; i < pagesToAdd; i++) {
            // Attempt to append an empty block
            RC status = appendEmptyBlock(fileHandle);

            // Step 6: If appending a block fails, return the error status
            if (status != RC_OK) {
                return status;  // Error: Failed to append a block
            }
        }
    }

    // Step 7: Return success if the required capacity has been ensured
    return RC_OK;  // Success: File has enough pages
}
