# Buffer Manager Project

## Project Description

This project extends Assignment-1: Storage Manager to implement a Buffer Manager for a DBMS. The Buffer Manager manages memory pages of a file, caching pages from disk to reduce I/O operations. It supports multiple buffer pools, each using a page replacement strategy to decide which page to evict when the cache is full.

### Key Features
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
5. Run the test program 1:
    ```bash
    ./test1
    ```
    OR
    ```bash
    make run_test1
    ```
6. Run the test program 2:
    ```bash
    ./test2
    ```
    OR
    ```bash
    make run_test2
    ```

## Functionalities

- **`initBufferPool`**: Initialize a buffer pool.
- **`shutdownBufferPool`**: Shut down the buffer pool.
- **`forceFlushPool`**: Force write all dirty pages to disk.
- **`pinPage`**: Load and pin a page in the buffer pool.
- **`unpinPage`**: Unpin a page.
- **`markDirty`**: Mark a page as dirty.
- **`forcePage`**: Write a specific dirty page to disk.
- **`getFrameContents`**: Get current page numbers in the buffer pool.
- **`getDirtyFlags`**: Get dirty flags for pages in the buffer pool.
- **`getFixCounts`**: Get fix counts for pages in the buffer pool.
- **`getNumReadIO`**: Get number of read operations from disk.
- **`getNumWriteIO`**: Get number of write operations to disk.

### PageFrame Structure

- `SM_PageHandle data`: Actual page data.
- `PageNumber pageNum`: Page identifier.
- `int dirtyBit`: Tracks modified pages.
- `int fixCount`: Number of clients using the page.
- `int hitNum`: Used for the LRU algorithm.
- `int refNum`: Used for LFU algorithm.

### Buffer Pool Functions

- **`initBufferPool`**: Initialize a buffer pool with parameters like page file, number of pages, and replacement strategy.
- **`shutdownBufferPool`**: Shut down the buffer pool, releasing memory and ensuring all dirty pages are flushed.
- **`forceFlushPool`**: Write all dirty pages to disk.

### Page Management Functions

- **`markDirty`**: Marks a page as dirty after modification.
- **`unpinPage`**: Decreases the fix count of a page, allowing it to be replaced.
- **`forcePage`**: Writes the contents of a modified page back to the disk.
- **`pinPage`**: Pins a page in memory, using the replacement strategy when the pool is full.

### Statistics Functions

- **`getFrameContents`**: Retrieves the page numbers in the buffer pool.
- **`getDirtyFlags`**: Returns dirty flags indicating modified pages.
- **`getFixCounts`**: Returns fix counts for each page in the buffer pool.
- **`getNumReadIO`**: Tracks the number of read operations from disk.
- **`getNumWriteIO`**: Tracks the number of write operations to disk.

### Page Replacement Algorithms

- **FIFO**: Replaces the oldest page not in use.
- **LRU**: Replaces the least recently used page.
- **CLOCK**: A circular algorithm that gives a second chance to pages.
- **LFU**: Replaces the least frequently used page.

## Testing

- `test_assign2_1.c` and `test_assign2_2.c` contain various test cases for the Buffer Manager. Running these executables will output the results of each test case.

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

