#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "context.h"
#include "log.h"
#include "pswatch.h"

void Loop(void)
{
  int iterations = 0;
  int i;

  ProcessInfoInit();
  LogInit();

  while (1) {
    GlobProcesses();

    for (i = 0; i < indexcount; ++i) {
      struct ProcessInfo *p = processes[indices[i]];

      if (!p)
        continue;
    }

    if (iterations > 0 && iterations % conf.log_period == 0)
      DumpProcessInfo(iterations == 0 ? 1 : 0);

    sleep(1);
    
    ++iterations;
  }
}

int ParseCommandLine(int argc, char *argv[])
{
  return 0;
}

int main(int argc, char *argv[])
{
  pid_t pid;

  InitializeContext();
  
  if (ParseCommandLine(argc, argv) < 0)
    return -1;

  if (conf.daemonize) {
    pid = fork();
    
    if (pid < 0)
      return -1;
    else if (pid > 0)
      return 0;
    else
      Loop();
  } else {
    Loop();
  }

  return 0;
}
