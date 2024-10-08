#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// Custom data structure to define the Record Manager.
typedef struct RecordManager
{
	// PageHandle for using Buffer Manager to access the page files
	BM_PageHandle pageHandle;
	// Buffer Pool	
	BM_BufferPool bufferPool;
	// Record ID	
	RID recordID;
	// condition for scanning the records in the table
	Expr *condition;
	// total no of tuples in the table
	int tuplesCount;
	// stores the location of first free page which has empty slots in table
	int freePage;
	// This variable stores the count of the no of records scanned
	int scanCount;
} RecordManager;

const int maxNumberOfPages = 100;

// Size of attribute's name
const int attributeSize = 15; 

RecordManager *recordManager;

int getFreeSpace(char* data, int sizeOfRecord) {
	for (int i = 0; i < PAGE_SIZE / sizeOfRecord; i++) {
		if (!(data[i * sizeOfRecord] == '+')) {
			return i;
		}
	}
	return -1;
}


/*
	- description : Initialization of the Record Manager
	- return : RC code
*/
extern RC initRecordManager (void *mgmtData)
{
	// Initiliazing Storage Manager
	initStorageManager();
	return RC_OK;
}

/*
	- description : Shuts down the record manager 
	- return : RC code
*/
extern RC shutdownRecordManager ()
{
	recordManager = NULL;
	free(recordManager);
	return RC_OK;
}

/*
	- description : Create a TABLE with table name "name" having schema specified by "schema"
	- param : 
		1. name - name of the table
		2. schema - schema of the table 
	- return : RC code
*/
extern RC createTable (char *name, Schema *schema)
{
	// Allocating memory space to the record manager custom data structure
	recordManager = (RecordManager*) malloc(sizeof(RecordManager));

	// Initalizing the Buffer Pool using LFU page replacement policy
	initBufferPool(&recordManager->bufferPool, name, maxNumberOfPages, RS_LRU, NULL);

	char data[PAGE_SIZE];
	int output;
	char *pageHandle = data;
	 
	// Setting no. of tuples to 0
	*(int*)pageHandle = 0; 

	// Setting pageHandle intial value
	pageHandle = pageHandle + sizeof(int);
	*(int*)pageHandle = 1;
	pageHandle = pageHandle + sizeof(int);

	// Setting the number of attributes in pageHandle
	*(int*)pageHandle = schema->numAttr;
	pageHandle = pageHandle + sizeof(int); 

	// Setting the Key Size of the attributes in pageHandle
	*(int*)pageHandle = schema->keySize;
	pageHandle = pageHandle + sizeof(int);

	for(int k = 0; k < schema->numAttr; k++){
		// Setting attribute name and it's data type
		strncpy(pageHandle, schema->attrNames[k], attributeSize);
		pageHandle = pageHandle + attributeSize;

		*(int*)pageHandle = (int)schema->dataTypes[k];
		pageHandle = pageHandle + sizeof(int);

		// Setting length of datatype of the attribute
		*(int*)pageHandle = (int) schema->typeLength[k];
		pageHandle = pageHandle + sizeof(int);
    }

	SM_FileHandle fileHandle;
		
	// Creating a page file as table name using the storage manager
	if((output = createPageFile(name)) != RC_OK)
		return output;
		
	// Opening the newly created page using the storage manager
	if((output = openPageFile(name, &fileHandle)) != RC_OK)
		return output;
	
	// Write to first location of the page file using the storage manager
	if((output = writeBlock(0, &fileHandle, data)) != RC_OK)
		return output;
		
	// Closing the file after writing to file using the storage manager
	if((output = closePageFile(&fileHandle)) != RC_OK)
		return output;

	return RC_OK;
}

// This function opens the table with table name "name"
/*
	- description : Opens the table with table name "name"
	- param : 
		1. rel - pointer to the table into which will insert
		2. name - name of the table
	- return : RC code
*/
extern RC openTable (RM_TableData *rel, char *name)
{
	SM_PageHandle pageHandle;    
	
	int attrCount;
	
	// Setting the meta data of the table and name using the record manager meta data structure
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
 	
	Schema *schema;

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

/*
	- description : Close the table
	- param : 
		1. rel - pointer to the table from which to delete
	- return : RC code
*/
extern RC closeTable (RM_TableData *rel)
{
	// Storing the table's meta data
	RecordManager *recordManager = (*rel).mgmtData;
	shutdownBufferPool(&recordManager->bufferPool);

	return RC_OK;
}

/*
	- description : Deletes the table
	- param : 
		1. name - name of the table
	- return : RC code
*/
extern RC deleteTable (char *name)
{
	// Removing the page file from memory
	destroyPageFile(name);
	
	return RC_OK;
}


/*
	- description : Returns the no. of records(tuples) in the table
	- param : 
		1. rel - pointer to the table from which to delete
	- return : RC code
*/
extern int getNumTuples (RM_TableData *rel)
{
	// Accessing our data structure's tuplesCount and returning it
	RecordManager *recordManager = rel->mgmtData;
	return recordManager->tuplesCount;
}

/*
	- description : Create a new record in the table rel and sets the id in the record. 
	- param : 
		1. rel - pointer to the table into which will insert
		2. record - the original record created with the data to insert
	- return : RC code
*/
extern RC insertRecord (RM_TableData* rel, Record* record) {
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

/*
	- description : Delete the record with id from the table rel. 
	- param : 
		1. rel - pointer to the table from which to delete
		2. id - the id of the record to delete
	- return : RC code
*/
extern RC deleteRecord (RM_TableData* rel, RID id) {
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


/*
	- description : Update the record in the table rel 
	- param : 
		1. rel - pointer to the table into which will update the data
		2. record - the original record to update with the data to insert
	- return : RC code
*/
extern RC updateRecord (RM_TableData* rel, Record* record) {	
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


/*
	- description : Get the record from the table rel 
	- param : 
		1. rel - pointer to the table with the record data
		2. id - the id of the record to retrieve
		3. record - the record that will hold the data
	- return : RC code
*/
extern RC getRecord (RM_TableData* rel, RID id, Record* record) {
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

// Scan all the records using the condition check
/*
	- description : Get the record from the table rel 
	- param : 
		1. rel - pointer to the table with the record data
		2. scan - object to RM_ScanHandleobject to RM_ScanHandle
		3. cond - condition for scanning the records in the table
	- return : RC code
*/
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
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

/*
	- description : Scans each record in the table and stores the resulted record in the record location
	- param : 
		1. scan - object to RM_ScanHandle
		2. record - the original record created with the data to insert
	- return : RC code
*/
extern RC next (RM_ScanHandle *scan, Record *record)
{
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


/*
	- description : Close the scanning process
	- param : 
		1. scan - object to RM_ScanHandle
	- return : RC code
*/
extern RC closeScan (RM_ScanHandle *scan)
{
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

/*
	- description : Returns the size of records in the schema
	- param : 
		1. schema - schema of the table schema of the table
	- return : RC code
*/
extern int getRecordSize (Schema *schema)
{
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

/*
	- description : Creates new schema
	- param : 
		1. numAttr - no. of attributes
		2. attrNames - name of attributes
		3. dataTypes - data types of schema
		4. typeLength - length of the attribute
		5. keySize - size of the key
		6. keys - no. of keys
	- return : RC code
*/
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
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

/*
	- description : Free up the schema space
	- param : 
		1. schema - schema of the table schema of the table
	- return : RC code
*/
extern RC freeSchema (Schema *schema)
{
	free(schema);
	return RC_OK;
}

/*
	- description : Creates a new record in the schema referenced by "schema"
	- param : 
		1. record - the original record created with the data to insert
		2. schema - schema of the table schema of the table
	- return : RC code
*/
extern RC createRecord (Record **record, Schema *schema)
{
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

/*
	- description : Sets the offset from initial position to specified attribute of the record 
	into the 'result' parameter
	- param : 
		1. schema - schema of the table schema of the table
		2.  attrNum - number of attributesnumber of attributes
		3. result - stores the result 
	- return : RC code
*/
RC attrOffset (Schema *schema, int attrNum, int *result)
{
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
/*
	- description : Removes the record from the memory.
	- param : 
		1. record - the original record created with the data to insert
	- return : RC code
*/
extern RC freeRecord (Record *record)
{
	// De-allocating memory space allocated to record and freeing up that space
	free(record);
	return RC_OK;
}

/*
	- description : Retrieves an attribute from the given record in the specified schema
	- param : 
		1. record - the original record created with the data to insert
		2. schema - schema of the table schema of the table
		3.  attrNum - number of attributesnumber of attributes
		4. value - value of the data types value of the data types
	- return : RC code
*/
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
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


/*
	- description : Sets the attribute value in the record in the specified schema
	- param : 
		1. record - the original record created with the data to insert
		2. schema - schema of the table 
		3.  attrNum - number of attributes
		4. value - value of the data types 
	- return : RC code
*/
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
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