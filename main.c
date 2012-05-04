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
  unsigned long memory_usage;
  int killflag = 0;

  ProcessInfoInit();
  LogInit();

  while (1) {
    GlobProcesses();

    memory_usage = 0L;
    for (i = 0; i < upperbound; ++i) {
      struct ProcessInfo *p = &processes[i];

      if (!p->pid)
        continue;

      memory_usage += GetMemoryUsage(p);
      
      /*if (ExamineMemoryUsage(p)) {
        LogProcessKill(p);
        ProcessInfoExpire(p->pid);
      }*/
    }

    if (conf.process_killer_threshold > 0.0f) {
      while (memory_usage / (float)global.system_memory * 100.0f >=
             conf.process_killer_threshold) {
        memory_usage -= KillHighestMemoryUsage();
        killflag = 1;
      }

      if (killflag) {
        FlushKillLog();
        killflag = 0;
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
