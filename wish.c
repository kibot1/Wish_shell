#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define INTERACTIVE 1
#define BATCH 2
#define MAX_SIZE 64

void error_message();
void *prog_path(char *buffer, char *path, char *cmd);



int main(int argc, char *argv[]) {
    FILE *stream; //stream from where to get input from
    char *line = NULL; //the line of input
    size_t len = 0; //allocated size of line
    ssize_t nread; //number of characters read from getline

    //check what mode to run in. and if valid num of args
    switch(argc){
        case INTERACTIVE:
            stream = stdin;
            break;

        case BATCH: 
            //stream = argv[1];

        default: //invalid number of arg's
            error_message();
            exit(1);
    }

    char *paths[MAX_SIZE]; //array of paths
    paths[0] = "/bin"; //set initial path
    int num_paths = 1;

    while(true){ //loop prompt
        printf("wish> ");

        //get line
        nread = getline(&line, &len, stream);

        //run cmd's
        char *cmd = NULL;
        while((cmd = strsep(&line, " ")) != NULL){
            printf("%s", cmd);

            for(int i = 0; i < num_paths; i++){ //check cmd with each path
                char *buffer = NULL;
                prog_path(buffer, paths[i], cmd);
            }
        }

        exit(0);
    }

}

void *prog_path(char *buffer, char *path, char *cmd){ //returns the complete path in the from of <path/cmd>
    int size = strlen(path) + 1 + strlen(cmd);
    printf("%d\n", size);
    buffer[size];

    strcpy(buffer, path);
    
}

void cmd_exit(){
    return exit(0);
}

void error_message(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}