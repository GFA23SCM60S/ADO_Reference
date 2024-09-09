# Record Manager Assignment 3

## Code running instructions

1. Navigate to the project's root directory (assign3) using the Terminal.

2. Use the `ls` command to list the files and ensure that you are in the correct directory.

3. Execute `make clean` to remove any previously compiled `.o` files.

4. Run `make` to compile all project files, including `test_assign3_1.c`.

5. Start the script by running `./recordmgr`.

6. For testing expression-related files, use `make test_expr` to compile, and then run `./test_expr`.


## Team Members
This project was developed by a group of four individuals:
- Yash Patel - [A20547099]
- Devarshi Patel - [A20553154]
- Helee Patel - [A20551881]
- Denish Asodariya - [A20525465]


## Solution Description

This Record Manager implementation focuses on efficient memory management, minimizing variable usage, and incorporating the Tombstone mechanism for handling deleted records.

## 1. Table and Record Manager Functions

### `initRecordManager(...)`

- Initializes the record manager.
- Utilizes `initStorageManager(...)` from the Storage Manager to initialize storage.

### `shutdownRecordManager(...)`

- Shuts down the record manager, releasing all allocated resources.
- Ensures proper memory deallocation.
- Sets the recordManager data structure pointer to NULL and utilizes the C function `free()` to release memory.

### `createTable(...)`

- Opens a table with the specified name.
- Initializes the Buffer Pool using `initBufferPool(...)`.
- Sets table attributes (name, datatype, size).
- Creates a page file, opens it, writes the block containing the table, and closes the page file.

### `openTable(...)`

- Creates a table with the specified name and schema.

### `closeTable(...)`

- Closes the table referenced by the parameter 'rel'.
- Invokes Buffer Manager's `shutdownBufferPool(...)` to write changes to the table in the page file before closing the buffer pool.

### `deleteTable(...)`

- Deletes the table with the specified name.
- Uses Storage Manager's `destroyPageFile(...)` to delete the page from disk and release memory space.

### `getNumTuples(...)`

- Retrieves the number of tuples in the table referenced by the parameter 'rel'.

## 2. Record Functions

### `insertRecord(...)`

- Inserts a record into the table and updates the 'record' parameter with the Record ID.
- Locates an empty slot by pinning the page, marks it as a new record using '+', and flags the page as dirty.
- Copies the record's data to the new record via `memcpy()` and unpins the page.

### `deleteRecord(...)`

- Deletes a record with the specified Record ID from the table.
- Sets the table's metadata `freePage` to the Page ID of the page with the record to be deleted.
- Locates the record by pinning the page, marks it as deleted using '-', flags the page as dirty, and unpins the page.

### `updateRecord(...)`

- Updates a record referenced by the 'record' parameter in the table.
- Locates the page with the record, sets the Record ID, navigates to the data location, copies the updated data, marks the page as dirty, and unpins the page.

### `getRecord(...)`

- Retrieves a record with the specified Record ID from the table.
- Sets the Record ID of the 'record' parameter, copies the data, and unpins the page.

## 3. Scan Functions

### `startScan(...)`

- Initiates a scan using the RM_ScanHandle data structure provided as an argument.
- Initializes custom data structure's scan-related variables.
- Returns an error code (`RC_SCAN_CONDITION_NOT_FOUND`) if the condition is NULL.

### `next(...)`

- Provides the next tuple that meets the scan condition.
- Returns an error code (`RC_SCAN_CONDITION_NOT_FOUND`) if the condition is NULL.
- Returns `RC_RM_NO_MORE_TUPLES` if there are no tuples in the table.
- Iterates through the tuples, pins the page, copies data, and evaluates the test expression.
- Returns `RC_OK` if the tuple fulfills the condition; otherwise, returns `RC_RM_NO_MORE_TUPLES`.

### `closeScan(...)`

- Concludes the scan operation.
- Checks for incomplete scans based on the scanCount value in the table's metadata.
- If the scan was incomplete, unpins the page, resets scan-related variables, and deallocates memory space used by the metadata.

## 4. Schema Functions

### `getRecordSize(...)`

- Determines the size of a record in the specified schema.
- Iterates through schema attributes, adding the size required by each attribute to the 'size' variable.

### `freeSchema(...)`

- Removes the schema specified by the 'schema' parameter from memory.
- Uses the `free(...)` function to release memory space occupied by the schema.

### `createSchema(...)`

- Creates a new schema in memory with the specified parameters.

## 5. Attribute Functions

### `createRecord(...)`

- Creates a new record in the specified schema and assigns it to the 'record' parameter.
- Allocates memory for the record and its data.
- Adds '-' and '\0' to indicate a new blank record.

### `attrOffset(...)`

- Calculates the offset (in bytes) from the initial position to the specified attribute of the record and sets it in the 'result' parameter.

### `freeRecord(...)`

- Releases the memory space allocated to the 'record' parameter using the `free()` function.

### `getAttr(...)`

- Retrieves an attribute from the given record in the specified schema.
- Sets the 'value' parameter with the attribute details.

### `setAttr(...)`

- Sets the attribute value in the record in the specified schema.
- Copies the data from the 'value' parameter to the attribute's datatype and value.

This Record Manager implementation offers comprehensive functionality for managing tables, records, scans, schemas, and attributes. It prioritizes efficient memory management and incorporates a Tombstone mechanism to handle deleted records.