#ifndef _LOG_H_
#define _LOG_H_

#include "pswatch.h"

void LogInit(void);
void LogProcessKill(struct ProcessInfo *p);
int FlushKillLog(void);
void DumpProcessInfo(void);

#endif
