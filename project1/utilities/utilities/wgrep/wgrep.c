#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void findingTerm(FILE *f, char *searchTerm)
{
  char *line = NULL;         // Buffer for line to be read in from file
  int length;                // Length of line
  size_t line_buf_size = 0; 
  
  while((length = getline(&line, &line_buf_size, f)) != -1)
  {
    if (strstr(line, searchTerm) != NULL)
    {
      printf("%s", line);
    }
  }
    free(line);
}


int main(int argc, char *argv[])
{
  FILE *file = NULL;
  //no search word
  if (argc == 1)
  {
    printf("wgrep: searchterm [file ...]\n");
    exit(1);
  }
  //has a search term but no file
  else if (argc == 2)
  {
    findingTerm(stdin, argv[1]);
  }
  else
  {
    for (int i = 2; i < argc; ++i)
    {
      file = fopen(argv[i], "r");
      if (file == NULL)
      {
        printf("wgrep: cannot open file\n");
        exit(1);
      } 
      findingTerm(file, argv[1]);
      fclose(file);
    }
  }
  return 0;
}

