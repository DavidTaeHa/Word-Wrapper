#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
// gcc ww3.c -fsanitize=address,undefined
#ifndef DEBUG
#define DEBUG 0
#endif

#define BUFSIZE 32

int exitCode = EXIT_SUCCESS;

void wrap_file(int file_in, int file_out, int columns)
{
    int length = BUFSIZE, bytes = 0, totBytes = 0, brite = 0, last = 0, nLin = 0, nPar = 0, n = 0, fLen;
    char *buf = malloc(sizeof(char) * BUFSIZE);
    char *word = calloc(length, sizeof(char));
    if (DEBUG)
    {
        printf("Debugging:\n");
    }
    while ((bytes = read(file_in, buf, BUFSIZE)) > 0)
    {
        if (DEBUG)
        {
            for (size_t i = 0; i < BUFSIZE; i++)
            {
                printf("%ld: '%c'\n", i, buf[i]);
            }
        }
        for (size_t i = 0; i < bytes; ++i)
        {
            if (isspace(buf[i]))
            {
                if (n)
                {
                    fLen = *&brite + n;
                    if (*&brite != 0)
                    {
                        fLen++;
                    }
                    if (fLen > columns)
                    {
                        write(file_out, "\n", 1);
                        *&brite = 0;
                    }
                    if (brite != 0 && !last)
                    {
                        *&brite += write(file_out, " ", 1);
                    }
                    *&brite += write(file_out, &word[0], n);
                    if (brite > columns)
                    {
                        if (DEBUG)
                        {
                            printf("    Error: Word '%s' has exceeded wrapping parameters\n", word);
                        }
                        exitCode = EXIT_FAILURE;
                    }
                    totBytes += brite;
                    if (DEBUG)
                    {
                        printf("        Added %d Bytes of Word '%s'\n", brite, word);
                    }
                    free(word);
                    word = calloc(length, sizeof(char));
                    n = 0;
                }
                if (buf[i] == 10)
                {
                    if (!nLin)
                    {
                        if (DEBUG)
                        {
                            printf("        New Line\n", word);
                        }
                        nLin = 1;
                    }
                    else if (!nPar)
                    {
                        if (DEBUG)
                        {
                            printf("        Paragraph Ended\n", word);
                        }
                        write(file_out, "\n\n", 2);
                        brite = nLin = 0;
                        nPar = 1;
                    }
                }
                continue;
            }
            else
            {
                if (n == (length - 1))
                {
                    word = realloc(word, sizeof(char) * (length *= 2));
                }
                word[n++] = buf[i];
                nLin = nPar = 0;
            }
        }
        if (DEBUG)
        {
            printf("    Read %d Bytes\n", bytes);
            printf("    Wrote %d Bytes\n", totBytes);
        }
        totBytes = 0;
        free(buf);
        buf = malloc(sizeof(char) * BUFSIZE);
    }
    fLen = *&brite + n;
    if (*&brite != 0)
    {
        fLen++;
    }
    if (fLen > columns)
    {
        write(file_out, "\n", 1);
        *&brite = 0;
    }
    if (brite != 0 && !last)
    {
        *&brite += write(file_out, " ", 1);
    }
    *&brite += write(file_out, &word[0], n);
    if (DEBUG)
    {
        printf("        Added %d Bytes of Word '%s'\n", brite, word);
    }
    if (brite > columns)
    {
        if (DEBUG)
        {
            printf("    Error: Word '%s' has exceeded wrapping parameters\n", word);
        }
        exitCode = EXIT_FAILURE;
    }
    write(file_out, "\n", 1);
    free(word);
    free(buf);
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 2)
    {
        printf("Incorrect number of arguments\n");
        exitCode = EXIT_FAILURE;
    }

    struct stat temp;

    //Input gets taken from STDIN
    if (argc == 2)
    {
        if (DEBUG)
        {
            printf("\nSTDIN wrapped to STDOUT\n");
        }
        //char *input = malloc(sizeof(char) * 100);
        //scanf("%s", &input);
        //printf("%s\n", input);
        int inText = open(0, O_RDONLY);
        wrap_file(inText, 1, atoi(argv[1]));
        close(inText);
    }
    else if (stat(argv[2], &temp) != -1) // Check the return value of stat
    {
        // Second arg is a file
        if (S_ISREG(temp.st_mode))
        {
            if (DEBUG)
            {
                printf("\nFile '%s' wrapped to STDOUT\n", argv[2]);
            }
            int inText = open(argv[2], O_RDONLY);
            wrap_file(inText, 1, atoi(argv[1]));
            close(inText);
        }
        // Second arg is a directory
        else if (S_ISDIR(temp.st_mode))
        {
            if (DEBUG)
            {
                printf("Wrapping files in directory '%s'\n", argv[2]);
            }
            struct dirent *f;
            DIR *fd = opendir(argv[2]);
            chdir(argv[2]);
            int count = 1;
            while ((f = readdir(fd)) != NULL)
            {
                if (strstr(f->d_name, ".txt"))
                {
                    int inText = open(f->d_name, O_RDONLY);
                    char *newFile = calloc(strlen(f->d_name) + 5, sizeof(char));
                    strcpy(newFile, "wrap.");
                    strcat(newFile, f->d_name);
                    if (DEBUG)
                    {
                        printf("\n%d: Wrapping file '%s' to '%s'\n", count, f->d_name, newFile);
                    }
                    int outText = open(newFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                    wrap_file(inText, outText, atoi(argv[1]));
                    close(inText);
                    close(outText);
                    free(newFile);
                    count++;
                }
            }
            closedir(fd);
        }
    }
    else
    {
        perror(argv[2]);
        exitCode = EXIT_FAILURE;
    }
    return exitCode;
}