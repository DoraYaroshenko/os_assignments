#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void finishProgram(int inputFileDescriptor,int outputFileDescriptor, char* buffer){
    if(buffer!=NULL){
        free(buffer);
    }
    if (inputFileDescriptor!=-1){
        int res = close(inputFileDescriptor);
        if (res==-1){
            perror("Error closing the input file");
        }
    }
    if (outputFileDescriptor!=-1){
        int res = close(outputFileDescriptor);
        if (res==-1){
            perror("Error closing the output file");
        }
    }
}

void errorHandling(const char* errorMessage, int inputFileDescriptor,int outputFileDescriptor, char* buffer){
    perror(errorMessage);
    finishProgram(inputFileDescriptor,outputFileDescriptor,buffer);
    exit(1);
}

void copyFile(int inputFileDescriptor,int outputFileDescriptor,char* buffer, int bufferSize){
    ssize_t bytesRead;
    while ((bytesRead=read(inputFileDescriptor,buffer,bufferSize))>0){
        ssize_t writtenBytes = write(outputFileDescriptor,buffer,bytesRead);
        if(writtenBytes==-1)
        errorHandling("Error writing to output file",inputFileDescriptor,outputFileDescriptor,buffer);
    }
    if (bytesRead==-1)
        errorHandling("Error reading from input file",inputFileDescriptor,outputFileDescriptor,buffer);
}

int main(int argc,char* argv[]){
    if (argc!=4){
        printf("Invalid number of arguments\n");
        exit(1);
    }
    int bufferSize;
    int result = sscanf(argv[3],"%d",&bufferSize);
    if (result!=1){
        printf("Error parsing n\n");
        exit(1);
    }
    if (bufferSize<1){
        printf("Invalid buffer size\n");
        exit(1);
    }
    int inputFileDescriptor = open(argv[1],O_RDONLY);
    if (inputFileDescriptor==-1)
        errorHandling("Error opening input file",-1,-1,NULL);
    
    int outputFileDescriptor = open(argv[2],O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR | S_IXUSR);
    if (outputFileDescriptor==-1)
        errorHandling("Error creating output file",inputFileDescriptor,-1,NULL);
    
    char* buffer = (char*)malloc(bufferSize);
    if(buffer==NULL)
        errorHandling("Failed to allocate memory",inputFileDescriptor,outputFileDescriptor,NULL);
    
    copyFile(inputFileDescriptor,outputFileDescriptor,buffer,bufferSize);
    finishProgram(inputFileDescriptor,outputFileDescriptor,buffer);
    return 0;
}