#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>

/*
TODO:
-Foreground and Background Processes
  -Alert the user when a process has started in the background.
  -Alert the user when a background process ends.
-Built-In Functions
  -Set
    -Allow passing of environmental variables to child processes
  -CD
    -Allow changing of current directory with "CD <dir>"
  -Quit and Exit
  -Jobs
    -Update this table when a process ends.
    -Allow user to display this table with "jobs"
-I/O Redirection
  -Implement injection into and output from command line with "<" and ">" respectively.
-Batch Execution
  -Allow batches of commands to be read from a file.

KNOWN BUGS:
-Piped programs do not return input to user after execution
*/

char** env_path;
char* env_home;

int background_pids[100] = {-1};
bool is_running = true;

// FILE UTILITY FUNCTIONS ----------------------------------------
bool fileExists(char* filename)
{
  //Returns true if a file exists, false otherwise
  return (access(filename, F_OK ) != -1);
}

char* getCorrectEnvPath(char* executable)
{
  //Returns the correct path for an executable. (assuming env_path has been populated - see setUpEnv)
  //Returns "" if there is no file path that meets these specifications.
  for (int i = 1; (i <= 100) && (env_path[i] != NULL); i++)
  {
    char* element = env_path[i];
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
  //Sets env_path and env_home. Note that the envp that is passed here should be the envp that is retrieved from the envp argument of main.
  char* buf = envp[0]+5;
  for (int i = 0; envp[i] != NULL; i++)
  {
    char* element = envp[i];
    if ((element[0] == 'P') && (element[1] == 'A') && (element[2] == 'T') && (element[3] == 'H'))
    {
      buf = element+5;
    }
    if ((element[0] == 'H') && (element[1] == 'O') && (element[2] == 'M') && (element[3] == 'E'))
    {
      env_home = element+5;
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

  env_path = array_to_return;
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
  //printf("HERE: %s\n", toReturn[1]);
  return toReturn;
}

void setUpPIDList()
{
  int my_background[100];
  for (int i = 0; i < 100; i++)
  {
    my_background[i] = -1;
  }
}

void deregisterPID(int pid)
{
  int i = 0;
  while (background_pids[i] != pid){ i++; }
  background_pids[i] = -1;
}

void registerPID(int pid)
{
  int i = 0;
  while (background_pids[i] == -1) i++;
  background_pids[i] = pid;
}
//----------------------------------------------------------------
//SIGNAL HANDLERS-------------------------------------------------
void handleEndedProcess()
{
  // pid_t pid_to_kill;
  // while ((pid_to_kill = waitpid((pid_t)(-1), NULL, WNOHANG)) != -1)
  // {
  //   deregisterPID(pid_to_kill);
  // }
  pid_t pid;
  pid = wait(NULL);
  printf("PID %d has exited\n", pid);
  deregisterPID(pid);
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
      printf("I couldn't find that.\n");
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

int spawnPipedProcess(char* toExec, char* simple_args, int* pipe, bool front)
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
//SHELL COMMANDS--------------------------------------------------
void changeCurrentDirectory(char* newDirectory)
{
  printf("Not yet implemented, dude!\n");
}

void showJobs()
{
  for (int i = 0; i < 100; i++)
  {
    if (background_pids[i] != -1)
    {
      printf("[%d] %d\n", i, background_pids[i]);
    }
  }
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
  pid_t pid1 = spawnPipedProcess(splitCommand1[0], splitCommand1[1], new_pipe, true);
  waitpid(pid1, &status, 0);
  pid_t pid2 = spawnPipedProcess(splitCommand2[0], splitCommand2[1], new_pipe, false);
  waitpid(pid2, &status, 0);

  int* to_return = malloc(2);
  to_return[0] = pid1;
  to_return[1] = pid2;

  return to_return;
}

void shellSet(char* variable){
  char* str_VarID;
  char* str_NewValue;
  str_VarID = strtok(variable, "=");
  str_NewValue = strtok(NULL, "=");
  //setting PATH
  if(strcmp(str_VarID, "PATH") == 0){
    env_path = NULL;
    env_path = malloc(sizeof(char*) * 100);
    int i = 0;
    while((str_NewValue != NULL) && (i < 100)){
      char* singlePathEntry = NULL;
      if(i == 0){
        singlePathEntry = strtok(str_NewValue, ":");
      }
      else{
        singlePathEntry = strtok(NULL, ":");
      }
      if(singlePathEntry != NULL){
        env_path[i] = singlePathEntry;
      }
      i++;
    }
  }
  //setting HOME
  else if(strcmp(str_VarID, "HOME") == 0){
    env_home = str_NewValue;
  }
}

bool handleShellCommand(char* command)
{
  //Executes the shell command passed to it. If it is not a shell command, return False. Otherwise, return True.
  char* commandCopy = malloc(strlen(command)); //Make a copy of the command so as not to cause any upstream problems.
  strcpy(commandCopy, command);
  char** splitCommand = commandSplitter(commandCopy);
  char* theShellCommand = splitCommand[0];

  if (strcmp(theShellCommand, "quit") == 0)
  {
    is_running = false;
    return true;
  }
  if (strcmp(theShellCommand, "exit") == 0)
  {
    is_running = false;
    return true;
  }
  if (strcmp(theShellCommand, "set") == 0) //Not implemented - uncomment this when zach is done
  {
    shellSet(splitCommand[1]);
    return true;
  }
  if (strcmp(theShellCommand, "cd") == 0)
  {
    changeCurrentDirectory(splitCommand[1]);
    return true;
  }
  if (strcmp(theShellCommand, "jobs") == 0)
  {
    showJobs();
    return true;
  }

  return false;
}

int createBackgroundProcess(char* command)
{
  //A wrapper around handleCommand. Currently does not do anything special.
  pid_t new_pid = handleCommand(command);
  registerPID(new_pid);
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
  if (handleShellCommand(input))
  {
    return -1;
  }
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
  setUpPIDList(); //Causes a segfault.
  signal(SIGCHLD, handleEndedProcess);
  char input[100];
  pid_t child;

  while (is_running){
    printf("> ");
    fgets(input,100,stdin);
    child = handleInput(input);
    if (child != -1)
    {
      waitpid(child, &status, 0);
    }
  }

  printf("QUASH, OVER AND OUT\n");
}
