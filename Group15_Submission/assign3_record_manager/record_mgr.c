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
	recordManager = NULL;
	free(recordManager);
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
		strncpy(pageHandle, schema->attrNames[k], attributeSize);
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
    
	// Setting parameters of schema
	schema->numAttr = attrCount;
	schema->typeLength = (int*) malloc(sizeof(int) *attrCount);
	schema->attrNames = (char**) malloc(sizeof(char*) *attrCount);
	
	schema->dataTypes = (DataType*) malloc(sizeof(DataType) *attrCount);
	
	// Allocate memory for storing attribute's name for each attribute
	for(int k = 0; k < attrCount; k++)
		schema->attrNames[k]= (char*) malloc(attributeSize);
      
	for(int k = 0; k < schema->numAttr; k++){
		// Setting each attribute name and it's data type
		strncpy(schema->attrNames[k], pageHandle, attributeSize);
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

	// cond condition checking
	if(cond != NULL){
		// Opening the table
		openTable(rel, "ScanTable");

		RecordManager *Scanner;
		RecordManager *TableManager;

		// Allocating some memory to the Scanner
		Scanner = (RecordManager*) malloc(sizeof(RecordManager));

		// Setting the scan's meta data to our meta data
		(*scan).mgmtData = Scanner;

		// To start the scan from 1st page
		(*Scanner).recordID.page = 1;
			
		// To start the scan from 1st slot	
		(*Scanner).recordID.slot = 0;
		
		// Initializing counter	
		(*Scanner).scanCount = 0;

		// Condition for scanning
		(*Scanner).condition = cond;
			
		// Assigning meta data to table meta data
		TableManager = rel->mgmtData;
		
		TableManager->tuplesCount = attributeSize;

		(*scan).rel= rel;

		return RC_OK;
	}else{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}
		
}

extern RC next (RM_ScanHandle *scan, Record *record)
{
    if (scan == NULL || record == NULL) {
        printf(scan == NULL ? "Error: [next]: scan pointer is null.\n" : "Error: record pointer is null.\n");
        return RC_ERROR;
    }

	// Initiliazing scan data
	RecordManager *tableManager = scan->rel->mgmtData;
	RecordManager *scanManager = scan->mgmtData;
	
	Schema *schema = scan->rel->schema;
	
	// Check if scanning condition is present
	if (scanManager->condition == NULL){
		return RC_SCAN_CONDITION_NOT_FOUND;
	}

	Value *result = (Value *) malloc(sizeof(Value));
    char *data;
   	
	// Retrieve record size of the schema and calculate total no. of slots
	int sizeOfRecord = getRecordSize(schema);
	int totalSlots = PAGE_SIZE / sizeOfRecord;

	// Retrieve Scan Count and count of tuples
	int scanCount = scanManager->scanCount;
	int tuplesCount = tableManager->tuplesCount;

	// Check if the table has tuples
	if (tuplesCount == 0)
		return RC_RM_NO_MORE_TUPLES;

	while(scanCount <= tuplesCount)
	{  
		// If all the tuples have been scanned set page & slot to first position
		if (scanCount <= 0){
			scanManager->recordID.page = 1;
			scanManager->recordID.slot = 0;
		}
		else{
			scanManager->recordID.slot++;

			// If all the slots have been scanned set slot to first position
			if(scanManager->recordID.slot >= totalSlots)
			{
				scanManager->recordID.slot = 0;
				scanManager->recordID.page++;
			}
		}

		// Pinning the page in buffer pool
		pinPage(&tableManager->bufferPool, &scanManager->pageHandle, scanManager->recordID.page);
			
		// Retrieve data of the page			
		data = scanManager->pageHandle.data;

		// Calulate data location from slot and size of the record
		data = data + (scanManager->recordID.slot * sizeOfRecord);
		
		// Set the record's slot and page to scan manager's slot and page
		record->id.page = scanManager->recordID.page;
		record->id.slot = scanManager->recordID.slot;

		char *dataPointer = record->data;

		// We used '-' for Tombstone mechanism 
		*dataPointer = '-';
		
		memcpy(++dataPointer, data + 1, sizeOfRecord - 1);

		// Increment scan count after scanning 1 record
		scanManager->scanCount++;
		scanCount++;

		// Testing the specified condition for the record (test expression)
		evalExpr(record, schema, scanManager->condition, &result); 

		// If the record satisfies the condition, Unpin the page from buffer pool
		if(result->v.boolV == TRUE)
		{
			unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);			
			return RC_OK;
		}
	}
	
	// Unpin the page 
	unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
	
	// Reset the values of scan manager
	scanManager->recordID.page = 1;
	scanManager->recordID.slot = 0;
	scanManager->scanCount = 0;
	
	// No tuple satifies the conition
	return RC_RM_NO_MORE_TUPLES;
}

extern RC closeScan (RM_ScanHandle *scan)
{
    if (scan == NULL) {
        printf("Error: [closeScan]: scan pointer is null.\n");
        return RC_ERROR;
    }

	RecordManager *scanManager = (*scan).mgmtData;
	RecordManager *recordManager = (*scan).rel->mgmtData;

	// Scan Count check
	if((*scanManager).scanCount > 0)
	{
		// Unpin the page from the buffer pool.
		unpinPage(&recordManager->bufferPool, &scanManager->pageHandle);
		
		// Resetting the values
		(*scanManager).recordID.page = 1;
		(*scanManager).recordID.slot = 0;
		(*scanManager).scanCount = 0;
	}
	
	// Freeing the memory space allocated to scans's meta data
	(*scan).mgmtData = NULL;
	free((*scan).mgmtData);
	
	return RC_OK;
}

extern int getRecordSize (Schema *schema)
{
    if (schema == NULL) {
        printf("Error: [getRecordSize]: schema pointer is null.\n");
        return -1;
    }

	int length = 0, i; // Initial Value set to 0
	
	// Iterating the attributes of the schema
	for(i = 0; i < (*schema).numAttr; i++)
	{
		// Checking the Data Type of the attribute
		// Setting the length as per the data type value
		if((*schema).dataTypes[i] == DT_STRING){
			length = length + (*schema).typeLength[i];
		}else if ((*schema).dataTypes[i] == DT_INT){
			length = length + sizeof(int);
		}else if((*schema).dataTypes[i] == DT_FLOAT){
			length = length + sizeof(float);
		}else if((*schema).dataTypes[i] == DT_BOOL){
			length = length + sizeof(bool);
		}
	}
	return length+1;
}

extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    if (attrNames == NULL || dataTypes == NULL) {
        printf(attrNames == NULL ? "Error: [createSchema]: attrNames pointer is null.\n" : "Error: dataTypes pointer is null.\n");
        return NULL;
    }

	// Allocate memory space to schema
	Schema *tempSchema = (Schema *) malloc(sizeof(Schema));
	//Setting the arguments in the schema
	(*tempSchema).numAttr = numAttr;
	(*tempSchema).attrNames = attrNames;
	(*tempSchema).dataTypes = dataTypes;
	(*tempSchema).typeLength = typeLength;
	(*tempSchema).keySize = keySize;
	(*tempSchema).keyAttrs = keys;

	return tempSchema;
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
	
	// Retrieve the record size and allocate memory for new record data
	int sizeOfRecord = getRecordSize(schema);
	newRecord->data= (char*) malloc(sizeOfRecord);

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
	for(i = 0; i < attrNum; i++){	
		if (schema->dataTypes[i] == DT_STRING) {
		// If attribute is STRING, then size = typeLength (Defined Length of STRING)
		*result = *result + schema->typeLength[i];
    	} else if (schema->dataTypes[i] == DT_INT) {
			// If attribute is INTEGER, then add size of INT
			*result = *result + sizeof(int);
    	} else if (schema->dataTypes[i] == DT_FLOAT) {
			// If attribute is FLOAT, then add size of FLOAT
			*result = *result + sizeof(float);
    	} else if (schema->dataTypes[i] == DT_BOOL) {
			// If attribute is BOOLEAN, then add size of BOOLEAN
			*result = *result + sizeof(bool);
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

	int varOffset = 0;

	// Obtaining the attribute offset value based on the attribute number
	attrOffset(schema, attrNum, &varOffset);

	// Finding the beginning location in memory of the record's data
	char *PointerOfData;
	PointerOfData = record->data;

	// Allocating memory space for the Value data structure where the attribute values will be stored
	Value *attr = malloc(sizeof(Value));

	// Setting aside memory to store the attribute values in the Value data structure
	PointerOfData += varOffset;

	if (attrNum == 1) {
    	schema->dataTypes[attrNum] = 1;
	}

	// Depending on the data type of the attribute, return its value

	if (schema->dataTypes[attrNum] == DT_STRING) {

		// Obtaining the value of an attribute from one of type STRING
		int len = schema->typeLength[attrNum];
		attr->v.stringV = (char *) malloc(len + 1);

		//Adding "\0," which indicates the end of the text in C, and copying the string to the position indicated by dataPointer
		memcpy(attr->v.stringV, PointerOfData, len);
		attr->v.stringV[len] = '\0';
		attr->dt = DT_STRING;

	} else if (schema->dataTypes[attrNum] == DT_INT) {
		
		int val = 0;
		
		// obtaining the value of an attribute from an INTEGER-type attribute
		memcpy(&val, PointerOfData, sizeof(int));
		attr->v.intV = val;
		attr->dt = DT_INT;
	} 
	else if (schema->dataTypes[attrNum] == DT_FLOAT) {
		// Obtaining the value of an attribute from a FLOAT-type attribute
		float val;
		memcpy(&val, PointerOfData, sizeof(float));
		attr->v.floatV = val;
		attr->dt = DT_FLOAT;
	}
	else if (schema->dataTypes[attrNum] == DT_BOOL) {
		
		bool val;
		
		// obtaining the value of an attribute from a BOOLEAN-type attribute
		memcpy(&val, PointerOfData, sizeof(bool));
		attr->v.boolV = val;
		attr->dt = DT_BOOL;
	} 
	else {
    	printf("For the given data type no searializer\n");
	}

	*value = attr;
	
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

	int varOffset = 0;

	// Depending on the attribute number, obtaining the ofset value of the attributes
	attrOffset(schema, attrNum, &varOffset);

	// locating the beginning of the record's data in memory
	char *pointerOfData = record->data;
	
	// Increasing the beginning position's offset
	pointerOfData = pointerOfData + varOffset;
		
	if (schema->dataTypes[attrNum] == DT_STRING) {
		int len = schema->typeLength[attrNum];
		strncpy(pointerOfData, value->v.stringV, len);
		pointerOfData = pointerOfData + schema->typeLength[attrNum];
	}
	else if (schema->dataTypes[attrNum] == DT_INT) {
		*(int *)pointerOfData = value->v.intV;
		pointerOfData = pointerOfData + sizeof(int);
	}
	else if (schema->dataTypes[attrNum] == DT_FLOAT) {
		*(float *)pointerOfData = value->v.floatV;
		pointerOfData = pointerOfData + sizeof(float);
	}
	else if (schema->dataTypes[attrNum] == DT_BOOL) {
		*(bool *)pointerOfData = value->v.boolV;
		pointerOfData = pointerOfData + sizeof(bool);
	}
	else {
		printf("No Serializer for the defined datatype.\n");
	}
	return RC_OK;
}