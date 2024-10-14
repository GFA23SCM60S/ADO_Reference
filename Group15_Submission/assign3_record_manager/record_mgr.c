#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// mac number of pages
const int maxNumberOfPages = 100;

// Size of attribute's name
const int attributeSize = 15;

// Custom data structure to define the Record Manager.
typedef struct RecordManager
{
	// total no of tuples in the table
	int tuplesCount;
	// stores the location of first free page which has empty slots in table
	int freePage;
	// This variable stores the count of the no of records scanned
	int scanCount;
	// PageHandle for using Buffer Manager to access the page files
	BM_PageHandle pageHandle;
	// Buffer Pool
	BM_BufferPool bufferPool;
	// Record ID	
	RID recordID;
	// condition for scanning the records in the table
	Expr *condition;
} RecordManager;

RecordManager *recordManager;

/* helper functions */
int getFreeSpace(char* data, int sizeOfRecord) {
	for (int i = 0; i < PAGE_SIZE / sizeOfRecord; i++) {
		if (!(data[i * sizeOfRecord] == '+')) {
			return i;
		}
	}
	return -1;
}

void writeIntToPage(void** pageHandle, int value) {
    *(int*)(*pageHandle) = value;
    *pageHandle = *pageHandle + sizeof(int);
}

extern RC initRecordManager (void *mgmtData)
{
	// Initiliazing Storage Manager
	initStorageManager();
	printf("[init]: Record manager initialized success!\n");
	return RC_OK;
}

extern RC shutdownRecordManager ()
{
	free(recordManager);
	recordManager = NULL;
	printf("[shutdown]: Record manager shutdown success!\n");
	return RC_OK;
}

extern RC createTable (char *name, Schema *schema)
{
	char data[PAGE_SIZE];
	RC result;
	char *pageHandle = data;
	SM_FileHandle fileHandle;

    if (name == NULL || schema == NULL) {
        printf(name == NULL ? "Error: [createTable]: name is null.\n" : "Error: schema pointer is null.\n");
        return RC_ERROR;
    }

	// Allocating memory space to the record manager custom data structure
	recordManager = (RecordManager*) malloc(sizeof(RecordManager));

    // initialized record manager memory to zero
    memset(recordManager, 0, sizeof(RecordManager));

	// Initalizing the Buffer Pool using LFU page replacement policy
	initBufferPool(&recordManager->bufferPool, name, maxNumberOfPages, RS_LRU, NULL);

	// Setting pageHandle intial value
	writeIntToPage(&pageHandle, 0);
	writeIntToPage(&pageHandle, 1);

	// Setting the number of attributes in pageHandle
    writeIntToPage(&pageHandle, schema->numAttr);

	// Setting the Key Size of the attributes in pageHandle
    writeIntToPage(&pageHandle, schema->keySize);

	for(int k = 0; k < schema->numAttr; k++){
		// Setting attribute name and it's data type
		if (strlen(schema->attrNames[k]) >= attributeSize) {
            printf("\n\n ERROR: [createTable]: attribute name is too long!!! \n\n");
            return RC_ERROR;
        }
		strncpy(pageHandle, schema->attrNames[k], attributeSize);
		pageHandle[attributeSize - 1] = '\0';
		pageHandle = pageHandle + attributeSize;
        writeIntToPage(&pageHandle, (int)schema->dataTypes[k]);

		// Setting length of datatype of the attribute
		writeIntToPage(&pageHandle, (int) schema->typeLength[k]);
    }

	// Creating a page file as table name using the storage manager
	if((result = createPageFile(name)) != RC_OK) {
	    printf("[createTable]: create page file failed!\n");
	    return result;
	}

	// Opening the newly created page using the storage manager
	if((result = openPageFile(name, &fileHandle)) != RC_OK) {
		printf("[createTable]: open page file failed!\n");
		return result;
	}

	// Write to first location of the page file using the storage manager
	if((result = writeBlock(0, &fileHandle, data)) != RC_OK) {
		printf("[createTable]: write block failed!\n");
		return result;
	}

	// Closing the file after writing to file using the storage manager
	if((result = closePageFile(&fileHandle)) != RC_OK) {
		printf("[createTable]: close page file failed!\n");
		return result;
	}

	return RC_OK;
}

extern RC openTable (RM_TableData *rel, char *name)
{
	int attrCount;
	SM_PageHandle pageHandle;
	Schema *schema;

    if (rel == NULL || name == NULL) {
        printf(rel == NULL ? "Error: [openTable]: table data pointer is null.\n" : "Error: name is null.\n");
        return RC_ERROR;
    }

	// Setting the meta data of the table and name
	rel->mgmtData = recordManager;
	rel->name = name;
    
	// Pinning a page using the Buffer Manager
	pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);
	
	// Setting the pointer intially to the 0th location
	pageHandle = (char*) recordManager->pageHandle.data;

	// Retrieving total no. of tuples from the page file
	recordManager->tuplesCount= *(int*)pageHandle;
	pageHandle = pageHandle + sizeof(int);

	// Retriving free page from the page file
	recordManager->freePage= *(int*) pageHandle;
    pageHandle = pageHandle + sizeof(int);
	
	// Retriving the number of attributes from the page file
    attrCount = *(int*)pageHandle;
	pageHandle = pageHandle + sizeof(int);

	// Allocating memory space in schema
	schema = (Schema*) malloc(sizeof(Schema));
	memset(schema, 0, sizeof(Schema));
    
	// Setting parameters of schema
	schema->numAttr = attrCount;
	schema->typeLength = (int*) malloc(sizeof(int) *attrCount);
	memset(schema->typeLength, 0, sizeof(int) *attrCount);
	schema->attrNames = (char**) malloc(sizeof(char*) *attrCount);
	memset(schema->attrNames, 0, sizeof(char*) *attrCount);
	schema->dataTypes = (DataType*) malloc(sizeof(DataType) *attrCount);
	memset(schema->dataTypes, 0, sizeof(DataType) *attrCount);
	// Allocate memory for storing attribute's name for each attribute
	for(int k = 0; k < attrCount; k++) {
		schema->attrNames[k]= (char*) malloc(attributeSize);
		memset(schema->attrNames[k], 0, attributeSize);
	}
      
	for(int k = 0; k < schema->numAttr; k++){
		// Setting each attribute name and it's data type
		if (strlen(pageHandle) >= attributeSize) {
            printf("\n\n ERROR: [openTable]: attribute name is too long!!! \n\n");
            return RC_ERROR;
        }

		strncpy(schema->attrNames[k], pageHandle, attributeSize);
		schema->attrNames[k][attributeSize - 1] = '\0';
		pageHandle = pageHandle + attributeSize;

		schema->dataTypes[k]= *(int*) pageHandle;
		pageHandle = pageHandle + sizeof(int);

		// Setting length of datatype of the attribute
		schema->typeLength[k]= *(int*)pageHandle;
		pageHandle = pageHandle + sizeof(int);
	}
	
	// Setting newly created schema to the table's schema
	rel->schema = schema;	

	// Unpinning the page from Buffer Pool
	unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

	// Write the page back to disk
	forcePage(&recordManager->bufferPool, &recordManager->pageHandle);

	return RC_OK;
}   

extern RC closeTable (RM_TableData *rel)
{
    if (rel == NULL) {
        printf("Error: [closeTable]: table data pointer is null.\n");
        return RC_ERROR;
    }
	// Storing the table's meta data
	RecordManager *recordManager = (*rel).mgmtData;
	shutdownBufferPool(&recordManager->bufferPool);

	return RC_OK;
}

extern RC deleteTable (char *name)
{
    if (name == NULL) {
        printf("Error: [deleteTable]: name of table is null.\n");
        return RC_ERROR;
    }

	// Removing the page file from memory
	destroyPageFile(name);
	
	return RC_OK;
}

extern int getNumTuples (RM_TableData *rel)
{
    if (rel == NULL) {
        printf("Error: [getNumTuples]: table data pointer is null.\n");
        return RC_ERROR;
    }

	// Accessing our data structure's tuplesCount and returning it
	RecordManager *recordManager = rel->mgmtData;
	return recordManager->tuplesCount;
}

extern RC insertRecord (RM_TableData* rel, Record* record) {
    if (rel == NULL || record == NULL) {
        printf(rel == NULL ? "Error: [insertRecord]: table data pointer is null.\n" : "Error: record pointer is null.\n");
        return RC_ERROR;
    }

	// Set the reference ID for the new record
	RecordManager *recordMngr = rel->mgmtData;	
	RID *rID = &record->id; 
	
	// Pin the page
	rID->page = recordMngr->freePage;
	int rSize = getRecordSize(rel->schema);
	pinPage(&recordMngr->bufferPool, &recordMngr->pageHandle, rID->page);
	
	// Set the memory position for the record
	char* auxPointer = recordMngr->pageHandle.data;

	// Find the free slot
	while((rID->slot = getFreeSpace(auxPointer, rSize)) == -1) {
		// If no free slots, change the page
		unpinPage(&recordMngr->bufferPool, &recordMngr->pageHandle);	
		rID->page++;
		pinPage(&recordMngr->bufferPool, &recordMngr->pageHandle, rID->page);
		
		// Update the start of data		
		auxPointer = recordMngr->pageHandle.data;
	}
	// Now the page is dirty
	markDirty(&recordMngr->bufferPool, &recordMngr->pageHandle);
	
	// Set the memory position for the record
	auxPointer += (rID->slot * rSize);

	// Tombstone method
	*auxPointer = '+';

	// Copy the rest of the record to the next space
	memcpy(auxPointer+1, record->data + 1, rSize - 1);

	// Unpinning, update the quantity of records and pin
	unpinPage(&recordMngr->bufferPool, &recordMngr->pageHandle);

	recordMngr->tuplesCount++;

	pinPage(&recordMngr->bufferPool, &recordMngr->pageHandle, 0);

	return RC_OK;
}

extern RC deleteRecord (RM_TableData* rel, RID id) {
    if (rel == NULL) {
        printf("Error: [deleteRecord]: table data pointer is null.\n");
        return RC_ERROR;
    }

	// Pin page to prevent flushing and mark dirty before we are modifying it
	RecordManager *recordMngr = rel->mgmtData;

	pinPage(&recordMngr->bufferPool, &recordMngr->pageHandle, id.page);

	markDirty(&recordMngr->bufferPool, &recordMngr->pageHandle);

	// Point to character
	int rSize = getRecordSize(rel->schema);
	char* dataP = recordMngr->pageHandle.data + (id.slot * rSize);
	*dataP = '-';
	// Set free page id
	recordMngr->freePage = id.page;

	// Free page from pin / Unpin
	unpinPage(&recordMngr->bufferPool, &recordMngr->pageHandle);
	return RC_OK;
}

extern RC updateRecord (RM_TableData* rel, Record* record) {	
    if (rel == NULL || record == NULL) {
        printf(rel == NULL ? "Error: [updateRecord]: table data pointer is null.\n" : "Error: record pointer is null.\n");
        return RC_ERROR;
    }

	// Pin page to prevent flushing and mark dirty before we are modifying it
	RecordManager *recordMngr = rel->mgmtData;

	pinPage(&recordMngr->bufferPool, &recordMngr->pageHandle, record->id.page);

	markDirty(&recordMngr->bufferPool, &recordMngr->pageHandle);

	// Point to character
	int rSize = getRecordSize(rel->schema);
	RID id = record->id;
	char* dataP = recordMngr->pageHandle.data + (id.slot * rSize);
	
	*dataP = '+';
	// Set data
	memcpy(dataP+1, record->data + 1, rSize - 1 );
	
	// Free page from pin / Unpin
	unpinPage(&recordMngr->bufferPool, &recordMngr->pageHandle);
	return RC_OK;	
}

extern RC getRecord (RM_TableData* rel, RID id, Record* record) {
    if (rel == NULL || record == NULL) {
        printf(rel == NULL ? "Error: [getRecord]: table data pointer is null.\n" : "Error: record pointer is null.\n");
        return RC_ERROR;
    }

	// Pin page to prevent flushing
	RecordManager* recordMngr = rel->mgmtData;
	pinPage(&recordMngr->bufferPool, &recordMngr->pageHandle, id.page);

	// Point to character
	int rSize = getRecordSize(rel->schema);
	char* dataP = recordMngr->pageHandle.data + (id.slot * rSize);
	
	if(*dataP != '+') {
		return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
	}
	// Get data
	record->id = id;
	char* data = record->data;
	memcpy(++data, dataP + 1, rSize - 1);
	// Free page from pin / Unpin
	unpinPage(&recordMngr->bufferPool, &recordMngr->pageHandle);
	return RC_OK;
}

extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    if (rel == NULL || scan == NULL) {
        printf(rel == NULL ? "Error: [startScan]: table data pointer is null.\n" : "Error: scan pointer is null.\n");
        return RC_ERROR;
    }

   // Check if the condition is provided
    if (cond) {
        // Open the table for scanning
        openTable(rel, "ScanTable");

        // Allocate memory for the scan manager
        RecordManager *scanManager = (RecordManager *)malloc(sizeof(RecordManager));
        if (!scanManager) {
            printf("Error: [startScan]: Memory allocation failed.\n");
            return RC_ERROR;
        }
        memset(scanManager, 0, sizeof(RecordManager));

        // Initialize scan manager fields
        scanManager->recordID.page = 1;  // Start from the first page
        scanManager->recordID.slot = 0;  // Start from the first slot
        scanManager->scanCount = 0;      // Initialize scan count
        scanManager->condition = cond;   // Set the scan condition

        // Set the scan handle's management data
        scan->mgmtData = scanManager;
        scan->rel = rel;

        // Initialize table manager
        RecordManager *tableManager = rel->mgmtData;
        tableManager->tuplesCount = attributeSize;

        return RC_OK;
    } else {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }
}

extern RC next(RM_ScanHandle *scan, Record *record) {
    // Validate input parameters
    if (!scan || !record) {
        printf(!scan ? "Error: [next]: scan pointer is null.\n" : "Error: record pointer is null.\n");
        return RC_ERROR;
    }

    // Initialize scan data
    RecordManager *tableManager = scan->rel->mgmtData;
    RecordManager *scanManager = scan->mgmtData;
    Schema *schema = scan->rel->schema;

    // Ensure a scanning condition is present
    if (!scanManager->condition) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    // Allocate and initialize result value
    Value *result = (Value *)malloc(sizeof(Value));
    memset(result, 0, sizeof(Value));
    char *data;

    // Calculate record size and total slots per page
    int recordSize = getRecordSize(schema);
    int totalSlots = PAGE_SIZE / recordSize;

    // Retrieve scan count and total tuples
    int scanCount = scanManager->scanCount;
    int totalTuples = tableManager->tuplesCount;

    // Check if there are any tuples in the table
    if (totalTuples == 0) {
        free(result);
        return RC_RM_NO_MORE_TUPLES;
    }

    // Iterate through the records
    while (scanCount <= totalTuples) {
        // Set initial page and slot if starting scan
        if (scanCount > 0) {
            scanManager->recordID.slot++;
            // Move to the next page if all slots are scanned
            if (scanManager->recordID.slot >= totalSlots) {
                scanManager->recordID.slot = 0;
                scanManager->recordID.page++;
            }
        } else {
            scanManager->recordID.page = 1;
            scanManager->recordID.slot = 0;
        }

        // Pin the current page in the buffer pool
        pinPage(&tableManager->bufferPool, &scanManager->pageHandle, scanManager->recordID.page);

        // Calculate the data location for the current slot
        data = scanManager->pageHandle.data + (scanManager->recordID.slot * recordSize);

        // Set the record's ID to the current scan position
        record->id.page = scanManager->recordID.page;
        record->id.slot = scanManager->recordID.slot;

        // Copy the record data
        char *dataPointer = record->data;
        *dataPointer = '-'; // Tombstone mechanism
        memcpy(++dataPointer, data + 1, recordSize - 1);

        // Increment scan count
        scanManager->scanCount++;
        scanCount++;

        // Evaluate the condition on the current record
        evalExpr(record, schema, scanManager->condition, &result);

        // If the record satisfies the condition, unpin the page and return
        if (result->v.boolV == TRUE) {
            unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
            free(result);
            return RC_OK;
        }

        // Unpin the page after processing
        unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
    }

    // Reset scan manager values after completing the scan
    scanManager->recordID.page = 1;
    scanManager->recordID.slot = 0;
    scanManager->scanCount = 0;

    // Free allocated memory for result
    free(result);

    // No more tuples satisfying the condition
    return RC_RM_NO_MORE_TUPLES;
}

extern RC closeScan (RM_ScanHandle *scan)
{
    if (scan == NULL) {
        printf("Error: [closeScan]: scan pointer is null.\n");
        return RC_ERROR;
    }

    // Retrieve scan manager and record manager
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *recordManager = scan->rel->mgmtData;

    // Check if there are any scanned records
    if (scanManager->scanCount > 0) {
        // Unpin the page from the buffer pool
        unpinPage(&recordManager->bufferPool, &scanManager->pageHandle);

        // Reset scan manager values
        scanManager->recordID.page = 1;
        scanManager->recordID.slot = 0;
        scanManager->scanCount = 0;
    }

    // Free the memory allocated for the scan manager
    free(scan->mgmtData);
    scan->mgmtData = NULL;

    return RC_OK;
}

extern int getRecordSize (Schema *schema)
{
    if (schema == NULL) {
        printf("Error: [getRecordSize]: schema pointer is null.\n");
        return -1;
    }

    int totalLength = 0;

    // Iterate through each attribute in the schema
    for (int i = 0; i < schema->numAttr; i++) {
        // Determine the size of each attribute based on its data type
        switch (schema->dataTypes[i]) {
            case DT_BOOL:
                totalLength += sizeof(bool);
                break;
            case DT_FLOAT:
                totalLength += sizeof(float);
                break;
            case DT_INT:
                totalLength += sizeof(int);
                break;
            case DT_STRING:
                totalLength += schema->typeLength[i];
                break;
            default:
                printf("Error: [getRecordSize]: Unknown data type.\n");
                return -1;
        }
    }

    // Return the total length plus one for the tombstone mechanism
    return totalLength + 1;
}

extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    if (attrNames == NULL || dataTypes == NULL) {
        printf(attrNames == NULL ? "Error: [createSchema]: attrNames pointer is null.\n" : "Error: dataTypes pointer is null.\n");
        return NULL;
    }

    // Allocate memory for the schema
    Schema *newSchema = (Schema *)malloc(sizeof(Schema));
    if (!newSchema) {
        printf("Error: [createSchema]: Memory allocation failed.\n");
        return NULL;
    }

    // Initialize the schema structure
    memset(newSchema, 0, sizeof(Schema));
    newSchema->numAttr = numAttr;
    newSchema->attrNames = attrNames;
    newSchema->dataTypes = dataTypes;
    newSchema->typeLength = typeLength;
    newSchema->keySize = keySize;
    newSchema->keyAttrs = keys;

	return newSchema;
}

extern RC freeSchema (Schema *schema)
{
    if (schema == NULL) {
        printf("Error: [freeSchema]: schema pointer is null.\n");
        return RC_ERROR;
    }
	free(schema);
	return RC_OK;
}

extern RC createRecord (Record **record, Schema *schema)
{
    if (record == NULL || schema == NULL) {
        printf(record == NULL ? "Error: [createRecord]: record pointer is null.\n" : "Error: schema pointer is null.\n");
        return RC_ERROR;
    }

	// Allocate memory for the new record
	Record *newRecord = (Record*) malloc(sizeof(Record));
	memset(newRecord, 0, sizeof(Record));
	
	// Retrieve the record size and allocate memory for new record data
	int sizeOfRecord = getRecordSize(schema);
	newRecord->data= (char*) malloc(sizeOfRecord);
	memset(newRecord->data, 0, sizeOfRecord);

	// Setting page and slot position to -1 and Retrieve the starting position in memory of the record's data
	newRecord->id.page = newRecord->id.slot = -1;
	char *dataPointer = newRecord->data;
	
	// We used '-' for Tombstone mechanism
	*dataPointer = '-';
	
	// Append '\0' to dataPointer
	*(++dataPointer) = '\0';
	*record = newRecord;

	return RC_OK;
}

RC attrOffset (Schema *schema, int attrNum, int *result)
{
    if (schema == NULL) {
        printf("Error: [attrOffset]: schema pointer is null.\n");
        return RC_ERROR;
    }

	int i;
	*result = 1;

	// Iterating through all the attributes in the schema
    for (int i = 0; i < attrNum; i++) {
        switch (schema->dataTypes[i]) {
            case DT_FLOAT:
                *result += sizeof(float);
                break;
            case DT_STRING:
                *result += schema->typeLength[i];
                break;
            case DT_BOOL:
                *result += sizeof(bool);
                break;
            case DT_INT:
                *result += sizeof(int);
                break;
            default:
                printf("Error: Unknown data type.\n");
                break;
        }
    }

	return RC_OK;
}

extern RC freeRecord (Record *record)
{
    if (record == NULL) {
        printf("Error: [freeRecord]: record pointer is null.\n");
        return RC_ERROR;
    }
	// De-allocating memory space allocated to record and freeing up that space
	free(record);
	return RC_OK;
}

extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
    if (record == NULL || schema == NULL) {
        printf(record == NULL ? "Error: [getAttr]: record pointer is null.\n" : "Error: schema pointer is null.\n");
        return RC_ERROR;
    }

    if (value == NULL) {
        printf("Error: [getAttr]: value pointer is null.\n");
        return RC_ERROR;
    }

    int offset = 0;

    // Calculate the offset for the attribute
    attrOffset(schema, attrNum, &offset);

    // Locate the starting position of the attribute in the record's data
    char *dataPointer = record->data + offset;

    // Allocate memory for the Value structure
    Value *attrValue = (Value *)malloc(sizeof(Value));
    memset(attrValue, 0, sizeof(Value));

    // Retrieve the attribute value based on its data type
    switch (schema->dataTypes[attrNum]) {
        case DT_STRING: {
            int length = schema->typeLength[attrNum];
            attrValue->v.stringV = (char *)malloc(length + 1);
            memset(attrValue->v.stringV, 0, length + 1);
            memcpy(attrValue->v.stringV, dataPointer, length);
            attrValue->v.stringV[length] = '\0';
            attrValue->dt = DT_STRING;
            break;
        }
        case DT_INT: {
            int intValue;
            memcpy(&intValue, dataPointer, sizeof(int));
            attrValue->v.intV = intValue;
            attrValue->dt = DT_INT;
            break;
        }
        case DT_FLOAT: {
            float floatValue;
            memcpy(&floatValue, dataPointer, sizeof(float));
            attrValue->v.floatV = floatValue;
            attrValue->dt = DT_FLOAT;
            break;
        }
        case DT_BOOL: {
            bool boolValue;
            memcpy(&boolValue, dataPointer, sizeof(bool));
            attrValue->v.boolV = boolValue;
            attrValue->dt = DT_BOOL;
            break;
        }
        default:
            printf("Error: [getAttr]: Unsupported data type.\n");
            free(attrValue);
            return RC_ERROR;
    }

    *value = attrValue;
    return RC_OK;
}

extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    if (record == NULL || schema == NULL) {
        printf(record == NULL ? "Error: [setAttr]: record pointer is null.\n" : "Error: schema pointer is null.\n");
        return RC_ERROR;
    }

    if (value == NULL) {
        printf("Error: [setAttr]: value pointer is null.\n");
        return RC_ERROR;
    }

    int offset = 0;

    // Calculate the offset for the attribute
    attrOffset(schema, attrNum, &offset);

    // Locate the starting position of the attribute in the record's data
    char *dataPointer = record->data + offset;

    // Set the attribute value based on its type
    switch (schema->dataTypes[attrNum]) {
        case DT_STRING: {
            int length = schema->typeLength[attrNum];
            if (strlen(value->v.stringV) >= length) {
                strncpy(dataPointer, value->v.stringV, length - 1);
                dataPointer[length - 1] = '\0';
            } else {
                strncpy(dataPointer, value->v.stringV, length);
                dataPointer[length - 1] = '\0';
            }
            break;
        }
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
            printf("Error: [setAttr]: Unsupported data type.\n");
            return RC_ERROR;
    }

    return RC_OK;
}