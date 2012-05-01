#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "context.h"
#include "pswatch.h"

struct ProcessInfo *processes[PID_MAX];
int indices[PID_MAX];
int indexcount;

void ProcessInfoInit(void)
{
  int i;

  for (i = 0; i < PID_MAX; ++i)
    processes[i] = NULL;
}

struct ProcessInfo* ProcessInfoRetrieve(int pid)
{
  if (!processes[pid]) {
    processes[pid] = (struct ProcessInfo *) malloc(sizeof(struct ProcessInfo));
    memset(processes[pid], 0, sizeof(struct ProcessInfo));
  }

  return processes[pid];
}

void ProcessInfoExpire(int pid)
{
  if (!processes[pid])
    return;

  free(processes[pid]);
  processes[pid] = NULL;
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

  return 0;
}

int GlobProcesses(void)
{
  DIR *dir;
  struct dirent *entry;
  int pid;

  dir = opendir("/proc");
  if (!dir)
    return -1;

  indexcount = 0;
  while ((entry = readdir(dir))) {
    if (!isdigit(entry->d_name[0]))
      continue;

    pid = atoi(entry->d_name);
    UpdateProcessInfo(pid);
    indices[indexcount++] = pid;
  }

  return 0;
}

#if 0
int main(void)
{
  int i;

  ProcessInfoInit();

  while (1) {
    GlobProcesses();
    
    for (i = 0; i < indexcount; ++i) {
      struct ProcessInfo *p = processes[indices[i]];
      
      if (p && p->rss_initial != p->rss) {
        int rssdelta = p->rss - p->rss_initial;
        printf("%d %s: %fmb\n", p->pid, p->procname, (rssdelta * PAGE_SIZE / 1048576.0f));
      }
    }

    sleep(1);
  }

  return 0;
}
#endif
