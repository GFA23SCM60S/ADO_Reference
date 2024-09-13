
#include <stdio.h>
#include <stdlib.h>
#include "dberror.h"
#include "create.h"

/**
 * Assignment: Create a new page file fileName. The initial file size should be one
  page. This method should fill this single page with ’\0’ bytes.
 */
RC createPageFile (char* fileName){ 

	int errno = 0;
	FILE* fp = fopen(fileName,"wb"); // wb mode opens an empty file for writting in binary mode.
	if(fp == NULL){
		if (errno == ENOENT) return RC_FILE_NOT_FOUND;
		return RC_FILE_HANDLE_NOT_INIT; // If no error code, just return generic error
	    }
	
	char c = '\0';
	int nc = fwrite(&c, sizeof(char), 4096, fp); // write '\0' 4096 times
	if (nc != 4096) {
		// If the number of characters is not 4096, something happened. Check errors
		fclose();
		return RC_WRITE_FAILED;
	}
	fclose(fp);
	return RC_OK;
}
