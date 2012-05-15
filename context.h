#ifndef _CONTEXT_H_
#define _CONTEXT_H_

struct ConfContext {
  char logpath[256];
  int daemonize;
  int log_period;
  float process_killer_threshold;
  int sleep_msec;
};

struct GlobalContext {
  unsigned long system_memory;
  int pid_max;
  int page_size;
};

extern struct ConfContext conf;
extern struct GlobalContext global;

unsigned long GetPhysicalRamSize(void);
void InitializeContext(void);

#endif
