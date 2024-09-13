/*Took file from Andres create function*/
/*Arup is working on this file: This is the code for oepn function for file manipulation*/
#include <stdio.h>
#include<stdlib.h>

int main() {
    FILE *file;
    char filename[100];

    printf("Enter the name of the file to open: ");
    scanf("%s", filename); //can use "%[^\n]" instead of "%s" to read file name with spaces"

   

    
 // Open the file in read mode
    file = fopen(filename, "r+"); // can add a mode check here too*/

    // Open the file with the default application (Windows)(if needed)
/*char openCommand[200];
sprintf(openCommand, "start %s", filename); // Windows command to open a file
system(openCommand);*/


    // file open failure check
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return 1;
    } 

    printf("File opened successfully.\n");

    /*Rahul will work here*/
    


    return 0;
}

 
