#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


int main(int argc, char *argv[])
{   
    // no file
    if (argc == 1)
    {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }
    char iterChar, currentChar = '\0';
    int charCount = 0;
    // loop through all files
    for (int i = 1; i < argc; i++)
    {
        FILE *file = fopen(argv[i], "r");
        if (file == NULL)
        {
            printf("wzip: unable to open file\n");
            exit(1);
        }
        // loop through all characters
        while (1)
        {
            iterChar = fgetc(file);
            if (feof(file)) break;
            if (currentChar == '\0') currentChar = iterChar;
            if (currentChar == iterChar)
            {
                charCount++;
            }
            else
            {
                fwrite(&charCount, 4, 1, stdout);
                fwrite(&currentChar, 1, 1, stdout);
                currentChar = iterChar;
                charCount = 1;
            }
        }
        fclose(file);
    }
    if (charCount != 0)
    {
        fwrite(&charCount, 4, 1, stdout);
        fwrite(&currentChar, 1, 1, stdout);
    }
    return 0;
}

