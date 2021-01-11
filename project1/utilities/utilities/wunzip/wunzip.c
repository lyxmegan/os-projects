#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{ 
  //no file in the commmand line 
  if (argc < 2)
  {
    printf("wunzip: file1 [file2 ...]\n");
    exit(1);
  }
  
  FILE *file = NULL;
  char readChar = '\0';
  int charCount = 0;
  for (int i = 1; i < argc; ++i)
  {
    file = fopen(argv[i], "r");
    if(file == NULL)
    {
      printf("wgrep: cannot open file\n");
      exit(1);
    }
    // unpack all characters
    while (1)
    {      
      if(!fread(&charCount, 4, 1, file))
        break;  
      
      if(!fread(&readChar, 1, 1, file))
        break;
      
      for (int i = 0; i < charCount; ++i)
        printf("%c", readChar);
    }
    fclose(file);
  }
  return 0;
}
