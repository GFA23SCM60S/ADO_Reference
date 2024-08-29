#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>
#include <stdbool.h> 
#include "storage_mgr.h"

//Creating a file pointer 
FILE *strFilePointer;

void initStorageManager (void){
    printf("\nInitializing the File Pointer");
    strFilePointer = NULL;
}

RC createPageFile (char *strFileName){

    //Creating a file and opening in Write Mode.
    strFilePointer = fopen(strFileName,"w+");

    if(strFilePointer == NULL){
        return RC_FILE_NOT_FOUND;

    }else{
        printf("Initializing the Memory Block with Page Size of 4096 and size of each element to Char size (1byte)");
        SM_PageHandle *createBlankPage = (SM_PageHandle *)calloc(PAGE_SIZE, sizeof(char)); 
        printf("Setting the memory block to '\0' ");
        memset(createBlankPage, '\0', PAGE_SIZE);

        if(createBlankPage == NULL){
            return RC_CALLOC_FAILED;
        }
    	int createBlankPagePt = fwrite(createBlankPage, sizeof(char), PAGE_SIZE,strFilePointer);
    	fclose(strFilePointer);

    	if(createBlankPagePt >= PAGE_SIZE){
        	printf("\nFile created successfully\n\n");
    	}
        else{
            return RC_FILE_NOT_FOUND;
        }

	    free(createBlankPage);
    	printf("\nMemory Allocated is free and File is Closed\n");
    	return RC_OK;
    }
}

RC openPageFile(char *strFileName, SM_FileHandle *smFileHandler){
    
    //Open file in read mode
    strFilePointer = fopen(strFileName,"r+");   

    if(strFilePointer == NULL){
		//fclose(strFilePointer);
    	return RC_FILE_NOT_FOUND;
    }
    else{

        //Moving the File to SEEK_END ie. at the end of the file
        fseek(strFilePointer, 0, SEEK_END);
    	int tnumberOfPages = 0;
        int endPointer = ftell(strFilePointer);


        printf("\nThe last page in the file: %d\n",endPointer);
        endPointer = endPointer + 1 ;
	    tnumberOfPages = (endPointer / PAGE_SIZE);

        
        smFileHandler->totalNumPages = tnumberOfPages;
        smFileHandler->curPagePos = 0;
        smFileHandler->fileName = strFileName;

        //Saving Meta data???
        smFileHandler->mgmtInfo = strFilePointer;
        RC_message = 'File is Opened Successfully';
		//fclose(strFilePointer);
        return RC_OK;
    }
}

RC closePageFile(SM_FileHandle *smFileHandler) {  

    if(smFileHandler->mgmtInfo == NULL){       
        return RC_FILE_NOT_FOUND;
     
    }else{      
        strFilePointer = NULL;
        RC ind = fclose(smFileHandler->mgmtInfo);
        if(ind == 0 ){
            printf("\nFile Closed Successfully"); 
            return RC_OK; 
        }else{
            printf("\nError in Closing the file.");
            return RC_FILE_NOT_FOUND;
        }            
    }   
}


RC destroyPageFile (char *strFileName)
{	
    strFilePointer = fopen(strFileName, "r");
    if(fclose(strFilePointer) == 0 && unlink(strFileName) == 0 ){
        strFilePointer = NULL; 
        printf("\nFile Destroyed Successfully.\n");
        return RC_OK;
    }else{
        return RC_FILE_NOT_FOUND;
    }
}


RC readBlock (int filePageNum, SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler){

    int totalNumberOfPgs, startingPosition, seekStart, newPosition, curPosition;

    if(smFileHandler != NULL ){
         strFilePointer = fopen(smFileHandler->fileName, "r+") ;
    }else{ 
       return RC_FILE_HANDLE_NOT_INIT;
    }
	
	if(strFilePointer != NULL){
	   
        totalNumberOfPgs = smFileHandler->totalNumPages;
	    if(totalNumberOfPgs > filePageNum && filePageNum >= 0){
	    	    printf("\nThe page exists to read\n");
        }

	    startingPosition = (filePageNum *PAGE_SIZE);
	    seekStart = fseek(strFilePointer, filePageNum *PAGE_SIZE, SEEK_SET);

	    if(seekStart!=0){
	    	return RC_message="Pointer not Moving to start position" ;
	    }else{
             printf("\nThe pointer is moved to the starting position\n\n");
        }

	    fread(smMemPageHandler, sizeof(char), PAGE_SIZE, strFilePointer);
	    newPosition=ftell(strFilePointer);
	    curPosition = (newPosition % PAGE_SIZE)==0 ? (newPosition/PAGE_SIZE)-1 : (newPosition/PAGE_SIZE);
	    
	    smFileHandler->curPagePos = curPosition;
	   if(fclose(strFilePointer) != 0){
            printf("\nThe File is not closed properly\n\n");
                 
        }else{
            printf("\nSuccessfully File Closed\n\n");
            return RC_OK;
                 
        }
            fclose(strFilePointer);
	} else {
		printf("\nThe file doesnt exists\n\n");
            	return RC_FILE_NOT_FOUND;
    }
}


int getBlockPos (SM_FileHandle *smFileHandler) 
{   
   printf("\n Checking if filehandler object is NULL ");
   if (smFileHandler == NULL){  
        return 0;
   }
   else{
    int ind  = ((fopen(smFileHandler->fileName, "r")) == NULL) ? 0 : 1;  //Checking if File exits 
    if(ind == 0 ){
        return RC_FILE_NOT_FOUND;
    }
    else{
        return smFileHandler->curPagePos; 
    }
   }
}


RC readFirstBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler)
{
    int firstBlockPos = 0 ; 
    if (smFileHandler != NULL){
        printf("File handler is not NULL and thus calling readBlock()");
        return readBlock(firstBlockPos, smFileHandler, smMemPageHandler);
    }else{
        return RC_FILE_HANDLE_NOT_INIT;
    }
}

RC readPreviousBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler)
{
    if(smFileHandler == NULL ){
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int currentPgPtrPos = getBlockPos(smFileHandler);
    int previousPgPtrPos = currentPgPtrPos - 1;
    printf("The Pervious block position is : %d" + previousPgPtrPos);
    return readBlock(previousPgPtrPos, smFileHandler, smMemPageHandler);
}


RC readCurrentBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler)
{
    if(smFileHandler != NULL){
        printf("Getting the current block position");
        int currentPgPtrPos  = getBlockPos(smFileHandler);
        return readBlock(currentPgPtrPos, smFileHandler, smMemPageHandler);
    }
    return RC_FILE_HANDLE_NOT_INIT;
}


RC readNextBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler)
{
    if(smFileHandler == NULL){
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int currentPgPtrPos = getBlockPos(smFileHandler);
    int nextPgPtrPos =currentPgPtrPos + 1;
    printf("The next block positon of the file is : %d" + nextPgPtrPos);
    return readBlock( nextPgPtrPos, smFileHandler, smMemPageHandler);
}


RC readLastBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler)
{
    int lastPgPtrPos; 
    if(smFileHandler != NULL){
        lastPgPtrPos = smFileHandler->totalNumPages -1; 
        printf("The last block of the file is : %d" + lastPgPtrPos);
        return readBlock(lastPgPtrPos, smFileHandler, smMemPageHandler);
    }
    return RC_FILE_HANDLE_NOT_INIT;
}


RC writeBlock(int filePageNum, SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler)
{
    int totalNumberOfPgs, seekPointer; 
    
    if(strFilePointer != NULL) {
        if(smFileHandler != NULL && smFileHandler->mgmtInfo != NULL){
            seekPointer = fseek(smFileHandler->mgmtInfo, filePageNum * PAGE_SIZE * sizeof(char), SEEK_SET);

            if(seekPointer == 0 ){
                printf("After all check points of FileHandler, seekPointer we are writing into the file");
                size_t totalNumWritten = fwrite(smMemPageHandler, sizeof(char), PAGE_SIZE, smFileHandler->mgmtInfo);
                if (totalNumWritten == PAGE_SIZE){
                  
                    smFileHandler->curPagePos = filePageNum;  // Successful Write Function
                    return RC_OK;                 
                }
                else {
                    return RC_WRITE_FAILED;   
                }       
            }else{
                return RC_WRITE_FAILED; 
            }
        }
        else{
            return RC_FILE_HANDLE_NOT_INIT;
        }
    }else{
        return RC_FILE_NOT_FOUND;
    }
    
}


RC writeCurrentBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler)
{
    if(smFileHandler == NULL){
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int currentPgPtrPos = smFileHandler->curPagePos;
    printf("Wirting from the current position %d : " + currentPgPtrPos);
    return writeBlock(currentPgPtrPos, smFileHandler, smMemPageHandler);
    
}

RC appendEmptyBlock(SM_FileHandle *smFileHandler) {
    if (smFileHandler == NULL) {
        puts("\n\n Pointer is Null \n\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if (smFileHandler->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    char *emptyPage = (char *)calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL) {
        puts("\n\n Empty page \n\n");
        return RC_CALLOC_FAILED;
    }

    int setPosition = fseek(smFileHandler->mgmtInfo, 0, SEEK_END);
    if (setPosition != 0) {
        free(emptyPage);
        return RC_WRITE_FAILED;
    }

    if (fwrite(emptyPage, PAGE_SIZE, 1, smFileHandler->mgmtInfo) != 1) {
        free(emptyPage);
        return RC_WRITE_FAILED;
    }

    puts("\nSuccessfully appended an empty block.\n\n");

    smFileHandler->totalNumPages++;
    smFileHandler->curPagePos++;
    free(emptyPage);

    return RC_OK;
}


RC ensureCapacity(int pageCount, SM_FileHandle *smFileHandler) {
    int count = smFileHandler->totalNumPages;
    strFilePointer = fopen(smFileHandler->fileName, "r");
	puts("\n\n File opened in read mode \n\n");
    if (strFilePointer != NULL) {
        if (pageCount <= count) {
            fclose(strFilePointer);
            return RC_OK;
        } else {
            while (pageCount > count) {
                appendEmptyBlock(smFileHandler);
                count = smFileHandler->totalNumPages;
            }
        }
    } else {
		puts("\n\n File Not Found \n\n");
        return RC_FILE_NOT_FOUND;
    }

    fclose(strFilePointer);
    return RC_OK;
}
