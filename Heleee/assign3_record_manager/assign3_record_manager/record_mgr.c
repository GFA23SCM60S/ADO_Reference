#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

// Custom data structure for managing records
typedef struct RecordManager
{
    BM_PageHandle pageHandle;     // PageHandle for Buffer Manager
    BM_BufferPool bufferPool;     // Buffer Pool for Buffer Manager
    RID recordID;                 // Record ID
    Expr *condition;              // Condition for record scanning
    int tuplesCount;              // Total number of tuples in the table
    int freePage;                 // Location of the first free page with empty slots
    int scanCount;                // Number of records scanned
} RecordManager;

const int MAX_NUMBER_OF_PAGES = 100;
const int ATTRIBUTE_SIZE = 15; // Size of the attribute name

RecordManager *recordManager;

// Custom function to find a free slot within a page
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

// Initialize the Record Manager
extern RC initRecordManager(void *mgmtData)
{
    // Initialize Storage Manager
    initStorageManager();
    return RC_OK;
}

// Shutdown the Record Manager
extern RC shutdownRecordManager()
{
    if (recordManager != NULL) {
        free(recordManager);
        recordManager = NULL;
    }
    return RC_OK;
}

// Create a table with the given schema
extern RC createTable(char *name, Schema *schema)
{
    // Allocate memory for the custom Record Manager data structure
    recordManager = (RecordManager*) malloc(sizeof(RecordManager));

    // Initialize the Buffer Pool with LFU page replacement policy
    initBufferPool(&recordManager->bufferPool, name, MAX_NUMBER_OF_PAGES, RS_LRU, NULL);

    char data[PAGE_SIZE];
    char *pageHandle = data;

    int k;

    // Set the number of tuples to 0
    *(int*)pageHandle = 0;
    pageHandle = pageHandle + sizeof(int);

    // Set the first page to 1 (0th page is for schema and metadata)
    *(int*)pageHandle = 1;
    pageHandle = pageHandle + sizeof(int);

    // Set the number of attributes
    *(int*)pageHandle = schema->numAttr;
    pageHandle = pageHandle + sizeof(int);

    // Set the Key Size of the attributes
    *(int*)pageHandle = schema->keySize;
    pageHandle = pageHandle + sizeof(int);

    k = 0;
    while (k < schema->numAttr)
    {
        // Set attribute name
        strncpy(pageHandle, schema->attrNames[k], ATTRIBUTE_SIZE);
        pageHandle = pageHandle + ATTRIBUTE_SIZE;

        // Set data type of attribute
        *(int*)pageHandle = (int)schema->dataTypes[k];
        pageHandle = pageHandle + sizeof(int);

        // Set the length of the data type of the attribute
        *(int*)pageHandle = (int)schema->typeLength[k];
        pageHandle = pageHandle + sizeof(int);

        k++;
    }

    SM_FileHandle fileHandle;

    // Create a page file with the table name using the Storage Manager
    switch (createPageFile(name)) {
        case RC_OK:
            break;
        default:
            return RC_FILE_CREATION_FAILED;
    }

    // Open the newly created page
    switch (openPageFile(name, &fileHandle)) {
        case RC_OK:
            break;
        default:
            return RC_FILE_OPEN_FAILED;
    }

    // Write the schema to the first location of the page file
    switch (writeBlock(0, &fileHandle, data)) {
        case RC_OK:
            break;
        default:
            return RC_WRITE_FAILED;
    }

    // Close the file after writing
    switch (closePageFile(&fileHandle)) {
        case RC_OK:
            break;
        default:
            return RC_FILE_CLOSE_FAILED;
    }

    return RC_OK;
}


// Open the table with the specified name
extern RC openTable(RM_TableData *rel, char *name) {
    SM_PageHandle pageHandle;

    // Associate the table's metadata with our custom Record Manager metadata
    rel->mgmtData = recordManager;
    // Set the table's name
    rel->name = name;

    // Pin a page into the Buffer Pool using the Buffer Manager
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);

    // Get the initial pointer (0th location) of the Record Manager's page data
    pageHandle = (char *)recordManager->pageHandle.data;

    // Retrieve the total number of tuples from the page file
    recordManager->tuplesCount = *(int *)pageHandle;
    pageHandle = pageHandle + sizeof(int);

    // Get the free page from the page file
    recordManager->freePage = *(int *)pageHandle;
    pageHandle = pageHandle + sizeof(int);

    // Retrieve the number of attributes from the page file
    int attributeCount = *(int *)pageHandle;
    pageHandle = pageHandle + sizeof(int);

    Schema *schema;

    // Allocate memory space for 'schema'
    schema = (Schema *)malloc(sizeof(Schema));

    // Set schema parameters
    schema->numAttr = attributeCount;
    schema->attrNames = (char **)malloc(sizeof(char *) * attributeCount);
    schema->dataTypes = (DataType *)malloc(sizeof(DataType) * attributeCount);
    schema->typeLength = (int *)malloc(sizeof(int) * attributeCount);

    // Allocate memory space for storing the attribute name for each attribute
    int k = 0;
    while (k < attributeCount) {
        schema->attrNames[k] = (char *)malloc(ATTRIBUTE_SIZE);
        k++;
    }

    k = 0;
    while (k < schema->numAttr) {
        // Set the attribute name
        strncpy(schema->attrNames[k], pageHandle, ATTRIBUTE_SIZE);
        pageHandle = pageHandle + ATTRIBUTE_SIZE;

        // Set the data type of the attribute
        schema->dataTypes[k] = *(int *)pageHandle;
        pageHandle = pageHandle + sizeof(int);

        // Set the length of the data type (length of STRING) of the attribute
        schema->typeLength[k] = *(int *)pageHandle;
        pageHandle = pageHandle + sizeof(int);
        k++;
    }

    // Set the newly created schema as the table's schema
    rel->schema = schema;

    // Unpin the page, removing it from the Buffer Pool using the Buffer Manager
    unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

    // Write the page back to disk using the Buffer Manager
    forcePage(&recordManager->bufferPool, &recordManager->pageHandle);

    return RC_OK;
}

// Close the table associated with "rel"
extern RC closeTable(RM_TableData *rel) {
    RecordManager *recordManager = rel->mgmtData;

    // Check if the Record Manager is initialized
    if (recordManager == NULL) {
        return RC_TABLE_NOT_FOUND;
    }

    // Shutdown the Buffer Pool
    shutdownBufferPool(&recordManager->bufferPool);

    // Free the memory allocated for the schema
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

// Delete the table with the specified name
extern RC deleteTable(char *name) {
    
    // Remove the page file associated with the table using the Storage Manager
    if (destroyPageFile(name) != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }

    return RC_OK;
}

// Get the number of tuples (records) in the table associated with "rel"
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

// Insert a new record into the table associated with "rel" and update the 'record' parameter with the Record ID of the newly inserted record
extern RC insertRecord(RM_TableData *rel, Record *record) {
    RecordManager *recordManager = rel->mgmtData;
    
    // Set the Record ID for this record
    RID *recordID = &record->id;
    
    char *data, *slotPointer;
    
    // Calculate the size in bytes needed to store a record based on the schema
    int recordSize = getRecordSize(rel->schema);
    
    // Initialize the slot to -1, indicating no available slot found
    int availableSlot = -1;
    
    // Iterate through the pages using a for loop
    for (int page = recordManager->freePage; page < MAX_NUMBER_OF_PAGES; page++) {
        // Pin the page to check for available slots
        pinPage(&recordManager->bufferPool, &recordManager->pageHandle, page);
        
        // Set the data to the initial position of the record's data
        data = recordManager->pageHandle.data;
        
        // Check for a free slot using our custom function
        availableSlot = findFreeSlot(data, recordSize);
        
        // If a free slot is found on this page, exit the loop
        if (availableSlot != -1) {
            // Assign the available slot to recordID
            recordID->page = page;
            recordID->slot = availableSlot;
            
            // Mark the page as dirty to indicate that it was modified
            markDirty(&recordManager->bufferPool, &recordManager->pageHandle);
            
            slotPointer = data;
            // Calculate the slot's starting position
            slotPointer = slotPointer + (recordID->slot * recordSize);

            // Append '+' as a tombstone to indicate this is a new record and should be removed if space is limited
            *slotPointer = '+';

            // Copy the record's data to the memory location pointed by slotPointer
            memcpy(++slotPointer, record->data + 1, recordSize - 1);

            // Unpin the page (remove it from the Buffer Pool)
            unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
            
            // Increment the count of tuples
            recordManager->tuplesCount++;
            
            // Pin back the first page
            pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);

            return RC_OK;
        }
        
        // Unpin the page (remove it from the Buffer Pool)
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    }
    
    // If no available slot is found, return an error
    return RC_NO_MORE_TUPLES;
}

// Delete a record with Record ID "id" from the table associated with "rel"
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

// Update a record referenced by "record" in the table associated with "rel"
extern RC updateRecord(RM_TableData *rel, Record *record) {
    RecordManager *recordManager = rel->mgmtData;

    // Check if the record and schema are not NULL
    if (record == NULL || rel->schema == NULL) {
        return RC_NULL_PARAMETER;
    }

    // Get the page and slot
    RID id = record->id;

    // Check if the page and slot are valid
    if (id.page < 0 || id.page >= recordManager->bufferPool.numPages || id.slot < 0) {
        return RC_INVALID_RID;
    }

    // Pin the page that has the record to update
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page);

    char *data;

    // Get the size of the record
    int recordSize = getRecordSize(rel->schema);

    // Get the memory location of record data and calculate the start position of the new data
    data = recordManager->pageHandle.data;
    data = data + (id.slot * recordSize);

    // Use '+' to represent a Tombstone. It indicates that the record is not empty
    *data = '+';

    // Copy the new record data to the existing record
    memcpy(++data, record->data + 1, recordSize - 1);

    // Mark the page as dirty because it has been modified
    markDirty(&recordManager->bufferPool, &recordManager->pageHandle);

    // Unpin the page after the record is updated since the page is no longer required to be in memory
    unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

    return RC_OK;
}

// Retrieve a record with the specified Record ID and store it in the location referenced by "record"
extern RC getRecord(RM_TableData *rel, RID id, Record *record) {
    RecordManager *recordManager = rel->mgmtData;

    // Pin the page that contains the record we want to retrieve
    switch (pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page)) {
        case RC_OK:
            break;
        case RC_PAGE_PIN_FAILED:
            return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
        default:
            return RC_ERROR;
    }

    // Get the size of the record
    int recordSize = getRecordSize(rel->schema);
    char *dataPointer = recordManager->pageHandle.data;
    dataPointer = dataPointer + (id.slot * recordSize);

    if (*dataPointer != '+') {
        // Return an error if no matching record for Record ID 'id' is found in the table
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
        return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
    } else {
        // Set the Record ID
        record->id = id;

        // Set the pointer to the data field of 'record' so that we can copy the data of the record
        char *data = record->data;

        // Copy data using C's function memcpy(...)
        memcpy(++data, dataPointer + 1, recordSize - 1);
    }

    // Unpin the page after the record is retrieved since the page is no longer required to be in memory
    switch (unpinPage(&recordManager->bufferPool, &recordManager->pageHandle)) {
        case RC_OK:
            break;
        default:
            return RC_ERROR;
    }

    return RC_OK;
}



// ******** SCAN FUNCTIONS ******** //

// Initialize a scan operation with a given condition
extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
    // Check if a scan condition is provided
    if (cond == NULL) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    // Open the table for scanning
    if (openTable(rel, "ScanTable") != RC_OK) {
        return RC_TABLE_NOT_FOUND; // Handle the case when the table is not found
    }

    // Allocate memory for the scan manager
    RecordManager *scanManager = (RecordManager *)malloc(sizeof(RecordManager));
    
    if (scanManager == NULL) {
        return RC_MEMORY_ALLOCATION_FAILED; // Handle memory allocation failure
    }

    // Initialize scan parameters
    scanManager->recordID.page = 1; // Start scanning from the first page
    scanManager->recordID.slot = 0; // Start scanning from the first slot
    scanManager->scanCount = 0; // Initialize the scan count
    scanManager->condition = cond; // Set the scan condition

    // Set the scan manager's metadata
    scan->mgmtData = scanManager;

    // Set the table's metadata
    RecordManager *tableManager = rel->mgmtData;

    // You may need to adapt this based on your data
    tableManager->tuplesCount = ATTRIBUTE_SIZE;

    // Set the scanned table in the scan handle
    scan->rel = rel;

    return RC_OK;
}

// Retrieve the next record satisfying the scan condition
extern RC next(RM_ScanHandle *scan, Record *record) {
    // Retrieve scan metadata
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *tableManager = scan->rel->mgmtData;
    Schema *schema = scan->rel->schema;

    // Check if a scan condition is present
    if (scanManager->condition == NULL) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    Value *result = (Value *)malloc(sizeof(Value));
    char *data;

    // Get the size of a record from the schema
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

// Close the scan operation and release resources
extern RC closeScan(RM_ScanHandle *scan) {
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *recordManager = scan->rel->mgmtData;

    // If the scan is in progress, unpin the page
    if (scanManager->scanCount > 0) {
        unpinPage(&recordManager->bufferPool, &scanManager->pageHandle);
    }

    // Reset the Scan Manager's values
    memset(scanManager, 0, sizeof(RecordManager));

    // Deallocate memory space for the scan's metadata
    free(scan->mgmtData);
    scan->mgmtData = NULL;

    return RC_OK;
}


// ******** SCHEMA FUNCTIONS ******** //

// Calculate and return the record size of a given schema
extern int getRecordSize(Schema *schema) {
    int size = 0;

    // Iterate through each attribute in the schema
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

// Create and return a new schema
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

// Release memory allocated for a schema
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

// Create a new record in the given schema
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

    // Initialize the record data with a tombstone and a NULL character
    memset(newRecord->data, '-', 1);
    newRecord->data[1] = '\0';

    *record = newRecord;

    return RC_OK;
}

// Calculate the offset (in bytes) to the specified attribute of the record and store it in the 'result' parameter
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

// Release memory allocated for a record
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

// Retrieve an attribute from the given record in the specified schema
extern RC getAttr(Record *record, Schema *schema, int attrNum, Value **value) {
    int offset = 0;

    // Check if the attribute number is valid
    if (attrNum < 0 || attrNum >= schema->numAttr) {
        return RC_INVALID_ATTR_NUM;
    }

    // Calculate the offset value of attributes depending on the attribute number
    attrOffset(schema, attrNum, &offset);

    // Allocate memory space for the Value data structure where the attribute values will be stored
    Value *attribute = (Value *)malloc(sizeof(Value));

    // Check if memory allocation failed
    if (attribute == NULL) {
        return RC_MEMORY_ALLOCATION_FAILED;
    }

    // Get the starting position of the record's data in memory
    char *dataPointer = record->data;

    // Add the offset to the starting position
    dataPointer = dataPointer + offset;

    // Set the attribute's data type based on the attribute number
    if (attrNum == 1) {
        schema->dataTypes[attrNum] = DT_STRING;
    }

    // Retrieve the attribute's value depending on the attribute's data type
    if (schema->dataTypes[attrNum] == DT_STRING) {
        // Get the attribute value from an attribute of type STRING
        int length = schema->typeLength[attrNum];
        // Allocate space for a string with size 'length'
        attribute->v.stringV = (char *)malloc(length + 1);

        if (attribute->v.stringV == NULL) {
            free(attribute);
            return RC_MEMORY_ALLOCATION_FAILED;
        }

        // Copy the string to the location pointed by dataPointer and append '\0' at the end
        strncpy(attribute->v.stringV, dataPointer, length);
        attribute->v.stringV[length] = '\0';
        attribute->dt = DT_STRING;
    } else if (schema->dataTypes[attrNum] == DT_INT) {
        // Get the attribute value from an attribute of type INTEGER
        int intValue = 0;
        memcpy(&intValue, dataPointer, sizeof(int));
        attribute->v.intV = intValue;
        attribute->dt = DT_INT;
    } else if (schema->dataTypes[attrNum] == DT_FLOAT) {
        // Get the attribute value from an attribute of type FLOAT
        float floatValue;
        memcpy(&floatValue, dataPointer, sizeof(float));
        attribute->v.floatV = floatValue;
        attribute->dt = DT_FLOAT;
    } else if (schema->dataTypes[attrNum] == DT_BOOL) {
        // Get the attribute value from an attribute of type BOOLEAN
        bool boolValue;
        memcpy(&boolValue, dataPointer, sizeof(bool));
        attribute->v.boolV = boolValue;
        attribute->dt = DT_BOOL;
    } else {
        printf("Serializer not defined for the given datatype. \n");
    }

    // Set the output value pointer
    *value = attribute;
    return RC_OK;
}

// Set the attribute value in the record in the specified schema
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
