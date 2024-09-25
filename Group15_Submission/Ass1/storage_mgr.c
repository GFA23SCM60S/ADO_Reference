#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<errno.h>
#include <sys/stat.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * manipulating page files *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void initStorageManager(void)
{
    printf("[init]: Storage Manager");
    return;
}

RC createPageFile(char *fileName)
{
    // open file
    FILE *file = fopen(fileName, "w+");
    if (!file)
    {
        return RC_FILE_NOT_FOUND;
    }

    // allocate page
    SM_PageHandle page = malloc(PAGE_SIZE);
    if (!page)
    {
        perror("[createPage]: malloc failed!!!");
        fclose(file);
        return RC_FILE_HANDLE_NOT_INIT; // memory allocation failure
    }

    // memset page to 0
    memset(page, 0, PAGE_SIZE);

    // write to the file
    size_t writeP = fwrite(page, sizeof(char), PAGE_SIZE, file);

    // free the pag and close the file
    free(page);
    fclose(file);

    if (writeP < PAGE_SIZE)
    {
        return RC_WRITE_FAILED;
    }

    return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
    // Open file in read and write mode
    FILE *fileStream = fopen(fileName, "r+");

    // Check if file opening was successful
    if (!fileStream)
    {
        return RC_FILE_NOT_FOUND;
    }

    // Set file handle properties
    fHandle->fileName = strdup(fileName);
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = fileStream;

    // Calculate total number of pages
    struct stat fileInfo;
    if (fstat(fileno(fileStream), &fileInfo) == 0)
    {
        fHandle->totalNumPages = (fileInfo.st_size + PAGE_SIZE - 1) / PAGE_SIZE;
    }
    else
    {
        fclose(fileStream);
        free(fHandle->fileName);
        return RC_FILE_HANDLE_NOT_INIT;
    }

    return RC_OK;
}

RC closePageFile(SM_FileHandle *fHandle)
{
    // Check if the file handle or management info is NULL
    if (fHandle == NULL || fHandle->mgmtInfo == NULL)
    {
        perror("[closePage]: File handle or file pointer is NULL!!!");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    // close the file
    int closeResult = fclose(filePtr);
    if (closeResult != 0)
    {
        // file close failed
        perror("[closePage]: close page file failed!!!");
        return RC_FILE_NOT_FOUND;
    }
    else
    {
        fHandle->mgmtInfo = NULL;
        return RC_OK;
    }
}

RC destroyPageFile(char *fileName)
{
    FILE *pageFile = fopen(fileName, "r");
    if (pageFile == NULL)
    {
        return RC_FILE_NOT_FOUND;
    }
    // close the file
    fclose(pageFile);

    // delete the file
    if (remove(fileName) != 0)
    {
        perror("[destroyPage]: File not found");
        return RC_FILE_NOT_FOUND;
    }

    return RC_OK;
}

/* Read a block from a page file */
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    printf("Enterning readBlock function\n");
    if (fHandle == NULL || memPage == NULL || fHandle->mgmtInfo == NULL) {
        fprintf(stderr, "Error: file handle, memmory page or management info is null\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        fprintf(stderr, "Error: Invalid page number\n");
        return RC_READ_NON_EXISTING_PAGE;
    }

    FILE *filePointer = (FILE *)fHandle->mgmtInfo;
    printf("Seeking to position in file\n");
    if (fseek(filePointer, pageNum * PAGE_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Error during fseek. errno: %d\n", errno);
        if (errno == EBADF) {
            fprintf(stderr, "Error: Bad file descriptor\n");
            return RC_FILE_HANDLE_NOT_INIT;
        }
        if (errno == EFBIG) {
            fprintf(stderr, "Error: File too large\n");
            return RC_READ_NON_EXISTING_PAGE;
        }
        fprintf(stderr, "Error: Unknown fseek error\n");
        return RC_ERROR;
    }

    printf("Reading bytes from file\n");
    size_t readBytes = fread(memPage, sizeof(char), PAGE_SIZE, filePointer);
    if (readBytes != PAGE_SIZE) {
        fprintf(stderr, "Error: Byte size not equal\n");
        return RC_READING_FAILED;
    }

    fHandle->curPagePos = pageNum;
    printf("Successfully read page. Current page position updated\n");
    return RC_OK;
}

/* Get the current block position */
int getBlockPos(SM_FileHandle *fHandle) {
    printf("Getting current block position\n");
    if (fHandle == NULL) {
        fprintf(stderr, "Error: File handle is NULL\n");
        return -1;
    }
    printf("Current block position: %d\n", fHandle->curPagePos);
    return fHandle->curPagePos;
}

/* Read the first block in a page file */
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    printf("Reading first block\n");
    return readBlock(0, fHandle, memPage);
}

/* Read the previous block in a page file */
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    printf("Reading previous block\n");
    if (fHandle == NULL) {
        fprintf(stderr, "Error: File handle is NULL\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->curPagePos <= 0) {
        fprintf(stderr, "Error: Already at the first page\n");
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

/* Read the current block in a page file */
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    printf("Reading current block\n");
    if (fHandle == NULL) {
        fprintf(stderr, "Error: File handle is NULL\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

/* Read the next block in a page file */
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    printf("Reading next block\n");
    if (fHandle == NULL) {
        fprintf(stderr, "Error: File handle is NULL\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->curPagePos >= fHandle->totalNumPages - 1) {
        fprintf(stderr, "Error: Already at the last page\n");
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

/* Read the last block in a page file */
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    printf("Reading last block\n");
    if (fHandle == NULL) {
        fprintf(stderr, "Error: File handle is NULL\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}


RC writeBlock(int pageNumber, SM_FileHandle *fileHandle, SM_PageHandle pageBuffer) {
    printf("Entering writeBlock function.\n");

    if (fileHandle == NULL || pageBuffer == NULL) {
        fprintf(stderr, "Error: Invalid inputs. File handle or page buffer is NULL.\n");
        return RC_WRITE_FAILED;
    }

    if (pageNumber < 0) {
        fprintf(stderr, "Error: Page number out of bounds. Page number is negative.\n");
        return RC_WRITE_FAILED;
    }

    if (pageNumber >= fileHandle->totalNumPages) {
        fprintf(stderr, "Error: Page number exceeds file limits. Page number: %d, Total pages: %d\n", pageNumber, fileHandle->totalNumPages);
        return RC_WRITE_FAILED;
    }

    printf("Seeking to page number %d in the file.\n", pageNumber);
    int seekResult = fseek(fileHandle->mgmtInfo, pageNumber * PAGE_SIZE, SEEK_SET);

    if (seekResult != 0) {
        fprintf(stderr, "Error: Seek operation failed.\n");
        return RC_WRITE_FAILED;
    }

    long filePos = ftell(fileHandle->mgmtInfo);
    if (filePos < 0) {
        fprintf(stderr, "Error: File position invalid after seeking.\n");
        return RC_WRITE_FAILED;
    }

    printf("Writing data to page number %d.\n", pageNumber);
    size_t bytesWritten = fwrite(pageBuffer, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo);

    if (bytesWritten != PAGE_SIZE) {
        fprintf(stderr, "Error: Write operation failed. Bytes written: %zu, Expected: %d\n", bytesWritten, PAGE_SIZE);
        return RC_WRITE_FAILED;
    }

    if (ferror(fileHandle->mgmtInfo)) {
        fprintf(stderr, "Error: File error after writing.\n");
        return RC_WRITE_FAILED;
    }

    printf("Successfully wrote block to page number %d.\n", pageNumber);
    return RC_OK;
}


RC writeCurrentBlock(SM_FileHandle *fileHandle, SM_PageHandle memoryPage) {

    if (fileHandle == NULL || memoryPage == NULL) {
        return RC_WRITE_FAILED;  // Return an error if inputs are invalid
    }

    int currentPageNumber = fileHandle->curPagePos / PAGE_SIZE;


    if (currentPageNumber < 0 || currentPageNumber >= fileHandle->totalNumPages) {
        return RC_WRITE_FAILED;  // Return an error for invalid page position
    }


    RC writeStatus = writeBlock(currentPageNumber, fileHandle, memoryPage);


    if (writeStatus != RC_OK) {
        // If the write failed, return the error code
        return RC_WRITE_FAILED;  // Return failure
    }

    fileHandle->curPagePos += PAGE_SIZE;

    return RC_OK;
}



RC appendEmptyBlock(SM_FileHandle *fileHandle) {

    SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));


    if (emptyPage == NULL) {
        return RC_WRITE_FAILED;  // Error: Memory allocation failed
    }

    int seekStatus = fseek(fileHandle->mgmtInfo, 0, SEEK_END);

    if (seekStatus != 0) {
        free(emptyPage);  // Clean up before returning
        return RC_WRITE_FAILED;  // Error: Seeking failed
    }

    size_t bytesWritten = fwrite(emptyPage, sizeof(char), PAGE_SIZE, fileHandle->mgmtInfo);

    free(emptyPage);  // Clean up the empty page buffer

    if (bytesWritten != PAGE_SIZE) {
        return RC_WRITE_FAILED;  // Error: Write operation failed
    }

    fileHandle->totalNumPages++;  // Increment the page count after a successful write

    return RC_OK;  // Success: Block appended
}


RC ensureCapacity(int numberOfPages, SM_FileHandle *fileHandle) {
    printf("Entering ensureCapacity function.\n");

    // Validate the file handle and internal management information
    if (fileHandle == NULL || fileHandle->mgmtInfo == NULL) {
        fprintf(stderr, "Error: File handle or management info is NULL.\n");
        return RC_FILE_HANDLE_NOT_INIT;  // Error: File handle not initialized
    }

    // Get the current number of pages in the file
    int currentPages = fileHandle->totalNumPages;
    printf("Current number of pages: %d\n", currentPages);

    // Compare the requested number of pages to the current page count
    if (numberOfPages > currentPages) {
        printf("Number of pages required: %d\n", numberOfPages);
        int pagesToAdd = numberOfPages - currentPages;
        printf("Pages to add: %d\n", pagesToAdd);

        // Loop through and append empty blocks for each page required
        for (int i = 0; i < pagesToAdd; i++) {
            printf("Appending empty block %d of %d.\n", i + 1, pagesToAdd);
            RC status = appendEmptyBlock(fileHandle);

            // If appending a block fails, return the error status
            if (status != RC_OK) {
                fprintf(stderr, "Error: Failed to append block %d.\n", i + 1);
                return status;  // Error: Failed to append a block
            }
        }
    } else {
        printf("No additional pages needed.\n");
    }

    // Return success if the required capacity has been ensured
    printf("ensureCapacity completed successfully.\n");
    return RC_OK;  // Success: File has enough pages
}
