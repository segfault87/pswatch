#include <stdio.h>
#include <time.h>

#include "context.h"
#include "log.h"
#include "pswatch.h"

struct {
  int year;
  int month;
  int day;

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

    snprintf(filename, sizeof(filename), "%s/watcher-%04d-%02d-%02d.log",
             conf.logpath, private.year, private.month, private.day);

    private.logfile = fopen(filename, "a");
  }
}

void LogInit(void)
{
  private.year = 0;
  private.month = 0;
  private.day = 0;

  private.logfile = NULL;

  LogRotateIfNeeded();
}

void DumpProcessInfo(void)
{
  static int flag;
  static const char *desc[] = {"up", "down"};
  time_t curtime = time(NULL);
  struct tm *tm = localtime(&curtime);
  int i;

  fprintf(private.logfile, "Log at %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
  for (i = 0; i < indexcount; ++i) {
    struct ProcessInfo *p = processes[indices[i]];
    
    if (p && (!flag || p->rss_initial != p->rss) && p->rss > 0) {
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
