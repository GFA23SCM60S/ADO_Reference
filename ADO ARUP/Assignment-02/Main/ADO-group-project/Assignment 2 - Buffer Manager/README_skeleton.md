
# Buffer Manager (Group - 9)

## Header Files
- `buffer_mgr.h`: Contains the interface and data structures for the buffer manager.
- `storage_mgr.h`: Provides functions for managing storage files.

## Code Modules
- `buffer_mgr.c`: The core implementation of the buffer manager functions.
- `storage_mgr.c`: Functions for managing storage files.

Addtional Files
--------------------------------------------------------------------------------------

Makefile

Team members
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

The goal is to implement the functions defined in `buffer_mgr.h` while ensuring correctness and efficient memory management, and to use debugging and memory checking tools to maintain code reliability and prevent memory leaks.

Instructions to run the code
----------------------------

- Go to Project root directory using Terminal.

-  Run "make clean" to delete old compiled .o files.

- Run "make" to compile all project files including "test_assign1_1.c" file 

- Run the "test_assign1" executable file.

- Run the command "valgrind --leak-check=full --track-origins=yes ./test_assign1" for checking any memory leak.
  

## Function Declarations and Definitions
Function used to implement buffer manager are described below:
### Function: initBufferPool
**Declaration:**  
```c
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
          const int numPages, ReplacementStrategy strategy, 
          void *stratData);
```

**Definition:**  
This function initializes a buffer pool, setting up essential data structures to manage a cache of pages from a file. It takes parameters like the buffer pool, the name of the page file, the number of pages to cache, the replacement strategy (e.g., FIFO, LRU), and strategy-specific data (if needed). It allocates memory for the buffer pool and page frames, initializes metadata, and prepares the pool for page management.

### Function: shutdownBufferPool
**Declaration:**  
```c
RC shutdownBufferPool(BM_BufferPool *const bm);
```

**Definition:**  
The `shutdownBufferPool` function is responsible for closing and cleaning up the buffer pool. It ensures that any dirty pages (pages with modifications) are written back to the file, and checks if any pages are still being used by clients (fix count not zero). It releases the allocated memory and performs a graceful shutdown of the buffer pool.

### Function: forceFlushPool
**Declaration:**  
```c
RC forceFlushPool(BM_BufferPool *const bm);
```

**Definition:**  
When called, this function writes all the dirty (modified) pages back to the file on disk. It iterates through the pages in the buffer pool and writes changes to the file using low-level storage management functions. This helps ensure that all modifications are saved to persistent storage.

### Function: markDirty
**Declaration:**  
```c
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
```

**Definition:**  
The `markDirty` function marks a page as "dirty," indicating that it has been modified by a client. Given a buffer pool and a page handle, it searches for the page in the buffer pool and sets its dirty bit to 1 if found. This bit is crucial for tracking changes that need to be saved to disk.

### Function: unpinPage
**Declaration:**  
```c
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
```

**Definition:**  
When a client is done using a page, it "unpins" it, decreasing the page's fix count. The function looks for the page in the buffer pool and reduces its fix count if it's found. This helps keep track of how many clients are using a page at any given time.

### Function: forcePage
**Declaration:**  
```c
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
```

**Definition:**  
This function is responsible for writing a specific page (given a page handle) back to disk if it's marked as dirty. It looks for the page in the buffer pool, and if the page is dirty, it writes the changes to the file using storage management functions.

### Function: pinPage
**Declaration:**  
```c
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
        const PageNumber pageNum);
```

**Definition:**  
The `pinPage` function is used to pin (load) a specific page into the buffer pool. It takes a buffer pool, a page handle, and a page number as input. If the page is already in the buffer pool, it increments the page's fix count to indicate another client is using it. If the page is not in the pool, it loads it from the file into an available page frame. If no frames are available, it uses a page replacement strategy (e.g., FIFO, LRU) to make space for the new page.

### Function: FIFO
**Declaration:**  
```c
 void FIFO(BM_BufferPool *const bm, PageFrame *page);
```

**Definition:**  
FIFO (First-In, First-Out) is a page replacement algorithm used to decide which page to replace when the buffer pool is full. The FIFO function searches for the oldest page in the pool (based on the rearIndex) that is not currently being used by any client (fix count is zero). If found, it replaces this page with a new one.

### Function: LRU
**Declaration:**  
```c
 void LRU(BM_BufferPool *const bm, PageFrame *page);
```

**Definition:**  
LRU (Least Recently Used) is another page replacement algorithm that selects the least recently used page to replace. The LRU function looks for the page with the lowest hitNum (indicating it was used the longest time ago) among pages with a fix count of zero and replaces it with a new page.

### Function: CLOCK
**Declaration:**  
```c
 void CLOCK(BM_BufferPool *const bm, PageFrame *page);
```

**Definition:**  
The CLOCK algorithm, also known as the second chance algorithm, uses a circular queue to choose pages for replacement. It checks pages one by one (based on the clockPointer) and gives each page a "second chance" if it's not currently being used. It replaces the first page that doesn't receive a second chance.

### Function: LFU
**Declaration:**  
```c
 void LFU(BM_BufferPool *const bm, PageFrame *page);
```

**Definition:**  
LFU (Least Frequently Used) is a page replacement algorithm that selects the page with the least number of references (refNum) to replace. It searches for the page with the lowest refNum among pages with a fix count of zero and replaces it with a new page.

These functions collectively provide the core functionality of the buffer manager, enabling efficient caching, page management, and page replacement strategies in a database system.

Output Screenshots
---------------------------------------