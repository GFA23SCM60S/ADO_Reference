# Storage Manager (Group - 9)

Project Modules
--------------------------------------------------------------------------------------
C source files: storage_mgr.c, dberror.c, test_assign1_1.c,test_assign1_2.c
Header files: storage_mgr.h, dberror.h, test_helper.h
Additional files: Makefile

Team members
---------------------------------------
1) Arup Chauhan - createPageFile(), openPageFile(), closePageFile()
2) Rahul Mansharamani - destroyPageFile(), readBlock(), getBlockPos()
3) Andres Ghersi - readFirstBlock(), readLastBlock(), readPreviousBlock()
4) Vedant Chaubey - readCurrentBlock(), writeBlock(), writeCurrentBlock()
5) Swastik Patro - appendEmptyBlock(), ensureCapacity(), freePh()



Aim:
----------------------------------------------------------------------------------------
Implemented a storage manager designed to efficiently read blocks from a disk file into memory and write blocks from memory back to the disk.
The storage manager deals with pages (blocks) of fixed size (`PAGE_SIZE`). 

As part of our assignment:
We have written functions to peform file creation using create function and file manipulation using open, close, read, write and append functions respectively.

Instructions to run the code
----------------------------

- Go to Project root directory using Terminal.

-  Run "make clean" to delete old compiled .o files.

- Run "make" to compile all project files including "test_assign1_1.c" file 

- Run the "test_assign1" executable file.

- Run the command "valgrind --leak-check=full --track-origins=yes ./test_assign1" for checking any memory leak.
  
Functions:
------------
Function used to implement storage manager are described below:

initStorageManager()
------------------------------
Initializes the program.

createPageFile()
--------------------------
The `createPageFile` function is designed to create a new page file with the specified `fileName.` It first attempts to open the file in read mode to check if it already exists. If the file exists, it returns an error code `RC_FILE_ALREADY_EXISTS.` If the file doesn't exist or cannot be opened, it proceeds to open the file in write mode, creating an empty file for writing. It allocates memory for a buffer filled with null characters (size equal to `PAGE_SIZE`), writes this buffer to the file to initialize it, and checks if it wrote the full `PAGE_SIZE` characters, returning `RC_WRITE_FAILED` if not. After successfully creating the file, it checks if the file exists again and verifies that its content consists of null characters, ensuring it was generated correctly. If any checks fail during the process, appropriate error codes are returned. If everything is successful, the function returns `RC_OK`, indicating the successful creation of the page file.

openPageFile()
--------------------
The `openPageFile` function is designed to open an existing page file specified by `fileName` and initialize the `SM_FileHandle` struct `fHandle`. It attempts to open the file in read mode ('r'), and if successful, it updates the `fHandle` with the file's `fileName` and sets the current page position to the start (0). To determine the total number of pages, the function uses the `fstat` system call to obtain information about the file, specifically its total size in bytes. The total number of pages is then calculated by dividing the file size by `PAGE_SIZE`. If the file cannot be opened or if there is an error obtaining file information, it returns `RC_FILE_NOT_FOUND` or `RC_ERROR` accordingly. Finally, the file stream is closed to ensure all buffers are flushed, and the function returns `RC_OK` upon successful completion.

closePageFile()
---------------
The `closePageFile` function is responsible for closing a page file that corresponds to the given `SM_FileHandle` struct, `fHandle`. It begins by checking if the file pointer (`pageFile`) or the storage manager is initialized. If either is initialized, it sets the file pointer to `NULL`, effectively closing the file. This function is a straightforward operation to ensure proper cleanup of file resources. It then returns `RC_OK` to indicate successful execution.

destroyPageFile()
-------------------

The `destroyPageFile` function deletes a file specified by `fileName`. It checks if the file exists by attempting to open it in read mode. If the file does not exist or cannot be opened, it returns an error code. If the file exists, it is closed using `fclose`, and then the function attempts to delete the file using `remove`. If the deletion operation fails, it returns an error code. If the file is successfully deleted or does not exist, it returns a success code.

getBlockPos()
--------------------
The function `getBlockPos` is an external function that takes a pointer to a `SM_FileHandle` struct as its parameter. It returns an integer value representing the current page position within the file. This function retrieves the `curPagePos` member variable from the `fHandle` struct and returns its value. The `curPagePos` variable keeps track of the current page position within the file. 

readBlock()
----------------

The readBlock function is designed to read a specific block (page) from a file and store it in a memory page. It takes three parameters: `pageNum`, which represents the page number to be read; `fHandle`, a pointer to the `SM_FileHandle` structure that contains information about the file; and `memPage`, a pointer to the memory page where the block will be stored.


readFirstBlock ()
----------------------
This function is a wrapper for the `readBlock` function. It reads the first block (page 0) of a file into the memory page provided. The `SM_FileHandle` parameter is a structure that contains information about the file, such as the current page position and the total number of pages. The `SM_PageHandle` parameter is a pointer to a memory page where the block will be stored. The function returns an RC (Return Code) indicating whether the operation was successful or not.


readPreviousBlock ()
------------------------

This function is also a wrapper for the `readBlock` function. It reads the block (page) that comes before the current page in the file into the memory page provided. The current page position is obtained from the `SM_FileHandle` structure. The function calculates the current page number by dividing the current page position by the page size. Then, it subtracts 1 from the current page number to get the previous page number. The `readBlock` function is called with the previous page number to read the block into the memory page. The function returns an RC indicating the success or failure of the operation.


readCurrentBlock ()
----------------------
This function is another wrapper for the `readBlock` function. It reads the current block (page) of the file into the memory page provided. The current page position is obtained from the `SM_FileHandle` structure. The function calculates the current page number by dividing the current page position by the page size. The `readBlock` function is called with the current page number to read the block into the memory page. The function returns an RC indicating the success or failure of the operation.


readNextBlock ()
--------------------

Similar to the previous functions, this is a wrapper for the `readBlock` function. It reads the block (page) that comes after the current page in the file into the memory page provided. The current page position is obtained from the `SM_FileHandle` structure. The function calculates the current page number by dividing the current page position by the page size. Then, it adds 1 to the current page number to get the next page number. The `readBlock` function is called with the next page number to read the block into the memory page. The function returns an RC indicating the success or failure of the operation.

readLastBlock ()
-------------------

This function is also a wrapper for the `readBlock` function. It reads the last block (page) of the file into the memory page provided. The `SM_FileHandle` structure contains information about the file, including the total number of pages. The `readBlock` function is called with the page number `totalNumPages - 1` to read the last block into the memory page. The function returns an RC code indicating the success or failure of the operation.


writeBlock()
-------------
The `writeBlock` function is defined as an external function that takes three parameters: an integer `pageNum`, a pointer to a `SM_FileHandle` struct `fHandle`, and a `SM_PageHandle` `memPage`. It returns an RC code indicating the success or failure of the write operation. First, the function checks if the `pageNum` is less than 0 or greater than the total number of pages in the file. If either condition is true, it returns an error code indicating a write failure. Next, the function opens the file specified by `fHandle->fileName` in read-write mode. If the file cannot be opened (i.e., `pageFile` is `NULL`), it returns an error code indicating that the file was not found. If the file is successfully opened, the function uses `fseek` to set the file position indicator to the appropriate position based on `pageNum` and `PAGE_SIZE`. It then writes the contents of `memPage` to the file using `fwrite`. After writing, it updates the `curPagePos` member variable of `fHandle` with the current file position using `ftell`. Finally, it closes the file and returns a success code.

writeCurrentBlock()
------------------------
The function `writeCurrentBlock` is also defined as an external function that takes two parameters: a pointer to a `SM_FileHandle` struct `fHandle` and a `SM_PageHandle memPage`. It first calculates the current page number by dividing `fHandle->curPagePos` by `PAGE_SIZE`. Then, it increments the `totalNumPages` member variable of `fHandle` by 1. Finally, it calls the `writeBlock` function with the current page number, `fHandle`, and `memPage` as arguments and returns the result.

appendEmptyBlock()
----------------------

The `appendEmptyBlock` function adds an empty block to the end of a file. It takes a file handle (`fHandle`) as input. First, it creates a new empty page (`emptyBlock`) of size `PAGE_SIZE` bytes. It then moves the cursor position to the end of the file using `fseek`. If successful, it writes the contents of `emptyBlock` to the file using `fwrite`. If the write operation fails, it returns the error code `RC_WRITE_FAILED`. The function then frees the memory previously allocated to `emptyBlock` using `free`. It increments the total number of pages (`totalNumPages`) in the file by one. Finally, it returns the success code `RC_OK`.

ensureCapacity()
--------------------
The `ensureCapacity` function ensures that a file has a minimum number of pages specified by `numberOfPages`. It opens the file in append mode, and if the file is not found, it returns an error code. It then checks if the current total number of pages is less than `numberOfPages`. If so, it repeatedly calls the `appendEmptyBlock` function to add empty blocks until the desired capacity is reached. Finally, it closes the file and returns a success code.

freePh()
--------------------
The `freePh` function frees the additional memory if there is one.

Output Screenshots
---------------------------------------

Snapshot of the test case

![alt text](./output%20screenshots/Test%20Case%20File%201.jpeg)

Memory Leak Check - 

![alt text](./output%20screenshots/Memory%20Leak%20Check.jpeg)