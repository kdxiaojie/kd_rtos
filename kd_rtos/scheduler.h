#ifndef __SCHEDULER_H
#define __SCHEDULER_H

void bitmap_set(uint32_t prio);
void bitmap_clear(uint32_t prio);
uint32_t get_highest_priority(void);
void switch_context_logic(void);

#endif