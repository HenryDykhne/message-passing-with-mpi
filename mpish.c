#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>

const int MAX_INPUT_CHARS = 4096;
enum Commands {put, get, ls, lls, lrm, end, unknown};

void getInput(int * commSz, enum Commands * command, char * fileName[], int * procNum, char *** args) {
    char * endPtr = NULL;
    char * temp;
    char input[MAX_INPUT_CHARS];
    *fileName = NULL;
    *procNum = -1;
    *command = unknown;

    printf("mpish> ");
    fflush(stdout);

    while(!fgets(input, sizeof(input), stdin));
    input[strcspn(input,  "\r\n")] = 0;
    printf("\n");
    temp = strtok(input, " ");
    if(temp == NULL) {
        return;
    }
    //command is assigned at the end of parsing to ensure rest of string is ok
    if(strcmp(temp, "put") == 0) {
        *fileName = strtok(NULL, " ");
        if(*fileName == NULL || strcmp(*fileName, "") == 0) {
            return;
        }
        temp = strtok(NULL, " ");
        printf("tempcheck: %s\n", temp);
        if(temp != NULL && strcmp(temp, "") != 0) {
            *procNum = strtol(temp, &endPtr, 10);
            if(temp == endPtr || *procNum <= 0 || *procNum > (*commSz - 1)){
                *procNum = -1;
                return;
            }
        }
        
        *command = put;
    } else if (strcmp(temp, "get") == 0) {
        fflush(stdout);
        *fileName = strtok(NULL, " ");
        if(*fileName == NULL || strcmp(*fileName, "") == 0) {
            return;
        }
        *command = get;
    } else if (strcmp(temp, "ls") == 0) {
        *command = ls;
    } else if (strcmp(temp, "lls") == 0) {
        *command = lls;
    } else if (strcmp(temp, "lrm") == 0) {
        char ** res  = NULL;
        int numSpaces = 0;
        while (temp) {
            res = realloc (res, sizeof (char*) * ++numSpaces);
            res[numSpaces-1] = temp;
            temp = strtok (NULL, " ");
        }

        /* realloc one extra element for the last NULL */
        res = realloc (res, sizeof (char*) * (numSpaces+1));
        res[numSpaces] = NULL;
        *args = res;
        *command = lrm;
    } else if (strcmp(temp, "exit") == 0) {
        *command = end;
    }
}

void printUsage() {
    printf("USAGE:\n");
    printf("put <filename> processNumber(optional)\n");
	printf("get <filename>\n");
	printf("ls\n");
    printf("lls\n");
    printf("lrm <list of files to remove>\n");
    printf("exit\n");
}

void storageProcess(int *myRank, int *commSz, bool *endProg) {
    enum Commands command;
    MPI_Status status;
    // MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    // MPI_Get_count(&status, MPI_CHAR, &number_amount);
    // status.MPI_TAG
    MPI_Recv(&command, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("%d\n", command);
    switch (command){
        case put:
            // statements
            break;

        case get:
            // statements
            break;

        case ls:
            // statements
            break;

        case lls:
            // statements
            break;

        case lrm:
            // statements
            break;

        case end:
            *endProg = true;
            break;

        default:
            printf("Could not understand command.\n");
            printUsage();
    }
}

void shellPut(int *commSz){

}

void shellGet(enum Commands command, int *commSz){
    for (int i = 1; i < *commSz; i++) {
        
    }
}

void shellLs(enum Commands command, int *commSz){
    for (int i = 1; i < *commSz; i++) {
        
    }
}

void shellEnd(enum Commands command, int *commSz){
    for (int i = 1; i < *commSz; i++) {
        MPI_Send(&command, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
}

void runLs(){
    char* argv[] = {"ls", NULL};
    pid_t childPid;

    childPid = fork();
    if(childPid == 0) {
        execvp(argv[0], argv);
    } else {
        wait(NULL);
    }
}

void runRm(char *** argv){
    pid_t childPid;

    childPid = fork();
    if(childPid == 0) {
        execvp(*argv[0], *argv);
    } else {
        wait(NULL);
    }
}


void shellProcess(int *commSz, bool *endProg) {
    enum Commands command;
    char **argv;
    char *fileName;
    int procNum;
    char *baseFileName;
    getInput(commSz, &command, &fileName, &procNum,  &argv);
    baseFileName = basename(fileName);
    printf("%d | %s | %s | %d\n", command, fileName, baseFileName, procNum);
    switch (command){
        case put:
            // statements
            break;

        case get:
            // statements
            break;

        case ls:
            //MPI_Send(&command, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            break;

        case lls:
            runLs();
            break;

        case lrm:
            runRm(&argv);
            free(*argv);
            break;

        case end:
            shellEnd(command, commSz);
            *endProg = true;
            break;

        default:
            printf("Could not understand command.\n");
            printUsage();
    }
}

int main(void) {
    //char input[MAX_INPUT_CHARS];
    //fgets(input, sizeof(input), stdin); 
    int myRank, commSz;

    /* Let the system do what it needs to start up MPI */
    MPI_Init(NULL, NULL);

    /* Get my process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    /* Find out how many processes are being used */
    MPI_Comm_size(MPI_COMM_WORLD, &commSz);

    if(myRank == 0) {
        printf("\n");
    }
        
    bool endProg = false;


    while(!endProg){
        if(myRank == 0) {
            shellProcess(&commSz, &endProg);
        }else{
            storageProcess(&myRank, &commSz, &endProg);
        }
    }

    /* Shut down MPI */
    MPI_Finalize();

   return 0;
}