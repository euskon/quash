#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

_Bool fileExists(char* filename)
{
  return (access(filename, F_OK ) != -1);
}

_Bool executableInPath(char* path, char* executable)
{
  char* pathDivider = "/";
  char* result = malloc(strlen(path) + strlen(pathDivider) + strlen(executable) + 1);
  strcpy(result, path);
  strcat(result, pathDivider);
  strcat(result, executable);
  return fileExists(result);
}

char* getCorrectPath(char** pathList, char* executable)
{
  for (int i = 0; pathList[i] != NULL; i++)
  {
    char* element = pathList[i];
    if (executableInPath(element, executable))
    {
      return element;
    }
  }
  return "";
}

int spawnProcess(char* toExec, char* simple_args)
{
  /*
  toExec - complete path to an executable.
  simple_args - args you wish to pass to the executable. A string.
  */
  pid_t new_pid = fork();
  if (new_pid == 0)
  {
    printf("SPAWNER EXEC: '%s' ARGS: '%s'\n", toExec, simple_args);
    int SIZE = 256;
    char cmdbuf[SIZE];
    bzero(cmdbuf, SIZE);
    sprintf(cmdbuf, "%s", toExec);

    char argbuf[SIZE];
    bzero(argbuf, SIZE);
    sprintf(argbuf, "%s", simple_args);

    if (strcmp(simple_args, "") != 0)
    {
      char* myArgs[] = {cmdbuf, argbuf, (char*) 0};
      execv(toExec, myArgs);
    }
    else
    {
      char* myArgs[] = {cmdbuf, (char*) 0};
      execv(toExec, myArgs);
    }

  }
  else if (new_pid < 0)
  {
    printf("Error creating process.");
  }

  return new_pid;
}

int handleCommand(char* command)
{
  char* args;
  char* exec;
  if (strchr(command, ' ') != NULL)
  {
    char delim[] = " ";
    char* beginningOfArgs = strchr(command, ' ');
    exec = strtok(command, delim);
    args = beginningOfArgs+1;
    if (args[strlen(args)-1] == '\n'){
      args[strlen(args)-1] = 0;
    }
  }
  else
  {
    exec = command;
    if (exec[strlen(exec)-1] == '\n'){
      exec[strlen(exec)-1] = 0;
    }
    args = "";
  }
  printf("HANDLER EXEC: '%s' ARGS: '%s'\n", exec, args);
  return spawnProcess(exec, args);
}

int createBackgroundProcess(char* command)
{
  pid_t new_pid = handleCommand(command);
  printf("[%d] '%s' %d\n", new_pid, command, new_pid);
  return new_pid;
}

int createForegroundProcess(char* command)
{
  int status;
  pid_t new_pid = handleCommand(command);
  waitpid(new_pid, &status, 0);
  return new_pid;
}

int handleInput(char* input)
{
  pid_t child;
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

int main(int argc, char* argv[])
{
  printf("QUASH v0.3\n");
  int status;

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
