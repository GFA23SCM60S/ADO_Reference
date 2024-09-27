Project Description
=========================
This project is an extension of Assignment-1: Storage Manager. This project aims to implement a Buffer Manager for managing the memory pages of a file in a DBMS. The buffer manager handles a fixed number of pages (called frames) and caches pages from the disk to reduce I/O operations. The buffer manager supports multiple buffer pools, with each buffer pool using a page replacement strategy to decide which page to evict when the cache is full.
The functionalities implemented include Buffer pool management, Page Management (Page pinning and Unpinning), Page Replacement, Handling Dirty Pages, Concurrency support, Statistics Tracking and Error Handling and Debugging. 
The Page Replacement functionality implements the strategies of FIFO (First In First Out) and LRU(Least Recently Used). 

File Structure
=========================
README.md : Contains a brief description of your solution and how to use it.
Makefile : Build file for compiling the project.
buffer_mgr.h : Header file for buffer manager functions and data structures.
buffer_mgr_stat.c : Source file for functions that print statistics about the buffer pool.
buffer_mgr_stat.h : Header file for buffer manager statistics.
dberror.c : Implementation of error handling functions.
dberror.h : Header file for error handling.
dt.h : Data types file (boolean definitions and others).
storage_mgr.h : Header file for storage manager (from assignment 1).
test_assign2_1.c : Test cases for buffer manager (FIFO and LRU strategies).
test_helper.h : Helper functions for the test program.

Compilation and Execution
=========================
To compile and run the project, follow these steps:
Ensure that all the necessary files are in the same directory (as listed above).
Open a terminal or command prompt in this directory.
Build the project using the provided Makefile by running:
make
The Makefile will compile the program and generate the test binaries. The expected output file will be named test_assign2_1 for the first set of tests.
After successful compilation, execute the program with:
./test_assign2_1
This will run the test cases specified in test_assign2_1.c to validate the FIFO and LRU strategies for the Buffer Manager.




Functionalities Included
=========================
In addition to Assignment-1, the functionalities included are:
initBufferPool: Initializes a buffer pool for managing pages from a page file with a given replacement strategy.
shutdownBufferPool: Shuts down the buffer pool and ensures all resources are released, writing dirty pages to disk.
forceFlushPool: Forces all dirty pages in the buffer pool to be written to disk.
pinPage: Loads a page from the disk into the buffer pool and pins it for use.
unpinPage: Unpins a page, making it available for eviction if necessary.
markDirty: Marks a page as dirty, indicating it has been modified and needs to be written to disk before eviction.
forcePage: Writes a specific dirty page back to disk from memory.
getFrameContents: Returns the list of page numbers currently stored in the buffer pool.
getDirtyFlags: Returns a list of flags indicating which pages in the buffer pool are dirty.
getFixCounts: Retrieves the fix count (number of pins) for each page in the buffer pool.
getNumReadIO: Returns the number of read operations from disk since buffer pool initialization.
getNumWriteIO: Returns the number of write operations to disk since buffer pool initialization.

Testing
=========================
The test_assign2_1.c file contains tests for different functionalities of the Buffer Manager. Running the executable will execute these tests and output the results, indicating the success or failure of each test case.

Requirements
=========================
# GCC (GNU Compiler Collection)
GCC is an essential tool for compiling C programs. It's widely used and supports various standards of C. 

# Installation:
- **Unix/Linux**: GCC is usually pre-installed or can be installed via the package manager. For Ubuntu-based systems, use:
    `sudo apt-get update`
    `sudo apt-get install build-essential`
    
- **Windows**: GCC can be installed using MinGW. Download and install from [MinGW](http://www.mingw.org/). After installation, add the MinGW bin directory (e.g., `C:\MinGW\bin`) to your system's PATH environment variable.

- **macOS**: GCC can be installed using Homebrew. If Homebrew is not installed, visit [brew.sh](https://brew.sh/) to install it, then run:
    `brew install gcc`

# Verification:
To verify that GCC is installed, open a terminal or command prompt and type:
    `gcc --version`

This should display the version of GCC installed on your system.

# Unix-like Environment
A Unix-like environment is required for running the `.c` and `.exe` files, especially for certain shell commands and scripts.

- **Windows**: Use [Cygwin](https://www.cygwin.com/) or [WSL (Windows Subsystem for Linux)](https://docs.microsoft.com/en-us/windows/wsl/install) for a Unix-like environment.

- **macOS** and most **Linux distributions** already provide a Unix-like environment.

# Additional Notes
- Ensure all file paths in the source code and commands are correctly set based on your directory structure. Relative paths are preferred for portability.
- If modifications to the source files are necessary, understand the impact on other parts of the project. Testing is essential to ensure functionality remains intact.


