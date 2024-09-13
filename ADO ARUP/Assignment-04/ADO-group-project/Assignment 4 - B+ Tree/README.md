# B+ Tree Index Manager / Index Manager in C (Group -9)

Project Modules
--------------------------------------------------------------------------------------
#### 1. Code Modules:
- btree_mgr.c
- buffer_mgr_stat.c
- dberror.c
- expr.c
- record_mgr.c
- rm_serializer.c
- storage_mgr.c
- test_assign4_1.c
- test_expr.c

#### 2. Header Files:
- btree_mgr.h
- buffer_mgr_stat.h
- const.h
- dberror.h
- dt.h
- expr.h
- record_mgr.h
- storage_mgr.h
- tables.h
- test_helper.h

#### 3. Additional Files:
- makefile
- README.md

Team members // To be Edited by Rahul
---------------------------------------
1. Arup Chauhan - initIndexManager(), shutdownIndexManager(), createBtree(), openBtree()
2. Rahul Mansharamani - closeBtree(), deleteBtree(), insertKey(), deleteKey()
3. Andres Ghersi - searchKey(), openTreeScan(), nextEntry(), closeTreeScan()
4. Vedant Chaubey - getNumNodes(), getNumEntries(), getKeyType(), getAttributeNumber()
5. Swastik Patro - createTreeScan(), findKey(), insertSortedEntries(), insertNodeEntry()

Aim:
----------------------------------------------------------------------------------------

This B+ Tree Manager is implemented in C to provide efficient indexing and searching capabilities for managing large datasets. It works in conjunction with the buffer manager and storage manager to ensure optimal performance.

Instructions to run the code // we need to alter
----------------------------

- Go to the project root directory using the terminal.
- Run "make clean" to delete old compiled .o files.
- Run "make" to compile all project files, including "test_assign4_1.c".
- Run the test_assign4 executable file.
- Run "make test_expr" to compile "test_expr.c" file.
- Run the test_expr executable file.

## Table of Contents
- [Structure](#structure)
- [Custom Functions](#custom-functions)
- [B+ Tree Manager Functions](#b-tree-manager-functions)
- [Key and Node Functions](#key-and-node-functions)
- [Tree Scan Functions](#tree-scan-functions)
- [Handling Key Entries and Nodes](#handling-key-entries-and-nodes)

## Structure

The B-Tree Index Manager includes custom data structures and functions to manage B-trees efficiently. The central data structure is the `BTreeManager`, which holds information about the B-trees and other relevant data.

## Custom Functions

- `findFreeNodeSlot`: This function finds a free slot within a node to store a new entry.

## B+ Tree Manager Functions

These functions provide basic B-tree management operations.

- `initIndexManager`: Initializes the B-Tree Index Manager.
- `shutdownIndexManager`: Shuts down the B-Tree Index Manager.
- `createBtree`: Creates a new B-tree with the specified order.
- `openBtree`: Opens an existing B-tree.
- `closeBtree`: Closes a B-tree.
- `deleteBtree`: Deletes a B-tree.
- `getNumNodes`: Returns the number of nodes in a B-tree.
- `getNumEntries`: Returns the number of entries in a B-tree.
- `getKeyType`: Returns the data type of the keys in a B-tree.

## Handling Key Entries and Nodes
These functions manage the creation, access, and modification of key entries and nodes in the B-tree structure.

- `findKey`: Finds a key in a B-tree node.
- `insertKey`: Inserts a new key into a B-tree.
- `deleteKey`: Deletes a key from a B-tree.

## Tree Scan Functions

These functions handle the scanning of a B-tree based on conditions.

- `openTreeScan`: Opens a scan on a B-tree with a specified condition.
- `nextEntry`: Moves to the next entry in the scan.
- `closeTreeScan`: Closes the scan.



## INNOVATION (Notions for Optional Extensions)

### 1. Integrate the B+ tree with the Record Manager

To seamlessly integrate the B+-tree with the Record Manager, we propose modifying the Record Manager to include a key attribute during table creation. Users will specify the key attribute, and the Record Manager will create and maintain a B+-tree for storing keys from this attribute. During record insertion, the corresponding key will be inserted into the B+-tree. For searches based on a key, the B+-tree will be utilized, significantly improving search efficiency compared to scanning through all records.

### 2. Allow Different Data Types as Keys

Enabling support for different data types as keys involves extending the B+-tree interface. The Record Manager will be updated to handle conversions between various data types when interacting with the B+-tree. This modification ensures flexibility in defining key attributes, accommodating a broader range of data types as valid keys.

### 3. Implement Pointers Swizzling

To implement pointers swizzling, we propose ensuring that B+-tree nodes use actual memory pointers. This will involve extending the buffer manager with a callback function for page evictions. During evictions, a bookkeeping data structure will be utilized to track replaced pointers. When a page is evicted, the callback function will access this structure, identifying locations where memory pointers need replacement with page numbers. This mechanism ensures the consistency of memory pointers across B+-tree nodes.

### 4. Support Multiple Entries for One Key

Extending support for multiple entries for one key will involve adding a new method, `findAllEntries`, to the B-tree manager interface. This method will return an array or list of entries associated with a specific key. In cases where the entries exceed the capacity of a single page, additional pages will be organized as a linked list. This modification allows the Record Manager to store and retrieve multiple entries for a given key, enhancing the versatility of the data storage and retrieval system.


Output Screenshots // To be added
---------------------------------------

Snapshot of the test cases

![Alt text](<Screenshot of B-Tree test cases>)
