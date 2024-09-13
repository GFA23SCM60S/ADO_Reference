			              #Storage Manager

Project Modules
--------------------------------------------------------------------------------------
C source files: storage_mgr.c, dberror.c, test_assign1_1.c,test_assign1_2.c
Header files: storage_mgr.h, dberror.h, test_helper.h
Additional files: Makefile

Team members
---------------------------------------
1) Arup Chauhan
2) Rahul Mansharamani
3) Vedant Chaubey
4) Andres Ghersi
5) Swastik Patro



Aim:
----------------------------------------------------------------------------------------
The goal of this assignment is to implement a storage manager - a module that 
is capable of reading blocks from a file on disk into memory and writing blocks from 
memory to a file on disk. The storage manager deals with pages (blocks) of fixed 
size (PAGE_SIZE). 

As part of our assignment:
We have written functions to peform file creation using create function and file manipulation using open, close, read, write and append functions respectively.

----NEED TO EDIT THIS---

Read functions can also read current block ,next block, previous block etc.

Instructions to run the code
----------------------------
1) Go to the path where the extracted files are present.
2) Run the below command:
   make
3) Run the below command for testing test_assign1.c:
   make run
4) To remove object files, run following command:
   make clean
5) We have provided additional testcases in file "test_assign1_2.c 
   To run those testcases use the below command:
   ./test_assign1_2
  
Functions:
------------
Function used to implement storage manager are described below:

initStorageManager()
------------------------------
Initializes the program.

createPageFile()
--------------------------
Create function is used to create a file and fill it with values, addtionally it is also handling an edge case where a file is already existing, returning an error flag [flag name to be added]

openPageFile()
--------------------
Open function is used to:
Open an already existing file and give it's file handling to the variable pageFile. 
Subsequently adding the file information to the variable: ADD THE VARIABLE NAME
Handling the edge case of failing to open the file, returning an error flag.

closePageFile()
---------------
Close function is used to close the already opened file. The function is handling the edge case the opened is not getting closed, return an error flag [FLAG NAME TO BE ADDED]

destroyPageFile()
-------------------
Removes the existing file and returns the success flag [FLAG NAME TO BE ADDED].The function is handling the edge case the is not getting removed, return an error flag [FLAG NAME TO BE ADDED]

readBlock()
----------------

Moves the  file descriptor to a particular block from beginning of the file and reads data of 1 page size (4096 bytes)and loads it to the memory specified by mempage.Finds out the current page position using 'ftell' and stores it in 'curPagePos' for further operations.
Returns error code if: i)File descriptor is not initialised, ii)file does not exist and is unable to open in 'read' mode ,iii)The page accessed is out of bound for the file , iv)Unable to read the given page.

getBlockPos()
--------------------
Finds out current page position using file descriptor. Returns error code if file handler is not intialised or file  failed to open in 'read' mode. 

readFirstBlock ()
----------------------
Moves the file descriptor in file handler to the beginning of the file and reads 1st page contents into 'mempage'. Returns error code if: i) i)File descriptor is not initialised, ii)file does not exist or is unable to open in 'read' mode  iii)Unable to read the given page.

readPreviousBlock ()
------------------------

Moves the file descriptor in filehandler to the previous page with respect to current page postion and  reads the content of the previous page to buffer specified by mempage.
Returns error code if: i) i)File handler is not initialised, ii)file does not exist or is unable to open in 'read' mode iii) Current page position in file handler points to non-existing page  iv)Unable to read the given page.

readCurrentBlock ()
----------------------
Sets the  file descriptor in file handler to current page position and reads the page content to 'mempage'.
Returns error code if: i) i)File handler is not initialised, ii)file does not exist or is unable to open in 'read' mode iii) Current page position in file handler points to non-existing page  iv)Unable to read the given page.

readNextBlock ()
--------------------

Sets the  file descriptor in file handler to next page position and reads the page content to 'mempage'.
Returns error code if: i) File handler is not initialised, ii)file does not exist or is unable to open in 'read' mode iii) Current page position in file handler points to non-existing page  iv)Unable to read the given page.

readLastBlock ()
-------------------

Sets the  file descriptor in file handler to last page  and reads the page content to buffer specified by 'mempage'.
Returns error code if: i) i)File handler is not initialised, ii)file does not exist or is unable to open in 'read' mode iii) Current page position in file handler points to non-existing page  iv)Unable to read the entire given page.

writeBlock()
-------------
Checks if file is present and moves the file pointer to the specific page from the beginning of the file and writes 1 page to it.
File handle variable are updated. 

Returns error code if: i) i)File handler is not initialised, ii)file does not exist or is unable to open in 'read' mode iii) Current page position in file handler points to non-existing page  iv)Unable to write the entire given page.

writeCurrentBlock()
------------------------
Checks if file is present and moves the file pointer to current page from the beginning of the file and writes 1 page to it.
File handle variable are updated. 

Returns error code if: i)File does not exist or is unable to open in 'read' mode iii) Current page position in file handler points to non-existing page  iv)Unable to write the entire given page.

appendEmptyBlock()
----------------------

Checks if file is present and can be opened in 'read' mode .
If these conditions are satisfied, then the existing file is opened in 'read+' mode to write the data. File pointer is set to the end of the file and one page is added with character '\0' in newly added empty block.
File handle variable are updated. 

Returns error code if: i)File does not exist or is unable to open in 'read' mode 

ensureCapacity()
--------------------
Checks if the file has capacity to to accomodate numberofpages. If the file does not sufficient pages, using 'appendEmptyBlock' function, add more pages.
Returns error code if: i)File handle is null ii)File does not exist or is unable to open in 'read' mode iii) Current page position in file handler points to non-existing page  

Test Cases:
-------------
Few more test cases are defined in 'test_assign1_2.c'.
Following fuctions are used:

testCreateOpenClose(void)
--------------------------
Creates, opens, close and destroys page file. If file is opened after being destroyed , must return error.

testSinglePageContent(void)
----------------------------
Creates, opens and reads the newly initialized page. Block will be empty. Then the block is written and read again.

testMultiPageContent(void)
----------------------------
Checks the contents of multiple pages by using the phases same as 'testSinglePageContent'.
