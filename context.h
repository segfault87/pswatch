#ifndef _CONTEXT_H_
#define _CONTEXT_H_

struct ConfContext {
  char logpath[256];

  int daemonize;

  int log_period;

  int process_killer;
  float process_killer_threshold;
  int process_killer_iteration;
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
