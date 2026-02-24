#include <stdio.h>
#include <syslog.h>
#include <string.h>


/* writer.c
*  
*  Takes as command line parameters:
*     filesdir:  a filename (including path) 
*     searchstr: a string to write. 
*
*  Purpose: Writes the string searchstr to the file at location filesdir.
*
*  Requirements: The file directory must exist before this is executed.
*     If an existing file of the same name exists, it will be overwritten.
*/

int main(int argc, char* argv[]) {

   int MAX_STRING_SIZE = 4096;
   char filesdir[MAX_STRING_SIZE];
   char searchstr[MAX_STRING_SIZE];
   char debugString[MAX_STRING_SIZE];
   FILE* filesdirPtr;
   
   openlog(NULL, 0, LOG_USER);

   // Check the number of parameters
   if (argc != 3) {
      syslog(LOG_DEBUG, "Wrong number of parameters. 2 required. (1) Filename with path (2) String to print");
      return 1;
   }

   // Read in arguments
   strncpy(filesdir, argv[1], MAX_STRING_SIZE);
   strncpy(searchstr, argv[2], MAX_STRING_SIZE);
   
   // Open the file. Creates it if it doesn't exist. Log any error.
   filesdirPtr = fopen(filesdir, "w");
   if (filesdirPtr == NULL) {
      syslog(LOG_DEBUG, "File could not be opened for writing.");
      return 1;
   }
   
   // write string file, close the file, and note the event in the syslog
   fprintf(filesdirPtr, searchstr);
   fclose(filesdirPtr);
   snprintf(debugString, sizeof(debugString), "Writing string %s to file %s", searchstr, filesdir);
   syslog(LOG_DEBUG, debugString);

   
}
