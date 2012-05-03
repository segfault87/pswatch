#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "context.h"
#include "pswatch.h"

struct ProcessInfo processes[PROCESSES_MAX];
int life;
int nprocs;
int upperbound;

void ProcessInfoInit(void)
{
  int i;

  nprocs = 0;
  upperbound = 0;
  life = 0;

  for (i = 0; i < PROCESSES_MAX; ++i)
    memset(&processes[i], 0, sizeof(struct ProcessInfo));
}

struct ProcessInfo* ProcessInfoRetrieve(int pid)
{
  int i;

  for (i = 0; i < upperbound; ++i) {
    if (processes[i].pid == pid)
      return &processes[i];
  }

  if (upperbound >= PROCESSES_MAX) {
    for (i = 0; i < upperbound; ++i) {
      if (processes[i].pid == 0) {
        ++nprocs;
        return &processes[i];
      }
    }
  }

  ++nprocs;

  return &processes[upperbound++];
}

void ProcessInfoExpire(int pid)
{
  int i;

  for (i = 0; i < upperbound; ++i) {
    if (processes[i].pid == pid) {
      processes[i].pid = 0;
      --nprocs;
      return;
    }
  }
}

int UpdateProcessInfo(int pid)
{
  int fd;
  char path[128];
  char readbuf[1024];
  size_t nread;
  char *saveptr = NULL, *ptr;
  struct ProcessInfo *process;

  process = ProcessInfoRetrieve(pid);

  /* get stat */
  snprintf(path, 128, "/proc/%d/stat", pid);
  fd = open(path, O_RDONLY);

  if (fd < 0)
    return -1;

  if ((nread = read(fd, readbuf, sizeof(readbuf))) < 0) {
    close(fd);
    return -1;
  }

  close(fd);
  readbuf[nread] = '\0';

  /* parse it up */

  ptr = strtok_r(readbuf, " ", &saveptr);
  process->pid = atoi(ptr);
  ptr = strtok_r(NULL, " ", &saveptr);
  strncpy(process->procname, ptr, sizeof(process->procname));
  
  /* skip */
  strtok_r(NULL, " ", &saveptr); // state
  strtok_r(NULL, " ", &saveptr); // ppid
  strtok_r(NULL, " ", &saveptr); // pgid
  strtok_r(NULL, " ", &saveptr); // session
  strtok_r(NULL, " ", &saveptr); // tty_nr
  strtok_r(NULL, " ", &saveptr); // tpgid
  strtok_r(NULL, " ", &saveptr); // flags
  strtok_r(NULL, " ", &saveptr); // minflt
  strtok_r(NULL, " ", &saveptr); // cminflt
  strtok_r(NULL, " ", &saveptr); // majflt
  strtok_r(NULL, " ", &saveptr); // cmajflt
  strtok_r(NULL, " ", &saveptr); // utime
  strtok_r(NULL, " ", &saveptr); // stime
  strtok_r(NULL, " ", &saveptr); // cutime
  strtok_r(NULL, " ", &saveptr); // cstime
  strtok_r(NULL, " ", &saveptr); // priority
  strtok_r(NULL, " ", &saveptr); // nice
  strtok_r(NULL, " ", &saveptr); // nthreads
  strtok_r(NULL, " ", &saveptr); // itrealvalue
  strtok_r(NULL, " ", &saveptr); // starttime
  strtok_r(NULL, " ", &saveptr); // vmsize
    
  ptr = strtok_r(NULL, " ", &saveptr);
  process->rss = strtoul(ptr, NULL, 10);

  if (!process->rss_initial)
    process->rss_initial = process->rss;

  /* oom_score */
  snprintf(path, 128, "/proc/%d/oom_score", pid);
  fd = open(path, O_RDONLY);

  if (fd >= 0) {
    if ((nread = read(fd, readbuf, sizeof(readbuf))) < 0) {
      close(fd);
      return -1;
    }
  }

  close(fd);
  readbuf[nread] = '\0';
  
  process->oom_score = atoi(readbuf);

  process->life = life;

  return 0;
}

int GlobProcesses(void)
{
  DIR *dir;
  struct dirent *entry;
  int pid;
  int i;

  ++life;

  dir = opendir("/proc");
  if (!dir)
    return -1;

  while ((entry = readdir(dir))) {
    if (!isdigit(entry->d_name[0]))
      continue;

    pid = atoi(entry->d_name);
    UpdateProcessInfo(pid);
  }

  for (i = 0; i < upperbound; ++i) {
    if (processes[i].pid && processes[i].life != life) {
      processes[i].pid = 0;
      --nprocs;
    }
  }

  return 0;
}

unsigned long GetMemoryUsage(struct ProcessInfo *p)
{
  if (!p)
    return 0;

  return p->rss * global.page_size;
}

void KillHighestMemoryUsage(void)
{
  int i;
  int pid = -1;
  int highest = 0;

  for (i = 0; i < upperbound; ++i) {
    struct ProcessInfo *p = &processes[i];
    if (p->pid && p->rss > highest) {
      pid = p->pid;
      highest = p->rss;
    }
  }

  if (pid >= 0) {
    kill(pid, SIGKILL);
    ProcessInfoExpire(pid);
  }
}

int ExamineMemoryUsage(struct ProcessInfo *p)
{
  float percent;

  if (conf.process_killer_threshold == 0.0f)
    return 0;

  percent = (p->rss * global.page_size) / (float)global.system_memory * 100.0f;
  if (percent > conf.process_killer_threshold) {
    kill(p->pid, SIGKILL);
    return 1;
  }

  return 0;
}

