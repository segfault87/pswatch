#ifndef _LOG_H_
#define _LOG_H_

#include "pswatch.h"

void LogInit(void);
int LogProcessKill(struct ProcessInfo *p);
void DumpProcessInfo(void);

#endif
