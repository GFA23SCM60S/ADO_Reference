# Record Manager in C (Group -9)

Project Modules
--------------------------------------------------------------------------------------
#### 1. Code Modules:
- buffer_mgr_stat.c
- buffer_mgr.c
- dberror.c
- expr.c
- record_mgr.c
- rm_serializer.c
- storage_mgr.c
- test_assign3_1.c
- test_expr.c

#### 2. Header Files:
- buffer_mgr_stat.h
- buffer_mgr.h
- dberror.h
- dt.h
- expr.h
- record_mgr.h
- rm_serializer.h
- storage_mgr.h
- tables.h
- test_helper.h


#### 3. Additional Files:
- makefile
- README.md


Team members
---------------------------------------

1. Arup Chauhan - initRecordManager(), shutdownRecordManager, createTable(), openTable()
2. Rahul Mansharamani - closeTable(), deleteTable(), getNumTuples(), insertRecord()
3. Andres Ghersi - deleteRecord(), updateRecord() getRecord(), startScan()
4. Vedant Chaubey - next(), closeScan(), getRecordSize(), createSchema()
5. Swastik Patro - freeSchema(), createRecord(), freeRecord(), getAttr(), setAttr()

Aim:
----------------------------------------------------------------------------------------

As per requirements of the assignment, this is a custom Record Manager implemented in C. 

The Record Manager provides functionalities for managing records and tables, including creating, deleting, updating, and retrieving records. It is designed to work with a buffer manager to handle data efficiently.

Instructions to run the code
----------------------------

- Go to Project root directory using Terminal.
- Run "make clean" to delete old compiled .o files.
- Run "make" to compile all project files including "test_assign3_1.c" file 
- Run the test assign3 executable file. 
- Run "make test_expr" to compile "test_expr.c" file
- Run the test_expr executable file.



## Table of Contents
- [Structure](#structure)
- [Custom Functions](#custom-functions)
- [Table and Record Manager Functions](#table-and-record-manager-functions)
- [Record Functions](#record-functions)
- [Scan Functions](#scan-functions)
- [Schema Functions](#schema-functions)
- [Dealing with Records and Attribute Values](#dealing-with-records-and-attribute-values)

## Structure

The Record Manager includes several custom data structures and functions to manage records and tables. The core data structure is the `RecordManager`, which holds information about the tables, records, and other relevant data.

## Custom Functions

- `findFreeSlot`: This function finds a free slot within a page to store a new record.

## Table and Record Manager Functions

These functions provide basic table and record management operations.

- `initRecordManager`: Initializes the Record Manager.
- `shutdownRecordManager`: Shuts down the Record Manager.
- `createTable`: Creates a new table with the specified schema.
- `openTable`: Opens an existing table.
- `closeTable`: Closes a table.
- `deleteTable`: Deletes a table.
- `getNumTuples`: Returns the number of tuples in a table.

## Record Functions

These functions provide operations for working with records.

- `insertRecord`: Inserts a new record into a table.
- `deleteRecord`: Deletes a record from a table.
- `updateRecord`: Updates an existing record in a table.
- `getRecord`: Retrieves a record from a table.

## Scan Functions

These functions handle scanning of records based on conditions.

- `startScan`: Starts a scan on a table with a specified condition.
- `next`: Moves to the next record in the scan.
- `closeScan`: Closes the scan.

## Schema Functions

These functions handle schema-related operations.

- `getRecordSize`: Calculates the size of a record in a schema.
- `createSchema`: Creates a new schema.
- `freeSchema`: Frees memory used by a schema.

## Dealing with Records and Attribute Values

These functions manage the creation, access, and modification of records and attribute values.

- `createRecord`: Creates a new record based on a schema.
- `attrOffset`: Calculates the offset of an attribute in a record.
- `freeRecord`: Frees memory used by a record.
- `getAttr`: Retrieves the value of an attribute from a record.
- `setAttr`: Sets the value of an attribute in a record.

 
## INNOVATION (Notions for Optional Extenstions)
### 1. TIDs and tombstones
Using a separate data structure to hold the TIDs and tombstones is one way we can implement the TID (Tuple ID) and tombstones notion.
In this method, the TIDs and tombstones might be kept on a different page or pages. The page and slot numbers where the related record is situated would make up each TID.
Instead of physically removing a record from the page when it is deleted, we can mark the record as a tombstone. A tombstone is a unique marker used to denote the deletion of a record. The TID of the deleted record would be written on the tombstone.

We can use a similar strategy to deal with updates. Instead of changing the existing record when a record is modified, we can generate a new record with the updated values and designate the previous record as a tombstone. The TID of the modified record would be written on the tombstone.

This method allows us to keep the TIDs accurate while also handling updates and removals without physically relocating the entries.

### 2. Null values
We can change the data structures and procedures to manage whether each attribute has a value or not in order to implement null values. Adding a flag or indicator for each attribute to show whether the value is null or not is one strategy. Along with the value of the attribute, this flag may be stored.

We can set the flag for each attribute to indicate whether it is null or has a valid value when inserting a record. The flag can also be updated along with the attribute value when changing a record.

It is crucial to take into account null values while running database queries. When comparing two numbers, for instance, the comparison result should be handled correctly if one of the values is null. The null flags must also be checked while retrieving records in order to establish whether a particular attribute has a valid value or is null.

We improve the system's capability to manage situations in which specific attributes may be missing by including support for null values, and enhancing flexibility in data representation and query operations.

Output Screenshots
---------------------------------------

Snapshot of the test cases

![Alt text](<WhatsApp Image 2023-10-22 at 23.53.50_1d5cb3ec.jpg>)


![Alt text](<WhatsApp Image 2023-10-22 at 23.54.39_9b2594b3.jpg>)