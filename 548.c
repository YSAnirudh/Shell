#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h> 
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// revert back to working state by copying cdir code to cd execution place

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
char** input(char* dir) {
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
		if (dir != NULL) {
			printf("<%s@%s:~%s", username, hostname, dir);
		} else {
			printf("<%s@%s:~", username, hostname);
		}
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

//change directory to changeTo from current directory.
//rootDir is used to ensure we don't go up a level from our shell's root.(Using cd .. at shell's root)
//bool canChange to see if we are able to change to the specified directory
char* changeDir(char* changeTo, char* rootDir, bool* canChange) {
	char* temp = (char*) malloc(sizeof(char) * ARG_SIZE);
	char* to = (char*) malloc(sizeof(char) * ARG_SIZE);
	
	//to get the current directory of shell + directory from shell.
	//i.e. full directory from root of terminal
	char currDir[ARG_SIZE];
	if (getcwd(currDir, sizeof(currDir)) == NULL) {
		printf("Error getting directory\n");
		free(temp);
		return NULL;
	}
	
	//if we perform a cd .. command from shell's root, don't go a level up
	if (strcmp(changeTo, "..") == 0 && strcmp(rootDir, currDir) == 0) {
		printf("Already in the root directory.\n");
		*canChange = false;
		strcpy(temp, currDir);
		return temp;
	}
	
	//add / and changeTo to current path 
	to = strcat(currDir, "/");
	to = strcat(to, changeTo);
	
	//chdir to the path 'to'
	//if it fails, canChange = false
	if (chdir(to) != 0) {
		fprintf(stderr, "Cannot change to Directory: %s\n", changeTo);
		*canChange = false;
	}
	
	//get the directory after change and return
	if (getcwd(currDir, sizeof(currDir)) == NULL) {
		printf("Error getting directory\n");
		free(temp);
		return NULL;
	}
	strcpy(temp, currDir);
	
	return temp;
}

//used to change directory to previous directory when cd - command is used
char* changeBTWNPrevDirs(char* prevDir, char* goToDir) {
	char* temp = (char*) malloc(sizeof(char) * ARG_SIZE);
	char* to = (char*) malloc(sizeof(char) * ARG_SIZE);
	
	//add the ddirectory to change to 
	strcpy(temp, goToDir);
	to = strcat(temp, prevDir);
	
	//change directory
	if (chdir(to) != 0) {
		fprintf(stderr, "Cannot change to Directory: %s\n", prevDir);
	}
	
	//get directory after change and return
	char currDir[ARG_SIZE];
	if (getcwd(currDir, sizeof(currDir)) == NULL) {
		printf("Error getting directory\n");
		free(temp);
		return NULL;
	}
	strcpy(temp, currDir);
	return temp;
}

//returns (char*)(fullDir - shellDir)
char* morphDir(char* fullDir, char* shellDir) {
	int differenceInLength = strlen(fullDir) - strlen(shellDir);
	if (differenceInLength <= 0) {
		return NULL;
	}
	char* temp = (char*) malloc(sizeof(char) * (differenceInLength + 1));
	
	int shellDirLength = strlen(shellDir);
	int i;
	for (i = 0; i < shellDirLength; i++) {
		if(fullDir[i] != shellDir[i]) {
			return NULL;
		}
		i++;
	}
	while(i < strlen(fullDir)) {
		temp[i-shellDirLength] = fullDir[i];
		i++;
	}
	temp[i] = '\0';
	return temp;
}

//prints the directory from the terminal's root
void getCurrDir() {
	char curr[ARG_SIZE];
	if (getcwd(curr, sizeof(curr)) != NULL) {
		printf("Current Directory: %s\n", curr);
	}
}

//prints the directory from the shell's root
void getShellDir(char* shellDir) {
	if (shellDir == NULL) {
		printf("root~\n");
		return;
	}
	printf("~%s\n", shellDir);
	return;
}

int main() {
	//char* array to store tokens to use execvp call to execute commands
	char** args;
	char* dir = (char*) malloc(sizeof(char) * ARG_SIZE);
	
	//for getting the root directory of the shell.
	char curr[ARG_SIZE];
	if (getcwd(curr, sizeof(curr)) == NULL) {
		printf("Error...Current Directory Fail.\n");
		return 0;
	}
	
	//for morphing the directory name
	char* shellRootDir = curr;
	
	//for using the directory name
	char* rootDir = (char*) malloc(sizeof(char) * strlen(shellRootDir));
	strcpy(rootDir, shellRootDir);
	
	//the directory from the root of the shell.
	char* shellDir;
	
	//the directory from which we came to the present directory
	//i.e. the previous directory
	char* prevDir;
	
	printf("Starting up the Shell...\n");
	while(true) {
		//take input and store tokenized string in the char* array
		args = input(shellDir);
		//printf("okay\n");
		if (args == NULL) continue;
		
		//if there is no input continue
		if (args[0] == NULL) continue;
		
		//if user enters stop it breaks the while loop and exits
		if (strcmp(args[0], "stop") == 0 && args[1] == NULL) {
			break;
		}
		//change directory command
		else if (strcmp(args[0], "cd") == 0) {
			//cdir(shellDir, shellRootDir, rootDir, prevDir, dir, args);
			bool prev = false;
			//char* dir = (char*)malloc(sizeof(char) * ARG_SIZE);
			//if no arguments, go to shell's root after storing the previous directory
			if (args[1] == NULL) {
				if (prevDir != NULL) {
					strcpy(prevDir, shellDir);
				} else {
					prevDir = (char*)malloc(sizeof(char) * strlen(shellDir));
					strcpy(prevDir, shellDir);
				}
				chdir(shellRootDir);
				shellDir = NULL;
			}
			//if cd - command, go back to the previous directory
			else if (strcmp(args[1], "-") == 0) {
				//if prevDir is NULL
				if (prevDir == NULL) {
					prev = true;
					char* cutTemp;
					char curr[ARG_SIZE];
					if (getcwd(curr, sizeof(curr)) == NULL) {
						printf("can't get current directory.\n");
						continue;
						//return;
					}
					//cutTemp = current directory - shell's root directory
					cutTemp = morphDir(curr, shellRootDir);
					//if cutTemp is not NULL store it as prevDir and go to shell's root as prevDir was NULL
					if (cutTemp != NULL) {
						printf("Previous Directory was: root~\n");
						char* temp;
						prevDir = (char*) malloc(sizeof(char) * strlen(cutTemp));
						strcpy(prevDir, cutTemp);
						dir = changeBTWNPrevDirs(temp, rootDir);
						shellDir = morphDir(dir, shellRootDir);
						continue;
						//return;
					}
					//else continue to the next user input
					else {
						printf("No previous directory.\n");
						continue;
						//return;
					}
				}
				//if prevDir is not NULL
				else {
					printf("Previous Directory was: %s\n", prevDir);
					//stor prevDir in temp
					char* temp = (char*) malloc(sizeof(char) * strlen(prevDir));
					
					//set prevDir
					strcpy(temp, prevDir);
					if (shellDir == NULL) {
						prevDir = NULL;
					} else {
						strcpy(prevDir, shellDir);
					}
					
					//change directory to prevDir by using temp
					dir = changeBTWNPrevDirs(temp, rootDir);
					
					//get directory from shell's root
					shellDir = morphDir(dir, shellRootDir);
					
					//free allocated memory
					free(temp);
					continue;
					//return;
				}
			}
			//if there are extra arguments, handle error
			else if (args[2] != NULL) {
				fprintf(stderr, "Cannot change to Directory: %s\n", args[1]);
				
				continue;
				//return;
			}
			//execution of normal cd command
			else {
				bool canChange = true;
				
				//get current directory
				char curr[ARG_SIZE];
				if (getcwd(curr, sizeof(curr)) == NULL) {
					printf("Can't get Current Directory.\n");
				}
				
				//set prevDir after storing it for if it fails to change directory
				char* temp;
				if (prevDir != NULL) {
					temp = (char*) malloc(sizeof(char) * strlen(prevDir));
					strcpy(temp, prevDir);
				} else {
					temp = NULL;
				}
				prevDir = morphDir(curr, shellRootDir);
				
				//change directory
				dir = changeDir(args[1], rootDir, &canChange);
				printf("%s\n", dir);
				//if it wasn't able to change directory 
				//revert back the value of prevDir
				if (!(canChange)) {
					if (temp == NULL) {
						prevDir = NULL;
					} else {
						if (prevDir != NULL) {
							strcpy(prevDir, temp);
						} else {
							prevDir = (char*)malloc(sizeof(char) * strlen(temp));
							strcpy(prevDir, temp);
						}
					}
				}
				shellDir = morphDir(dir, shellRootDir);
			}
		}
		//get the current location from shell's root
		else if (strcmp(args[0], "cwd") == 0) {
			getShellDir(shellDir);
		}
		//execute commands
		else {
			//to execute the given command.
			execute(args);
		}
	}
	printf("Exiting...\n");
	//free the memory if allocated.
	if (args != NULL) free(args);
	if (rootDir != NULL) free(rootDir);
	if (shellDir != NULL) free(shellDir);
	if (prevDir != NULL) free(prevDir);
	if (dir != NULL) free(dir);
	return 0;
}