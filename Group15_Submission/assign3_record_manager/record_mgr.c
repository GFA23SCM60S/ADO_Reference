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
int getFreeSpace(char* data, int recordSize) {
    for (int i = 0; i < PAGE_SIZE / recordSize; i++) {
        if (data[i * recordSize] != '+') {
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

RC openTable(RM_TableData *table, char *tableName)
{
    int attributeCount;
    SM_PageHandle pageHandle;
    Schema *schema;

    if (table == NULL || tableName == NULL) {
        printf(table == NULL ? "Error: [openTable]: table data pointer is null.\n" : "Error: tableName is null.\n");
        return RC_ERROR;
    }


    if (!table || !tableName) {
        printf("Error: Invalid table or table name.\n");
        return RC_ERROR;
    }

    table->mgmtData = recordManager;
    table->name = tableName;

    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);
    pageHandle = (char*) recordManager->pageHandle.data;

    recordManager->tuplesCount = *(int*)pageHandle;
    pageHandle += sizeof(int);
    recordManager->freePage = *(int*)pageHandle;
    pageHandle += sizeof(int);

    attributeCount = *(int*)pageHandle;
    pageHandle += sizeof(int);

    schema = (Schema*) malloc(sizeof(Schema));
    memset(schema, 0, sizeof(Schema));
    schema->numAttr = attributeCount;
    schema->typeLength = (int*) malloc(sizeof(int) * attributeCount);
    schema->attrNames = (char**) malloc(sizeof(char*) * attributeCount);
    schema->dataTypes = (DataType*) malloc(sizeof(DataType) * attributeCount);

    for (int i = 0; i < attributeCount; i++) {
        schema->attrNames[i] = (char*) malloc(attributeSize);
        strncpy(schema->attrNames[i], pageHandle, attributeSize);
        schema->attrNames[i][attributeSize - 1] = '\0';
        pageHandle += attributeSize;
        schema->dataTypes[i] = *(int*) pageHandle;
        pageHandle += sizeof(int);
        schema->typeLength[i] = *(int*)pageHandle;
        pageHandle += sizeof(int);
    }

    table->schema = schema;
    unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
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

RC insertRecord(RM_TableData* table, Record* record) {
    if (!table || !record) {
        printf("Error: Invalid table or record.\n");
        return RC_ERROR;
    }

    RecordManager *manager = table->mgmtData;
    RID *recordID = &record->id;
    recordID->page = manager->freePage;
    int recordSize = getRecordSize(table->schema);

    pinPage(&manager->bufferPool, &manager->pageHandle, recordID->page);
    char* dataPointer = manager->pageHandle.data;

    while ((recordID->slot = getFreeSpace(dataPointer, recordSize)) == -1) {
        unpinPage(&manager->bufferPool, &manager->pageHandle);
        recordID->page++;
        pinPage(&manager->bufferPool, &manager->pageHandle, recordID->page);
        dataPointer = manager->pageHandle.data;
    }

    markDirty(&manager->bufferPool, &manager->pageHandle);
    dataPointer += (recordID->slot * recordSize);
    *dataPointer = '+';
    memcpy(dataPointer + 1, record->data + 1, recordSize - 1);

    unpinPage(&manager->bufferPool, &manager->pageHandle);
    manager->tuplesCount++;
    pinPage(&manager->bufferPool, &manager->pageHandle, 0);

    return RC_OK;
}

RC deleteRecord(RM_TableData* table, RID id) {
    if (!table) {
        printf("Error: Invalid table.\n");
        return RC_ERROR;
    }

    RecordManager *manager = table->mgmtData;
    pinPage(&manager->bufferPool, &manager->pageHandle, id.page);
    markDirty(&manager->bufferPool, &manager->pageHandle);

    int recordSize = getRecordSize(table->schema);
    char* dataPointer = manager->pageHandle.data + (id.slot * recordSize);
    *dataPointer = '-';

    manager->freePage = id.page;
    unpinPage(&manager->bufferPool, &manager->pageHandle);

    return RC_OK;
}

RC updateRecord(RM_TableData* table, Record* record) {
    if (!table || !record) {
        printf("Error: Invalid table or record.\n");
        return RC_ERROR;
    }

    RecordManager *manager = table->mgmtData;
    pinPage(&manager->bufferPool, &manager->pageHandle, record->id.page);
    markDirty(&manager->bufferPool, &manager->pageHandle);

    int recordSize = getRecordSize(table->schema);
    char* dataPointer = manager->pageHandle.data + (record->id.slot * recordSize);
    *dataPointer = '+';
    memcpy(dataPointer + 1, record->data + 1, recordSize - 1);

    unpinPage(&manager->bufferPool, &manager->pageHandle);

    return RC_OK;
}

RC getRecord(RM_TableData* table, RID id, Record* record) {
    if (!table || !record) {
        printf("Error: Invalid table or record.\n");
        return RC_ERROR;
    }

    RecordManager* manager = table->mgmtData;
    pinPage(&manager->bufferPool, &manager->pageHandle, id.page);

    int recordSize = getRecordSize(table->schema);
    char* dataPointer = manager->pageHandle.data + (id.slot * recordSize);

    if (*dataPointer != '+') {
        return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
    }

    record->id = id;
    memcpy(record->data + 1, dataPointer + 1, recordSize - 1);

    unpinPage(&manager->bufferPool, &manager->pageHandle);

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

RC next(RM_ScanHandle *scan, Record *record) {
    if (!scan || !record) {
        printf("Error: Invalid scan or record.\n");
        return RC_ERROR;
    }

    RecordManager *tableManager = scan->rel->mgmtData;
    RecordManager *scanManager = scan->mgmtData;
    Schema *schema = scan->rel->schema;

    if (!scanManager->condition) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    Value *result = (Value *)malloc(sizeof(Value));
    memset(result, 0, sizeof(Value));
    char *data;
    int recordSize = getRecordSize(schema);
    int totalSlots = PAGE_SIZE / recordSize;
    int scanCount = scanManager->scanCount;
    int totalTuples = tableManager->tuplesCount;

    if (totalTuples == 0) {
        free(result);
        return RC_RM_NO_MORE_TUPLES;
    }

    while (scanCount <= totalTuples) {
        if (scanCount > 0) {
            scanManager->recordID.slot++;
            if (scanManager->recordID.slot >= totalSlots) {
                scanManager->recordID.slot = 0;
                scanManager->recordID.page++;
            }
        } else {
            scanManager->recordID.page = 1;
            scanManager->recordID.slot = 0;
        }

        pinPage(&tableManager->bufferPool, &scanManager->pageHandle, scanManager->recordID.page);
        data = scanManager->pageHandle.data + (scanManager->recordID.slot * recordSize);
        record->id.page = scanManager->recordID.page;
        record->id.slot = scanManager->recordID.slot;
        memcpy(record->data + 1, data + 1, recordSize - 1);

        scanManager->scanCount++;
        scanCount++;

        evalExpr(record, schema, scanManager->condition, &result);
        if (result->v.boolV == TRUE) {
            unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
            free(result);
            return RC_OK;
        }

        unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
    }

    scanManager->recordID.page = 1;
    scanManager->recordID.slot = 0;
    scanManager->scanCount = 0;
    free(result);

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

RC createRecord(Record **record, Schema *schema) {
    if (!record || !schema) {
        printf("Error: Invalid record or schema.\n");
        return RC_ERROR;
    }

    // Allocate memory for the new record
    Record *newRecord = (Record*) malloc(sizeof(Record));
    memset(newRecord, 0, sizeof(Record));

    // Determine the size of the record and allocate memory for its data
    int recordSize = getRecordSize(schema);
    newRecord->data = (char*) malloc(recordSize);
    memset(newRecord->data, 0, recordSize);

    // Initialize the record's ID and data
    newRecord->id.page = newRecord->id.slot = -1;
    char *dataPointer = newRecord->data;

    // Use '-' as a tombstone marker
    *dataPointer = '-';

    // Ensure the data is null-terminated
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