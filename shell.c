#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define MAX_DIRECTORY_LENGTH 1024

void freeTokens(char* tokens[]);
int tokenize_command(char* buff, char* tokens[]);
void read_command(char* buff, char* tokens[], _Bool* in_background);
void pwd(char* tokens[]);
void cd(char* tokens[]);
void executeBuiltIn(char* tokens[]);
_Bool isBuiltIn(char* command);
int tokenCount(char* tokens[]);
void freeTokens(char* tokens[]);
void printString(char* string);

// TODO: try and rewrite to not use strtok so we can have spaces in paths
int tokenize_command(char* buff, char* tokens[]) {
    int count = 0;
    char bufferCopy[1024];
    // strtok modifies buffer passed to it, so make a copy to keep buff intact
    strcpy(bufferCopy, buff);
    
    char* token = strtok(bufferCopy, " ");
    while(token != NULL) {
        tokens[count] = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(tokens[count], token);
        count++;        
        token = strtok(NULL, " ");
    }
    tokens[count] = NULL;    
    return count;
}

/**
* Read a command from the keyboard into the buffer 'buff' and tokenize it
* such that 'tokens[i]' points into 'buff' to the i'th token in the command. 
* buff: Buffer allocated by the calling code. Must be at least
* COMMAND_LENGTH bytes long.
* tokens[]: Array of character pointers which point into 'buff'. Must be at
* least NUM_TOKENS long. Will strip out up to one final '&' token.
* 'tokens' will be NULL terminated.
* in_background: pointer to a boolean variable. Set to true if user entered
* an & as their last token; otherwise set to false.
*/
void read_command(char *buff, char *tokens[], _Bool *in_background) {
    *in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);
	if ((length < 0) && (errno != EINTR))
	{
		perror("Unable to read command. Terminating.\n");
		exit(-1); /* terminate with error */
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
        // need to free the memory used in tokenize_command to hold the &
        free(tokens[token_count - 1]); 
        tokens[token_count - 1] = 0;
	}
}

void pwd(char* tokens[]) {
    char directory[MAX_DIRECTORY_LENGTH];
    size_t size = MAX_DIRECTORY_LENGTH;
    getcwd(directory, size);
    printString(directory);
    printString("\n");
}

void cd(char* tokens[]) {
    char* directory = tokens[1];
    if(chdir(directory) != 0) {
        perror("cd");
    }
}

void executeBuiltIn(char* tokens[]) {
    char* command = tokens[0];
    if(strcmp(command, "exit") == 0) {
        freeTokens(tokens);
        exit(EXIT_SUCCESS);
    } else if(strcmp(command, "pwd") == 0) {
        pwd(tokens);
    } else if(strcmp(command, "cd") == 0) {
        cd(tokens);
    }
}

_Bool isBuiltIn(char* command) {
    if(strcmp(command, "exit") == 0 || 
        strcmp(command, "pwd") == 0 || 
        strcmp(command, "cd") == 0) {
        return true;
    } else {
        return false;
    }
}

int tokenCount(char* tokens[]) {
    int tokenCount = 0;
    while(tokens[tokenCount] != NULL) {
        tokenCount++;
    }
    return tokenCount;
}

void freeTokens(char* tokens[]) {
    int token_count = tokenCount(tokens);
    for(int i = 0; i < token_count; i++) {
        if(tokens[i] != NULL) {
            free(tokens[i]);
        }
    }
}

void printString(char* string) {
    write(STDOUT_FILENO, string, strlen(string));
}

void printPrompt() {
    char directory[MAX_DIRECTORY_LENGTH];
    size_t size = MAX_DIRECTORY_LENGTH;
    getcwd(directory, size);
    strcat(directory, "> ");
    printString(directory);
}

/**
* Main and Execute Commands 
*/

int main(int argc, char* argv[]) {
    char input_buffer[COMMAND_LENGTH];
    char *tokens[NUM_TOKENS];
    // printf("Address: %p\n", tokens);
    // char** tokens = malloc(sizeof(char) * NUM_TOKENS);
    while (true) {
        // Get command
        // Use write because we need to use read()/write() to work with 
        // signals, and they are incompatible with printf().
        printPrompt();
        _Bool in_background = false;
        read_command(input_buffer, tokens, &in_background);
        /**
        * Steps For Basic Shell:
        * 1. Fork a child process
        * 2. Child process invokes execvp() using results in token array. 
        * 3. If in_background is false, parent waits for
        *   child to finish. Otherwise, parent loops back to 
        *   read_command() again immediately. 
        */
        // int token_count = tokenCount(tokens);
        // printf("Token count: %i\n", token_count);
        if(tokens[0] == NULL) continue; //keeps from segfault-ing on empty line
        
        if(isBuiltIn(tokens[0])) {
            executeBuiltIn(tokens);
        } else {
            pid_t pid = fork();
            if(pid == 0) {
                if(execvp(tokens[0], tokens) == -1) {
                    perror(tokens[0]);
                }
            }
            if(in_background == false) {
                int status;
                waitpid(pid, &status, 0);
            }
        }
        // wait on zombie background processes
        while(waitpid(-1, NULL, WNOHANG) > 0) {}
        freeTokens(tokens);
    }
    return 0;
}