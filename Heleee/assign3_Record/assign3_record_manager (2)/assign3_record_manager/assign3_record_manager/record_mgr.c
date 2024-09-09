#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// This is custom data structure defined for making the use of Record Manager.
typedef struct RecordManager
{
	// Buffer Manager's PageHandle for using Buffer Manager to access Page files
	BM_PageHandle pageHandle;	// Buffer Manager PageHandle 
	// Buffer Manager's Buffer Pool for using Buffer Manager	
	BM_BufferPool bufferPool;
	// Record ID	
	RID recordID;
	// This variable defines the condition for scanning the records in the table
	Expr *condition;
	// This variable stores the total number of tuples in the table
	int tuplesCount;
	// This variable stores the location of first free page which has empty slots in table
	int freePage;
	// This variable stores the count of the number of records scanned
	int scanCount;
} RecordManager;

const int MAX_NUMBER_OF_PAGES = 100;
const int ATTRIBUTE_SIZE = 15; // Size of the name of the attribute

RecordManager *recordManager;

// ******** CUSTOM FUNCTIONS ******** //

// This function returns a free slot within a page
int findFreeSlot(char *data, int recordSize)
{
    int totalSlots = PAGE_SIZE / recordSize;

    for (int i = 0; i < totalSlots; i++) {
        if (data[i * recordSize] != '+') {
            return i;  // Found a free slot
        }
    }
    
    return -1;  // No free slots available
}


// ******** TABLE AND RECORD MANAGER FUNCTIONS ******** //

// This function initializes the Record Manager
extern RC initRecordManager (void *mgmtData)
{
	// Initiliazing Storage Manager
	initStorageManager();
	return RC_OK;
}

// This functions shuts down the Record Manager
extern RC shutdownRecordManager ()
{
	if (recordManager != NULL) {
        free(recordManager);
        recordManager = NULL;
    }
	return RC_OK;
}

// This function creates a TABLE with table name "name" having schema specified by "schema"
extern RC createTable (char *name, Schema *schema)
{
	// Allocating memory space to the record manager custom data structure
	recordManager = (RecordManager*) malloc(sizeof(RecordManager));

	// Initalizing the Buffer Pool using LFU page replacement policy
	initBufferPool(&recordManager->bufferPool, name, MAX_NUMBER_OF_PAGES, RS_LRU, NULL);

	char data[PAGE_SIZE];
	char *pageHandle = data;
	 
	int result, k;

	// Setting number of tuples to 0
	*(int*)pageHandle = 0; 

	// Incrementing pointer by sizeof(int) because 0 is an integer
	pageHandle = pageHandle + sizeof(int);
	
	// Setting first page to 1 since 0th page if for schema and other meta data
	*(int*)pageHandle = 1;

	// Incrementing pointer by sizeof(int) because 1 is an integer
	pageHandle = pageHandle + sizeof(int);

	// Setting the number of attributes
	*(int*)pageHandle = schema->numAttr;

	// Incrementing pointer by sizeof(int) because number of attributes is an integer
	pageHandle = pageHandle + sizeof(int); 

	// Setting the Key Size of the attributes
	*(int*)pageHandle = schema->keySize;

	// Incrementing pointer by sizeof(int) because Key Size of attributes is an integer
	pageHandle = pageHandle + sizeof(int);
	
	for(k = 0; k < schema->numAttr; k++)
    	{
		// Setting attribute name
       		strncpy(pageHandle, schema->attrNames[k], ATTRIBUTE_SIZE);
	       	pageHandle = pageHandle + ATTRIBUTE_SIZE;
	
		// Setting data type of attribute
	       	*(int*)pageHandle = (int)schema->dataTypes[k];

		// Incrementing pointer by sizeof(int) because we have data type using integer constants
	       	pageHandle = pageHandle + sizeof(int);

		// Setting length of datatype of the attribute
	       	*(int*)pageHandle = (int) schema->typeLength[k];

		// Incrementing pointer by sizeof(int) because type length is an integer
	       	pageHandle = pageHandle + sizeof(int);
    	}

	SM_FileHandle fileHandle;
		
	// Creating a page file page name as table name using storage manager
	if((result = createPageFile(name)) != RC_OK)
		return result;
		
	// Opening the newly created page
	if((result = openPageFile(name, &fileHandle)) != RC_OK)
		return result;
		
	// Writing the schema to first location of the page file
	if((result = writeBlock(0, &fileHandle, data)) != RC_OK)
		return result;
		
	// Closing the file after writing
	if((result = closePageFile(&fileHandle)) != RC_OK)
		return result;

	return RC_OK;
}

// This function opens the table with table name "name"
extern RC openTable (RM_TableData *rel, char *name)
{
	SM_PageHandle pageHandle;    
	
	int attributeCount, k;
	
	// Setting table's meta data to our custom record manager meta data structure
	rel->mgmtData = recordManager;
	// Setting the table's name
	rel->name = name;
    
	// Pinning a page i.e. putting a page in Buffer Pool using Buffer Manager
	pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);
	
	// Setting the initial pointer (0th location) if the record manager's page data
	pageHandle = (char*) recordManager->pageHandle.data;
	
	// Retrieving total number of tuples from the page file
	recordManager->tuplesCount= *(int*)pageHandle;
	pageHandle = pageHandle + sizeof(int);

	// Getting free page from the page file
	recordManager->freePage= *(int*) pageHandle;
    	pageHandle = pageHandle + sizeof(int);
	
	// Getting the number of attributes from the page file
    	attributeCount = *(int*)pageHandle;
	pageHandle = pageHandle + sizeof(int);
 	
	Schema *schema;

	// Allocating memory space to 'schema'
	schema = (Schema*) malloc(sizeof(Schema));
    
	// Setting schema's parameters
	schema->numAttr = attributeCount;
	schema->attrNames = (char**) malloc(sizeof(char*) *attributeCount);
	schema->dataTypes = (DataType*) malloc(sizeof(DataType) *attributeCount);
	schema->typeLength = (int*) malloc(sizeof(int) *attributeCount);

	// Allocate memory space for storing attribute name for each attribute
	for(k = 0; k < attributeCount; k++)
		schema->attrNames[k]= (char*) malloc(ATTRIBUTE_SIZE);
      
	for(k = 0; k < schema->numAttr; k++)
    	{
		// Setting attribute name
		strncpy(schema->attrNames[k], pageHandle, ATTRIBUTE_SIZE);
		pageHandle = pageHandle + ATTRIBUTE_SIZE;
	   
		// Setting data type of attribute
		schema->dataTypes[k]= *(int*) pageHandle;
		pageHandle = pageHandle + sizeof(int);

		// Setting length of datatype (length of STRING) of the attribute
		schema->typeLength[k]= *(int*)pageHandle;
		pageHandle = pageHandle + sizeof(int);
	}
	
	// Setting newly created schema to the table's schema
	rel->schema = schema;	

	// Unpinning the page i.e. removing it from Buffer Pool using BUffer Manager
	unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

	// Write the page back to disk using BUffer Manger
	forcePage(&recordManager->bufferPool, &recordManager->pageHandle);

	return RC_OK;
}   
  
// This function closes the table referenced by "rel"
extern RC closeTable(RM_TableData *rel) {
    RecordManager *recordManager = rel->mgmtData;

    // Check if the Record Manager is initialized
    if (recordManager == NULL) {
        return RC_TABLE_NOT_FOUND;
    }

    // Shut down the Buffer Pool
    shutdownBufferPool(&recordManager->bufferPool);

    // Free the schema's memory
    for (int i = 0; i < rel->schema->numAttr; i++) {
        free(rel->schema->attrNames[i]);
    }
    free(rel->schema->attrNames);
    free(rel->schema->dataTypes);
    free(rel->schema->typeLength);
    free(rel->schema);

    // Reset the table's metadata
    rel->mgmtData = NULL;

    return RC_OK;
}


// This function deletes the table having table name "name"
extern RC deleteTable(char *name) {
    
    // Remove the page file associated with the table using the Storage Manager
    if (destroyPageFile(name) != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }

    return RC_OK;
}


// This function returns the number of tuples (records) in the table referenced by "rel"
extern int getNumTuples(RM_TableData *rel) {
    int numTuples = 0;
    
    // Retrieve the number of tuples by scanning the table
    RM_ScanHandle scan;
    RC status = startScan(rel, &scan, NULL);
    
    if (status != RC_OK) {
        return status;
    }
    
    while (next(&scan, NULL) == RC_OK) {
        numTuples++;
    }
    
    // Close the scan handle
    closeScan(&scan);
    
    return numTuples;
}



// ******** RECORD FUNCTIONS ******** //

// This function inserts a new record in the table referenced by "rel" and updates the 'record' parameter with the Record ID of he newly inserted record
extern RC insertRecord (RM_TableData *rel, Record *record)
{
	// Retrieving our meta data stored in the table
	RecordManager *recordManager = rel->mgmtData;	
	
	// Setting the Record ID for this record
	RID *recordID = &record->id; 
	
	char *data, *slotPointer;
	
	// Getting the size in bytes needed to store on record for the given schema
	int recordSize = getRecordSize(rel->schema);
	
	// Setting first free page to the current page
	recordID->page = recordManager->freePage;

	// Pinning page i.e. telling Buffer Manager that we are using this page
	pinPage(&recordManager->bufferPool, &recordManager->pageHandle, recordID->page);
	
	// Setting the data to initial position of record's data
	data = recordManager->pageHandle.data;
	
	// Getting a free slot using our custom function
	recordID->slot = findFreeSlot(data, recordSize);

	while(recordID->slot == -1)
	{
		// If the pinned page doesn't have a free slot then unpin that page
		unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);	
		
		// Incrementing page
		recordID->page++;
		
		// Bring the new page into the BUffer Pool using Buffer Manager
		pinPage(&recordManager->bufferPool, &recordManager->pageHandle, recordID->page);
		
		// Setting the data to initial position of record's data		
		data = recordManager->pageHandle.data;

		// Again checking for a free slot using our custom function
		recordID->slot = findFreeSlot(data, recordSize);
	}
	
	slotPointer = data;
	
	// Mark page dirty to notify that this page was modified
	markDirty(&recordManager->bufferPool, &recordManager->pageHandle);
	
	// Calculation slot starting position
	slotPointer = slotPointer + (recordID->slot * recordSize);

	// Appending '+' as tombstone to indicate this is a new record and should be removed if space is lesss
	*slotPointer = '+';

	// Copy the record's data to the memory location pointed by slotPointer
	memcpy(++slotPointer, record->data + 1, recordSize - 1);

	// Unpinning a page i.e. removing a page from the BUffer Pool
	unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
	
	// Incrementing count of tuples
	recordManager->tuplesCount++;
	
	// Pinback the page	
	pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);

	return RC_OK;
}

// This function deletes a record having Record ID "id" in the table referenced by "rel"
extern RC deleteRecord(RM_TableData *rel, RID id) {
    RecordManager *recordManager = rel->mgmtData;
    
    // Pin the page containing the record to delete
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page);

    char *data = recordManager->pageHandle.data;

    // Calculate the record size based on the schema
    int recordSize = getRecordSize(rel->schema);

    // Move the data pointer to the specific slot of the record
    data = data + (id.slot * recordSize);

    char slotStatus = data[0];

    if (slotStatus == '+') {
        // Mark the slot as deleted by using '-'
        data[0] = '-';
        
        // Mark the page as dirty
        markDirty(&recordManager->bufferPool, &recordManager->pageHandle);

        // Unpin the page
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

        // Return success
        return RC_OK;
    } else if (slotStatus == '-') {
        // The record is already marked as deleted, return an error
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
        return RC_RM_RECORD_ALREADY_DELETED;
    } else {
        // The slot is empty, return an error
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
        return RC_RM_NO_MORE_TUPLES;
    }
}

// This function updates a record referenced by "record" in the table referenced by "rel"
extern RC updateRecord (RM_TableData *rel, Record *record)
{	
	// Retrieving our meta data stored in the table
	RecordManager *recordManager = rel->mgmtData;
	
	// Pinning the page which has the record which we want to update
	pinPage(&recordManager->bufferPool, &recordManager->pageHandle, record->id.page);

	char *data;

	// Getting the size of the record
	int recordSize = getRecordSize(rel->schema);

	// Set the Record's ID
	RID id = record->id;

	// Getting record data's memory location and calculating the start position of the new data
	data = recordManager->pageHandle.data;
	data = data + (id.slot * recordSize);
	
	// '+' is used for Tombstone mechanism. It denotes that the record is not empty
	*data = '+';
	
	// Copy the new record data to the exisitng record
	memcpy(++data, record->data + 1, recordSize - 1 );
	
	// Mark the page dirty because it has been modified
	markDirty(&recordManager->bufferPool, &recordManager->pageHandle);

	// Unpin the page after the record is retrieved since the page is no longer required to be in memory
	unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
	
	return RC_OK;	
}

// The result record is stored in the location referenced by "record"
extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{
	// Retrieving our meta data stored in the table
	RecordManager *recordManager = rel->mgmtData;
	
	// Pinning the page which has the record we want to retreive
	pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page);

	// Getting the size of the record
	int recordSize = getRecordSize(rel->schema);
	char *dataPointer = recordManager->pageHandle.data;
	dataPointer = dataPointer + (id.slot * recordSize);
	
	if(*dataPointer != '+')
	{
		// Return error if no matching record for Record ID 'id' is found in the table
		return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
	}
	else
	{
		// Setting the Record ID
		record->id = id;

		// Setting the pointer to data field of 'record' so that we can copy the data of the record
		char *data = record->data;

		// Copy data using C's function memcpy(...)
		memcpy(++data, dataPointer + 1, recordSize - 1);
	}

	// Unpin the page after the record is retrieved since the page is no longer required to be in memory
	unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

	return RC_OK;
}



// ******** SCAN FUNCTIONS ******** //

// This function scans all the records using the condition
extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    // Check if a scan condition (test expression) is provided
    if (cond == NULL) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    // Open the table
    openTable(rel, "ScanTable");

    // Create a new scan manager
    RecordManager *scanManager = (RecordManager *)malloc(sizeof(RecordManager));
    
    // Set the scan's metadata to our scan manager
    scan->mgmtData = scanManager;

    // Initialize scan parameters
    scanManager->recordID.page = 1; // Start scanning from the first page
    scanManager->recordID.slot = 0; // Start scanning from the first slot
    scanManager->scanCount = 0; // Initialize the scan count

    // Set the scan condition
    scanManager->condition = cond;

    // Set the metadata to the table's metadata
    RecordManager *tableManager = rel->mgmtData;

    // Set the tuple count to an initial value (for future use)
    tableManager->tuplesCount = ATTRIBUTE_SIZE; // You might want to adjust this

    // Set the scanned table in the scan handle
    scan->rel = rel;

    return RC_OK;
}

// This function scans each record in the table and stores the result record (record satisfying the condition)
// in the location pointed by  'record'.
extern RC next(RM_ScanHandle *scan, Record *record) {
    // Retrieve scan metadata
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *tableManager = scan->rel->mgmtData;
    Schema *schema = scan->rel->schema;

    // Check if a scan condition (test expression) is present
    if (scanManager->condition == NULL) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    Value *result = (Value *)malloc(sizeof(Value));
    char *data;

    // Get record size from the schema
    int recordSize = getRecordSize(schema);
    int totalSlots = PAGE_SIZE / recordSize;

    // Initialize scan parameters
    int tuplesCount = tableManager->tuplesCount;
    int scanCount = scanManager->scanCount;

    // If there are no tuples in the table, return an error
    if (tuplesCount == 0) {
        return RC_RM_NO_MORE_TUPLES;
    }

    while (scanCount < tuplesCount) {
        // Calculate page and slot for the next record
        int page = 1 + (scanCount / totalSlots);
        int slot = scanCount % totalSlots;

        // Pin the page and retrieve data
        pinPage(&tableManager->bufferPool, &scanManager->pageHandle, page);
        data = scanManager->pageHandle.data;
        data = data + (slot * recordSize);

        // Set the record's ID
        record->id.page = page;
        record->id.slot = slot;

        // Initialize the record data
        char *dataPointer = record->data;
        *dataPointer = '-';

        memcpy(++dataPointer, data + 1, recordSize - 1);

        // Increment scan counters
        scanManager->scanCount++;
        scanCount++;

        // Test the record against the condition
        evalExpr(record, schema, scanManager->condition, &result);

        if (result->v.boolV == TRUE) {
            unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
            return RC_OK; // The record satisfies the condition
        }

        // Unpin the page
        unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
    }

    // Reset scan counters when all tuples are scanned
    scanManager->scanCount = 0;

    return RC_RM_NO_MORE_TUPLES; // No more tuples satisfy the condition
}


// This function closes the scan operation.
extern RC closeScan(RM_ScanHandle *scan) {
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *recordManager = scan->rel->mgmtData;

    // If the scan is in progress, unpin the page
    if (scanManager->scanCount > 0) {
        unpinPage(&recordManager->bufferPool, &scanManager->pageHandle);
    }

    // Reset the Scan Manager's values
    memset(scanManager, 0, sizeof(RecordManager));

    // De-allocate memory space for the scan's metadata
    free(scan->mgmtData);
    scan->mgmtData = NULL;

    return RC_OK;
}

// ******** SCHEMA FUNCTIONS ******** //

// This function returns the record size of the schema referenced by "schema"
extern int getRecordSize(Schema *schema) {
    int size = 0;
    
    // Iterate through all the attributes in the schema
    for (int i = 0; i < schema->numAttr; i++) {
        switch (schema->dataTypes[i]) {
            // Determine the size based on the data type of the attribute
            case DT_STRING:
                size += schema->typeLength[i];
                break;
            case DT_INT:
                size += sizeof(int);
                break;
            case DT_FLOAT:
                size += sizeof(float);
                break;
            case DT_BOOL:
                size += sizeof(bool);
                break;
            default:
                // Handle unknown data types
                return -1; // Return an error code or throw an error
        }
    }

    return size;
}


// This function creates a new schema
extern Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
    Schema *schema = (Schema *)malloc(sizeof(Schema));
    
    if (schema == NULL) {
        // Handle memory allocation failure
        return NULL;
    }
    
    schema->numAttr = numAttr;
    
    if (numAttr > 0) {
        schema->attrNames = (char **)malloc(numAttr * sizeof(char *));
        schema->dataTypes = (DataType *)malloc(numAttr * sizeof(DataType));
        schema->typeLength = (int *)malloc(numAttr * sizeof(int));
        schema->keyAttrs = (int *)malloc(keySize * sizeof(int));

        if (schema->attrNames == NULL || schema->dataTypes == NULL || schema->typeLength == NULL || schema->keyAttrs == NULL) {
            // Handle memory allocation failure
            freeSchema(schema);
            return NULL;
        }

        for (int i = 0; i < numAttr; i++) {
            schema->attrNames[i] = strdup(attrNames[i]);
            schema->dataTypes[i] = dataTypes[i];
            schema->typeLength[i] = typeLength[i];
        }

        for (int i = 0; i < keySize; i++) {
            schema->keyAttrs[i] = keys[i];
        }
    }

    schema->keySize = keySize;

    return schema;
}


// This function removes a schema from memory and de-allocates all the memory space allocated to the schema.
extern RC freeSchema(Schema *schema) {
    if (schema != NULL) {
        if (schema->numAttr > 0) {
            for (int i = 0; i < schema->numAttr; i++) {
                if (schema->attrNames[i] != NULL) {
                    free(schema->attrNames[i]);
                }
            }

            if (schema->attrNames != NULL) {
                free(schema->attrNames);
            }

            if (schema->dataTypes != NULL) {
                free(schema->dataTypes);
            }

            if (schema->typeLength != NULL) {
                free(schema->typeLength);
            }

            if (schema->keyAttrs != NULL) {
                free(schema->keyAttrs);
            }
        }
        
        free(schema);
    }
    
    return RC_OK;
}


// ******** DEALING WITH RECORDS AND ATTRIBUTE VALUES ******** //

// This function creates a new record in the schema referenced by "schema"
extern RC createRecord(Record **record, Schema *schema) {
    if (schema == NULL) {
        return RC_SCHEMA_NOT_FOUND;
    }

    // Allocate memory for the new record
    Record *newRecord = (Record *)malloc(sizeof(Record));
    if (newRecord == NULL) {
        return RC_MALLOC_FAILED;
    }

    // Calculate the record size based on the schema
    int recordSize = getRecordSize(schema);

    // Allocate memory for the data of the new record
    newRecord->data = (char *)malloc(recordSize);
    if (newRecord->data == NULL) {
        free(newRecord);
        return RC_MALLOC_FAILED;
    }

    // Initialize the page and slot positions to -1 as it's a new record
    newRecord->id.page = newRecord->id.slot = -1;

    // Initialize the record data with tombstone and NULL character
    memset(newRecord->data, '-', 1);
    newRecord->data[1] = '\0';

    *record = newRecord;

    return RC_OK;
}


// This function sets the offset (in bytes) from initial position to the specified attribute of the record into the 'result' parameter passed through the function
RC attrOffset(Schema *schema, int attrNum, int *result) {
    if (schema == NULL || attrNum < 0 || attrNum >= schema->numAttr) {
        return RC_INVALID_ARGUMENT;
    }

    int offset = 0;

    // Iterate through attributes to calculate the offset
    for (int i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_STRING:
                offset += schema->typeLength[i];
                break;
            case DT_INT:
                offset += sizeof(int);
                break;
            case DT_FLOAT:
                offset += sizeof(float);
                break;
            case DT_BOOL:
                offset += sizeof(bool);
                break;
        }
    }

    *result = offset + 1; // Adding 1 to account for the tombstone character
    return RC_OK;
}

// This function removes the record from the memory.
RC freeRecord(Record *record) {
    if (record == NULL) {
        return RC_OK; // No action needed for a NULL record.
    }

    // De-allocate the memory space for the data of the record
    if (record->data != NULL) {
        free(record->data);
        record->data = NULL;
    }

    // De-allocate the memory space for the record itself
    free(record);
    return RC_OK;
}


// This function retrieves an attribute from the given record in the specified schema
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
	int offset = 0;

	// Getting the ofset value of attributes depending on the attribute number
	attrOffset(schema, attrNum, &offset);

	// Allocating memory space for the Value data structure where the attribute values will be stored
	Value *attribute = (Value*) malloc(sizeof(Value));

	// Getting the starting position of record's data in memory
	char *dataPointer = record->data;
	
	// Adding offset to the starting position
	dataPointer = dataPointer + offset;

	// If attrNum = 1
	schema->dataTypes[attrNum] = (attrNum == 1) ? 1 : schema->dataTypes[attrNum];
	
	// Retrieve attribute's value depending on attribute's data type
	switch(schema->dataTypes[attrNum])
	{
		case DT_STRING:
		{
     			// Getting attribute value from an attribute of type STRING
			int length = schema->typeLength[attrNum];
			// Allocate space for string hving size - 'length'
			attribute->v.stringV = (char *) malloc(length + 1);

			// Copying string to location pointed by dataPointer and appending '\0' which denotes end of string in C
			strncpy(attribute->v.stringV, dataPointer, length);
			attribute->v.stringV[length] = '\0';
			attribute->dt = DT_STRING;
      			break;
		}

		case DT_INT:
		{
			// Getting attribute value from an attribute of type INTEGER
			int value = 0;
			memcpy(&value, dataPointer, sizeof(int));
			attribute->v.intV = value;
			attribute->dt = DT_INT;
      			break;
		}
    
		case DT_FLOAT:
		{
			// Getting attribute value from an attribute of type FLOAT
	  		float value;
	  		memcpy(&value, dataPointer, sizeof(float));
	  		attribute->v.floatV = value;
			attribute->dt = DT_FLOAT;
			break;
		}

		case DT_BOOL:
		{
			// Getting attribute value from an attribute of type BOOLEAN
			bool value;
			memcpy(&value,dataPointer, sizeof(bool));
			attribute->v.boolV = value;
			attribute->dt = DT_BOOL;
      			break;
		}

		default:
			printf("Serializer not defined for the given datatype. \n");
			break;
	}

	*value = attribute;
	return RC_OK;
}

// This function sets the attribute value in the record in the specified schema
RC setAttr(Record *record, Schema *schema, int attrNum, Value *value) {
    if (record == NULL || schema == NULL || value == NULL) {
        return RC_NULL_PARAMETER; // Return an error code for NULL parameters.
    }

    if (attrNum < 0 || attrNum >= schema->numAttr) {
        return RC_ATTR_NUM_OUT_OF_RANGE; // Return an error code for an invalid attribute number.
    }

    int offset = 0;
    // Calculate the offset for the specified attribute
    RC offsetResult = attrOffset(schema, attrNum, &offset);

    if (offsetResult != RC_OK) {
        return offsetResult; // Return any error code encountered during offset calculation.
    }

    char *dataPointer = record->data + offset;

    switch (schema->dataTypes[attrNum]) {
        case DT_STRING:
            strncpy(dataPointer, value->v.stringV, schema->typeLength[attrNum]);
            break;

        case DT_INT:
            *(int *)dataPointer = value->v.intV;
            break;

        case DT_FLOAT:
            *(float *)dataPointer = value->v.floatV;
            break;

        case DT_BOOL:
            *(bool *)dataPointer = value->v.boolV;
            break;

        default:
            return RC_DATATYPE_NOT_SUPPORTED; // Return an error code for an unsupported data type.
    }

    return RC_OK;
}
