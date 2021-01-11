#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int main(int argc, char *argv[])
{   
    //no file and exit 
    if(argc == 0)
    {
        exit(0);
    }
    
    FILE *file = NULL;
    // loop through all the files 
    for (int i = 1; i < argc; i++)
    {
        file = fopen(argv[i], "r");
        if(file == NULL)
        {
            printf("wcat: cannot open file\n");
            exit(1);
        }
        // read file line by line and output the lines
        char *buffer = (char *)malloc(INT_MAX * sizeof(char));
        while(fgets(buffer, INT_MAX, file) != NULL)
        {
            printf("%s", buffer);
        }

        fclose(file);
    }
    return 0;
}
