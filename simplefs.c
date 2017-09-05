/*
 * SimpleFS, un semplice filesystem
 */

// To avoid getline warning during compilation 
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



/*
 * Parsing a line representing a command.
 * The first argument is the line to be parsed; 
 * the second is the command itself and the other two are the two arguments.
 * A double pointer is needed because no memory is going to be allocated in here:
 * command_name, first_argument and second_argument are pointers to a region of the memory 
 * pointed by line.
 */
int simplefs_parse_command_line(char *line, char **command_name, char **first_argument, char **second_argument) {
    int number_of_arguments = 0;
    *command_name = line;
    while(*line != '\0'){
        /* 
         * The strategy is to iterare every character.
         * When a space is found it may be possible that an argument will follow
         * and we have to search for it.
         * When a character different from the space or the terminator is found,
         * we point the arguments respectively to it.
         */
        if(*line == ' ') {
            *line = '\0';
            if(*(line+1) != ' ' && *(line+1) != '\0') {
                number_of_arguments++;
                if(number_of_arguments == 1) {
                    *first_argument = line+1;
                } else if(number_of_arguments == 2) {
                    *second_argument = line+1;
                }
            }
        } 
        line++;
    }
    *(line-1) = '\0';
    return number_of_arguments; 
}

int simplefs_execute_command(char *command_name, char *first_argument, char *second_argument) {
    if(strcmp("create", command_name) == 0){
        printf("create called!\n");    
    } else if(strcmp("create_dir", command_name) == 0){
        printf("create_dir called!\n");    
    } else if(strcmp("read", command_name) == 0){
        printf("read called!\n");    
    } else if(strcmp("write", command_name) == 0){
        printf("write called!\n");    
    } else if(strcmp("delete", command_name) == 0){
        printf("delete called!\n");    
    } else if(strcmp("delete_r", command_name) == 0){
        printf("delete_r called!\n");    
    } else if(strcmp("find", command_name) == 0){
        printf("find called!\n");    
    } else if(strcmp("exit", command_name) == 0){
        printf("exit called!\n");    
    }
}

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
   
    char *command_name = NULL;
    char *first_argument = NULL;
    char *second_argument = NULL;
    
    int number_of_arguments = 0;

    while ((read = getline(&line, &len, stdin)) != -1) {
        number_of_arguments = simplefs_parse_command_line(line, &command_name, &first_argument, &second_argument);
        simplefs_execute_command(command_name, first_argument, second_argument); 
        command_name = NULL;
        first_argument = NULL;
        second_argument = NULL;
    }

    if (line)
        free(line);
    
    exit(EXIT_SUCCESS);
    
}
