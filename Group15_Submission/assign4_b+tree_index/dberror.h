#ifndef DBERROR_H
#define DBERROR_H

#include <stdlib.h>
#include "stdio.h"
#include <errno.h>

/* return code definitions */
typedef int RC;

#define RC_OK 0
#define RC_FILE_NOT_FOUND 1
#define RC_FILE_HANDLE_NOT_INIT 2
#define RC_WRITE_FAILED 3
#define RC_READ_NON_EXISTING_PAGE 4
#define RC_BLOCK_POSITION_ERROR 5
#define RC_FS_ERROR 6
#define RC_FILE_ALREADY_EXISTS 7
#define RC_MEM_ALLOCATION_ERROR 12

#define RC_ERROR 400 // Added a new definiton for ERROR
#define RC_PINNED_PAGES_IN_BUFFER 500 // Added a new definition for Buffer Manager

#define RC_READING_FAILED 401 //
#define RC_ERROR_CLOSING 402 //
#define RC_ERROR_DELETING 403 //

#define RC_NO_SPACE_IN_POOL 100
#define RC_STRATEGY_NOT_SUPPORTED 101
#define RC_ERROR_NO_PAGE 102
#define RC_ERROR_NOT_FREE_FRAME 103

#define RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE 200
#define RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN 201
#define RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN 202
#define RC_RM_NO_MORE_TUPLES 203
#define RC_RM_NO_PRINT_FOR_DATATYPE 204
#define RC_RM_UNKOWN_DATATYPE 205
#define RC_RM_LIMIT_EXCEEDED 210
#define RC_RM_NO_SUCH_TUPLE 211

#define RC_IM_KEY_NOT_FOUND 300
#define RC_IM_KEY_ALREADY_EXISTS 301
#define RC_IM_N_TO_LAGE 302
#define RC_IM_NO_MORE_ENTRIES 303

#define RC_GENERAL_ERROR 500

// Added new definitions for Record Manager
#define RC_RM_NO_TUPLE_WITH_GIVEN_RID 600
#define RC_SCAN_CONDITION_NOT_FOUND 601

#define RC_BT_NODE_NOT_FOUND 700
#define RC_BT_INVALID_TREE_STRUCTURE 701
#define RC_BT_DUPLICATE_KEY 702
#define RC_BT_TREE_FULL 703
#define RC_BT_INVALID_KEY_TYPE 704
#define RC_BT_OPERATION_NOT_SUPPORTED 705
#define RC_INVALID_PARAMETER 706

#define SA_ELEMENT_NOT_FOUND -1

/* holder for error messages */
extern char *RC_message;

/* print a message to standard out describing the error */
extern void printError (RC error);
extern char *errorMessage (RC error);


#define newArray(type, size) (type *) malloc(sizeof(type) * size)
#define newCleanArray(type, size) (type *) calloc(size, sizeof(type))
#define new(type) newArray(type, 1)
#define newStr(size) newCleanArray(char, size + 1) // +1 for \0 terminator
#define newIntArr(size) newArray(int, size)
#define newFloatArr(size) newArray(float, size)
#define newCharArr(size) newArray(char, size)


#define THROW(rc,message) \
  do {			  \
    RC_message=message;	  \
    return rc;		  \
  } while (0)		  \

// check the return code and exit if it is an error
#define CHECK(code)							\
  do {									\
    int rc_internal = (code);						\
    if (rc_internal != RC_OK)						\
      {									\
	char *message = errorMessage(rc_internal);			\
	printf("[%s-L%i-%s] ERROR: Operation returned error: %s\n",__FILE__, __LINE__, __TIME__, message); \
	free(message);							\
	exit(1);							\
      }									\
  } while(0);

void throwError();

#endif
