#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

/*
TODO:
-Foreground and Background Processes
  -Alert the user when a process has started in the background.
  -Alert the user when a background process ends.
-Built-In Functions
  -Set
    -Create system of environment vairables
    -Use set to change these
    -Allow passing of environmental variables to child processes
  -CD
    -Allow changing of current directory with "CD <dir>"
  -Quit and Exit
  -Jobs
    -Create table of running background processes.
    -Update this table when a process ends.
    -Allow user to display this table with "jobs"
-I/O Redirection
  -Implement injection into and output from command line with "<" and ">" respectively.
-Batch Execution
  -Allow batches of commands to be read from a file.

KNOWN BUGS:
-Piped programs do not return input to user after execution
*/

char** main_envp;


// FILE UTILITY FUNCTIONS ----------------------------------------
_Bool fileExists(char* filename)
{
  //Returns true if a file exists, false otherwise
  return (access(filename, F_OK ) != -1);
}

char* getCorrectEnvPath(char* executable)
{
  //Returns the correct path for an executable. (assuming main_envp has been populated - see setUpEnv)
  //Returns "" if there is no file path that meets these specifications.
  for (int i = 1; (i <= 100) && (main_envp[i] != NULL); i++)
  {
    char* element = main_envp[i];
    char* pathDivider = "/";
    char* full_path = malloc(strlen(element) + strlen(pathDivider) + strlen(executable) + 1);
    strcpy(full_path, element);
    strcat(full_path, pathDivider);
    strcat(full_path, executable);
    printf("PATH: %s\n", full_path);
    if (fileExists(full_path))
    {
      return full_path;
    }
  }
  printf("NOOOOO\n");
  return "";
}

char* getTruePath(char* command)
{
  //Gets the actual path that will be used to run the executable.
  if (command[0] == '/') //Absolute Path
  {
    if (fileExists(command))
    {
      return command;
    }
    else
    {
      return "";
    }
  }
  else
  {
    return getCorrectEnvPath(command);
  }
}

char** setUpEnv(char** envp)
{
  //Sets main_envp. Note that the envp that is passed here should be the envp that is retrieved from the envp argument of main.
  char* buf = envp[0]+5;
  for (int i = 0; envp[i] != NULL; i++)
  {
    char* element = envp[i];
    if ((element[0] == 'P') && (element[1] == 'A') && (element[2] == 'T') && (element[3] == 'H'))
    {
      buf = element+5;
    }
  }
  int i = 0;
  int number_of_elements;
  char *p = strtok (buf, ":");
  char *array[100];

  while (p != NULL)
  {
      array[i++] = p;
      p = strtok (NULL, ":");
  }
  array[i++] = '\0';
  number_of_elements = i;

  char** array_to_return = malloc(sizeof(char*)*number_of_elements);

  for (i = 0; (i < 99) && (array[i] != NULL); ++i)
  {
    array_to_return[i] = array[i];
  }

  main_envp = array_to_return;
  return array_to_return;
}
//----------------------------------------------------------------
//GENERAL UTILITY FUNCTIONS---------------------------------------
char** commandSplitter(char* command)
{
  //Splits a command into two parts - the part before the first space, and the part after it.
  //This function DOES change the parameter passed to it, so be careful with that, if that is a concern.
  char* args;
  char* exec;
  char* sanitizedCommand;
  if (command[0] == ' ')
  {
    sanitizedCommand = command+1;
  }
  else
  {
    sanitizedCommand = command;
  }
  if (sanitizedCommand[strlen(sanitizedCommand)-1] == '\n') //Strip the newline off the end of the command (from when user hits "enter")
  {
    sanitizedCommand[strlen(sanitizedCommand)-1] = 0; //We accomplish the stripping by replacing the newline with a null terminator.
  }

  if (strchr(sanitizedCommand, ' ') != NULL) //This branch runs when there are spaces in the command.
  {
    char delim[] = " ";
    char* beginningOfArgs = strchr(sanitizedCommand, ' '); //A pointer to the first space.
    exec = strtok(sanitizedCommand, delim); //The strtok function replaces the first instance of ' ' in command with a /0 (null terminator), indicating the end of the string.
    args = beginningOfArgs+1; //The first character iof the arguments is the one right after the first space. char* arrays are densly packed, meaning pointer arithmetic is valid.
  }
  else //This is for there being no spaces.
  {
    exec = sanitizedCommand; //If there are no spaces, then exec is the entirety of command.
    args = ""; //There are no args.
  }
  char** toReturn = malloc(2); //Create an array of two elements on the heap.
  toReturn[0] = exec;
  toReturn[1] = args;
  return toReturn;
}
//----------------------------------------------------------------
//SPAWNING FUNCTIONS ---------------------------------------------
int spawnProcess(char* toExec, char* simple_args)
{
  /*
  Creates a new process.
  toExec - complete path to an executable.
  simple_args - args you wish to pass to the executable. A string.
  */
  pid_t new_pid = fork();
  if (new_pid == 0)
  {
    printf("SPAWNER EXEC: '%s' ARGS: '%s'\n", toExec, simple_args);

    char* path = getTruePath(toExec);
    if (strcmp(path, "") == 0)
    {
      printf("I couldn't find that.");
      return -1;
    }

    int SIZE = 256;
    char cmdbuf[SIZE];
    bzero(cmdbuf, SIZE);
    sprintf(cmdbuf, "%s", path);

    char argbuf[SIZE];
    bzero(argbuf, SIZE);
    sprintf(argbuf, "%s", simple_args);

    if (strcmp(simple_args, "") != 0)
    {
      char* myArgs[] = {cmdbuf, argbuf, (char*) 0};
      execv(cmdbuf, myArgs);
    }
    else
    {
      char* myArgs[] = {cmdbuf, (char*) 0};
      execv(cmdbuf, myArgs);
    }

  }
  else if (new_pid < 0)
  {
    printf("Error creating process.");
  }

  return new_pid;
}

int spawnPipedProcess(char* toExec, char* simple_args, int* pipe, _Bool front)
{
  /*
  Like the first spawnProcess, but this also allows processes to be connected with pipes. The pipe variable is a fd list, set front to true if it is the start of the pipe.
  */

  pid_t new_pid = fork();
  if (new_pid == 0)
  {
    printf("SPAWNER EXEC: '%s' ARGS: '%s'\n", toExec, simple_args);



    char* path = getTruePath(toExec);
    if (strcmp(path, "") == 0)
    {
      printf("I couldn't find that.");
      return -1;
    }

    int SIZE = 256;
    char cmdbuf[SIZE];
    bzero(cmdbuf, SIZE);
    sprintf(cmdbuf, "%s", path);

    char argbuf[SIZE];
    bzero(argbuf, SIZE);
    sprintf(argbuf, "%s", simple_args);

    //Do special stuff with duplication
    if (front)
    {
      dup2(pipe[1], STDOUT_FILENO);
    }
    else
    {
      dup2(pipe[0], STDIN_FILENO);
    }

    if (strcmp(simple_args, "") != 0)
    {
      char* myArgs[] = {cmdbuf, argbuf, (char*) 0};
      execv(cmdbuf, myArgs);
    }
    else
    {
      char* myArgs[] = {cmdbuf, (char*) 0};
      execv(cmdbuf, myArgs);
    }

  }
  else if (new_pid < 0)
  {
    printf("Error creating process.");
  }

  return new_pid;
}

//----------------------------------------------------------------
//COMMAND HANDLERS------------------------------------------------
int handleCommand(char* command)
{
  //Handles a single command.
  //Does NOT check for pipes, backgrounders, or shell tasks.
  char** splitResults = commandSplitter(command);
  char* exec = splitResults[0];
  char* args = splitResults[1];
  printf("HANDLER EXEC: '%s' ARGS: '%s'\n", exec, args);
  return spawnProcess(exec, args);
}

int* handlePipedInput(char* input)
{
  //Handles two commands linked together by a pipe.
  //Does NOT check for backgrounders or shell commands.
  char delim[] = "|";
  char* pipePtr = strtok(input, delim);
  char* command1 = pipePtr;
  char* command2 = strtok(NULL, delim);

  int new_pipe[2];
  pipe(new_pipe);

  char** splitCommand1 = commandSplitter(command1);
  char** splitCommand2 = commandSplitter(command2);

  int status;
  pid_t pid1 = spawnPipedProcess(splitCommand1[0], splitCommand1[1], new_pipe, (1==1));
  waitpid(pid1, &status, 0);
  pid_t pid2 = spawnPipedProcess(splitCommand2[0], splitCommand2[1], new_pipe, (1==0));
  waitpid(pid2, &status, 0);

  int* to_return = malloc(2);
  to_return[0] = pid1;
  to_return[1] = pid2;

  return to_return;
}

int createBackgroundProcess(char* command)
{
  //A wrapper around handleCommand. Currently does not do anything special.
  pid_t new_pid = handleCommand(command);
  printf("[%d] '%s' %d\n", new_pid, command, new_pid);
  return new_pid;
}

int createForegroundProcess(char* command)
{
  //A wrapper around handleCommand. Currently does not do anything special.
  int status;
  pid_t new_pid = handleCommand(command);
  waitpid(new_pid, &status, 0);
  return new_pid;
}
//------------------------------------------------------------------
int handleInput(char* input)
{
  pid_t child;
  if (strchr(input, '|') != NULL)
  {
    return handlePipedInput(input)[0];
  }
  if (input[0] == '&')
  {

    if (input[1] == ' ')
    {
      child = createBackgroundProcess(input+2);
    }
    else
    {
      child = createBackgroundProcess(input+1);
    }
  }
  else
  {
    child = createForegroundProcess(input);
  }
  return child;
}

int main(int argc, char* argv[], char** envp)
{
  printf("QUASH v0.3\n");
  int status;
  setUpEnv(envp);

  char test[20];
  pid_t child;

  while (1){
    printf("> ");
    fgets(test,20,stdin);
    child = handleInput(test);
    waitpid(child, &status, 0);
  }

  printf("QUASH, OVER AND OUT\n");
}
