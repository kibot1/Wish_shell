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
int parse_args(char *cmd_line, char ***args_out, char **output_name);
void pre_process(char *buffer, char *cmd);
void find_path(char **full_path, char *paths[], int path_count, char *cmd);
void run_cmd(char *output_name, char *full_path, char *cmd_args[]);
//int update_paths(char **paths[], int path_count, char *arr[], int arg_count);
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
    paths[0] = strdup("/bin");
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

            char buffer[strlen(cmds[i]) * 3 + 1];
            //adds whitespace between any '>'
            //helps in parsing args
            pre_process(buffer, cmds[i]);

            char **cmd_args;//hold cmd's arguments
            char *output_name = NULL;//save redirect file name if any
            int arg_count = parse_args(buffer, &cmd_args, &output_name);
            if(arg_count == -1){
                print_error();
                continue;
            }

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
                //update_paths(paths, path_count, cmd_args, arg_count);
                continue;
            }
            else{//external command
                char *full_path = NULL;
                find_path(&full_path, paths, path_count, cmd);
                if(full_path == NULL){
                    print_error();
                    continue;
                }
                        
                //executable found run cmd
                pid_t id = fork();

                if (id < 0){//error
                    free(full_path);
                }
                if (id == CHILD){//child run cmd
                    run_cmd(output_name, full_path, cmd_args);
                }
                else if (id > 0){ //parent keep running shell
                    pids[pid_count] = id;
                    pid_count++;
                    free(full_path);
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
    int size = BASE_SIZE; //initial size
    *cmds_out = malloc(size * sizeof(char*));
    int cmd_count = 0;
    char *cmd = strtok(line, "&");

    while(cmd != NULL){
        if(cmd_count == size){//if size limit reached, grow array
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
-adds whitespace between redirect('>') symbol to make 
command argument processing easier*/
void pre_process(char *buffer, char *cmd){
    int index = 0; //index for buffer
    for(int i = 0; i < strlen(cmd); i++){
        if(cmd[i] == '>'){
            //add space around '>'
            buffer[index] = ' ';
            index++;
            buffer[index] = '>';
            index++;
            buffer[index] = ' ';
        }
        else{
            //if not '>' just copy over the character
            buffer[index] = cmd[i];
        }

        index++;
    }

    buffer[index] = '\0';
}

/*helper function
-parses cmd_line and returns array with arguments
-if found redirection saves output name
success: return arg_count
fail: returns -1*/
int parse_args(char *cmd_line, char ***args_out, char **output_name){
    int size = BASE_SIZE;
    *args_out = malloc(size * sizeof(char*));
    int arg_count = 0;
    char *arg = strtok(cmd_line, " ");

    while(arg != NULL && strcmp(arg, ">") != 0){
        //check if size of args_out reached
        //account for null termination needed at end
        if(arg_count == size - 1){
            size*=2;
            char **temp = realloc(*args_out, size * sizeof(char*));
            if(temp == NULL){
                free(*args_out);
                return -1;

            }
            //successful realloc
            *args_out = temp;

        }
        (*args_out)[arg_count] = arg;
        arg_count++;
        arg = strtok(NULL, " ");
    }

    if(arg != NULL && strcmp(arg, ">") == 0){
        arg = strtok(NULL, " ");
        if(arg == NULL){
            free(*args_out);
            return -1;
        }
        *output_name = arg;
        if(strtok(NULL, " ") != NULL){
            free(*args_out);
            return -1;
        }
    }
    (*args_out)[arg_count] = NULL;
    return arg_count;
}

/*helper function
-looks for executable in each path in paths[]
success: returns executable
fail: returns NULL*/
void find_path(char **full_path, char *paths[], int path_count, char *cmd){
    //check paths for executable
    for(int i = 0; i < path_count; i++){
        //combine cmd with path
        char *temp = malloc(strlen(paths[i]) + 1 + strlen(cmd) + 1);
        strcpy(temp, paths[i]);
        strcat(temp, "/");
        strcat(temp, cmd);

        if(access(temp, X_OK) == 0){
            *full_path = temp;
            return;
        }
        else{
            free(temp);
        }
    }
}

void run_cmd(char *output_name, char *full_path, char *cmd_args[]){
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

//prints error message.
void print_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}