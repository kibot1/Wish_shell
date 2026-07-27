#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syscall.h>

#define INTERACTIVE 1
#define BATCH 2
#define MAX_SIZE 64
#define CHILD 0
#define FAILED -1

char *paths[MAX_SIZE];
int path_count;

void update_paths(char *cmd_args[], int arg_count);
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
            //open file and set as output stream
            stream = fopen(argv[1], "r");
            if (stream == NULL) {
                print_error();
                exit(1);
            }
            break;

        default: //invalid number of arg's
            print_error();
            exit(1);
    }

    //initialize paths array
    extern char *paths[];
    extern int path_count;
    paths[0] = strdup("/bin");
    path_count = 1;
    


    while(true){ //loop prompt
        if(argc == INTERACTIVE){
            printf("wish> ");
        }

        //get line
        nread = getline(&line, &len, stream);
        if(nread == FAILED){ //if EOF, exit
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

        pid_t pids[MAX_SIZE]; //array to save child pid's
        int pid_count = 0; //keep count of pids

        // loop thru each cmd and parse it's arguments
        for(int i = 0; i < cmd_count; i++){
            char *cmd_args[MAX_SIZE]; //hold args
            char *arg = strtok(cmds[i], " "); //first parse call to specify src
            int arg_count = 0; //keep count of args

            //parse arguments in command
            while(arg != NULL){
                cmd_args[arg_count] = arg;
                arg_count++;
                arg = strtok(NULL, " ");
            }
            cmd_args[arg_count] = NULL; //null terminate the list so it works with execv()

            //debug test 2 - print command args
            // printf("arguments: \n");
            // for(int j = 0; j < arg_count; j++){
            //     printf("- %s\n", args[j]);
            // }

            //check if command built-in or external
            char *cmd = cmd_args[0];
            if(strcmp(cmd, "exit") == 0){
                if(arg_count > 1){ //should have zero arguments
                    print_error();
                    continue;
                }

                exit(0);
            }
            else if(strcmp(cmd, "cd") == 0){
                if(arg_count != 2){ //should have one argument
                    print_error();
                    continue;
                }

                if(chdir(cmd_args[1]) == FAILED){
                    print_error();
                }
            }
            else if(strcmp(cmd, "path") == 0){
                update_paths(cmd_args, arg_count);
            }
            else{//external cmd
                bool found = false; //keep track of executable found

                //check paths for executable
                for(int j = 0; j < path_count; j++){

                    //combine cmd with path
                    char full_path[MAX_SIZE];
                    strcpy(full_path, paths[j]);
                    strcat(full_path, "/");
                    strcat(full_path, cmd);

                    //debug test 3 - print full path
                    //printf("full Path: %s\n", full_path);

                    if(access(full_path, X_OK) == 0){
                        found = true;
                        //executable found run cmd
                        pid_t id = fork();

                        if (id == CHILD){//child run cmd
                            execv(full_path, cmd_args);
                            exit(1); //exit on fail
                        }
                        else if (id > 0){ //parent keep running shell
                            pids[pid_count] = id;
                            pid_count++;
                            break;
                        }
                    }
                }
                //error if no executable found
                if (found == false){
                    print_error();
                }
            }
        }
        //after running commands wait on all
        for(int i = 0; i < pid_count; i++){
            waitpid(pids[i], NULL, WUNTRACED);
        }

    }

}

//update paths array with new. replace old.
void update_paths(char *cmd_args[], int arg_count){
    extern char *paths[];
    extern int path_count;

    //free mem of old path
    for(int i = 0; i < path_count; i++){
        free(paths[i]);
    }

    //save new paths
    path_count = 0;
    for(int i = 1; i < arg_count; i++){
        paths[path_count] = strdup(cmd_args[i]);
        path_count++;
    }
}

//prints error message.
void print_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}