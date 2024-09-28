Here's a more readable and organized version of the README file:

# Buffer Manager Project

## Project Description

This project extends Assignment-1: Storage Manager to implement a Buffer Manager for a DBMS. The Buffer Manager manages memory pages of a file, caching pages from disk to reduce I/O operations. It supports multiple buffer pools, each using a page replacement strategy to decide which page to evict when the cache is full.

Key features include:
- Buffer pool management
- Page management (pinning and unpinning)
- Page replacement strategies (FIFO and LRU)
- Handling dirty pages
- Concurrency support
- Statistics tracking
- Error handling and debugging

## File Structure

- `README.md`: Project description and usage instructions
- `Makefile`: Build file for compiling the project
- `buffer_mgr.h`: Header file for buffer manager functions and data structures
- `buffer_mgr.c`: Source file for buffer manager functions
- `buffer_mgr_stat.h`: Header file for buffer manager statistics
- `buffer_mgr_stat.c`: Source file for buffer pool statistics functions
- `dberror.h`: Header file for error handling
- `dberror.c`: Implementation of error handling functions
- `dt.h`: Data types file (boolean definitions and others)
- `storage_mgr.h`: Header file for storage manager (from assignment 1)
- `storage_mgr.c`: Source file for storage manager (from assignment 1)
- `test_assign2_1.c`: Test cases for buffer manager (FIFO and LRU strategies)
- `test_assign2_2.c`: Additional test cases for buffer manager
- `test_helper.h`: Helper functions for the test program

## Compilation and Execution

1. Ensure all files are in the same directory
2. Open a terminal in this directory
3. Clean the project
    ```bash
   make clean
   ```
4. Build the project:
   ```bash
   make
   ```
5. Run the test programs 1:
   ```bash
   ./test1
   ```
   OR
   ```bash 
   make run_test1
   ```

6. Run the test programs 2:
   ```bash
   ./test2
   ```
   OR
   ```bash 
   make run_test2
   ```

## Functionalities

- `initBufferPool`: Initialize a buffer pool
- `shutdownBufferPool`: Shut down the buffer pool
- `forceFlushPool`: Force write all dirty pages to disk
- `pinPage`: Load and pin a page in the buffer pool
- `unpinPage`: Unpin a page
- `markDirty`: Mark a page as dirty
- `forcePage`: Write a specific dirty page to disk
- `getFrameContents`: Get current page numbers in the buffer pool
- `getDirtyFlags`: Get dirty flags for pages in the buffer pool
- `getFixCounts`: Get fix counts for pages in the buffer pool
- `getNumReadIO`: Get number of read operations from disk
- `getNumWriteIO`: Get number of write operations to disk

## Testing

`test_assign2_1.c` and `test_assign2_2.c` contain various test cases for the Buffer Manager. Running these executables will output the results of each test case.

## Requirements

### GCC (GNU Compiler Collection)

#### Installation:
- **Unix/Linux**:
  ```
  sudo apt-get update
  sudo apt-get install build-essential
  ```
- **Windows**: Install MinGW from [mingw.org](http://www.mingw.org/)
- **macOS**:
  ```
  brew install gcc
  ```

#### Verification:
```
gcc --version
```

### Unix-like Environment

- **Windows**: Use [Cygwin](https://www.cygwin.com/) or [WSL](https://docs.microsoft.com/en-us/windows/wsl/install)
- **macOS** and **Linux**: Native support