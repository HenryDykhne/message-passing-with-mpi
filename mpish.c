#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <dirent.h> 

const int MAX_INPUT_CHARS = 4096;
enum Commands {put, get, ls, lls, lrm, checkHave, end, unknown};
int lastSent = 1;

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
        if(temp != NULL && strcmp(temp, "") != 0) {
            *procNum = strtol(temp, &endPtr, 10);
            if(temp == endPtr || *procNum <= 0 || *procNum > (*commSz - 1)){
                *procNum = -1;
                return;
            }
        }
        
        *command = put;
    } else if (strcmp(temp, "get") == 0) {
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

        res = realloc(res, sizeof (char*) * ++numSpaces);
        res[numSpaces-1] = "rm";
        temp = strtok(NULL, " ");

        while (temp) {
            res = realloc(res, sizeof (char*) * ++numSpaces);
            res[numSpaces-1] = temp;
            temp = strtok(NULL, " ");
        }

        /* realloc one extra element for the last NULL */
        res = realloc(res, sizeof (char*) * (numSpaces+1));
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

void storageProcGet(int myRank){
    int length;
    MPI_Status status;
    //recive fileName
    MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &length);
    char * fileName = (char*)malloc(sizeof(char) * length);
    MPI_Recv(fileName, length, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //sendFile
    MPI_File  file;
    struct stat st;

    char filePath[MAX_INPUT_CHARS];
    char strNum[99]; 
    sprintf(strNum, "%d", myRank); 
    strcpy(filePath, "p");
    strcat(filePath, strNum);
    strcat(filePath, "/");
    strcat(filePath, fileName);

    stat(filePath, &st);
    int size = st.st_size;
    unsigned char * contents = (unsigned char*)calloc(sizeof(unsigned char), size);
 
    MPI_File_open(MPI_COMM_SELF, filePath, MPI_MODE_RDWR, MPI_INFO_NULL, &file);
    MPI_File_read(file, contents, size, MPI_BYTE, MPI_STATUS_IGNORE);
    MPI_Send(contents, size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    MPI_File_close(&file);
}

void storageProcPut(int myRank){
    int length;
    MPI_Status status;

    //fileName
    MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &length);
    char * fileName = (char*)malloc(sizeof(char) * length);
    MPI_Recv(fileName, length, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //fileContents
    MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_BYTE, &length);
    unsigned char * content = (unsigned char*)calloc(sizeof(unsigned char), length);
    MPI_Recv(content, length, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    char filePath[MAX_INPUT_CHARS];
    char strNum[99]; 
    sprintf(strNum, "%d", myRank); 
    strcpy(filePath, "p");
    strcat(filePath, strNum);
    strcat(filePath, "/");
    strcat(filePath, fileName);
    
    MPI_File file;
    MPI_File_open(MPI_COMM_SELF, filePath, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &file);
    MPI_File_write(file, content, length, MPI_BYTE, MPI_STATUS_IGNORE);
    MPI_File_close(&file);
    free(content);
    free(fileName);
}

void sendCheckHaveFile(int myRank) {
    int length;
    MPI_Status status;
    MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &length);
    char * fileName = (char*)malloc(sizeof(char) * length);
    MPI_Recv(fileName, length, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    char filePath[MAX_INPUT_CHARS];
    char strNum[99]; 
    sprintf(strNum, "%d", myRank); 
    strcpy(filePath, "p");
    strcat(filePath, strNum);
    strcat(filePath, "/");
    strcat(filePath, fileName);

    int response;
    if(access(filePath, F_OK ) == -1 ) {
        response = -1;
    } else {
        response = myRank;
    }
    MPI_Send(&response, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

void storageProcLs(int myRank) {
    char directoryName[100];
    char strNum[99]; 
    sprintf(strNum, "%d", myRank); 
    strcpy(directoryName, "p");
    strcat(directoryName, strNum);

    struct dirent *de;  // Pointer for directory entry 
    char output[MAX_INPUT_CHARS];
    DIR *dr = opendir(directoryName); 
    strcpy(output, "");
    while ((de = readdir(dr)) != NULL) {
        if(strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, ".") != 0){
            strcat(output, "        ");
            strcat(output, de->d_name);
            strcat(output, "\n");
        }
    }
    closedir(dr); 
    MPI_Send(output, strlen(output) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
}

void storageProcess(int *myRank, int *commSz, bool *endProg) {
    enum Commands command;
    MPI_Status status;
    // MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    // MPI_Get_count(&status, MPI_CHAR, &number_amount);
    // status.MPI_TAG
    MPI_Recv(&command, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    switch (command){
        case put:
            storageProcPut(*myRank);
            break;

        case get:
            storageProcGet(*myRank);
            break;

        case ls:
            storageProcLs(*myRank);
            break;
        case checkHave:
            sendCheckHaveFile(*myRank);
            break;

        case end:
            *endProg = true;
            break;

        default:
            printf("Could not understand command.\n");
            printUsage();
    }
}

int whoHasFile(int commSz, char * baseFileName){
    enum Commands command = checkHave;
    int buff = -1;
    int loc = -1;
    for (int i = 1; i < commSz; i++) {
        //telling what the process needs to do
        MPI_Send(&command, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(baseFileName, strlen(baseFileName) + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
    }
    for (int i = 1; i < commSz; i++) {
        MPI_Recv(&buff, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(buff != -1) {
            loc = i;
            buff = -1;
        }
    }
    return loc;
}

void shellPut(int *commSz, enum Commands command, char * fileName, char * baseFileName, int procNum){
    if( access(fileName, F_OK ) == -1 ) {
        printf("File does not exist");
        return;
    }
    
    if(procNum == -1){
        procNum = whoHasFile(*commSz, baseFileName);
        if(procNum == -1){
            procNum = lastSent;
            lastSent = (lastSent) % (*commSz - 1) + 1;
        }
    }

    //telling what the process needs to do
    MPI_Send(&command, 1, MPI_INT, procNum, 0, MPI_COMM_WORLD);
    
    //sendTitle
    MPI_Send(baseFileName, strlen(baseFileName) + 1, MPI_CHAR, procNum, 0, MPI_COMM_WORLD);

    //sendFile
    MPI_File  file;
    MPI_Status status;
    struct stat st;

    stat(fileName, &st);
    int size = st.st_size;
    unsigned char * contents = (unsigned char*)calloc(sizeof(unsigned char), size);
    char filePath[MAX_INPUT_CHARS];
    strcpy(filePath, fileName);
    MPI_File_open(MPI_COMM_SELF, filePath, MPI_MODE_RDWR, MPI_INFO_NULL, &file);
    MPI_File_read(file, contents, size, MPI_BYTE, MPI_STATUS_IGNORE);
    MPI_Send(contents, size, MPI_BYTE, procNum, 0, MPI_COMM_WORLD);
    MPI_File_close(&file);
}

void shellGet(int *commSz, enum Commands command, char * baseFileName){
    int fileLoc = whoHasFile(*commSz, baseFileName);
    if(fileLoc == -1){
        printf("The file could not be found in the system\n");
        return;
    }

    //telling what the process needs to do
    MPI_Send(&command, 1, MPI_INT, fileLoc, 0, MPI_COMM_WORLD);

    //sendTitle
    MPI_Send(baseFileName, strlen(baseFileName) + 1, MPI_CHAR, fileLoc, 0, MPI_COMM_WORLD);
    
    //recive fileContents
    MPI_File file;
    MPI_Status status;
    int length;
    MPI_Probe(fileLoc, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_BYTE, &length);
    unsigned char * content = (unsigned char*)calloc(sizeof(unsigned char), length);
    MPI_Recv(content, length, MPI_BYTE, fileLoc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //write file localy
    char filePath[MAX_INPUT_CHARS];
    strcpy(filePath, baseFileName);
    MPI_File_open(MPI_COMM_SELF, filePath, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &file);
    MPI_File_write(file, content, length, MPI_BYTE, MPI_STATUS_IGNORE);
    MPI_File_close(&file);
    free(content);
}

void shellLs(int *commSz, enum Commands command) {
    char output[MAX_INPUT_CHARS];
    strcpy(output, "");
  
    for (int i = 1; i < *commSz; i++) {
        MPI_Send(&command, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    for (int i = 1; i < *commSz; i++) {
        int length;
        MPI_Status status;
        MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_CHAR, &length);
        char * directoryContents = (char*)malloc(sizeof(char) * length);
        MPI_Recv(directoryContents, length, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        char strNum[99]; 
        sprintf(strNum, "%d", i); 
        strcat(output, "    p");
        strcat(output, strNum);
        strcat(output, "\n");
        strcat(output, directoryContents);
        free(directoryContents);
    }
    printf("%s\n", output);
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
    switch (command){
        case put:
            shellPut(commSz, command, fileName, baseFileName, procNum);
            break;

        case get:
            shellGet(commSz, command, baseFileName);
            break;

        case ls:
            shellLs(commSz, command);
            break;

        case lls:
            runLs();
            break;

        case lrm:
            runRm(&argv);
            free(argv);
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
    } else {
        char str[100];
        char strNum[99]; 
        sprintf(strNum, "%d", myRank); 
        strcpy(str, "p");
        strcat(str, strNum);
        mkdir(str, 0777);
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