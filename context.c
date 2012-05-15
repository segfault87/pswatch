#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "context.h"
#include "pswatch.h"

struct ConfContext conf;
struct GlobalContext global;


// XXX: 물리적 메모리만 계산한다. swap 은?

unsigned long GetPhysicalRamSize(void)
{
  char data[128];
  char *token, *saveptr = NULL;
  size_t nread;
  int fd;

  fd = open("/proc/meminfo", O_RDONLY);

  if (fd < 0)
    return -1;

  if ((nread = read(fd, data, sizeof(data))) < 0) {
    close(fd);
    return -1;
  }

  close(fd);

  if (nread >= 128)
    nread = 127;
  
  data[nread] = '\0';
  token = strtok_r(data, " \t\n\r", &saveptr);
  token = strtok_r(NULL, " \t\n\r", &saveptr);

  return atol(token) * 1024;
}

void InitializeContext(void)
{
  /* initialize config */
  strcpy(conf.logpath, ".");

  conf.daemonize = 1;

  conf.log_period = 60 * 3; /* three mins */

  conf.process_killer_threshold = 0.0f;

  conf.sleep_msec = 1000;

  /* initialize global context */
  global.system_memory = GetPhysicalRamSize();
  global.pid_max = PID_MAX;		 // cat /proc/sys/kernel/pid_max
  global.page_size = PAGE_SIZE;
}
