/********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and argumentuments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the single handler code (should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

typedef enum {EXE_SINGLE, EXE_PARALLEL, EXE_SEQUENTIAL, EXE_REDIRECT}STATUS_CODE;
#define MAXARGLEN 128

/** This part is specifically designed for creation of a hash map for fast lookup. Its purpose is for the fast lookup of all the non built-in commands and execute them using execvp
*/
typedef struct MapNode{
  char* commandName;
}MapNode;

//Signal handling in normal cases (IGNORE and print ^C)
void sigint_handler(int signo) {
}

STATUS_CODE parseInput(char *arguments[MAXARGLEN], char** parsedInput[MAXARGLEN], int* parsedInputLen) {
	// This function will parse the input string into multiple commands or a single command with argumentuments depending on the delimiter (&&, ##, >, or spaces).
  STATUS_CODE returnStatus=EXE_SINGLE;
  int i=0;
  int j=0; parsedInput[j]=(char**)malloc(sizeof(char*)*MAXARGLEN);
  int p=0;
  while(arguments[i]!=NULL) {
    if(strcmp(arguments[i],"&&")!=0 && strcmp(arguments[i],"##")!=0 && strcmp(arguments[i],">")!=0) {
      parsedInput[j][p]=arguments[i];
      p++;
    } else {
      if(strcmp(arguments[i],"##")==0) returnStatus=EXE_SEQUENTIAL;
      else if(strcmp(arguments[i],"&&")==0) returnStatus=EXE_PARALLEL;
      else if(strcmp(arguments[i],">")==0) returnStatus=EXE_REDIRECT;
      parsedInput[j][p]=NULL;
      p=0;
      j++;
      parsedInput[j]=(char**)malloc(sizeof(char*)*MAXARGLEN);
    }
    i++;
  }
  parsedInput[j][p]=NULL;
  *parsedInputLen=j+1;

  return returnStatus;
}

void executeCommand(char** parsedInput[MAXARGLEN], int currentInput) {
	// This function will fork a new process to execute a command
  char* command=parsedInput[currentInput][0];
  if(strcmp(command,"cd")==0) {
    if(parsedInput[currentInput][2]!=NULL) {
      printf("bash: cd: too many arguments\n");
      return;
    }
    int st_code=chdir(parsedInput[currentInput][1]);
    
    if (st_code!=0) {
      fprintf(stderr,"bash: cd: %s: ",parsedInput[currentInput][1]);
      perror("");
    }
  } else if(strcmp(command,"pwd")==0) {
    char currentWorkingDirectory[200];
    getcwd(currentWorkingDirectory,200);
    printf("%s\n",currentWorkingDirectory);
  } else if(strcmp(command,"echo")==0) {
    int i=1;
    while(parsedInput[currentInput][i]!=NULL) {printf("%s ",parsedInput[currentInput][i]); i++;}
    printf("\n");
  } else if(strcmp(command,"ls")==0) {
    int pid=fork();
    if(pid==0) {
      execvp(parsedInput[currentInput][0], parsedInput[currentInput]);
      perror("bash");
      exit(1);
    } else if(pid>0) {
      wait(NULL);
    }
  } else if(strcmp(command,"cat")==0) {
    int pid=fork();
    if(pid==0) {
      execvp(parsedInput[currentInput][0],parsedInput[currentInput]);
      perror("bash");
      exit(1);
    } else if(pid>0) {
      wait(NULL);
    }
  }
  else {
    printf("Shell: Incorrect command\n");
  }
}

void executeParallelCommands(char **parsedInput[MAXARGLEN], int parsedInputLen) {
	// This function will run multiple commands in parallel
  int pids[parsedInputLen];
  for(int i=0;i<parsedInputLen;i++) {
    int pid=fork();
    if(pid==0) {
      //execute command
      char *command=parsedInput[i][0];
      if(strcmp(command,"cd")==0) {
        if(parsedInput[i][2]!=NULL) {
          printf("bash: cd: too many arguments\n");
        } else {
          int st_code=chdir(parsedInput[i][1]);
          if(st_code!=0) perror("bash: cd");
        }
        exit(0);  // child exits
      } else if(strcmp(command,"pwd")==0) {
          char currentWorkingDirectory[200];
          getcwd(currentWorkingDirectory,200);
          printf("%s\n", currentWorkingDirectory);
          exit(0);
      } else if(strcmp(command,"echo")==0) {
        int j=1;
        while(parsedInput[i][j]!=NULL) {
          printf("%s ",parsedInput[i][j]);
          j++;
        }
        printf("\n");
        exit(0);
      } else if(strcmp(command,"ls")==0) {
        execvp(command, parsedInput[i]);
        perror("bash");
        exit(1);
      } else if(strcmp(command,"cat")==0) {
        execvp(command, parsedInput[i]);
        perror("bash");
        exit(1);
      } 
      else {
        printf("Shell: Incorrect command\n");
        exit(1);
      }
    } else if(pid>0) pids[i]=pid;
  }

  for(int i=0;i<parsedInputLen;i++) {
    waitpid(pids[i],NULL,0);
  }
}

void executeSequentialCommands(char **parsedInput[MAXARGLEN], int parsedInputLen) {	
	// This function will run multiple commands sequentially
  for(int i=0;i<parsedInputLen;i++) {
    executeCommand(parsedInput,i);
  }
}

void executeCommandRedirection(char **parsedInput[MAXARGLEN]) {
  char *command = parsedInput[0][0];  
  char *file    = parsedInput[1][0];  
  int pid = fork();

  if (pid == 0) {
    int fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);

    if (strcmp(command, "cd") == 0) {
      if (parsedInput[0][2] != NULL) {
        fprintf(stderr, "bash: cd: too many arguments\n");
      } else if (chdir(parsedInput[0][1]) != 0) {
        perror("bash: cd");
      }
      exit(0);
    } else if (strcmp(command,"pwd")==0) {
      char cwd[200];
      getcwd(cwd,sizeof(cwd));
      printf("%s\n", cwd);
      exit(0);
    } else if(strcmp(command,"echo") == 0) {
      for (int i = 1; parsedInput[0][i] != NULL; i++) {
        printf("%s ", parsedInput[0][i]);
      }
      printf("\n");
      exit(0);
    }
    else if (strcmp(command, "exit") == 0) {
      printf("Exiting shell...\n");
      exit(0);
    }
    else {
      execvp(command, parsedInput[0]);
      perror("execvp");
      exit(1);
    }
  } 
  else if (pid > 0) {
    waitpid(pid, NULL, 0);
  } 
  else {
    perror("fork");
  }
}

int main() {
  signal(SIGINT, sigint_handler);
  signal(SIGTSTP, SIG_IGN);
	// Initial declarations
  STATUS_CODE executionType;
  int i=0;
  char *input=NULL, *arguments[MAXARGLEN];
  size_t buff_size=0;
  ssize_t len=0;
  char currentWorkingDirectory[200];
  char** parsedInput[MAXARGLEN]; int parsedInputLen=0;
	
	while(1)	{// This loop will keep your shell running until user exits.
    getcwd(currentWorkingDirectory,200);
		printf("%s$ ",currentWorkingDirectory);// Print the prompt in format - currentWorkingDirectory$
		
		// accept input with 'getline()'
    len=getline(&input,&buff_size,stdin);
    input[strcspn(input,"\n")]='\0';
    char* argument; i=0; char *copy=input;

    if(strlen(input)==0) continue;

		//Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
    argument=strsep(&copy," ");
    while(argument!=NULL) {
      if(*argument!='\0') {
      arguments[i]=argument;
      i++;
      }
      argument=strsep(&copy," ");
    }
    arguments[i]=NULL;

		
		if(strcmp(input,"exit")==0)	{// When user uses exit command.
			printf("Exiting shell...");
			break;
		}

    executionType=parseInput(arguments,parsedInput,&parsedInputLen);
		
		if(executionType==EXE_PARALLEL)
			executeParallelCommands(parsedInput,parsedInputLen);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(executionType==EXE_SEQUENTIAL)
			executeSequentialCommands(parsedInput,parsedInputLen);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(executionType==EXE_REDIRECT)
			executeCommandRedirection(parsedInput);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(parsedInput,0);		// This function is invoked when user wants to run a single commands
				
    while(parsedInputLen) {
      free(parsedInput[parsedInputLen-1]);
      parsedInputLen--;
    }
	}
  free(input);
	
	return 0;
}
