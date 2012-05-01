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

      if (ExamineMemoryUsage(p)) {
        LogProcessKill(p);
        ProcessInfoExpire(p->pid);
      }
    }

    if (iterations > 0 && iterations % conf.log_period == 0)
      DumpProcessInfo();

    sleep(1);
    
    ++iterations;
  }
}

int ParseCommandLine(int argc, char *argv[])
{
  int opt;
  
  while ((opt = getopt(argc, argv, "fp:l:k:")) != -1) {
    switch (opt) {
      case 'f':
        conf.daemonize = 0;
        break;
      case 'p':
        strncpy(conf.logpath, optarg, sizeof(conf.logpath));
        break;
      case 'l':
        conf.log_period = atoi(optarg);
        break;
      case 'k':
        conf.process_killer_threshold = atof(optarg);
        break;
      default:
        fprintf(stderr, "usage: %s\n", argv[0]);
        fprintf(stderr, "\t-f\t\tRun in foreground\n");
        fprintf(stderr, "\t-p <path>\tSpecify log path.\n");
        fprintf(stderr, "\t-l <secs>\tSpecify log rate. (default: every three minutes)\n");
        fprintf(stderr, "\t-k <percent>\tKill process if more than given percent of memory usage.\n");
        return -1;
    }
  }

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
