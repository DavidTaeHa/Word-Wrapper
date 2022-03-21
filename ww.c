#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#define BUFSIZE 32
#define LISTLEN 16

char **lines;
int line_count, line_array_size;

// This method is supposed to initialize the lines of text
void init_lines(void)
{
    lines = malloc(sizeof(char *) * LISTLEN);
    line_count = 0;
    line_array_size = LISTLEN;
}

void add_line(char *p)
{
    if (DEBUG)
        printf("Adding |%s|\n", p);
    if (line_count == line_array_size)
    {
        line_array_size *= 2;
        lines = realloc(lines, line_array_size * sizeof(char *));
    }
    lines[line_count] = malloc(sizeof(char) * strlen(p));
    strcpy(lines[line_count], p);
    line_count++;
}

void wrap_file(char *file_name, int columns)
{
    printf("file name: %s\n", file_name);
    printf("Columns: %d\n", columns);
    int fd, bytes;
    char buf[BUFSIZE];
    char *crnt = malloc(sizeof(char) * 10);
    int crnt_max = 10;
    memset(crnt, 0, strlen(crnt));
    int crnt_len = 0;
    int prev_space = 0;
    int prev_newline = 0;

    char *token;
    int line_len = 0;

    fd = open(file_name, O_RDONLY);
    while ((bytes = read(fd, buf, BUFSIZE)) > 0)
    {
        // read buffer and break file into lines
        if (DEBUG)
            printf("Read %d bytes\n", bytes);
        for (int i = 0; i < bytes; i++)
        {
            // Increasing length of array
            if (crnt_len == crnt_max)
            {
                crnt = realloc(crnt, sizeof(char) * crnt_len * 2);
                crnt_max = crnt_max * 2;
            }

            // Two lines in a row
            if (prev_newline == 1 && buf[i] == '\n')
            {
                crnt[crnt_len] = '\0';
                add_line(crnt);
                crnt_len = 0;
                memset(crnt, 0, strlen(crnt));
                continue;
            }

            if (buf[i] == '\n')
            {
                buf[i] = ' ';
                prev_newline = 1;
            }
            else if (prev_newline == 1 && buf[i] != '\n')
            {
                prev_newline = 0;
            }

            // Handle consevutive spaces
            if (prev_space == 1 && buf[i] == ' ')
            {
                continue;
            }
            else if (buf[i] == ' ')
            {
                prev_space = 1;
            }
            else if (prev_space == 1 && buf[i] != ' ')
            {
                prev_space = 0;
            }

            crnt[crnt_len] = buf[i];
            crnt_len++;
        }
    }
    crnt[crnt_len] = '\0';
    // printf("length: %d\n", crnt_len);
    add_line(crnt);
    // printf("------------------\n");
    //  printf("%s\n", crnt);
    /*
    for (int i = 0; i < line_count; i++)
    {
        printf("line %d: %s\n", i + 1, lines[i]);
    }
    printf("\n");
    */
    // bool to check if a word length exceeds the max number of columns
    int too_long = 0;
    int start = 0;
    char *line = malloc(sizeof(char) * crnt_max);
    int line_max = crnt_max;
    memset(line, 0, strlen(line));
    for (int i = 0; i < line_count; i++)
    {
        if (strlen(lines[i]) == 0)
        {
            printf("\n");
            continue;
        }
        token = strtok(lines[i], " ");
        start = 0;
        while (token != NULL)
        {
            if (start == 0 && strlen(token) + line_len <= columns)
            {
                start = 1;
                line_len = strlen(token) + line_len;
                strcat(line, token);
            }
            else if (start == 1 && strlen(token) + line_len + 1 <= columns)
            {
                strcat(line, " ");
                line_len = strlen(token) + line_len + 1;
                strcat(line, token);
            }
            // extra line error on two long words in a row and starting paragraph with long word
            else if (strlen(token) > columns)
            {
                if (strlen(line) != 0)
                {
                    printf("%s\n", line);
                }
                memset(line, 0, strlen(line));
                printf("%s :too long\n", token);
                line_len = 0;
                too_long = 1;
                start = 0;
            }
            else
            {
                printf("%s\n", line);
                memset(line, 0, strlen(line));
                strcat(line, token);
                line_len = strlen(token);
            }
            token = strtok(NULL, " ");
        }

        printf("%s\n", line);
        if (i < line_count - 1)
        {
            printf("\n");
        }

        memset(line, 0, strlen(line));
        line_len = 0;
        start = 0;
    }

    if (too_long == 1)
    {
        printf("Warning: file contains word(s) that exceed(s) given input of columns");
        exit(EXIT_FAILURE);
    }

    // free statements for any allocated memory
    for (int i = 0; i < line_count; i++)
    {
        free(lines[i]);
    }
    free(lines);
    free(crnt);
    free(line);
    close(fd);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Incorrect number of arguments\n");
        return EXIT_FAILURE;
    }

    struct stat temp;
    if (stat(argv[2], &temp) != -1) // Check the return value of stat
    {
        // Second arg is a file
        if (S_ISREG(temp.st_mode))
        {
            printf("Wrapping text file...\n");
            wrap_file(argv[2], atoi(argv[1]));
        }
        // Second arg is a directory
        else if (S_ISDIR(temp.st_mode))
        {
            printf("Wrapping files in directory...\n");
            // ADDME
        }
    }
    else
    {
        perror(argv[2]);
        return EXIT_FAILURE;
    }
}