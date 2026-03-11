#ifndef TASK_SCHED_H
#define TASK_SCHED_H
#include "stm32f10x.h"                  // Device header

void TaskHandler(void);
void TaskSchedule(void);


void Balance_Control_Loop(void);

#endif
