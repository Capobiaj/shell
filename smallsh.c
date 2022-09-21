// Author: Joseph Capobianco
// Date: 7/24/22
// Description: Smallsh assignment 3 - shell implementation

#define _POSIX_SOURCE

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

int cdFunc(char *input)
{
    // function handles for when cd is called as a command
    int dirChange;
    // home directory
    char *homedir = getenv("HOME");

    if(strcmp(input, "cd") == 0){
        // handles when input is only cd
        dirChange = chdir(homedir);
        if (dirChange != 0){
            fprintf(stderr, "Change directory failed 1\n");
            fflush(stdout);
            return 1;
        }
    } else {
        // handles when the function is given a path
        dirChange = chdir(input);
        if (dirChange != 0){
            fprintf(stderr, "Change directory failed 2\n");
            fflush(stdout);
            return 1;
        }
    }

    return 0;
}


int main()
/*
    # Citation for the following function: main()
    -- Adapted use of memset to clear a string, and how to clear the last character
    # Date: 7/22/22
    # Adapted memset from: https://stackoverflow.com/questions/8107826/proper-way-to-empty-a-c-string
    # Adapted clear last character with \0 from: https://stackoverflow.com/questions/18949552/removing-last-character-in-c#
*/
{
    // allow the user to input commands until the
    // exit command is recieved
    printf("$ smallsh\n");
    fflush(stdout);

    int last = 0;
    int lastExit = 0;
    int lastSig;
    pid_t pidArray[5] = {1, 1, 1, 1, 1};
    int pidArrayCount = 0;

    // loop through getting user input until exit is recieved
    for (;;)
    {
        int userMaxLine = 2048;
        //int userArgs = 512;
        char *userInput;
        char *token;
        int funcReturn;
        char *newInput;
        
        // handle for any background processes that might be running
        if (pidArrayCount != 0){
            int childStatus2;
            // loop through the array to check for background processes that 
            // are finished running
            for(int i = 0; i < sizeof pidArray; i++){
                pid_t childPid = pidArray[i];
                childPid = waitpid(childPid, &childStatus2, WNOHANG);
                // waitPid immediately returns 0 when WNOHANG is set.
                // 1 is what we have set the instance of a blank array as
                // I found that sometimes -1 will be returned on a finished
                // process so will ignore that here as well
                if (childPid == 0 || childPid == -1 || childPid == 1){
                    break;
                }
                else {
                    // Catch the exit or signal and update for status.
                    // Replace the pid in the array
                    if (WIFEXITED(childStatus2)){
                        last = 0;
                        lastExit = WEXITSTATUS(childStatus2);
                        printf("background pid %d is done: exit value %d\n", pidArray[i], lastExit);
                        fflush(stdout);
                        pidArray[i] = 1;
                        break;
                    } else{
                        last = 1;
                        lastSig = WTERMSIG(childStatus2);  
                        printf("background pid %d is done: terminated by signal %d\n", pidArray[i], lastSig);
                        fflush(stdout);
                        pidArray[i] = 1;
                        break;
                    }
                }
            }
        }        

        printf(": ");
        fflush(stdout);

        // allocate memory for user input and for possible expansion
        userInput = malloc(userMaxLine + 1);
        if (userInput == NULL){
            // out of space
            fprintf(stderr, "Error allocating memory");
            fflush(stdout);
            exit(1);
        }
        newInput = malloc(userMaxLine + 1);
        if (newInput == NULL){
            // out of space
            fprintf(stderr, "Error allocating memory");
            fflush(stdout);
            exit(1);
        }
        // get the user input
        if(fgets(userInput, userMaxLine + 1, stdin) != NULL){
            
            userInput[strlen(userInput)-1] = '\0';
            
            char *needle = "$$";
            char *haystack;
            char *haystack2;
            char *haystack3;
            int index = 0;
        
            haystack = userInput;
            
            // Use strstr to check for any instances of $$
            for(;;){
                haystack3 = haystack;

                // strstr will return a pointer to the begining of the instance
                haystack = strstr(haystack, needle);
                if (haystack == NULL){
                    // not found or end of string
                    sprintf(newInput + strlen(newInput),"%s", haystack3);
                    break;
                }

                index = strlen(haystack3) - strlen(haystack);

                // copy over the characters that are not $$ we will need for the 
                // new string
                for (int i = 0; i < index; i++){
                    newInput[i] = haystack3[i];
                }

                sprintf(newInput + strlen(newInput), "%jd", (intmax_t) getpid());
                
                haystack2 = newInput;
                haystack += 2;
            }

            // copy expanded input back into the userInput
            index = 0;
            if (strlen(newInput) != 0){
                memset(userInput, 0, sizeof userInput);
                userInput = malloc(userMaxLine + 1);
                if (userInput == NULL){
                    // out of space
                    fprintf(stderr, "Error allocating memory");
                    fflush(stdout);
                    exit(1);
                }
                strcpy(userInput, newInput);
            }

            // first token
            token = strtok(userInput, " ");

            // compare token to 3 handled shell commands (exit, cd, status)
            // compare to empty line
            if (strlen(userInput) < 1){
                continue;
            }
            else if (strncmp(token, "exit", 4) == 0){
                // handles for exit
                break;
            }
            else if (strncmp(token, "cd", 2) == 0){
                // handles for cd calls
                char *path;
                while(token != NULL){
                    path = token;
                    token = strtok(NULL, " ");
                }

                funcReturn = cdFunc(path);
                if (funcReturn != 0){
                    fprintf(stderr, "Error trying to use cd\n");
                    fflush(stdout);
                }
            }
            else if (strncmp(token, "status", 5) == 0){
                // handles for status calls
                if (last == 0){
                    printf("exit value %d\n", lastExit );
                    fflush(stdout);
                }
                else{
                    printf("terminated by signal %d\n", lastSig);
                    fflush(stdout);
                }
            }
            // compare to commented out line
            else if (strncmp(token, "#", 1) == 0){
                continue;
            }
            else {
                // process rest of userInput
                char *token1;
                char *token2;
                char *token3;
                char *token4;
                char *input = "<";
                char *output = ">";
                char *background = "&";
                char *backGroundCheck;
                char *inputSource;
                char *outputDest;
                int hasInput = 0;
                int hasOutput = 0;
                int inputSourceChange = 0;
                int outputDestChange = 0;
                int hasBackground = 0;
                int inputFile;
                int outputFile;
                int inputSet = 0;
                int outputSet = 0;
                char *stringArray[512];
                int tokenCount = 0;

                // token1 is the command
                token1 = token;
                
                // Add command to first element of array for execvp
                stringArray[tokenCount] = token1;
                tokenCount += 1;

                // tokenize the input and check for background and redirects
                while(token != NULL){
                    if (hasInput == 1 ){
                        inputSource = token;
                        hasInput = 0;
                        inputSourceChange = 1;
                    }
                    else if (hasOutput == 1){
                        outputDest = token;
                        hasOutput = 0;
                        outputDestChange = 1;
                    }
                    else if (strcmp(token, input) == 0){
                        hasInput = 1;
                    }
                    else if (strcmp(token, output) == 0){
                        hasOutput = 1;
                    }
                    else if (strcmp(token, token1) != 0){
                        stringArray[tokenCount] = token;
                        tokenCount += 1;
                    }
                    backGroundCheck = token;
                    token = strtok(NULL, " ");
                }
                // Check if we need to handle for a background process
                if (strcmp(backGroundCheck, background) == 0){
                    hasBackground = 1;
                    stringArray[tokenCount-1] = NULL;
                }

                // Have the last entry as null for execvp
                stringArray[tokenCount] = NULL;

                // perform exec and fork 
                int childStatus;

                // fork new process
                pid_t spawnPid = fork();

                switch(spawnPid){
                case -1:
                    // child process ran into an error
                    perror("fork()\n");
                    exit(1);
                    last = 0;
                    lastExit = 1;
                    break;

                case 0:
                    // In the child process
                    // If there is a redirection of input or output
                    // we will open the designated destination or source
                    if (inputSourceChange == 1){
                        inputFile = open(inputSource, O_RDONLY);
                        if (inputFile < 0){
                            perror("open()");
                            last = 0;
                            lastExit = 1;
                            exit(1);
                        }
                        inputSet = 1;
                    }

                    if (outputDestChange == 1){
                        outputFile = open(outputDest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (outputFile < 0){
                            perror("open()");
                            exit(1);
                            last = 0;
                            lastExit = 1;
                        }
                        outputSet = 1;
                    }
                    // if the output or input isnt set for a background
                    // process it will do so on /dev/null
                    if (hasBackground == 1 && inputSet == 0){
                        inputFile = open("/dev/null", O_RDONLY);
                        if (inputFile < 0){
                            perror("open()");
                            last = 0;
                            lastExit = 1;
                            exit(1);
                        }
                        inputSet = 1;
                    }

                    if (hasBackground == 1 && outputSet == 0){
                        outputFile = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (outputFile < 0){
                            perror("open()");
                            exit(1);
                            last = 0;
                            lastExit = 1;
                        }
                        outputSet = 1;
                    }
                    
                    // Redirect stdout or stdin
                    int resultIn;
                    int resultOut;
                    if (inputSet == 1){
                        resultIn = dup2(inputFile, 0);
                        if (resultIn == -1){
                            perror("source dup2()");
                            exit(2);
                            last = 0;
                            lastExit = 2;
                        }
                    }

                    if (outputSet == 1){
                        resultOut = dup2(outputFile, 1);
                        if (resultOut == -1){
                            perror("source dup2()");
                            exit(2);
                            last = 0;
                            lastExit = 2;
                        }
                    }
                    
                    
                    // Replace current program with token command
                    // exec will return if there is an error
                    execvp(token1, stringArray);
                    perror("exec function");
                    exit(1);
                    last = 0;
                    lastExit = 1;
                    break;
                
                default:
                    // In the parent process

                    // If the command should be running in the background
                    if (hasBackground == 1){
                        // when running as a background process the parent will not wait for the child
                        // and will return access to cli for the user
                        printf("background pid is %d\n", spawnPid);
                        fflush(stdout);
                        for (int x = 0; x < sizeof pidArray; x++)
                            // Pid of -1 will represent a space that was utilized by a finished process
                            // and 2496 represents an unused segment of the pidArray
                            if (pidArray[x] == -1 || pidArray[x] == 1){
                                pidArray[x] = spawnPid;
                                pidArrayCount += 1;
                                break;
                            }
                        break;
                    }
                    else{
                        // Wait for child's termination
                        spawnPid = waitpid(spawnPid, &childStatus, 0);
                        
                        // Set exit status if normal exit
                        if (WIFEXITED(childStatus)){
                            last = 0;
                            lastExit = WEXITSTATUS(childStatus);
                            break;
                        // Set exit status if abnormal exit
                        } else{
                            last = 1;
                            lastSig = WTERMSIG(childStatus);
                            break;   
                        }
                    }
                }
            }
        }
        if (ferror(stdin) != 0)
        {
            // error indicator set on fgets
            perror("Error: ");
            exit(1);
        }
    }
    printf("$\n");
    fflush(stdout);
    // send a kill signal to all remaining children
    for (int z = 0; z < 5; z++){
        if (pidArray[z] == 1){
            continue;
        }
        else{
            kill(pidArray[z], SIGKILL);
        }
    }
    exit(0);
}