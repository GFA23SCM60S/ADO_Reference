#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>

#include "storage_mgr.h"

FILE *pageFile;

extern void initStorageManager (void) {
	// Initialising file pointer i.e. storage manager.
	pageFile = NULL;
}

extern RC createPageFile (char *fileName){}

extern RC openPageFile (char *fileName, SM_FileHandle *fHandle){

	   
 // Open the file in read mode
    pageFile = fopen(fileName, "r+"); // can add a mode check here too*/

    // Open the file with the default application (Windows)(if needed)
/*char openCommand[200];
sprintf(openCommand, "start %s", filename); // Windows command to open a file
system(openCommand);*/


    // file open failure check
    if (pageFile == NULL) {
        //printf("Failed to open the file.\n");
        return RC_FILE_NOT_FOUND;
    } 

    //printf("File opened successfully.\n");
	return RC_OK; 
}

extern RC closePageFile (SM_FileHandle *fHandle) {}

extern RC destroyPageFile (char *fileName) {}
