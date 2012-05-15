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
  FILE *killfile;
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

    snprintf(filename, sizeof(filename), "%s/watcher-%02d-%02d-%02d.log",
             conf.logpath, private.year, private.month, private.day);

    private.logfile = fopen(filename, "w");
  }
}

void LogInit(void)
{
  static sflag = 0;
  int d;
  time_t curtime = time(NULL);
  struct tm *tm = localtime(&curtime);

  d = tm->tm_mday;

  if (sflag && (d == private.day)) 
    return;

  sflag = 1;

  private.year = 0;
  private.month = 0;
  private.day = 0;

  private.crashlog = 0;

  private.logfile = NULL;
  private.killfile = NULL;

  LogRotateIfNeeded();
}

void LogProcessKill(struct ProcessInfo *p)
{
  if (!private.killfile) {
    char path[256];
    snprintf(path, sizeof(path), "%s/watcher.kill.log", conf.logpath);
    private.killfile = fopen(path, "w");
  }

  fprintf(private.killfile, "Process %d %s killed due to excessive memory usage. (%.3f mbytes)\n\n",
          p->pid, p->procname, p->rss * global.page_size / 1048576.0f); // 1048576 = 1024 * 1024
}

int FlushKillLog(void)
{
  char cmd[256];

  if (!private.killfile)
    return 0;

  fflush(private.killfile);
  fclose(private.killfile);
  private.killfile = NULL;

  snprintf(cmd, sizeof(cmd), "mv \"%s/watcher.kill.log\" \"%s/watcher.kill.%d.log\"",
           conf.logpath, conf.logpath, private.crashlog++);
  return system(cmd);
}

void DumpProcessInfo(int kill)
{
  static int flag;
  static const char *desc[] = {"up", "down"};
  struct tm tm;
  char logdate[32];
  time_t curtime = time(NULL);
  int i;
  FILE *ofp;

  if (kill)
    ofp = private.killfile;
  else
    ofp = private.logfile;

  localtime_r(&curtime, &tm);

  strftime(logdate, 32, "%F %T", &tm);
  fprintf(ofp, "Log at %s\n", logdate);
  for (i = 0; i < upperbound; ++i) {
    struct ProcessInfo *p = &processes[i];
    
    if (p->pid && (!flag || p->rss_initial != p->rss || kill) && p->rss > 0) {
      int rssdelta = p->rss - p->rss_initial;
      int sign;

      if (kill)
        rssdelta = 0;

      if (rssdelta == 0) {
        fprintf(ofp, "%d %s %.3f mbytes, oom_score: %d\n",
                p->pid, p->procname, p->rss * global.page_size / 1048576.0f, p->oom_score);
      } else {
        if (rssdelta > 0) {
          sign = 0;
        } else {
          rssdelta = -rssdelta;
          sign = 1;
        }

        fprintf(ofp, "%d %s %.3f mbytes (%.3f mbytes %s), oom_score: %d\n",
                p->pid, p->procname, p->rss * global.page_size / 1048576.0f,
                rssdelta * global.page_size / 1048576.f, desc[sign], p->oom_score);
      }
    }

    if (p && !kill)
      p->rss_initial = 0;
  }

  fprintf(ofp, "\n");
  fflush(ofp);

  ++flag;
}
