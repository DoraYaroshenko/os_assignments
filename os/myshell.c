#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

int handleWait(pid_t childPid, int *exitCode)
{
    while (waitpid(childPid, exitCode, 0) == -1)
    {
        if (errno == EINTR)
            continue;
        if (errno != ECHILD)
        {
            perror("waitpid");
            return 0;
        }
        break;
    }
    return 1;
}

int executeCommand(int count, char **arglist)
{
    int exitCode = -1;
    pid_t childPid = fork();
    if (childPid > 0)
    {
        if (handleWait(childPid, &exitCode) == 0)
            return 0;
    }
    else
    {
        // In shell we ignore SIGINT, but in process children we want to catch them, therefore added SIG_DFL
        if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        {
            perror("signal");
            exit(1);
        };
        execvp(arglist[0], arglist);
        perror("execvp");
        exit(1);
    }
    return 1;
}

int executeInBackground(int count, char **arglist)
{
    pid_t pid = fork();
    arglist[count - 1] = NULL;
    if (pid == 0)
    {
        execvp(arglist[0], arglist);
        perror("execvp");
        exit(1);
    }
    return 1;
}

int countPipes(int count, char **arglist)
{
    int numOfPipes = 0;
    for (int i = 0; i < count; i++)
    {
        if (!strcmp(arglist[i], "|"))
            numOfPipes++;
    }
    return numOfPipes;
}

int createPipes(int numOfPipes, int pipes[numOfPipes][2])
{
    for (int i = 0; i < numOfPipes; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe");
            return 0;
        }
    }
    return 1;
}

void closePipes(int numOfPipes, int pipes[numOfPipes][2])
{
    for (int j = 0; j < numOfPipes; j++)
    {
        if (close(pipes[j][0]) == -1 || close(pipes[j][1]) == -1)
        {
            perror("close");
            exit(1);
        }
    }
}

void executePipe(char **arglist, int numOfPipes, int pipes[numOfPipes][2], int numOfCommands, int commandStart, int i, int j)
{
    // In shell we ignore SIGINT, but in process children we want to catch them, therefore added SIG_DFL
    if (signal(SIGINT, SIG_DFL) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }
    if (j > 0 && dup2(pipes[j - 1][0], 0) == -1)
    {
        perror("dup2");
        exit(1);
    }
    if (j < numOfPipes && dup2(pipes[j][1], 1) == -1)
    {
        perror("dup2");
        exit(1);
    }
    closePipes(numOfPipes, pipes);
    arglist[i] = NULL;
    execvp(arglist[commandStart], arglist + commandStart);
    perror("execvp");
    exit(1);
}

int executePiping(int count, char **arglist)
{
    int numOfPipes = countPipes(count, arglist);
    int numOfCommands = numOfPipes + 1;
    if (numOfCommands < 2 || numOfCommands > 10)
    {
        perror("Incorrect number of commands");
        return 1;
    }
    int commandStart = 0;
    pid_t children[numOfCommands];
    int pipes[numOfPipes][2];
    if (createPipes(numOfPipes, pipes) == 0)
        return 0;
    for (int i = 0, j = 0; i < count + 1; i++)
    {
        if (arglist[i] == NULL || !strcmp(arglist[i], "|"))
        {
            pid_t child = fork();
            if (child < 0)
            {
                perror("fork");
                return 0;
            }
            else if (child == 0)
                executePipe(arglist, numOfPipes, pipes, numOfCommands, commandStart, i, j);
            else
            {
                commandStart = i + 1;
                children[j] = child;
            }
            j++;
        }
    }
    closePipes(numOfPipes, pipes);
    int exitCode;
    for (int j = 0; j < numOfCommands; j++)
    {
        if (handleWait(children[j], &exitCode) == 0)
            return 0;
    }
    return 1;
}

int inputRedirecting(int count, char **arglist)
{
    pid_t childPid = fork();
    int exitCode = -1;
    if (childPid < 0)
    {
        perror("fork");
        return 0;
    }
    if (childPid == 0)
    {
        // In shell we ignore SIGINT, but in process children we want to catch them, therefore added SIG_DFL
        if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        {
            perror("signal");
            exit(1);
        }
        int fileDesc = open(arglist[count - 1], O_RDONLY, S_IRUSR | S_IWUSR);
        if (fileDesc < 0)
        {
            perror("open");
            exit(1);
        }
        if (dup2(fileDesc, 0) == -1)
        {
            perror("dup2");
            exit(1);
        };
        if (close(fileDesc) == -1)
        {
            perror("close");
            exit(1);
        }
        arglist[count - 2] = NULL;
        execvp(arglist[0], arglist);
        perror("execvp");
        exit(1);
    }
    else
    {
        if (handleWait(childPid, &exitCode) == 0)
            return 0;
    }
    return 1;
}

int outputRedirecting(int count, char **arglist)
{
    pid_t childPid = fork();
    int exitCode = -1;
    if (childPid < 0)
    {
        perror("fork");
        return 0;
    }
    if (childPid == 0)
    {
        // In shell we ignore SIGINT, but in process children we want to catch them, therefore added SIG_DFL
        if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        {
            perror("signal");
            exit(1);
        }
        int file_desc = open(arglist[count - 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (file_desc < 0)
        {
            perror("open");
            exit(1);
        }
        if (dup2(file_desc, 1) == -1)
        {
            perror("dup2");
            exit(1);
        }
        if (close(file_desc) == -1)
        {
            perror("close");
            exit(1);
        }
        arglist[count - 2] = NULL;
        execvp(arglist[0], arglist);
        perror("execvp");
        exit(1);
    }
    else
    {
        if (handleWait(childPid, &exitCode) == 0)
            return 0;
    }
    return 1;
}

int checkPiping(int count, char **arglist)
{
    for (int i = 0; i < count; i++)
    {
        if (!strcmp(arglist[i], "|"))
        {
            return 1;
        }
    }
    return 0;
}

// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char **arglist)
{
    int result = 1;
    if (!strcmp(arglist[count - 1], "&"))
    {
        result = executeInBackground(count, arglist);
    }
    else if (count > 1 && !strcasecmp(arglist[count - 2], "<"))
        result = inputRedirecting(count, arglist);
    else if (count > 1 && !strcasecmp(arglist[count - 2], ">"))
        result = outputRedirecting(count, arglist);
    else
    {
        result = checkPiping(count, arglist) ? executePiping(count, arglist) : executeCommand(count, arglist);
    }
    if (result == 0)
        perror("An error has occured");
    return result;
}

//Preventing zombies
void sigchld_handler(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
}

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void)
{
    //We want to ignore SIGINT in parent process
    signal(SIGINT, SIG_IGN);
    //Preventing zombies
    struct sigaction newAction = {.sa_handler = sigchld_handler};
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &newAction, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }
    return 0;
}

int finalize(void)
{
    return 0;
}
