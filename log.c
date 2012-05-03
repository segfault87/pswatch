#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "context.h"
#include "log.h"
#include "pswatch.h"

struct {
  int year;
  int month;
  int day;

  int crashlog;

  FILE *logfile;
} private;

void LogRotateIfNeeded(void)
{
  int y, m, d;
  time_t curtime = time(NULL);
  struct tm *tm = localtime(&curtime);

  y = tm->tm_year + 1900;
  m = tm->tm_mon + 1;
  d = tm->tm_mday;

  if (private.year != y || private.month != m || private.day != d) {
    char filename[256];

    private.year = y;
    private.month = m;
    private.day = d;

    if (private.logfile)
      fclose(private.logfile);

    snprintf(filename, sizeof(filename), "%s/watcher.%d.log",
             conf.logpath, private.day % 2);

    private.logfile = fopen(filename, "w");
  }
}

void LogInit(void)
{
  private.year = 0;
  private.month = 0;
  private.day = 0;

  private.crashlog = 0;

  private.logfile = NULL;

  LogRotateIfNeeded();
}

int LogProcessKill(struct ProcessInfo *p)
{
  char cmd[256];
  int ret;

  fprintf(private.logfile, "Process %d %s killed due to excessive memory usage. (%.3f mbytes)\n\n",
          p->pid, p->procname, p->rss * global.page_size / 1048576.0f);

  snprintf(cmd, 256, "cp \"%s/watcher.%d.log\" \"%s/watcher.kill.%d.log\"",
           conf.logpath, private.day % 2, conf.logpath, private.crashlog++);
  ret = system(cmd);

  return ret;
}

void DumpProcessInfo(void)
{
  static int flag;
  static const char *desc[] = {"up", "down"};
  time_t curtime = time(NULL);
  struct tm *tm = localtime(&curtime);
  int i;

  fprintf(private.logfile, "Log at %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
  for (i = 0; i < upperbound; ++i) {
    struct ProcessInfo *p = &processes[i];
    
    if (p->pid && (!flag || p->rss_initial != p->rss) && p->rss > 0) {
      int rssdelta = p->rss - p->rss_initial;
      int sign;

      if (rssdelta == 0) {
        fprintf(private.logfile, "%d %s %.3f mbytes, oom_score: %d\n",
                p->pid, p->procname, p->rss * global.page_size / 1048576.0f, p->oom_score);
      } else {
        if (rssdelta > 0) {
          sign = 0;
        } else {
          rssdelta = -rssdelta;
          sign = 1;
        }

        fprintf(private.logfile, "%d %s %.3f mbytes (%.3f mbytes %s), oom_score: %d\n",
                p->pid, p->procname, p->rss * global.page_size / 1048576.0f,
                rssdelta * global.page_size / 1048576.f, desc[sign], p->oom_score);
      }
    }

    if (p)
      p->rss_initial = 0;
  }

  fprintf(private.logfile, "\n");

  fflush(private.logfile);

  ++flag;
}
