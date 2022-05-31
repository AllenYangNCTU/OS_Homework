#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

#define INPUT_BUFFER_SIZE 256

int main() {
  char input[INPUT_BUFFER_SIZE];

  cout << ">" << flush;
  while(fgets(input, INPUT_BUFFER_SIZE, stdin) != NULL){

		// Discard the last new-line character
		*strchr(input, '\n') = '\0';
    
    // Get number of arguments (determined by num of spaces)
		// (include the last NULL pointer)
		// ex: "ls -l"
		// will turned to: ["ls", "-l", NULL], 3 arguments
    int numArguments = 2;
    char *pos = input;
    while(*pos++ != '\0'){
      if(*pos == ' '){
        ++numArguments;
      }
    }

    // Split input string into string arrays deliminate by spaces
    char** argumentArray = (char**)malloc(numArguments * sizeof(char*));

			// First substring starts at the beginning of s
		argumentArray[0] = input;

			// Find other substrings
		int i = 1;
		char* hit = input;
		while((hit = strchr(hit, ' ')) != NULL) { // Find next delimiter
    		// In-place replacement of the delimiter
    	*hit++ = '\0';
    		// Next substring starts right after the hit
    	argumentArray[i++] = hit;
		}
		// Last argument of exec() should be NULL
		argumentArray[numArguments-1] = NULL;


		// Check whether input is ended with "&"
		bool shouldWait = true;
		if(strcmp(argumentArray[numArguments-2], "&") == 0){
			shouldWait = false;
			// "&" should not be feed into exec(), so replace it by NULL
			argumentArray[numArguments-2] = NULL;
		}

		// Check whether there is a IO redirection
		bool shouldRedirect = false;
    char redirectFileName[20]; 
		if(numArguments > 3 && strcmp(argumentArray[numArguments-3], ">") == 0){
			shouldRedirect = true;
      strcpy(redirectFileName, argumentArray[numArguments-2]);
			// ">" and the following filename should not be feed into exec()
      // so replace it by NULL
			argumentArray[numArguments-3] = NULL;
    }

    // Check pipe

    
    // This means where pipe character ('|') appears in argumentArray
    // -1 means no pipe
    int pipeIndex = -1;
    
    char **secondArgumentArray;

    if (!shouldRedirect) {
      // Find pipe index
      for(int i = 1; i < numArguments-2; ++i){
        if(strcmp(argumentArray[i], "|") == 0){
          pipeIndex = i;
          // Replace "|" to NULL, seperate commands
          argumentArray[i] = NULL;
          secondArgumentArray = &argumentArray[i+1];
          break;
        }
      }
    }


    if (pipeIndex > 0) { 
      // Should pipe
      int pfd[2];
      if (pipe(pfd) < 0) {
        cout << "Pipe error" << endl;
        return -1;
      }

      int pid1 = fork();
      if (pid1 == 0) {
        // child 1
        dup2(pfd[1], 1);  // redirect stdout to pipe output
        close(pfd[0]);    // close pipe input
        close(pfd[1]);    // close pipe output
         if (execvp(argumentArray[0], argumentArray) < 0){
          cout << "Exec error" << endl;
        }
        exit(0);
      } else {
        // parent
        int pid2 = fork();
        if (pid2 == 0) {
          // child 2
          dup2(pfd[0], 0);  // redirect stdin to pipe input
          close(pfd[0]);    // close pipe input
          close(pfd[1]);    // close pipe output
          if (execvp(secondArgumentArray[0], secondArgumentArray) < 0){
            cout << "Exec error" << endl;
          }
          exit(0);
        } else {
          // still parent
          close(pfd[0]);    // close pipe input
          close(pfd[1]);    // close pipe output
          if(wait(&pid1) < 0){
            cout << "Wait error" << endl;
          }
          if(wait(&pid2) < 0){
            cout << "Wait error" << endl;
          }
        }
      }
    } else {
      // No pipe, normal fork
      int childpid = fork();
      if (childpid == 0){
        // child process
        if (!shouldWait) { // don't need to wait
          // fork twice to prevent zombie process
          if (fork() != 0){
            // mid-child, exit immediately
            exit(1);
          }
        }
        // child or grandchild
        if (shouldRedirect){
          int fd = open(redirectFileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
          dup2(fd, 1); // redirect stdout to file
          dup2(fd, 2); // redirect stderr to file
          close(fd);
        }
        if (execvp(argumentArray[0], argumentArray) < 0){
          cout << "Exec error" << endl;
        }
        exit(0);
      } else {
        //parent process
        if(wait(&childpid) < 0){
          cout << "Wait error" << endl;
        }
      }
    }

		free(argumentArray);
    cout << ">" << flush;
  }

  return 0;
}
