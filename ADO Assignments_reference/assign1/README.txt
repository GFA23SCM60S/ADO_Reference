ADO(CS525) Assignment 1: Storage Manager 

Group Number: cs525-SP24-group#10

/************************************************************************/
Synopsis:
/************************************************************************/
Our Database Management System's Storage Manager prioritizes efficient memory management. To promote responsible resource usage, we implement practices like closing file streams when necessary and releasing allocated memory space. Our approach to organizing data, managing disk space, and optimizing data access within memory constraints aims to minimize memory leaks and enhance system performance. By incorporating reliable memory allocation and deallocation techniques, we advocate for responsible memory usage, fostering system stability and reliability.


/************************************************************************/
Step by step execution:
/************************************************************************/
Note:- Before execution, make sure "make" is installed in your Linux environment 

A. From bitbucket download, repo "cs525-SP24-group#10".
B. From this Navigate to "assign1" folder
C. Open terminal in "assign1" and execute "make clean" command which will remove all the *.o old files.
D. Now execute "make" command for recompiling the project and creating the new *.o files
E. finally execute "./test1" command which will run the test file.

/************************************************************************/
Functions Used: 
/************************************************************************/

1.  Void initStorageManager (void):
	The role of this function is to initialize the 'strFilePointer' which is the default file point to NULL, which helps in removal of any prior reference or associations, and storage manage get the clean starting point. 

2.  RC createPageFile(char *strFileName):
	The role of this function is to initialize a new page file by opening it in write mode, then to make sure memory is handled properly it writes an empty page. finally it returns success code if function was executed successfully.	

3. RC openPageFile(char *strFileName, SM_FileHandle *smFileHandler)
	The role of this function is to take the freshly created file from the above funtion (createPageFile) and a structure called SM_FileHandle as parameters. The details holded by this handler are, fileName, totalNumPages, CurPagePos (initialized to 0), and mgmtInfo (a reference pointer for the file).


4. RC closePageFile(SM_FileHandle *smFileHandler)
	The role of this function is to close the file associated with the SM_FileHandle. while closing the file, storageFilePtr is set to NULL, and success code status is returned. It also manages scenarios where the file handler is not located, returning an error code indicating file not found

5. RC destroyPageFile(char *strFileName)
	The role of this function is to remove a specified page provided in the arguments of the function. success code is returned if the page was deleted successfully else returns an error code.

/************************************************************************/
READ OPERATIONS
/************************************************************************/

1. RC readBlock (int filePageNum, SM_FileHandle *smFileHandler, SM_PageHandle pagesmFileHandler): 
	The role of this function is to retrieve a block of data from the designamted page position within the file, and also updates the current page position,and also returns success code. Additionally, it manages error scenarios such as a non-existent page or a file not found by returning the corresponding error codes.


2. int getBlockPos (SM_FileHandle *smFileHandler):
	The role of this function is to retrieve the position of the current block from an open file. it returns the postion as a number if the file is open (currentFile is not NULL), else it will return 0. 

3. RC readFirstBlock (SM_FileHandle *smFileHandler, SM_PageHandle pagesmFileHandler):
	The role of this function is to read the initial block of data from thr file. By using readBlock function when the page postion is set to 0, and return the readBlock result.

4. RC readPreviousBlock (SM_FileHandle *smFileHandler, SM_PageHandle pagesmFileHandler):
	The role of this function is to determine the page position of the preceding block using the current page position in fileHandler. It then utilizes the readBlock function with this calculated position and returns the result. 


5. RC readCurrentBlock (SM_FileHandle *smFileHandler, SM_PageHandle pagesmFileHandler): 
	The role of this function is to retrieve information from the present page location within fileHandle. It acquires the current page position, utilizes the readBlock function with this position, and preserves the outcome. Furthermore, it assigns a value to a variable named checkpoint, although this does not affect the execution of the code.


6. RC readNextBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler):
	In this function, we determine the page position of the next block by utilizing the current page position stored in fileHandle. If the calculated position is non-zero, we call the readBlock function with this calculated page position and return the result. However, if the calculated position is 0, we set a value for the lastPage variable, although this action does not impact the code execution.
 


7. RC readLastBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler):
	The role of this block is to determine the position of last block by subtracting 1 from the total number of pages in filehandle. It then utilizes this page position for calling the readBlock function and retrieves the result.


/************************************************************************/
WRITE OPERATIONS
/************************************************************************/
1. RC writeBlock(int filePageNum, SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler): 
	The role of this function is to write the data of smMemPageHandler to the defined block index in the file indicated by fileHand. After conducting necessary checks and returns error code if required. On success, it updates the current page position and closes the file. 

2. RC writeCurrentBlock (SM_FileHandle *smFileHandler, SM_PageHandle smMemPageHandler):
	The role of this function is to examine for any page error and write the data of smMemPageHandler  to the current page position in the file.

3. RC appendEmptyBlock (SM_FileHandle *smFileHandler):
	The role of this function is to check for file existence, creating an empty block, and appent it to the file as a new block.

4. RC ensureCapacity (int pageCount, SM_FileHandle *smFileHandler):
	The role of this function is to ensure the provide, extra pages in the file. It appends empty blocks as needed to the file to meet the required number of pages






