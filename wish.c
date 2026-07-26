#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define INTERACTIVE 1
#define BATCH 2
#define MAX_SIZE 64

void cmd_exit();
void print_error();

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
            break;

        default: //invalid number of arg's
            print_error();
            exit(1);
    }

    char *paths[MAX_SIZE]; //array of paths
    paths[0] = "/bin"; //set initial path
    int path_count = 1;

    while(true){ //loop prompt
        printf("wish> ");

        //get line
        nread = getline(&line, &len, stream);
        if(nread == -1){ //if EOF exit
            exit(0);
        }
        line[nread - 1] = '\0'; // remove '\n' from line

        char *cmds[MAX_SIZE]; //store commands from line
        char *cmd = strtok(line, "&"); //first call to specify src
        int cmd_count = 0; //keep count of commands

        //parse commands (delim with "&")
        while(cmd != NULL){
            cmds[cmd_count] = cmd;
            cmd_count++;
            cmd = strtok(NULL, "&");
        }

        //debug test 1 - print commands
        // printf("commands: \n");
        // for(int i = 0; i < cmd_count; i++){
        //     printf("- %s\n", cmds[i]);
        // }
        // printf("\n");

        //array to save child pid's
        pid_t pids[MAX_SIZE];
        int pid_count = 0;

        for(int i = 0; i < cmd_count; i++){ //loop thru each cmd to run
            char *args[MAX_SIZE];
            char *arg = strtok(cmds[i], " ");
            int arg_count = 0;

            //parse arguments in command
            while(arg != NULL){
                args[arg_count] = arg;
                arg_count++;
                arg = strtok(NULL, " ");
            }
            args[arg_count] = NULL; //null terminate the list

            //debug test 2 - print command args
            // printf("arguments: \n");
            // for(int j = 0; j < arg_count; j++){
            //     printf("- %s\n", args[j]);
            // }

            //check if command built-in or external
            if(strcmp(args[0], "exit") == 0){
                cmd_exit();
            }
            else if(strcmp(args[0], "cd") == 0){
                continue;
            }
            else if(strcmp(args[0], "path") == 0){
                continue;
            }
            else{//external cmd
                //check each path for executable
                bool found = false;
                for(int j = 0; j < path_count; j++){
                    //combine cmd with path
                    char full_path[MAX_SIZE];
                    strcpy(full_path, paths[j]);
                    strcat(full_path, "/");
                    strcat(full_path, args[0]);

                    //debug test 3 - print full path
                    //printf("full Path: %s\n", full_path);

                    if(access(full_path, X_OK) == 0){
                        found = true;
                        //if executable run cmd
                        pid_t id = fork();

                        if (id == 0){//child
                            execv(full_path, args);
                            exit(1); //exit on fail
                        }
                        else if (id > 0){
                            pids[pid_count] = id;
                            pid_count++;
                            break;
                        }
                    }
                }
                if (found == false){
                    print_error();
                }
            }
        }
        for(int i = 0; i < pid_count; i++){
            waitpid(pids[i], NULL, WUNTRACED);
        }

    }

}

void cmd_exit(){
    exit(0);
}

void print_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}