#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#define TRUE 1
#define FALSE !TRUE

struct DoubleLinkedList
{
    char *content;
    struct DoubleLinkedList *prev, *next;
};

void mallocErr(size_t condition)
{
    if (condition == 0)
    {
        fprintf(stderr, "malloc failed \n");
        exit(1);
    }
}

int main(int argc, char* argv[]){
    //too many arguments passed to the program
    if (argc > 3)
    {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }
    FILE *fin = stdin, *fout = stdout;//create objects 
    // if 3 arguments passed
    if (argc == 3)
    {
        // check if input is the same as output
        struct stat *statbuf1 = (struct stat *)malloc(sizeof(struct stat));
        struct stat *statbuf2 = (struct stat *)malloc(sizeof(struct stat));
        stat(argv[1], statbuf1);
        stat(argv[2], statbuf2);
        if(statbuf1->st_ino == statbuf2->st_ino)
        {
            fprintf(stderr, "reverse: input and output file must differ\n");
            exit(1);
        }
        // open input and output file  
        fin =  fopen(argv[1], "r");
        fout = fopen(argv[2], "w");
        // invalid input file
        if (fin == NULL)
        {
            fprintf(stderr, "%s \'%s\'\n", "reverse: cannot open file", argv[1]);
            exit(1);
        }
        //invalid output file 
        if (fout == NULL)
        {
            fprintf(stderr, "%s \'%s\'\n", "reverse: cannot open file", argv[2]);
            exit(1);
        }
    }
    // if 2 arguments are passed
    if (argc == 2)
    {
        // open input file
        fin = fopen(argv[1], "r");
        // if input file opening failed show error and exit
        if (fin == NULL)
        {
            fprintf(stderr, "%s \'%s\'\n", "reverse: cannot open file", argv[1]);
            exit(1);
        }
    }
    // load lines to a double linked list
    size_t buffer_size = INT_MAX;
    struct DoubleLinkedList *line = (struct DoubleLinkedList *)malloc(sizeof(struct DoubleLinkedList));
    mallocErr(line != NULL);

    // allocate memory for content, initialize prev and next to null
    line->content = (char *)malloc(buffer_size * sizeof(char));
    mallocErr(line->content != NULL);
    line->prev = NULL;
    line->next = NULL;
    while (getline(&(line->content), &buffer_size, fin) > -1)
    {
        line->next = (struct DoubleLinkedList *)malloc(sizeof(struct DoubleLinkedList));
        mallocErr(line->next != NULL);
        struct DoubleLinkedList *prev_line = line;
        line = line->next;
        // allocate memory for content, initialize prev to prev and next to null
        line->content = (char *)malloc(buffer_size * sizeof(char));
        mallocErr(line->content != NULL);
        line->prev = prev_line;
        line->next = NULL;
    }
    // output lines and clean up memory
    while (line->prev != NULL)
    {
        line = line->prev;
        free(line->next);
        fprintf(fout, "%s", line->content);
    }
    free(line);
    // close files
    if (argc == 2)
    {
        fclose(fin);
    }
    if (argc == 3)
    {    
        fclose(fin);
        fclose(fout);
    }

    return 0; 
}
    



    