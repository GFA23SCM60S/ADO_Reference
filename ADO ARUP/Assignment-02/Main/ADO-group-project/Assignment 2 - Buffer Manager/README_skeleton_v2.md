
# Buffer Manager (Group - 9)

## Header Files
- `buffer_mgr.h`: Contains the interface and data structures for the buffer manager.
- `storage_mgr.h`: Provides functions for managing storage files.

## Code Modules
- `buffer_mgr.c`: The core implementation of the buffer manager functions.
- `storage_mgr.c`: Functions for managing storage files.

Addtional Files
--------------------------------------------------------------------------------------

`Makefile`

Team members // Rahul Edit
---------------------------------------
1) Arup Chauhan - createPageFile(), openPageFile(), closePageFile()
2) Rahul Mansharamani - destroyPageFile(), readBlock(), getBlockPos()
3) Andres Ghersi - readFirstBlock(), readLastBlock(), readPreviousBlock()
4) Vedant Chaubey - readCurrentBlock(), writeBlock(), writeCurrentBlock()
5) Swastik Patro - appendEmptyBlock(), ensureCapacity(), freePh()



Aim:
----------------------------------------------------------------------------------------
The aim of this assignment is to build a Buffer Manager capable of handling memory pages representing pages from a file. 

It should support multiple buffer pools, each using a specific page replacement strategy, such as FIFO (First in first out) and LRU (Last recently used). 

We aim to implement the functions defined in `buffer_mgr.h` while ensuring correctness and efficient memory management, and to use debugging and memory checking tools to maintain code reliability and prevent memory leaks.

Instructions to run the code // Rahul edit
----------------------------

- Go to Project root directory using Terminal.

-  Run "make clean" to delete old compiled .o files.

- Run "make" to compile all project files including "test_assign1_1.c" file 

- Run the "test_assign1" executable file.

- Run the command "valgrind --leak-check=full --track-origins=yes ./test_assign1" for checking any memory leak.

## Structures

### PageFrame 

Represents a page frame in the buffer pool (memory).

- `SM_PageHandle data`: Actual data of the page.
- `PageNumber pageNum`: An identification integer given to each page.
- `int dirtyBit`: Used to indicate whether the contents of the page have been modified by the client.
- `int fixCount`: Used to indicate the number of clients using that page at a given instance.
- `int hitNum`: Used by LRU algorithm to get the least recently used page.
- `int refNum`: Used by LFU algorithm to get the least frequently used page.

  

## Function Declarations and Definitions
Function used to implement buffer manager are described below:



### Buffer Pool Functions 
### Function: initBufferPool

This function initializes a buffer pool with the specified parameters. It allocates memory for the buffer pool and initializes its page frames. The `pageFileName` parameter is the name of the page file being cached. `numPages` specifies the number of page frames in the buffer pool, and `strategy` indicates the page replacement strategy (e.g., FIFO, LRU). `stratData` can be used to pass additional data for a specific strategy.

### Function: shutdownBufferPool

This function shuts down a buffer pool, releasing all allocated memory. It ensures that there are no pinned pages in the buffer pool before shutting down. Any dirty pages are written back to disk.

### Function: forceFlushPool


The `forceFlushPool` function writes any dirty pages in the buffer pool back to the disk. This ensures that all modified pages are safely stored on disk.

### Page Management Functions 
### Function: markDirty



The `markDirty` function marks a page as dirty, indicating that its contents have been modified by the client. This is necessary to track which pages need to be written back to disk.

### Function: unpinPage
The `unpinPage` function unpins a page from memory. It decreases the fix count of the page, allowing it to be removed from the buffer pool when its fix count reaches zero.
### Function: forcePage


The `forcePage` function writes the contents of a modified page back to the page file on disk. It ensures that any changes made to the page in memory are persisted to disk.

### Function: pinPage


The `pinPage` function pins a page in the buffer pool, making it available for use by the client. If the buffer pool is full, it uses the specified page replacement strategy to replace an existing page.

### Statistics Functions 
### Function: getFrameContents


The `getFrameContents` function returns an array of page numbers currently stored in the buffer pool. It provides insight into which pages are present in memory.

### Function: getDirtyFlags


The `getDirtyFlags` function returns an array of boolean flags, with each flag indicating whether a corresponding page in the buffer pool is dirty (modified) or clean.

### Function: getFixCounts

The `getFixCounts` function returns an array of fix counts, one for each page frame in the buffer pool. Fix counts represent the number of clients currently using each page.

### Function: getNumReadIO


The `getNumReadIO` function returns the number of pages read from disk since the buffer pool was initialized. It helps track I/O operations.

### Function: getNumWriteIO

The `getNumWriteIO` function returns the number of pages written to the page file on disk since the buffer pool was initialized. It helps track I/O operations.

### Page Replacement Algorithms 
### Function: FIFO
  
FIFO (First-In, First-Out) is a page replacement algorithm used to decide which page to replace when the buffer pool is full. The FIFO function searches for the oldest page in the pool (based on the rearIndex) that is not currently being used by any client (fix count is zero). If found, it replaces this page with a new one.

### Function: LRU
 
LRU (Least Recently Used) is another page replacement algorithm that selects the least recently used page to replace. The LRU function looks for the page with the lowest hitNum (indicating it was used the longest time ago) among pages with a fix count of zero and replaces it with a new page.

### Function: CLOCK
  
The CLOCK algorithm, also known as the second chance algorithm, uses a circular queue to choose pages for replacement. It checks pages one by one (based on the clockPointer) and gives each page a "second chance" if it's not currently being used. It replaces the first page that doesn't receive a second chance.

### Function: LFU
  
LFU (Least Frequently Used) is a page replacement algorithm that selects the page with the least number of references (refNum) to replace. It searches for the page with the lowest refNum among pages with a fix count of zero and replaces it with a new page.

These functions collectively provide the core functionality of the buffer manager, enabling efficient caching, page management, and page replacement strategies in a database system.

Output Screenshots
---------------------------------------