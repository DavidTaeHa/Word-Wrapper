#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#ifndef DEBUG
#define DEBUG 0
#endif

// Set to 1, Best at 4.
#define BUFFSIZE 1
// Choosing large value for user input
#define INPTSIZE 4096

static int exitCode = EXIT_SUCCESS;

void wrap_file(int file_in, int file_out, int columns)
{
    int length = BUFFSIZE, bytes = 0, brite = 0, last = 0, nLin = 0, nPar = 0, n = 0, fLen;
    char *buf = malloc(sizeof(char) * BUFFSIZE);
    char *word = calloc(length, sizeof(char));
    if (DEBUG)
    {
        printf("Debugging:\n");
    }
    while ((bytes = read(file_in, buf, BUFFSIZE)) > 0)
    {
        for (size_t i = 0; i < bytes; ++i)
        {
            // Read char in buffer
            if (DEBUG)
            {
                printf("%ld: '%c'\n", i, buf[i]);
            }
            if (isspace(buf[i]))
            {
                if (n)
                {
                    fLen = *&brite + n;
                    if (*&brite != 0)
                    {
                        fLen++;
                    }
                    // Word will exceed maximum width, requires separate line
                    if (fLen > columns)
                    {
                        write(file_out, "\n", 1);
                        *&brite = 0;
                    }
                    // Adds space before next word on same line
                    if (brite != 0 && !last)
                    {
                        *&brite += write(file_out, " ", 1);
                    }
                    *&brite += write(file_out, &word[0], n);
                    // Written word exceeded given width
                    if (brite > columns)
                    {
                        if (DEBUG)
                        {
                            printf("    Error: Word '%s' has exceeded wrapping parameters\n", word);
                        }
                        exitCode = EXIT_FAILURE;
                    }
                    if (DEBUG)
                    {
                        printf("        Checked %d Bytes of the word '%s'\n", brite, word);
                        printf("        Added %d Bytes of the word '%s'\n", n, word);
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
                            printf("        New Line\n");
                        }
                        nLin = 1;
                    }
                    else if (!nPar)
                    {
                        if (DEBUG)
                        {
                            printf("        Paragraph Ended\n");
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
                if (DEBUG)
                {
                    printf("    Current Word: '%s'\n", word);
                }
                nLin = nPar = 0;
            }
        }
        free(buf);
        buf = malloc(sizeof(char) * BUFFSIZE);
    }
    fLen = *&brite + n;
    if (*&brite != 0)
    {
        fLen++;
    }
    // Word will exceed maximum width, requires separate line
    if (fLen > columns)
    {
        write(file_out, "\n", 1);
        *&brite = 0;
    }
    // Adds space before final word
    if (brite != 0 && !last)
    {
        *&brite += write(file_out, " ", 1);
    }
    *&brite += write(file_out, &word[0], n);
    if (DEBUG)
    {
        printf("        Checked %d Bytes of the word '%s'\n", brite, word);
        printf("        Added %d Bytes of the word '%s'\n", n, word);
    }
    // Final written word exceeded given width
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

    if (argc > 3 || argc < 2)
    {
        printf("Incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }

    struct stat temp;
    // If second argument is an existing file or directory
    if ((stat(argv[2], &temp) != -1) && (atoi(argv[1]) > 0))
    {
        // Second argument is a file that exists
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
        // Second argument is a directory that exists
        else if (S_ISDIR(temp.st_mode))
        {
            if (DEBUG)
            {
                printf("\nWrapping files in directory '%s'\n", argv[2]);
            }
            struct dirent *f;
            DIR *fd = opendir(argv[2]);
            chdir(argv[2]);
            int count = 1;
            while ((f = readdir(fd)) != NULL)
            {
                if (f->d_name[0] == '.')
                {
                    printf("Skipping: '%s'\n", f->d_name);
                }
                else if (strstr(f->d_name, "wrap.") == f->d_name)
                {
                    printf("File to overwrite: '%s'\n", f->d_name);
                }
                else if (strstr(f->d_name, ".txt"))
                {
                    int inText = open(f->d_name, O_RDONLY);
                    char *newFile = calloc(strlen(f->d_name) + 6, sizeof(char));
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
    // If second arguments file name does not exist read from STDIN
    else if (atoi(argv[1]) > 0 && argc == 2)
    {
        char *userStr = malloc(sizeof(char) * INPTSIZE);
        fgets(userStr, INPTSIZE, stdin);
        // Creating a temporary file
        FILE *fp = fopen("switcharoo.txt", "w+");
        if (fp)
        {
            fputs(userStr, fp);
        }
        // If there was an error retrieving the file
        else
        {
            perror("switcharoo.txt");
            exitCode = EXIT_FAILURE;
        }
        fclose(fp);
        free(userStr);
        // If the file exists proceed
        if ((stat("switcharoo.txt", &temp) != -1))
        {
            if (DEBUG)
            {
                printf("\nTemporary file '%s' wrapped to STDOUT\n", "switcharoo.txt");
            }
            int inText = open("switcharoo.txt", O_RDONLY);
            wrap_file(inText, 1, atoi(argv[1]));
            close(inText);
        }
        // Remove temporary file as it is no longer needed and should not exist.
        remove("switcharoo.txt");
    }
    else
    {
        if (atoi(argv[1]) < 0)
        {
            printf("Error: Width should be a positive integer");
        }
        else if (argc > 3 || argc < 2)
        {
            printf("Error: Not enough arguments");
        }
        else if ((stat(argv[2], &temp) == -1))
        {
            perror(argv[2]);
        }
        exitCode = EXIT_FAILURE;
    }
    return exitCode;
}