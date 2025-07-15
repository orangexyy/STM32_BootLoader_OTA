#ifndef __TIMER_H
#define __TIMER_H

#include "sys.h"
#include "stm32f10x.h"

//定时周期 = 时钟频率 / (ARR + 1) / (PSC + 1) (s)
//定时频率 = 1 / 定时周期


void timer3_init(void);
void timer3_start(void);
void timer3_stop(void);
uint32_t get_timer3_tick(void);
void set_timer3_tick(uint32_t tick);

#endif
