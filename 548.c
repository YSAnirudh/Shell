#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

//max size of args for running exec
const int NO_OF_ARGS = 128;
//max size of a user input string
const int ARG_SIZE = 1024;

//to make tokens(each string) and return an array of tokens
char** tokenize(char* line) {
	//seperator and buffer for splitting the string.
	char* seperator = " ";
	char* buffer;
	
	//array of tokens to be returned
	char** args = (char**) malloc(sizeof(char*) * NO_OF_ARGS);
	
	//using another char* to not change the initial given string
	int lineLength = strlen(line);
	char* bufferLine = (char*) calloc(lineLength + 1, sizeof(char));
	strcpy(bufferLine, line);
	
	int i = 0;
	
	//split start
	buffer = strtok(bufferLine, seperator);
	while(buffer != NULL) {
		//add only if the token size is not 0
		if (strlen(buffer) > 0) {
			args[i] = buffer;	//storing each token in char* array
		}
		i++;
		buffer = strtok(NULL, seperator);
	}
	args[i] = NULL;
	return args;
}

//to take user input and return the tokenized string
char** input() {
	char** args;
	
	//piping commands for taking input
	int fd[2];
	if (pipe(fd)) {
		fprintf(stderr, "Pipe Failed. Coder is noob. Sorry.\n");
		return NULL;
	}
	
	//forking to read from a process and write in another process
	int rc = fork();
	
	if (rc < 0) {
		//if fork fails print that it failed and return
		fprintf(stderr, "Fork Failed. Coder is noob. Sorry.\n");
		return NULL;
	} else if (rc == 0) {
		//Child
		char line[ARG_SIZE];
		char* buffer;
		
		//to get the hostname
		char hostname[1024];
		hostname[1023] = '\0';
		gethostname(hostname, 1024);
		
		//to get the username
		char* username = getenv("USER");
		
		//the display and reading of the user input
		printf("<%s@%s:~", username, hostname);
		buffer = readline(">");
		
		//if the user input is not NULL then copy the input 
		//to the required char*
		if (buffer != NULL) {
			add_history(buffer);
			strcpy(line, buffer);
		}
		
		//close the read end of the pipe
		close(fd[0]);
		//write to the write end of the pipe and exit from the process
		write(fd[1], line, sizeof(line));
		exit(1);
	} else {
		//Parent
		//waiting for the child process to write to the pipe
		wait(NULL);
		char line[ARG_SIZE];
		
		//close the write side
		close(fd[1]);
		//read from the read side of the pipe
		read(fd[0], line, sizeof(line));
		
		//tokenize the given input
		args = tokenize(line);
	}
	return args;
}

//to print all the tokens obtained
void printArgs(char** args) {
	int i = 0;
	while(args[i] != NULL) {
		printf("%s\n",args[i]);
		i++;
	}
	return;
}

//to fork and execute using the given token array
int execute(char** args) {
	int rc = fork();
	
	if (rc < 0) {
		//if fork fails print that it failed and return
		fprintf(stderr, "Fork Failed. Coder is noob. Sorry.\n");
		return EXIT_FAILURE;
	} else if (rc == 0) {
		//Child
		if (execvp(*args, args) < 0) {
			fprintf(stderr, "Please Enter a Valid Command. Exec Failed\n");
			exit(1);
		}
	} else {
		//Parent
		wait(NULL);
	}
	return 0;
}

int main() {
	//char* array to store tokens to use execvp call to execute commands
	char** args;
	printf("Starting up the Shell...\n");
	while(1) {
		//take input and store tokenized string in the char* array
		args = input();
		
		//if there is no input continue
		if (args[0] == NULL) continue;
		
		//if user enters stop it breaks the while loop and exits
		if (strcmp(args[0], "stop") == 0 && args[1] == NULL) {
			printf("Exiting...\n");
			break;
		}
		else {
			//to execute the given command.
			execute(args);
		}
	}
	
	//free the memory if allocated.
	if (args != NULL) free(args);
	return 0;
}