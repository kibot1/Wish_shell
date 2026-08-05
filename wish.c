#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syscall.h>

#define INTERACTIVE 1
#define BASE_SIZE 64
#define BATCH 2
#define CHILD 0
#define FAILED -1

int parse_commands(char *line, char ***cmds_out);
int parse_args(char *cmd_line, char ***args_out, char *output_name);
void pre_process(char **buffer, char *cmd);
void update_paths(char *paths[], int path_count, char *arr[], int arg_count);
int add_token(char ***dst,int dst_index, int dst_size, char *token);
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
    char *paths[BASE_SIZE];
    paths[0] = "/bin";
    int path_count = 1;
    


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

        //parse commands from 'line'
        char **cmds;
        int cmd_count = parse_commands(line, &cmds);
        if(cmd_count == -1){//error parsing
            print_error();
            continue;
        }

        pid_t pids[cmd_count]; //array to save child pid's
        int pid_count = 0; //keep count of pids

        // loop thru each cmd and parse it's arguments
        for(int i = 0; i < cmd_count; i++){
            /*preprocessing pass adding blank space between any ">"
            simplifies redirection handling */
            char *buffer = malloc(strlen(cmds[i]) * 3);//max possible size needed
            if(buffer == NULL){//error, failed allocation
                print_error();
                continue;
            }
            pre_process(&buffer, cmds[i]);

            char *cmd_args;
            char *output_name = NULL;
            int arg_count = parse_args(buffer, &cmd_args, &output_name);

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
                    continue;
                }
            }
            else if(strcmp(cmd, "path") == 0){
                update_paths(paths, path_count, cmd_args, arg_count);
            }
            else{//external command
                bool found = false; //keep track of executable found

                //check paths for executable
                for(int j = 0; j < path_count; j++){

                    //combine cmd with path
                    char full_path[strlen(paths[j]) + 1 + strlen(cmd) + 1]; 
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

                            //check for redirect
                            if(output_name != NULL){//redirect
                                FILE *output = freopen(output_name, "w", stdout); 
                                if(output == NULL){//error opening file
                                    print_error();
                                    exit(1);
                                }
                                dup2(STDOUT_FILENO, STDERR_FILENO);
                            }
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

/*helper function for parsing line of commands
success: return command count
error: return -1
*/
int parse_commands(char *line, char ***cmds_out){
    int size = BASE_SIZE;
    *cmds_out = malloc(size * sizeof(char*));
    int cmd_count = 0;
    char *cmd = strtok(line, "&");

    while(cmd != NULL){
        if(cmd_count == size){
            size*=2;
            char **temp = realloc(*cmds_out, size * sizeof(char*));
            if(temp == NULL){//failed realloc
                //free up memory
                free(*cmds_out);
                return -1;
            }
            //successful realloc
            *cmds_out = temp;
        }

        (*cmds_out)[cmd_count] = cmd;
        cmd_count++;
        cmd = strtok(NULL, "&");
    }

    //success parsing, return cmd count
    return cmd_count;
}

/*helper function
adds whitespace between redirect symbol to make 
command argument processing easier*/
void pre_process(char **buffer, char *cmd){
    int index = 0; //index for buffer
    for(int i = 0; i < strlen(cmd); i++){
        if(cmd[i] == '>'){
            (*buffer)[index] = ' ';
            index++;
            (*buffer)[index] = '>';
            index++;
            (*buffer)[index] = ' ';
        }
        else{
            (*buffer)[index] = cmd[i];
        }

        index++;
    }
}

int parse_args(char *cmd_line, char ***args_out, char **output_name){
    int size = BASE_SIZE;
    *args_out = malloc(size * sizeof(char*));
    int arg_count = 0;
    char *arg = strtok(cmd_line, " ");

    return arg_count;
}

//update paths array with new. replace old.
void update_paths(char *paths[], int path_count, char *arr[], int arg_count){
    //free memory
    for(int i = 0; i < path_count; i++){
        free(paths[i]);
    }

    //save new paths
    path_count = 0;
    for(int i = 1; i < arg_count; i++){
        paths[path_count] = strdup(arr[i]);
        printf("%s", paths[i]);
        path_count++;
    }
}

//prints error message.
void print_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}