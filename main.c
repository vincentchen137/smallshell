/*
* Vincent Chen
* CS 344 - Portfolio Assignment: smallshell
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

volatile sig_atomic_t bgMode = 0;

// Function prototypes
void catchBGTermination(int* execPid, int* childExitMethod, int* bgProcess);
void catchSIGTSTP(int signo);
void catchSIGINT(int signo);
void getInput(char* input[], size_t size, int* numArgs, int parentPid, int* background, char inputFile[], char outputFile[], int* execPid, int* childExitMethod, int* bgProcess);
void execute(int* childExitMethod, int parentPid, int* execPid, char** input, int* background, int* bgProcess, char inputFile[], char outputFile[]);
void outputExitStatus(int childExitMethod);


// To run type in "gcc --std=gnu99 -o smallshell main.c" and then "./smallshell"
int main()
{
    int flag = 1;           // flag to control the while loop
    int background = 0;     // 0 means not a background process
    // int backgroundMode = 0; // 0 means turned
    int bgProcess = 0;      // tracks if there are current background processes running
    int childExitMethod;    // to be used for waitpid parameter
    int parentPid = getpid();
    // printf("Main method parentPid (%d)\n", parentPid);
    size_t size = 2048;      // size for length of command line characters
    int maxArgs = 512;       // max size for arguments
    int numArgs = 0;         // counter for number of arguments user passed in 

    pid_t execPid;           // pid taken from exec

    // home for cd command
    char* home;
    home = getenv("HOME");

    // char* input[MAXARGS];
    char* input[maxArgs];
    // set up input to be filled with NULL
    for (int i = 0; i < maxArgs; i++)
    {
        input[i] = NULL;
    }
    char inputFile[256] = "";
    char outputFile[256] = "";

    // signal handlers
    
    // ignore SIGINT
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = SIG_IGN;     // ignore action handler
    sigfillset(&SIGINT_action.sa_mask);      
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    while(flag == 1)
    {
        if(bgProcess == 1){
            catchBGTermination(&execPid, &childExitMethod, &bgProcess);
        }
        getInput(input, size, &numArgs, parentPid, &background, inputFile, outputFile, &execPid, &childExitMethod, &bgProcess);

        if(bgProcess == 1){
            catchBGTermination(&execPid, &childExitMethod, &bgProcess);
        }
        // '#' or blank line
        while (input[0] == NULL){
            if(bgProcess == 1){
                catchBGTermination(&execPid, &childExitMethod, &bgProcess);
            }
            getInput(input, size, &numArgs, parentPid, &background, inputFile, outputFile, &execPid, &childExitMethod, &bgProcess);
        }
        // cd 
        if (strcmp(input[0], "cd") == 0)
        {
            // cd home
            if (input[1] == NULL || strcmp(input[1],"") == 0 || strcmp(input[1]," ") == 0)
            {
                
                chdir(home);
            }
            // cd specified directory
            else {
                int dirInt;
                dirInt = chdir(input[1]);
                // wrong directory or file path
                if (dirInt == -1)
                {
                    continue;
                }
            }
        }

        //  status
        else if (strcmp(input[0], "status") == 0)
        {
            outputExitStatus(childExitMethod);
        }
        
        // exit
        else if (strcmp(input[0], "exit") == 0)
        {
            flag = 0;
        }
        // all other commands will use exec
        else 
        {
            execute(&childExitMethod, parentPid, &execPid, input, &background, &bgProcess, inputFile, outputFile);
        }

        // reset input using numArgs that was passed into getInput
        for (int i =0; i < numArgs; i++)
        {
            input[i] = NULL;
        }
        // reset variables to be used for next user input
        background = 0;
		inputFile[0] = '\0';
		outputFile[0] = '\0';
    }
    return 0;
}

void catchBGTermination(int* execPid, int* childExitMethod, int* bgProcess)
{
    // catch background termination
    int exitStatus;
    int childExit = *childExitMethod;
    pid_t pid = *execPid;
    while ((pid = waitpid(-1, &childExit, WNOHANG)) > 0) { 
        // normal termination
        if (WIFEXITED(childExit) != 0)
        {
            exitStatus = WEXITSTATUS(childExit);
            printf("background pid %d is done: terminated by signal %d\n", pid, exitStatus);
            fflush(stdout);
        } // signal termination
        else if (WIFSIGNALED(childExit) != 0)
        {
            exitStatus = WTERMSIG(childExit);
            printf("background pid %d is done: terminated by %d\n", pid, exitStatus);
            fflush(stdout);
        }
    }
    *childExitMethod = childExit;
    *execPid = pid;
    // *bgProcess = 0;
}

void catchSIGTSTP(int signo)
{
    // if (bgMode == 0) {
    bgMode = 1;
    char* message = "Entering foreground-only mode (& is now ignore)\n";
    write(STDOUT_FILENO, message, 49);
    // } else if (bgMode == 1) {
    //     bgMode = 0;
    //     char* message = "Exiting foreground-only mode\n";
    //     write(STDOUT_FILENO, message, 30);
    //     main();
    // }
}

void catchSIGINT(int signo)
{
    char* message = "Caught SIGINT!\n";
    write(STDOUT_FILENO, message, 15);
}

void getInput(char* input[], size_t size, int* numArgs, int parentPid, int* background, char inputFile[], char outputFile[], int* execPid, int* childExitMethod, int* bgProcess)
{
    // // handle SIGTSTP
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigemptyset(&SIGTSTP_action.sa_mask);   
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);  

    // catch background termination before prompting user
    int exitStatus;
    int childExit = *childExitMethod;
    pid_t pid = *execPid;
    while ((pid = waitpid(-1, &childExit, WNOHANG)) > 0) { 
        // normal termination
        if (WIFEXITED(childExit) != 0)
        {
            exitStatus = WEXITSTATUS(childExit);
            printf("background pid %d is done: terminated by signal %d\n", pid, exitStatus);
            fflush(stdout);
        } // signal termination
        else if (WIFSIGNALED(childExit) != 0)
        {
            exitStatus = WTERMSIG(childExit);
            printf("background pid %d is done: terminated by signal %d\n", pid, exitStatus);
            fflush(stdout);
        }
    }
    *childExitMethod = childExit;
    *execPid = pid;

    // print prompt and get user input
    printf(": ");
    fflush(stdout);
    char userInput[size];
    fgets(userInput, size, stdin);

    // handle SIGTSTP
    int sigCounter = 0;
    while (bgMode == 1 && sigCounter < size){
        if(userInput[sigCounter] == '&')
        {
            userInput[sigCounter] = '\0';
        }
        sigCounter += 1;
    }
    // Handle '\n' from input
    int flag = 1;
    int counter = 0;
    while(flag && counter < size)
    {
        if(userInput[counter] == '\n')
        {
            userInput[counter] = '\0';
            flag = 0;
        }
        counter += 1;
    }

    // blank line
    if (userInput[0] == ' ') {
        return;
        *bgProcess = 1;
    }
    // is '&' the last arg in the line
    int bgCounter = counter - 2;
    if (userInput[bgCounter] == '&')
    {
        *background = 1;     // background flag now set to true
    }

    // Tokenize
    char *saveptr;          // for use with strtok_r
    char* token = strtok_r(userInput, " ", &saveptr);
    int i = 0;              // to be used to put tokens back into input[i] 
    int count = 0;
    while(token)
    {
        switch (token[0]){
            case '#':      // comment line
                return;
            case '&':       // background process
                // *background = 1;        // set the background flag to true
                break;
            case '<':       // input redirection
                token = strtok_r(NULL, " ", &saveptr);
                strcpy(inputFile, token);
                break;
            case '>':       // output redirection
                token = strtok_r(NULL, " ", &saveptr);
                strcpy(outputFile, token);
                break;
            default:
                input[i] = strdup(token);       // duplicate token into input
                // strcpy(input[i], token);        // whill this work as well?
                // handle $$ variable expansion
                if (strcmp(input[i], "$$") == 0)
                {
                    // replace $$ with pid
                    sprintf(input[i], "%d", getpid());
                }
                i += 1;
        }
        token = strtok_r(NULL, " ", &saveptr);
        *numArgs += 1;
    }
}

// void defaultSIGINT()
// {
//     struct sigaction SIGINT_action = {0};
//     SIGINT_action.sa_handler = SIG_DFL;     
//     sigfillset(&SIGINT_action.sa_mask);      
//     SIGINT_action.sa_flags = SA_RESTART;
//     sigaction(SIGINT, &SIGINT_action, NULL);
// }

// void ignoreSIGINT()
// {
//     struct sigaction SIGINT_action = {0};
//     SIGINT_action.sa_handler = SIG_IGN;     
//     sigfillset(&SIGINT_action.sa_mask);      
//     SIGINT_action.sa_flags = SA_RESTART;
//     sigaction(SIGINT, &SIGINT_action, NULL);
// }

void execute(int* childExitMethod, int parentPid, int* execPid, char** input, int* background, int* bgProcess, char inputFile[], char outputFile[])
{
    int childExit;
    childExit = *childExitMethod;       // to be used during waitpid
    int exitStatus;                     // stores actual exit status after child termination    

    int inputFD;                        // to be used with input redirect
    int outputFD;                       // to be used with output redirect
    int result;                         // dup2()

    int bg = *background;           

    pid_t childPid;
    childPid = fork();
    switch(childPid)
    {
        case -1:
            perror("fork() failed!\n");
            exit(1);
            break;
        case 0:
            // SIGTSTP - ignore on all children processes
            // struct sigaction SIGTSTP_action = {0};
            // SIGTSTP_action.sa_handler = SIG_IGN;     
            // sigfillset(&SIGTSTP_action.sa_mask);      
            // SIGTSTP_action.sa_flags = 0;
            // sigaction(SIGINT, &SIGTSTP_action, NULL);
            if (bg == 0)
            {    
                struct sigaction SIGINT_action = {0};
                SIGINT_action.sa_handler = &catchSIGINT;     
                sigfillset(&SIGINT_action.sa_mask);      
                SIGINT_action.sa_flags = SA_RESTART;
                sigaction(SIGINT, &SIGINT_action, NULL);
                // if (raise(SIGINT))
                // {
                //     pause();
                // }
            if (bg == 1)
            {
                struct sigaction SIGINT_action = {0};
                SIGINT_action.sa_handler = SIG_IGN;     
                sigfillset(&SIGINT_action.sa_mask);      
                SIGINT_action.sa_flags = 0;
                sigaction(SIGINT, &SIGINT_action, NULL);
            }
            // input redirection - redirect to specified file
            if (strcmp(inputFile, "")) {
                int inputFD = open(inputFile, O_RDONLY, 0777);      // read only
                if (inputFD == -1) {
                    // perror("cannot open %s for input\n", inputFile);
                    printf("cannot open %s for input\n", inputFile);
                    fflush(stdout);
                    exit(1);
                } else {
                    result = dup2(inputFD, 0);
                }
                fcntl(inputFD, F_SETFD, FD_CLOEXEC);     // close file descriptor
            }
            // output redirection - redirect to specified file
            if (strcmp(outputFile, "")) {
                // write only, created if it doesnt exist, truncate if it already exists
                int outputFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                if (outputFD == -1) {
                    // perror("cannot open %s for output\n", outputFile);
                    printf("cannot open %s for output\n", outputFile);
                    fflush(stdout);
                    exit(1);
                } else {
                    result = dup2(outputFD, 1);
                }
                fcntl(outputFD, F_SETFD, FD_CLOEXEC);   // close file descriptor
            }
            }
            if (execvp(*input, input) < 0){
                printf("%s: no such file or directory\n", *input);
                exit(1);
            }
            // following statements will execute if execvp failed
            perror("Exec failed!");
            exit(2);
            break;
        default:
            if (bg == 1){
                pid_t backgroundPid = waitpid(childPid, &childExit, WNOHANG);
                *execPid = childPid;       // update execPid
                *bgProcess = 1;
                printf("background pid is %d\n", childPid);
                fflush(stdout);
            } else if (bg == 0){
                pid_t foregroundPid = waitpid(childPid, &childExit, 0);
                // *execPid = foregroundPid;       // update execPid
            }
            // normal termination
            if (WIFEXITED(childExit) != 0)
            {
                exitStatus = WEXITSTATUS(childExit);
            } // signal termination
            else if (WIFSIGNALED(childExit) != 0)
            {
                exitStatus = WTERMSIG(childExit);
            }
            break;
    }
    *childExitMethod = exitStatus;
}

void outputExitStatus(int childExitMethod)
{
    int exitStatus;
    // normal termination
    if (WIFEXITED(childExitMethod))
    {
        exitStatus = WEXITSTATUS(childExitMethod);
        printf("exit value %d\n", exitStatus);
        fflush(stdout);
    } // abnormal(signal) termination
    else if (WIFSIGNALED(childExitMethod))
    {
        exitStatus = WTERMSIG(childExitMethod);
        printf("exit value %d\n", exitStatus);
        fflush(stdout);
    }
}