#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

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

int main(int argc, char* argv[])
{
  printf("QUASH v0.3\n");
  int status;

  char test[20];
  pid_t child;

  while (1){
    printf("> ");
    fgets(test,20,stdin);
    child = handleCommand(test);
    waitpid(child, &status, 0);
  }


  printf("QUASH, OVER AND OUT\n");
}
