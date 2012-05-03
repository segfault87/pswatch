#ifndef _PSWATCH_H_
#define _PSWATCH_H_

#define PID_MAX 32768
#define PAGE_SIZE 4096
#define PROCESSES_MAX 200

struct ProcessInfo {
  int pid;
  char procname[128];
  int oom_score;
  unsigned long rss;
  unsigned long rss_initial;
  int life;
};

extern struct ProcessInfo processes[PROCESSES_MAX];
extern int life;
extern int nprocs;
extern int upperbound;

void ProcessInfoInit(void);
struct ProcessInfo* ProcessInfoRetrieve(int pid);
void ProcessInfoExpire(int pid);
int UpdateProcessInfo(int pid);
int GlobProcesses(void);
unsigned long GetMemoryUsage(struct ProcessInfo *p);
void KillHighestMemoryUsage(void);
int ExamineMemoryUsage(struct ProcessInfo *p);

#endif
