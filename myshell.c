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
#define MAXPATHLEN 2048

char* trim(char* str) {
  int len=strlen(str);
  char *trimmed=(char*)malloc(sizeof(char)*(len+1));
  int start=0, end=len-1, p=0;
  while(start<len && str[start]==' ') start++;
  while(end>=0 && str[end]==' ') end--;
  while(start<=end) 
    trimmed[p++]=str[start++];
  trimmed[p]='\0';
  free(str);
  return trimmed;
}
char* preprocess(char* original_input) {
  char* preprocessed_input=(char*)malloc(sizeof(char)*(4*strlen(original_input)+1));
  int p=0;
  for(int i=0;i<strlen(original_input);i++) {
    char c=original_input[i];
    if(i<strlen(original_input)-1 && c=='&' && original_input[i+1]=='&') {
      preprocessed_input[p++]=' '; preprocessed_input[p++]='&'; preprocessed_input[p++]='&'; 
      preprocessed_input[p++]=' ';
      i++;
    }
    else if(i<strlen(original_input)-1 && c=='#' && original_input[i+1]=='#') {
      preprocessed_input[p++]=' '; preprocessed_input[p++]='#'; preprocessed_input[p++]='#'; 
      preprocessed_input[p++]=' ';
      i++;
    }
    else if(c=='>') {
      preprocessed_input[p++]=' '; preprocessed_input[p++]='>'; preprocessed_input[p++]=' '; 
    }
    else if(c==';') {
      preprocessed_input[p++]=' '; preprocessed_input[p++]=';'; preprocessed_input[p++]=' ';
    }
    else preprocessed_input[p++]=c;
  }
  preprocessed_input[p]='\0';
  free(original_input);
  return preprocessed_input;
}

STATUS_CODE parseInput(char *arguments[MAXARGLEN], char** parsedInput[MAXARGLEN], int* parsedInputLen) {
	// This function will parse the input string into multiple commands or a single command with argumentuments depending on the delimiter (&&, ##, >, or spaces).
  STATUS_CODE returnStatus=EXE_SINGLE;
  int i=0;
  int j=0; parsedInput[j]=(char**)malloc(sizeof(char*)*MAXARGLEN);
  int p=0;
  while(arguments[i]!=NULL) {
    if(strcmp(arguments[i],"&&")!=0 && strcmp(arguments[i],"##")!=0 && strcmp(arguments[i],">")!=0 && strcmp(arguments[i],";")!=0) {
      parsedInput[j][p]=arguments[i];
      p++;
    } else {
      if(strcmp(arguments[i],"##")==0 || strcmp(arguments[i],";")==0) returnStatus=EXE_SEQUENTIAL;
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
  int st_code;
  char* command=parsedInput[currentInput][0];
  if(strcmp(command,"cd")==0) {
    if(parsedInput[currentInput][2]!=NULL) {
      printf("Shell: Incorrect command\n");
      return;
    }
    st_code=chdir(parsedInput[currentInput][1]);
    if (st_code!=0) {
      printf("Shell: Incorrect command\n");
    }
  } else if(strcmp(command,"exit")==0) {
    printf("Exiting shell...\n");
    exit(0);
  } else {
    int pid=fork();
    if(pid==0) {
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      
      execvp(parsedInput[currentInput][0], parsedInput[currentInput]);
      printf("Shell: Incorrect command\n");
      exit(1);
    } else if(pid>0) {
      wait(NULL);
    }
  } 
}

void executeParallelCommands(char **parsedInput[MAXARGLEN], int parsedInputLen) {
	// This function will run multiple commands in parallel
  int pids[parsedInputLen];
  for(int i=0;i<parsedInputLen;i++) {
    int pid=fork();
    if(pid==0) {
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);  
      char *command=parsedInput[i][0];
      if(strcmp(command,"cd")==0) {
        int st_code;
        if(parsedInput[i][2]!=NULL) {
          printf("Shell: Incorrect command\n");
        }  else
          st_code=chdir(parsedInput[i][1]);
        if(st_code!=0) printf("Shell: Incorrect command\n");
        exit(0);  
      } else {
        execvp(command, parsedInput[i]);
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
  char *command=parsedInput[0][0];  
  char *file=parsedInput[1][0];  
  int pid=fork();

  if(pid==0) {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    int fd=open(file,O_CREAT|O_WRONLY|O_TRUNC,0644);
    dup2(fd,STDOUT_FILENO);
    close(fd);

    execvp(command,parsedInput[0]);
    exit(1);
  } 
  else if (pid > 0) {
    waitpid(pid, NULL, 0);
  } 
}

int main() {

  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);

	// Initial declarations
  STATUS_CODE executionType;
  int i=0;
  char *input=NULL, *arguments[MAXARGLEN];
  size_t buff_size=0;
  ssize_t len=0;
  char currentWorkingDirectory[MAXPATHLEN];
  char** parsedInput[MAXARGLEN]; int parsedInputLen=0;
	
	while(1)	{// This loop will keep your shell running until user exits.
    getcwd(currentWorkingDirectory,MAXPATHLEN);
		printf("%s$",currentWorkingDirectory);// Print the prompt in format - currentWorkingDirectory$
		
		// accept input with 'getline()'
    len=getline(&input,&buff_size,stdin);
    
    if(len==-1) {
      printf("Exiting shell...\n");
      break;
    }
    
    input[strcspn(input,"\n")]='\0';
    input=trim(input);
    char* argument; i=0; 

    if(strlen(input)==0) continue;
    input=preprocess(input);
    
    char *copy=strdup(input);
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
			printf("Exiting shell...\n");
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
