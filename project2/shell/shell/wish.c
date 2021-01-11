#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static const char error_message[] = "An error has occurred\n";
static char *path;
static FILE *fin;

void free_args(int argc, char **argv[]); 
void execute_commands(char *buffer)
{
    const char *delim = " \t\n\r\v\f";  //space
    char *found;
    char *redirect = buffer;
    int redirection = 0;
    char *word_token = NULL;
    int stdout_copy = dup(1);
    char **args = malloc(sizeof(*args));
    
    //redirection: 
    buffer = strsep(&redirect, ">"); 
    found = buffer;
    int i = 0;

    while((word_token = strsep(&found, delim)) != NULL){   
        // do not store delim chars
        if (strstr(delim, word_token)){
            continue;
        }
        i += 1;
        //reallocate space of args to include ptr to new string 
        args = realloc(args, (i + 1) * sizeof(*args)); 
         //allocate space for new string
        args[i-1] = malloc((strlen(word_token) + 1) * sizeof(**args)); 
        //terminated by a NULL pointer
        args[i] = NULL; 
        strcpy(args[i-1], word_token);            
    }
    //no left-hand side of ">"
    if (i == 0){
        if(redirect != NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        free(args);
        return;
    }
    //dealing with build-in commands: 
    if (!strcmp(args[0], "exit")) {// do not exit until exit is entered
        //exit with other argument 
        if (i > 1) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else{
            if (fin != NULL){
                fclose(fin);
            }

            free_args(i, &args);
            free(path);
            free(buffer);
            exit(0); 
        }   
    }
    //check the cd argument 
    else if (!strcmp(args[0], "cd")){
        // only need one argument 
        if (i > 2 || i == 1) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else if(chdir(args[1])){
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    else if (!strcmp(args[0], "path")){
        strcpy(path, "");
        for (int j = 1; j < i; ++j){
            // adding space for multiple path args 
            int space = j > 1 ? 1 : 0; 
            path = realloc(path, (strlen(path) + strlen(args[j]) + space + 1 ) * sizeof(*path));
            
            if (space){
                strcat(path, " ");
            }
            strcat(path, args[j]);
        }
    }    
    
    else {
        int count = 0;
        if (redirect != NULL){
            char *file = NULL;
            while((word_token = strsep(&redirect, delim)) != NULL){
                // do not store delim chars
                if (strstr(delim, word_token)){
                    continue;
                }
                if (count == 0){
                    file = word_token;
                }
                count += 1;
            }
            
            // only 1 argument allowed on right hand side of '>'
            if (count != 1) {
                write(STDERR_FILENO, error_message, strlen(error_message));                
                free_args(i, &args);
                return;
            }
            close(STDOUT_FILENO);
            redirection = open(file, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
        }    

        int rc = fork();

        if (rc < 0){
            write(STDERR_FILENO, error_message, strlen(error_message)); //failed
        }
        // parent process enters here
        else if (rc > 0) {
            wait(NULL);
        
            if (redirection > 0){
                dup2(stdout_copy, 1);
            }
        }
        
        //child process 
        else {
            found = path;
            int success = 0;
            while ((word_token = strsep(&found, " ")) != NULL)
            {   
                // +2 - 1 for / and 1 for null-terminated char
                char *new_path = malloc((strlen(word_token) + strlen(args[0]) + 2) * sizeof(char)); 
                strcpy(new_path, word_token);
                strcat(strcat(new_path, "/"), args[0]);

                if(!access(new_path, X_OK)){
                    success = 1;
                    execv(new_path, args);
                }
                free(new_path);
            }

            if (!success){
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            
            free_args(i, &args);         
            exit(0);
        }
    }

    free_args(i, &args);
    return;
}

int main(int argc, char *argv[]){
    fin = NULL;
    //more than two arguments 
    if (argc > 2){
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    
    //in batch mode
    else if (argc == 2){
        fin = fopen(argv[1], "r");
        //check input file 
        if(fin == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }
    
    //initialize the path
    char *line_buf = NULL;  
    size_t line_buf_size = 0;  
    path = malloc((strlen("/bin") + 1) * sizeof(*path));
    strcpy(path , "/bin");

    while (1) {
        //interactive mode
        if (fin == NULL){
            printf("wish> "); 
        }
        
        //hit EOF
        if(getline(&line_buf, &line_buf_size, fin ? fin : stdin) == -1) {
            if (fin != NULL){
                fclose(fin);
            }
            free(line_buf); 
            free(path);
            exit(0);
        } 
        
        char *itr = line_buf;
        char *word_token = NULL;
        word_token = strsep(&itr, "&");
        // finding & in command line
        if (itr != NULL) {
            int rc;
            do{
                //create child process
                rc = fork();
                //fork failed 
                if (rc < 0){
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                // parent process enters here
                else if (rc > 0) {  
                }
                
                else{
                    execute_commands(word_token);
                    exit(0);
                }
            } while ((word_token = strsep(&itr, "&")) != NULL);
            
            // parent process and wait for all child processes 
            if (rc > 0) {
                while ((wait(NULL)) > 0); 
            }
        }
        else{
            execute_commands(line_buf);
        }                
    }
    return 0;
}

void free_args(int argc, char **argv[])
{
    for (int i = 0; i < argc; ++i)
    {
        free((*argv)[i]);
    }
    free(*argv);
}
