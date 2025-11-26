#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "list.h"
#include "task.h"

extern list_t ReadyList[MAX_PRIORITY];
extern list_t DelayedList;
extern task_tcb *current_tcb;
extern volatile uint8_t OSSchedLockNesting;

void bitmap_set(uint32_t prio);
void bitmap_clear(uint32_t prio);
uint32_t get_highest_priority(void);
void switch_context_logic(void);
void OSSchedLock(void);
void OSSchedUnlock(void);
void os_init(void);

#endif
