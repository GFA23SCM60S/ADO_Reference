
# Record Manager Project

## Project Description

This project extends the Buffer Manager project to implement a Record Manager for a Database Management System (DBMS). The Record Manager allows clients to manipulate records stored in tables, including insertion, deletion, updating, and scanning based on conditions. It utilizes the previously implemented Buffer Manager to efficiently handle page-level data storage.

### Key Features
- Record representation with fixed-length attributes
- Page layout and metadata management
- Free space management on pages
- Record ID (RID) support for unique identification
- Conditional record scanning
- Optional extensions for enhanced functionality

## File Structure

- `README.md`: Project description and usage instructions
- `Makefile`: Build file for compiling the project
- `buffer_mgr.h`: Header file for buffer manager functions and data structures
- `record_mgr.h`: Header file for record manager functions and data structures
- `record_mgr.c`: Source file for record manager functions
- `dberror.h`: Header file for error handling
- `dberror.c`: Implementation of error handling functions
- `expr.c`: Source file for handling expressions and conditions
- `storage_mgr.h`: Header file for storage manager (from Buffer Manager project)
- `storage_mgr.c`: Source file for storage manager (from Buffer Manager project)
- `test_record_mgr.c`: Test cases for the Record Manager
- `test_helper.h`: Helper functions for the test program

## Compilation and Execution

1. Ensure all files are in the same directory.
2. Open a terminal in this directory.
3. Clean the project:
    ```bash
    make clean
    ```
4. Build the project:
    ```bash
    make
    ```
5. Run the test program:
    ```bash
    ./test_assign3
    ```
   OR
    ```bash
    make run test_assign3
    ```

## Functionalities

- **`initRecordManager`**: Initializes the Record Manager and prepares it for use.
- **`shutdownRecordManager`**: Shuts down the Record Manager and releases resources.
- **`insertRecord`**: Inserts a new record into the table.
- **`deleteRecord`**: Deletes a record identified by its Record ID (RID).
- **`updateRecord`**: Updates an existing record.
- **`scanRecords`**: Scans records based on specified conditions and retrieves matching records.

### Record Structure

Each record consists of fixed-length data types:
- **Integers**
- **Floats**
- **Strings** (of fixed length)
- **Booleans**

### Page Layout

Records are stored on pages, which include:
- Metadata (e.g., schema information)
- Free space management for record insertion and deletion
- Reserved slots for managing entries

### Optional Extensions

The following optional extensions can be implemented to enhance the functionality of the Record Manager:
- **TIDs and Tombstones**: Support for tombstones to manage deleted records without immediate compaction.
- **Null Values**: Support for SQL-style `NULL` values in records.
- **Primary Key Constraints**: Enforce primary key constraints during record insertion and updating.
- **Conditional Updates**: Allow for conditional updates based on scan results.

## Testing

The `test_record_mgr.c` file contains various test cases for the Record Manager, validating all functionalities, including insertion, deletion, updating, and scanning. Running this executable will output the results of each test case.

## Requirements

### GCC (GNU Compiler Collection)

#### Installation:
- **Unix/Linux**:
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential
    ```
- **Windows**: Install MinGW from [mingw.org](http://www.mingw.org/)
- **macOS**:
    ```bash
    brew install gcc
    ```

#### Verification:
```bash
gcc --version
```

### Unix-like Environment

- **Windows**: Use [Cygwin](https://www.cygwin.com/) or [WSL](https://docs.microsoft.com/en-us/windows/wsl/install)
- **macOS** and **Linux**: Native support


