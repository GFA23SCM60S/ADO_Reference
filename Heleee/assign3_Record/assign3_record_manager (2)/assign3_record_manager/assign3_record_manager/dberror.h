#ifndef DBERROR_H
#define DBERROR_H

#include "stdio.h"

/* module wide constants */
#define PAGE_SIZE 4096

/* return code definitions */
typedef int RC;

#define RC_OK 0
#define RC_FILE_NOT_FOUND 1
#define RC_FILE_HANDLE_NOT_INIT 2
#define RC_WRITE_FAILED 3
#define RC_READ_NON_EXISTING_PAGE 4
#define RC_ERROR 400 // Added a new definiton for ERROR
#define RC_PINNED_PAGES_IN_BUFFER 500 // Added a new definition for Buffer Manager


//new define error in this code
#define RC_PAGE_NOT_FOUND 501
#define RC_BUFFER_POOL_NOT_INIT 502
#define RC_PAGE_NOT_PINNED 503
#define RC_PAGE_NOT_DIRTY 504
#define RC_BUFFER_POOL_INIT_ERROR 505
#define RC_INVALID_PARAMETER 506
#define RC_UNKNOWN_STRATEGY 507


#define RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE 200
#define RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN 201
#define RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN 202
#define RC_RM_NO_MORE_TUPLES 203
#define RC_RM_NO_PRINT_FOR_DATATYPE 204
#define RC_RM_UNKOWN_DATATYPE 205

#define RC_IM_KEY_NOT_FOUND 300
#define RC_IM_KEY_ALREADY_EXISTS 301
#define RC_IM_N_TO_LAGE 302
#define RC_IM_NO_MORE_ENTRIES 303

// Added new definitions for Record Manager
#define RC_RM_NO_TUPLE_WITH_GIVEN_RID 600
#define RC_SCAN_CONDITION_NOT_FOUND 601
#define RC_FILE_CLOSE_FAILED 602
#define RC_PIN_PAGE_FAILED 603
#define RC_UNPIN_PAGE_FAILED 604
#define RC_FILE_CREATION_FAILED 605
#define RC_INVALID_VALUE 606
#define RC_DATATYPE_NOT_SUPPORTED 607
#define RC_ATTR_NUM_INVALID 608
#define RC_RECORD_NULL 609
#define RC_INVALID_ARGUMENT 610
#define RC_MEMORY_ALLOCATION_FAILED 611
#define RC_SCHEMA_NULL 612
#define RC_INVALID_SCAN_HANDLE 613
#define RC_GENERIC_ERROR 614
#define RC_MARK_PAGE_DIRTY_FAILED 615
#define RC_FILE_DELETE_FAILED 616
#define RC_TABLE_NOT_FOUND 617
#define RC_BUFFER_POOL_SHUTDOWN_FAILED 618
#define RC_BUFFER_POOL_INIT_FAILED 619
#define RC_BUFFER_POOL_PIN_FAILED 620
#define RC_MALLOC_FAILED 621
#define RC_RM_NOT_INITIALIZED 622
#define RC_RM_ALREADY_INITIALIZED 623
#define RC_RM_MEMORY_ALLOCATION_FAILED 624
#define RC_STORAGE_MGR_INIT_FAILED 625
#define RC_TABLE_ALREADY_EXISTS 626
#define RC_NO_FREE_SLOT 627
#define RC_RM_RECORD_ALREADY_DELETED 628
#define RC_RM_DELETED_RECORD 629
#define RC_SCHEMA_NOT_FOUND 630
#define RC_RECORD_MALLOC_FAILED 631
#define RC_RECORD_SIZE_INVALID 632
#define RC_RM_NO_ATTR 633
#define RC_RM_UNSUPPORTED_DATATYPE 634
#define RC_NULL_PARAMETER 635
#define RC_ATTR_NUM_OUT_OF_RANGE 636
#define RC_INSUFFICIENT_MEMORY 637
#define RC_UNSUPPORTED_DATA_TYPE 638
#define RC_MEM_ALLOC_ERROR 639
#define RC_UNSUPPORTED_DATATYPE 640
#define RC_ATTR_OFFSET_CALCULATION_FAILED 641

/* holder for error messages */
extern char *RC_message;

/* print a message to standard out describing the error */
extern void printError (RC error);
extern char *errorMessage (RC error);

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


#endif
