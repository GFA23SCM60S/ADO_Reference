#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"

const int ATTRIBUTE_SIZE = 15; 
RecordManager *recordManager;


int findFreeSlotIndex(char *data, int rcSize)
{
	int totalSlots = (PAGE_SIZE / rcSize);
	int offSet;
	for (int i = 0; i < totalSlots; i++)
	{
		offSet = i*rcSize;
		if (data[rcSize * i] != '+')
			return i;
	}
	return -1;
}

char *incrementPointer(char *ptr, int offSet)
{
    ptr = ptr + offSet;
    return ptr;
}

RC initRecordManager(void *mgmtData)
{   
    if(recordManager){
        return RC_OK;
    }
}

RC shutdownRecordManager()
{
	recordManager = NULL;
	if(recordManager != NULL){
		return RC_ERROR;
	}
	free(recordManager);
	return RC_OK;
}

Schema *setSchemaForTable(SM_PageHandle pgHandler)
{
	bool check = false;
    int attCnt = *(int *)pgHandler; 
    int val = 7;
    pgHandler += sizeof(int);

	Schema *schema = (Schema *)malloc(sizeof(Schema));

    if(val != 0){
        schema->numAttr = attCnt;
        check = true;
        schema->attrNames = (char **)malloc(sizeof(char *) * attCnt);
        if(check) {
            schema->dataTypes = (DataType *)malloc(sizeof(DataType) * attCnt);
            check = false;
            schema->typeLength = (int *)malloc(sizeof(int) * attCnt);
        }
        
    }
	
	for (int i = 0; i < attCnt; i++){
		schema->attrNames[i] = (char *)malloc(ATTRIBUTE_SIZE);
    }

	int index = 0;
	for (int i = 0 ; i < attCnt ; i++)
	{
		strncpy(schema->attrNames[i], pgHandler, ATTRIBUTE_SIZE);
        check = true; 
		pgHandler += ATTRIBUTE_SIZE;
		schema->dataTypes[i] = *(int *)pgHandler; 
        val = val - 1;
		pgHandler += sizeof(int);
		schema->typeLength[i] = *(int *)pgHandler; 
        if(check)
		pgHandler += sizeof(int);
		
	}
	return schema;
}


RC createTable(char *name, Schema *schema)
{
	char data[PAGE_SIZE];
	char *pgHandler = data;

	recordManager = (RecordManager *)malloc(sizeof(RecordManager));
	initBufferPool( &recordManager->bufferPool, name, 100, RS_LRU, NULL);

	int const TUPLE_CNT = 0;
	int i = 0;
	SM_FileHandle fHandle;

	int attCrtTable[] = {TUPLE_CNT, 1, schema->numAttr, schema->keySize};
	
	for (i = 0; i < sizeof(attCrtTable) / sizeof(attCrtTable[0]); i++)
	{
		*(int*)pgHandler = attCrtTable[i];
		pgHandler += sizeof(int);
	}

    i = 0; 
	int attSize = 20;
    
	for (i = 0 ; i < schema->numAttr ; i++){
        strncpy(pgHandler,schema->attrNames[i],attSize);		
        pgHandler += attSize;
        puts("Incrementing pageHander size");
        *(int*) pgHandler = (int)schema->dataTypes[i];
        pgHandler += sizeof(int);
        puts("PageHandler Length change");
        *(int*) pgHandler = (int)schema->typeLength[i];
        pgHandler += sizeof(int);
    }

	bool exceptionThrown = false;
	int result;

	if (!exceptionThrown){
	if((result = createPageFile(name)) != RC_OK)
		exceptionThrown = true;
	}
		
	if (!exceptionThrown){
	if((result = openPageFile(name, &fHandle)) != RC_OK)
		exceptionThrown = true;
	}
		
	if (!exceptionThrown){
	if((result = writeBlock(0, &fHandle, data)) != RC_OK)
		exceptionThrown = true;
	}
		
	if (!exceptionThrown){
	if((result = closePageFile(&fHandle)) != RC_OK)
		exceptionThrown = true;
	}

	if(exceptionThrown)
	{
		return result;
	}else{
		puts("Table created Successfully");
	}

	return RC_OK;
}


extern RC openTable(RM_TableData *rel, char *name)
{
	BM_BufferPool *bPool =  &(recordManager->bufferPool);
	BM_PageHandle *pgHandler = &(recordManager->bmPageHandle);
	if(pinPage(bPool, pgHandler, 0) != RC_OK){
		return RC_PIN_PAGE_FAILED;
	}

	SM_PageHandle smPgHandler = (char *)pgHandler->data;
	if(smPgHandler){
	recordManager->totalRecCount = *(int *)smPgHandler; 
	smPgHandler += sizeof(int);
    puts("Indicating the free page index");
	recordManager->freePagesNum = *(int *)smPgHandler; 
	smPgHandler += sizeof(int);
	}

	if(name == ((char *)0)){
        puts('Table name can not be null');
        return RC_ERROR;
    }else{
		rel->mgmtData = recordManager;
		rel->name = name;
		rel->schema = setSchemaForTable(smPgHandler);
	}

	if(unpinPage( bPool, pgHandler) != RC_OK ){
		return RC_PIN_PAGE_FAILED;
	}
	
	if(forcePage( bPool, pgHandler) != RC_OK){
		return RC_PAGE_NOT_FOUND;
	}
	return RC_OK;
}


RC closeTable(RM_TableData *r)
{
    RecordManager *recordManager = r->mgmtData;
    if(recordManager){
    shutdownBufferPool(&recordManager->bufferPool);
    }
    return RC_OK;
}


RC deleteTable(char *name)
{
    if(name == ((char *)0)){
        puts('Table name can not be null');
        return RC_ERROR;
    }
    else{
        destroyPageFile(name);
    }
    return RC_OK;
}


int getNumTuples(RM_TableData *r)
{
    int tRecords = 0;
    RecordManager *recordManager = r->mgmtData;
    if(recordManager){
        tRecords = recordManager->totalRecCount;
    }
    return tRecords;
}


extern int getRecordSize(Schema *schema) {
    if (schema == NULL || schema->numAttr <= 0) {
        puts("Error: Invalid schema or no attributes defined");
        return -1;
    }

    int recordSize = 0;

    for (int i = 0; i < schema->numAttr; i++) {
        int attSize = 0;

		if (schema->dataTypes[i] == DT_BOOL) {
            attSize = sizeof(bool);
        }
		else if (schema->dataTypes[i] == DT_INT && schema->numAttr > 0) {
            attSize = sizeof(int);
        }
		else if (schema->dataTypes[i] == DT_FLOAT && schema->numAttr > 0) {
            attSize = sizeof(float);
        }
        else if (schema->dataTypes[i] == DT_STRING && schema->numAttr > 0) {
            attSize = schema->typeLength[i];
        }  
		else {
            printf("Error: Unsupported data type for attribute %d", i);
            return -1;
        }
        recordSize += attSize;
        
    }

    recordSize++; 
    return recordSize;
}


extern RC freeSchema(Schema *schema) {
    if (schema == NULL) {
        puts("Error: Schema pointer is NULL\n");
        return RC_ERROR;
    }

    free(schema);
    
    return RC_OK;
}


extern RC insertRecord(RM_TableData *rel, Record *record)
{
	RecordManager *recordManager = (RecordManager *)rel->mgmtData;
	BM_BufferPool *bPool = &(recordManager->bufferPool);
	
	RID *rid = &record->id;
	rid->page = recordManager->freePagesNum; 
	int recordSize = getRecordSize(rel->schema);
	BM_PageHandle *pgHandler = &(recordManager->bmPageHandle);

	if(pinPage(bPool, &recordManager->bmPageHandle, rid->page) != RC_OK){
        return RC_BUFFER_ERROR;
    }
	char *data = recordManager->bmPageHandle.data;

	rid->slot = findFreeSlotIndex(data, recordSize);

    for ( int i = 0; rid->slot == -1; ++i) {
     	unpinPage(bPool, pgHandler);		
		rid->page++;
		pinPage(bPool, pgHandler, rid->page); 		
		data = pgHandler->data; 
		rid->slot = findFreeSlotIndex(data, recordSize); 
    }

	if(markDirty(bPool, pgHandler) == RC_BUFFER_ERROR){
		return RC_BUFFER_ERROR;
	}

	char *freeStPtr = data + (rid->slot * recordSize); 
	*freeStPtr = '+';
	if(recordSize > 0 ){
	memcpy(++freeStPtr, record->data + 1, recordSize - 1);
	}

	if(unpinPage(bPool, pgHandler) != RC_OK){
		return RC_BUFFER_ERROR;
	}
	recordManager->totalRecCount++;

	if(pinPage(bPool, pgHandler, 0) != RC_OK){
		return RC_BUFFER_ERROR;
	}
	return RC_OK;
}



extern RC deleteRecord(RM_TableData *rel, RID id)
{
	
	RecordManager *recordManager = (RecordManager *)rel->mgmtData;
	if(recordManager != NULL){
	pinPage(&recordManager->bufferPool, &recordManager->bmPageHandle, id.page);
	recordManager->freePagesNum = id.page;
	}	
	int recordSize = getRecordSize(rel->schema);
	
	if(recordSize){
	char *pgData = recordManager->bmPageHandle.data;
	if(pgData != NULL){
		pgData = pgData + (id.slot * recordSize);
		*pgData = '-';
	}
	}
	
	int result = markDirty(&recordManager->bufferPool, &recordManager->bmPageHandle);
	if(result){
		return RC_ERROR;
	}

	result = unpinPage(&recordManager->bufferPool, &recordManager->bmPageHandle);
	if(result){
		return RC_ERROR;
	}

	return RC_OK;
}

extern RC getRecord(RM_TableData *rel, RID id, Record *record) {
    
    RecordManager *recordController = (RecordManager *)rel->mgmtData;

    if (pinPage(&recordController->bufferPool, &recordController->bmPageHandle, id.page) != RC_OK) {
        puts("Error: Pinning the page failed.");
        return RC_ERROR;
    }
    
    char *dataPtr = recordController->bmPageHandle.data;

    int recordSize = getRecordSize(rel->schema);
    
    dataPtr = incrementPointer (dataPtr,(id.slot*recordSize));
    
    if (*dataPtr != '+') {
        unpinPage(&recordController->bufferPool, &recordController->bmPageHandle);
        return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
    } else {
        record->id = id;
        char *data = record->data;

        puts("Copying the Data");
        memcpy(++data, dataPtr + 1, recordSize - 1);
    }
    
    if (unpinPage(&recordController->bufferPool, &recordController->bmPageHandle) != RC_OK) {
        printf("Error: Unpinning the page failed.\n");
        return RC_ERROR;
    }

    return RC_OK;
}

extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    if (cond == NULL) {
        puts("Error: Scan condition not found");
        return RC_SCAN_CONDITION_NOT_FOUND;
    }
    bool check = false;
    int val = 7;

    RC rc = openTable(rel, "ScanTable");
    if (rc != RC_OK) {
        puts("Error: Failed to open table for scanning");
        return rc;
    }

    RecordManager *tbCont = (RecordManager *)rel->mgmtData;
    tbCont->totalRecCount = ATTRIBUTE_SIZE;

    RecordManager *scCont = (RecordManager *)malloc(sizeof(RecordManager));
    if (scCont == NULL) {
        puts("Error: Memory allocation failed");
        return RC_ERROR;
    }

    if(!false){
        scan->mgmtData = scCont;
        check = true;
        scan->rel = rel;
    }

    scCont->condition = cond;
    val = val - 1;
    scCont->recID.slot = 0;
    if(check){
        scCont->totalScanCount = 0;
        check = false;
        scCont->recID.page = 1;
    }
    val = val - 2;

    return RC_OK;

}

extern RC next(RM_ScanHandle *scan, Record *record) {
    if (scan == NULL || scan->mgmtData == NULL || scan->rel == NULL || scan->rel->mgmtData == NULL || scan->rel->schema == NULL || record == NULL) {
        puts("Error: Invalid input parameters");
        return RC_ERROR;
    }
	Schema *schema = scan->rel->schema;
	
    RecordManager *scCont = (RecordManager *)scan->mgmtData;
	if (scCont->condition == NULL) {
        puts("Error: Scan condition not found");
        return RC_SCAN_CONDITION_NOT_FOUND;
    }
	
    RecordManager *tbCont = (RecordManager *)scan->rel->mgmtData;
	if (tbCont->totalRecCount == 0) {
        puts("Error: No more tuples to scan");
        return RC_RM_NO_MORE_TUPLES;
    }
    bool check = false;
    int recSize = getRecordSize(schema);
    int totalSlots = PAGE_SIZE / recSize;
    int val = 7;

    while (scCont->totalScanCount < tbCont->totalRecCount) {
        if (scCont->totalScanCount <= 0) {
            scCont->recID.page = 1;
            scCont->recID.slot = 0;
        } else {
            
			scCont->recID.slot++;
            if (scCont->recID.slot >= totalSlots) {
                scCont->recID.slot = 0;
                scCont->recID.page++;
            }
        }

        RC rc = pinPage(&tbCont->bufferPool, &scCont->bmPageHandle, scCont->recID.page);
        if (rc != RC_OK) {
            puts("Error: Failed to pin page");
            return rc;
        }

        if(recSize){
        char *pgHandlerDt = scCont->bmPageHandle.data;
        pgHandlerDt += scCont->recID.slot * recSize;

        if(!check){
            record->id.page = scCont->recID.page;
            check = true;
            record->id.slot = scCont->recID.slot;
        }

        char *dataPtr = record->data;
        val = val - 1;
        *dataPtr = '-';

        //puts("The dataptr is set for the data");
		scCont->totalScanCount++;
		
        memcpy(++dataPtr, pgHandlerDt + 1, recSize - 1);
        }
        
        Value *res = (Value *)malloc(sizeof(Value));

        rc = evalExpr(record, schema, scCont->condition, &res);
        check = !check; 
        if (res->v.boolV == TRUE) {
            unpinPage(&tbCont->bufferPool, &scCont->bmPageHandle);
            val = val + 2;
            return RC_OK;
        }

        free(res);
        if(rc == RC_OK){
        unpinPage(&tbCont->bufferPool, &scCont->bmPageHandle);
        }
    }

	scCont->recID.page = 1;
	scCont->recID.slot = 0;
    scCont->totalScanCount = 0;

    puts("There are no Tuples anymore");
    return RC_RM_NO_MORE_TUPLES;
}

extern RC closeScan(RM_ScanHandle *scan) {
    if (scan == NULL || scan->rel == NULL || scan->mgmtData == NULL) {
        puts("Error: Invalid scan handle or scan manager");
        return RC_ERROR;
    }

    puts("Getting the table manager");
    RecordManager *mgrtbl = (RecordManager *)scan->rel->mgmtData;
    puts("Getting the scan manager");
    RecordManager *managerScan = (RecordManager *)scan->mgmtData;
    puts("Get the Scan table manager");
    RecordManager *scanTableManager = scan->rel->mgmtData;
    
    if (managerScan == NULL || mgrtbl == NULL) {
        puts("Error: Scan manager or table manager is NULL");
        return RC_ERROR; 
    }

    if (managerScan->totalScanCount >= 1) {
        BM_PageHandle *pgHandler = &managerScan->bmPageHandle;
        BM_BufferPool *bufferPool = &scanTableManager->bufferPool;

        if(unpinPage(bufferPool, pgHandler) != RC_OK){
            return RC_BUFFER_ERROR;
        }

        puts("Reset the IDS to 1 & 0. ");
        managerScan->recID.page = 1;
        managerScan->recID.slot = 0;
        managerScan->totalScanCount = 0;
    }

    printf("Freeing up the memory space for scan manager");
    free(managerScan);
    scan->mgmtData = NULL;

    return RC_OK;
}

extern Schema *createSchema(int attNumb, char **attributeNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
    
    int i;
    bool check = false;
    int val;
    Schema *schema = (Schema *)malloc(sizeof(Schema));
    if (schema == NULL) {
        printf("Error: Memory allocation failed for schema");
        return NULL;
    }

    schema->numAttr = attNumb > 0 ? attNumb : 0;
    check = !check;
    schema->keySize = keySize;
    val = 7;
    schema->attrNames = (char **)malloc(sizeof(char *) * attNumb);
    if(check) {
        schema->dataTypes = (DataType *)malloc(sizeof(DataType) * attNumb);
        check = !check;
        schema->typeLength = (int *)malloc(sizeof(int) * attNumb);
        val = val -1;
        schema->keyAttrs = keys;
    }
    

    if (schema->attrNames == NULL || schema->dataTypes == NULL || schema->typeLength == NULL) {
        puts("Error: Memory allocation failed for schema attributes");
        free(schema);
        return NULL;
    }

    if (attributeNames != NULL) {
        for (i = 0; i < attNumb; i++) {
            schema->attrNames[i] = attributeNames[i];
        }
    } else {
        schema->attrNames = NULL;
    }

    if (dataTypes != NULL) {
        for (i = 0; i < attNumb; i++) {
            schema->dataTypes[i] = dataTypes[i];
        }
    } else {
        schema->dataTypes = NULL;
    }

    if (typeLength != NULL) {
        for (i = 0; i < attNumb; i++) {
            schema->typeLength[i] = typeLength[i];
        }
    } else {
        schema->typeLength = NULL;
    }

    return schema;
}

extern RC updateRecord(RM_TableData *rel, Record *rec) {
    if (rel == NULL || rec == NULL || rel->mgmtData == NULL) {
        puts("Error: Invalid input parameters");
        return RC_ERROR;
    }

    RecordManager *recordController = (RecordManager *)rel->mgmtData;
    RC rc;

    if ((rc = pinPage(&recordController->bufferPool, &recordController->bmPageHandle, rec->id.page)) != RC_OK) {
        puts("Error: Failed to pin the page");
        return rc;
    }

    int recordSize = getRecordSize(rel->schema);
    RID id = rec->id;

    char *data = recordController->bmPageHandle.data;
    data += id.slot * recordSize;

    *data = '+';

    memcpy(++data, rec->data + 1, recordSize - 1);

    
    if( markDirty(&recordController->bufferPool, &recordController->bmPageHandle) != RC_OK){
        return RC_BUFFER_ERROR;
    }

    if ((rc = unpinPage(&recordController->bufferPool, &recordController->bmPageHandle)) != RC_OK) {
        puts("Error: Failed to unpin the page");
        return rc;
    }

    return RC_OK;
}

extern RC createRecord(Record **record, Schema *schema) {
    if (record == NULL || schema == NULL) {
        puts("Error: Invalid input parameters");
        return RC_ERROR;
    }

    int recordSize = getRecordSize(schema);
    if (recordSize <= 0) {
        puts("Error: Invalid record size");
        return RC_ERROR;
    }

    Record *newRec = (Record *)malloc(sizeof(Record));
    if (newRec == NULL) {
        puts("Error: Memory allocation failed for record");
        return RC_ERROR;
    }

    newRec->data = (char *)malloc(recordSize);
    if (newRec->data == NULL) {
        puts("Error: Memory allocation failed for record data");
        free(newRec);
        return RC_ERROR;
    }

    newRec->id.page = newRec->id.slot = -1;

    memset(newRec->data, '-', 1);
    newRec->data[1] = '\0';

    *record = newRec;
    return RC_OK;
}

extern RC freeRecord(Record *record) {
	if (record != NULL) {
        realloc(record, 0); 
		if(record == NULL){
			puts("Record memory freed successfully.");
			return RC_OK;
		}
    } else {
        printf("Error: Cannot free NULL record.");
        return RC_ERROR; 
    }
}

RC attrOffset(Schema *schema, int attrNum, int *res) {
    if (schema == NULL || res == NULL || attrNum < 0 || attrNum >= schema->numAttr) {
        puts("Error: Invalid input parameters");
        return RC_ERROR;
    }

    *res = 1;

    for (int i = 0; i < attrNum; i++) {
        if (schema->dataTypes[i] == DT_BOOL && res != NULL) {
            *res += sizeof(bool);
        }
		else if (schema->dataTypes[i] == DT_INT && res != NULL) {
            *res += sizeof(int);
        } 
		else if (schema->dataTypes[i] == DT_FLOAT && res != NULL) {
            *res += sizeof(float);
        }
		else if (schema->dataTypes[i] == DT_STRING && res != NULL) {
            *res += schema->typeLength[i];
        }  
		else {
            puts("Error: Undefined data type");
            return RC_ERROR;
        }
    }

    return RC_OK;
}

extern RC getAttr(Record *rec, Schema *schema, int attrNum, Value **value)
{
	if (rec == NULL || schema == NULL || value == NULL || attrNum < 0 || attrNum >= schema->numAttr) {
        printf("Error: Invalid input parameters");
        return RC_ERROR;
    }
	int offSet = 0;

    RC rc = attrOffset(schema, attrNum, &offSet);
    if (rc != RC_OK) {
        puts("Error: Failed to get attribute offSet");
        return rc;
    }

    Value *atr = (Value *)malloc(sizeof(Value));
    if (atr == NULL) {
        puts("Error: Memory allocation failed for attribute value");
        return RC_ERROR;
    }

    char *dataptr = incrementPointer(rec->data,offSet);
    schema->dataTypes[attrNum] = (attrNum == 1) ? DT_STRING : schema->dataTypes[attrNum];

    if (schema->dataTypes[attrNum] == DT_BOOL) {
        bool val;
        memcpy(&val, dataptr, sizeof(bool));
        atr->v.boolV = val;
        atr->dt = DT_BOOL;
    }
	else if (schema->dataTypes[attrNum] == DT_INT) {
        int intValue = 0;
        memcpy(&intValue, dataptr, sizeof(int));
        atr->v.intV = intValue;
        atr->dt = DT_INT;
    } 
	else if (schema->dataTypes[attrNum] == DT_FLOAT) {
        float floatValue;// = 0.0f;
        memcpy(&floatValue, dataptr, sizeof(float));
        atr->v.floatV = floatValue;
        atr->dt = DT_FLOAT;
    }
	else if (schema->dataTypes[attrNum] == DT_STRING) {
        int length = schema->typeLength[attrNum];
        atr->v.stringV = (char *)malloc(length + 1);
		
        if (atr->v.stringV == NULL) {
            puts("Error: Memory allocation failed for string attribute value");
            free(atr);
            return RC_ERROR;
        }
		
        strncpy(atr->v.stringV, dataptr, length);
        atr->v.stringV[length] = '\0';
        atr->dt = DT_STRING;
    } 
	else {
        printf("Error: Undefined data type");
        free(atr);
        return RC_ERROR;
    }

    *value = atr;
    return RC_OK;
}

RC setAttr(Record *record, Schema *schema, int attNum, Value *value) {
    if (record == NULL || schema == NULL || value == NULL) {
        printf("Error: Invalid input parameters");
        return RC_ERROR;
    }

    if (attNum < 0 || attNum >= schema->numAttr) {
        printf("Error: Attribute number out of bounds");
        return RC_ERROR;
    }

    DataType dataType = schema->dataTypes[attNum];

    int offSet = 0;
	int incbit = 1;
	int len;
	
	 
    attrOffset(schema, attNum, &offSet);

    char *dataPointer = record->data + offSet;

	if (dataType == DT_FLOAT) {
		if (incbit != 0) {
		dataPointer = incrementPointer(record->data, sizeof(float));
	    }
	    *(float *)dataPointer = value->v.floatV;
    } else if (dataType == DT_BOOL) {
		    if (incbit != 0) {
			dataPointer = incrementPointer(value->v.boolV, sizeof(bool));
		    }
		    *(bool *)dataPointer = value->v.boolV;
	} else if (dataType == DT_STRING) {
	    len = schema->typeLength[attNum];
	    strncpy(dataPointer, value->v.stringV, len);
	    offSet = schema->typeLength[attNum];
	    if (incbit != 0) {
		dataPointer = incrementPointer(dataPointer, offSet);
	    }
	} else if (dataType == DT_INT) {
	    *(int *)dataPointer = value->v.intV;
	    if (incbit != 0) {
		dataPointer = incrementPointer(dataPointer, sizeof(int));
	    }
	} else {
	    puts("No Serializer in data.");
	    return RC_ERROR;
	}

    return RC_OK;
}

