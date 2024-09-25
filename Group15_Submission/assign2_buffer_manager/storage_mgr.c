#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Global file pointer
FILE *pageFile;

void initStorageManager(void) {
    printf("[init]: Storage Manager\n");
    pageFile = NULL;
}

RC createPageFile(char *fileName) {
    // Open file in write mode
    FILE *file = fopen(fileName, "w+");
    if (file == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // Allocate and initialize an empty page
    SM_PageHandle page = malloc(PAGE_SIZE);
    if (!page) {
        fclose(file);
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    memset(page, 0, PAGE_SIZE);

    // Write the empty page to the file
    size_t writeP = fwrite(page, sizeof(char), PAGE_SIZE, file);
    free(page);
    fclose(file);

    if (writeP < PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    // Open file in read and write mode
    FILE *fileStream = fopen(fileName, "r+");
    if (!fileStream) {
        return RC_FILE_NOT_FOUND;
    }

    // Set file handle properties
    fHandle->fileName = strdup(fileName);
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = fileStream;

    // Calculate total number of pages
    struct stat fileInfo;
    if (fstat(fileno(fileStream), &fileInfo) == 0) {
        fHandle->totalNumPages = (fileInfo.st_size + PAGE_SIZE - 1) / PAGE_SIZE;
    } else {
        fclose(fileStream);
        free(fHandle->fileName);
        return RC_FILE_HANDLE_NOT_INIT;
    }

    return RC_OK;
}

RC closePageFile(SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        perror("[closePage]: File handle or file pointer is NULL!!!");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    FILE *filePtr = (FILE *)fHandle->mgmtInfo;
    int closeResult = fclose(filePtr);
    if (closeResult != 0) {
        perror("[closePage]: close page file failed!!!");
        return RC_FILE_NOT_FOUND;
    }

    // HIGHLIGHT: Free the fileName memory
    free(fHandle->fileName);
    fHandle->mgmtInfo = NULL;
    return RC_OK;
}

RC destroyPageFile(char *fileName) {
    if (remove(fileName) != 0) {
        return RC_FILE_NOT_FOUND;
    }
    return RC_OK;
}

// HIGHLIGHT: Updated readBlock function
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || memPage == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    FILE *filePointer = (FILE *)fHandle->mgmtInfo;
    if (fseek(filePointer, pageNum * PAGE_SIZE, SEEK_SET) != 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    size_t readBytes = fread(memPage, sizeof(char), PAGE_SIZE, filePointer);
    if (readBytes != PAGE_SIZE) {
        return RC_READING_FAILED;
    }

    fHandle->curPagePos = pageNum;
    return RC_OK;
}

int getBlockPos(SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        return -1;
    }
    return fHandle->curPagePos;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->curPagePos <= 0) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->curPagePos >= fHandle->totalNumPages - 1) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

// HIGHLIGHT: Updated writeBlock function
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || memPage == NULL) {
        return RC_WRITE_FAILED;
    }

    if (pageNum < 0) {
        return RC_WRITE_FAILED;
    }

    FILE *file = fHandle->mgmtInfo;
    long offset = (pageNum) * PAGE_SIZE;

    if (fseek(file, offset, SEEK_SET) != 0) {
        return RC_WRITE_FAILED;
    }

    size_t bytesWritten = fwrite(memPage, sizeof(char), PAGE_SIZE, file);
    if (bytesWritten != PAGE_SIZE) {
        return RC_WRITE_FAILED;
    }

    fHandle->curPagePos = pageNum;
    if (pageNum >= fHandle->totalNumPages) {
        fHandle->totalNumPages = pageNum + 1;
    }

    return RC_OK;
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || memPage == NULL) {
        return RC_WRITE_FAILED;
    }
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

// HIGHLIGHT: Updated appendEmptyBlock function
RC appendEmptyBlock(SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    if (emptyBlock == NULL) {
        return RC_WRITE_FAILED;
    }

    RC status = writeBlock(fHandle->totalNumPages, fHandle, emptyBlock);
    free(emptyBlock);

    if (status != RC_OK) {
        return status;
    }

    fHandle->totalNumPages++;
    return RC_OK;
}

// HIGHLIGHT: Updated ensureCapacity function
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if (numberOfPages <= fHandle->totalNumPages) {
        return RC_OK;
    }

    int pagesToAdd = numberOfPages - fHandle->totalNumPages;
    for (int i = 0; i < pagesToAdd; i++) {
        RC status = appendEmptyBlock(fHandle);
        if (status != RC_OK) {
            return status;
        }
    }

    return RC_OK;
}