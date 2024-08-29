Advanced Database Organization(CS525) Assignment 3: Record Manager 

Group Number: cs525-SP24-group#10


/************************************************************************/
SYNOPSIS:
/************************************************************************/
Implementing Record Manager for the Database management system. 

/************************************************************************/
Step by step installation and execution For Linux:
/************************************************************************/

a. Download the repository from git and unzip it.
b. Open 'assign3' folder in VS code(or any IDE). 
c. Make sure you have gcc and make install on your system. (commands : 'sudo apt install make' and 'sudo apt install gcc') 
d. Open Terminal and run command 'make clean'
e. On successful run, execute the test file using command './test1_assign3_1 and ./test_expr'

/************************************************************************/
TABLE AND RECORD MANAGER FUNCTIONS: 
/************************************************************************/
extern RC initRecordManager (void *mgmtData):
Prepares resources for record maintenance, and establishes a BufferPool and StorageManager for handling memory and disk storage

extern RC shutdownRecordManager ():
Freeing up all allocated resources, including memory and disk storage handled by  BufferPool and StorageManager.
    
extern RC createTable (char *name, Schema *schema):
Generates a new table using the provided name and schema, while reserving memory for its data structures.

extern RC openTable (RM_TableData *rel, char *name):
Accesses the specified table, creates a data structure to model it, and fetches the table's schema information for future operations.

extern RC closeTable (RM_TableData *rel):
Closes a table that was previously accessed, freeing resources such as the data structure representing the table

extern RC deleteTable (char *name):
This function will removes the table from the database, closing it if in use and also remove all associated data along with its metadata. Use caution, as this action is permanent.

extern int getNumTuples (RM_TableData *rel):
Offers the total row count (tuples) for the specified table. Remember, any changes to the table after using this function will not be included.


/************************************************************************/
RECORD FUNCTIONS: 
/************************************************************************/

extern RC insertRecord (RM_TableData *rel, Record *record):
Here we insert a new entry to a given table. It requires inputs like a buffer pool manager pointer, a record data pointer, and a table schema pointer. The function returns an integer to indicate whether the operation was successful or not.

extern RC deleteRecord (RM_TableData *rel, RID id):
It removes the record from the table this is specified in the ID.

extern RC updateRecord (RM_TableData *rel, Record *record):
It updates the recored in the table, that needs to be updated.

extern RC getRecord (RM_TableData *rel, RID id, Record *record):
It is used to get the records from the table, with the specified recored ID.


/************************************************************************/
SCAN FUNCTIONS: 
/************************************************************************/
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond):
Starts a scanning operation on a table.



extern RC next (RM_ScanHandle *scan, Record *record):
Retrieves the subsequent table entry that satisfies the specified criteria.

extern RC closeScan (RM_ScanHandle *scan):
Concludes the ongoing scan of a table.


/************************************************************************/
SCHEMA FUNCTIONS: 
/************************************************************************/
extern int getRecordSize (Schema *schema):
In this context, the getRecordSize() function is vital as it calculates and provides the byte size of a record based on the specified schema.

extern RC freeSchema (Schema *schema):
Managing memory resources responsibly is crucial, and the freeSchema function is vital for releasing the memory allocated for a specific schema.

extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys):
To begin crafting a new schema, you'd use the createSchema function. This involves assigning memory to the schema and defining its attributes, including the attribute count, names, data types, and sizes.

/************************************************************************/
ATTRIBUTE FUNCTIONS: 
/************************************************************************/
extern RC createRecord (Record **record, Schema *schema):
It produces a new record according to the provided schema, initializing all fields with NULL values as a default.

extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value):
With the provided schema, the "getAttr" function extracts the value of a specific attribute from the given record.

extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value):
It adjusts the value of a particular attribute in a record based on the given schema.

extern RC freeRecord (Record *record):
It frees up the memory allocated for a record.