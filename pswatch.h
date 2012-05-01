#ifndef _PSWATCH_H_
#define _PSWATCH_H_

#define PID_MAX 32768
#define PAGE_SIZE 4096

struct ProcessInfo {
  int pid;
  char procname[128];
  int oom_score;
  unsigned long rss;
  unsigned long rss_initial;
};

extern struct ProcessInfo *processes[PID_MAX];
extern int indices[PID_MAX];
extern int indexcount;

void ProcessInfoInit(void);
struct ProcessInfo* ProcessInfoRetrieve(int pid);
void ProcessInfoExpire(int pid);
int UpdateProcessInfo(int pid);
int GlobProcesses(void);

#endif
