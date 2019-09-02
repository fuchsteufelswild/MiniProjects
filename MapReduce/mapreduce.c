/*
==========================================
 Title:  MapReduce
 Author: Mehmet Alper CANAL
 Date:   16.03.2019
==========================================
    
All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEFAULT_LENGTH 1025 // Default length for buffer

// Prototypes of functions
int sizeFinder(int numb);
void StringGenerator(char str[], int numb);
char* GetInputFromStdin();
int Power(int numb, int pow);
int StringToInt(char *str);
char* FileNameRegulator(char *str);
void Map(char *strN, char *mapProgram);
void MapReduce(char *strN, char *mapProgram, char *reduceProgram);

// Number of digits in the number
int SizeFinder(int numb)
{
    if (numb == 0)
        return 1;
    int res = 0;
    while (numb > 0)
    {
        ++res;
        numb /= 10;
    }
    return res;
}

// String representation of given number
void StringGenerator(char str[], int numb)
{
    int sizeOfNumb = SizeFinder(numb);
    str[0] = '0';
    str[sizeOfNumb] = '\0';

    char *curr = str + sizeOfNumb - 1;

    for (; numb > 0; numb /= 10, curr--)
    {
        int temp = numb % 10;
        *curr = (char)temp + 48;
        if (numb < 10)
            break;
    }
}

// Get line from stdin
char* GetInputFromStdin()
{
    char *buffer = (char*)malloc(sizeof(char) * (DEFAULT_LENGTH + 3));
    int bufferSize = DEFAULT_LENGTH;
    int sizeCount = 0;
    char chr = '\0', *curr = buffer;

    while (chr != '\n')
    {
        // Realloc if buffer is full
        if (bufferSize == sizeCount)
        {
            bufferSize += DEFAULT_LENGTH;
            char *tempBuffer = (char *)realloc(buffer, bufferSize + 3);
            buffer = tempBuffer;
            curr = buffer + (bufferSize - DEFAULT_LENGTH);
        }
        scanf("%c", &chr);
        if (chr == '\0')
        {
            free(buffer);
            return NULL;
        }
        *curr = chr;
        ++curr;
        ++sizeCount;
    }

    *curr = '\0';
    curr = NULL;
    return buffer;
}

// Naive implementation for Nth power of a number
int Power(int numb, int pow)
{
    int count = 0, res = 1;
    while (count < pow)
    {
        res *= numb;
        ++count;
    }

    return res;
}

// String to int converter
int StringToInt(char *str)
{
    int res = 0, count = 0, len = strlen(str);
    for (int curr = len - 1; curr >= 0; curr--, ++count)
        res += ((int)str[curr] - 48) * Power(10, count);

    return res;
}

// Get the file name
char* FileNameRegulator(char *str)
{
    int fileNameSize = strlen(str);
    char *fileName = (char *)malloc(sizeof(char) * (fileNameSize + 3));
    fileName[0] = '.';
    fileName[1] = '/';
    char *fName = fileName + 2;
    for (int curr = 0; curr < fileNameSize; ++curr, ++fName)
        *fName = str[curr];

    *fName = '\0';
    fName = NULL;

    return fileName;
}

// Mapper function 1st Part
void Map(char *strN, char *mapProgram)
{
    // ---- Regulate and initialize variables ----
    char pwd[1024];
    getcwd(pwd, 1024);
    strncat(pwd, "/", 1);
    strncat(pwd, mapProgram, strlen(mapProgram));
    int N = StringToInt(strN);
    // -----------------------------------------------
    // Create pipes
    int fdSize = N * 2;
    int fd[fdSize];
    for(int i = 0; i < N * 2; i += 2)
        pipe(fd + i);
    // ----------
    // Create childs
    int count = 0;
    int pid;
    for(int i = 0; i < N; ++i)
    {
        if((pid = fork()) == 0)
        {
            close(fd[count * 2 + 1]);
            dup2(fd[count * 2], 0);
            close(fd[count * 2]);
            char strCount[128];
            StringGenerator(strCount, count);
            for(int k = 0; k < N * 2; k += 2)
                if(k != count * 2)
                {
                    close(fd[k]);
                    close(fd[k + 1]);
                }
            execl(pwd, mapProgram, strCount, (char *)0);

            printf("Error occured");
            exit(-1);
        }
        ++count;
    }


    // Close read ends
    if(pid != 0)
        for(int k = 0; k < N * 2; k += 2)
            close(fd[k]);
    // --------

    int d = 1;
    char *buffer;
    while ((buffer = GetInputFromStdin()))
    {
        int targetMap = (d % N) - 1;
        if (targetMap == -1)
            targetMap = N - 1;

        write(fd[targetMap * 2 + 1], buffer, strlen(buffer));
        free(buffer);
        ++d;
    }
    // Close write ends
    for(int k = 0; k < N * 2; k += 2)
        close(fd[k + 1]);
    // Wait for child processes to finish
    int status;
    for(int i = 0; i < N; ++i)
        wait(&status);
}

// MapReduce function 2nd Part
void MapReduce(char *strN, char *mapProgram, char *reduceProgram)
{
    // ---- Regulate and initialize variables ----
    char pwd[1024];
    getcwd(pwd, 1024);
    strncat(pwd, "/", 1);
    char mapFileName[1024];
    strncpy(mapFileName, pwd, strlen(pwd));
    mapFileName[strlen(pwd)] = '\0';
    char reduceFileName[1024];
    strncpy(reduceFileName, pwd, strlen(pwd));
    reduceFileName[strlen(pwd)] = '\0';
    strncat(mapFileName, mapProgram, strlen(mapProgram));
    strncat(reduceFileName, reduceProgram, strlen(reduceProgram));
    int N = StringToInt(strN);
    // -----------------------------------------------
    // Create pipes
    int fdSize = N * 2;
    int fd[fdSize];
    int fdMR[fdSize];
    int fdRR[fdSize];
    for (int i = 0; i < N * 2; i += 2)
    {
        pipe(fd + i);
        pipe(fdMR + i);
        pipe(fdRR + i);
    }
    // ----------
    // Create Mappers
    int count = 0;
    int pid;
    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) == 0)
        {
            close(fdMR[count * 2]);
            dup2(fdMR[count * 2 + 1], 1);
            close(fdMR[count * 2 + 1]);

            close(fd[count * 2 + 1]);
            dup2(fd[count * 2], 0);
            close(fd[count * 2]);
            char strCount[128];
            StringGenerator(strCount, count);
            for (int k = 0; k < N * 2; k += 2)
            {
                close(fdRR[k]);
                close(fdRR[k + 1]);
                if (k != count * 2)
                {
                    close(fdMR[k]);
                    close(fdMR[k + 1]);
                    close(fd[k]);
                    close(fd[k + 1]);
                }
            }
            execl(mapFileName, mapProgram, strCount, (char *)0);

            printf("Error occuredd");
            exit(-1);
        }
        ++count;
    }
    // Create Reducers
    count = 0;
    for(int i = 0; i < N; ++i)
    {
        if((pid = fork()) == 0)
        {
            close(fdMR[count * 2 + 1]);
            dup2(fdMR[count * 2], 0);
            close(fdMR[count * 2]);

            if(count != 0)
            {
                close(fdRR[(count - 1) * 2 + 1]);
                dup2(fdRR[(count - 1) * 2], 2);
                close(fdRR[(count - 1) * 2]);
            }
            if(count != N - 1)
            {
                close(fdRR[count * 2]);
                dup2(fdRR[count * 2 + 1], 1);
                close(fdRR[count * 2 + 1]);
            }
            
            if(count == N - 1)
            {
                close(fdRR[N * 2 - 2]);
                close(fdRR[N * 2 - 1]);
            }
            for(int k = 0; k < N * 2; k += 2)
            {
                close(fd[k]);
                close(fd[k + 1]);

                if(k != count * 2)
                {
                    if(k != (count - 1) * 2)
                    {
                        close(fdRR[k]);
                        close(fdRR[k + 1]);
                    }
                    close(fdMR[k]);
                    close(fdMR[k + 1]);
                }
            }
            char strCount[128];
            StringGenerator(strCount, count);
            execl(reduceFileName, reduceProgram, strCount, (char *)0);
            printf("Error occured");
            exit(-1);
        }
        ++count;
    }
    // Close read ends
    if (pid != 0)
        for (int k = 0; k < N * 2; k += 2)
        {
            close(fdRR[k]);
            close(fdRR[k + 1]);
            close(fdMR[k]);
            close(fdMR[k + 1]);
            close(fd[k]);
        }
    // --------

    int d = 1;
    char *buffer;
    while ((buffer = GetInputFromStdin()))
    {
        int targetMap = (d % N) - 1;
        if (targetMap == -1)
            targetMap = N - 1;

        // ---- Actual Process ----
        write(fd[targetMap * 2 + 1], buffer, strlen(buffer));
        // ---------------------------

        free(buffer);
        ++d;
    }
    // Close write ends
    for (int k = 0; k < N * 2; k += 2)
        close(fd[k + 1]);
    // Wait for child processes to finish
    int status;
    for (int i = 0; i < N; ++i)
        wait(&status);
}

int main(int argc, char *argv[])
{
    if (argc == 3)
        Map(argv[1], argv[2]);
    else if (argc == 4)
        MapReduce(argv[1], argv[2], argv[3]);
    else
        return -1;
    return 0;
}
