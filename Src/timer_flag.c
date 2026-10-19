/*
 * timer_flag.c
 *
 *  Created on: May 22, 2024
 *      Author: Selina
 */
#include "timer_flag.h"

#define TimerCountPeriod	50

void SetTimerCount(flagType *Flag){
// Timer 50ms
#ifdef FLAG_50ms
	Flag->t50ms_count += TimerCountPeriod;
#endif

// Timer 100ms
#ifdef FLAG_100ms
	Flag->t100ms_count += TimerCountPeriod;
#endif

// Timer 250ms
#ifdef FLAG_250ms
	Flag->t250ms_count += TimerCountPeriod;
#endif

// Timer 500ms
#ifdef FLAG_500ms
	Flag->t500ms_count += TimerCountPeriod;
#endif

// Timer 1s
#ifdef FLAG_1s
	Flag->t1s_count += TimerCountPeriod;
#endif

// Timer 1500ms
#ifdef FLAG_1500ms
	Flag->t1500ms_count += TimerCountPeriod;
#endif

// Timer 2s
#ifdef FLAG_2s
	Flag->t2s_count += TimerCountPeriod;
#endif

// Timer 3s
#ifdef FLAG_3s
	Flag->t3s_count += TimerCountPeriod;
#endif

// Timer 4s
#ifdef FLAG_4s
	Flag->t4s_count += TimerCountPeriod;
#endif

// Timer 5s
#ifdef FLAG_5s
	Flag->t5s_count += TimerCountPeriod;
#endif

// Timer 6s
#ifdef FLAG_6s
	Flag->t6s_count += TimerCountPeriod;
#endif

// Timer 10s
#ifdef FLAG_10s
	Flag->t10s_count += TimerCountPeriod;
#endif

// Timer 15s
#ifdef FLAG_15s
	Flag->t15s_count += TimerCountPeriod;
#endif

// Timer 30s
#ifdef FLAG_30s
	Flag->t30s_count += TimerCountPeriod;
#endif

// Timer 35s
#ifdef FLAG_35s
	Flag->t35s_count += TimerCountPeriod;
#endif

// Timer 15p
#ifdef FLAG_15p
	Flag->t15p_count += TimerCountPeriod;
#endif

// Timer 30p
#ifdef FLAG_30p
	Flag->t30p_count += TimerCountPeriod;
#endif

}

void Flag_CallBack(flagType *Flag){
	SetTimerCount(Flag);
// Timer 50ms
#ifdef FLAG_50ms
	if(Flag->t50ms_count == 50){
		Flag->t50ms = 1;
		Flag->t50ms_count = 0;
	}
#endif

// Timer 100ms
#ifdef FLAG_100ms
	if(Flag->t100ms_count == 100){
		Flag->t100ms = 1;
		Flag->t100ms_count = 0;
	}
#endif

// Timer 250ms
#ifdef FLAG_250ms
	if(Flag->t250ms_count == 250){
		Flag->t250ms = 1;
		Flag->t250ms_count = 0;
	}
#endif

// Timer 500ms
#ifdef FLAG_500ms
	if(Flag->t500ms_count == 500){
		Flag->t500ms = 1;
		Flag->t500ms_count = 0;
	}
#endif

// Timer 1s
#ifdef FLAG_1s
	if(Flag->t1s_count == 1000){
		Flag->t1s = 1;
		Flag->t1s_count = 0;
	}
#endif

// Timer 1500ms
#ifdef FLAG_1500ms
	if(Flag->t1500ms_count == 1500){
		Flag->t1500ms = 1;
		Flag->t1500ms_count = 0;
	}
#endif

// Timer 2s
#ifdef FLAG_2s
	if(Flag->t2s_count == 2000){
		Flag->t2s = 1;
		Flag->t2s_count = 0;
	}
#endif

// Timer 3s
#ifdef FLAG_3s
	if(Flag->t3s_count == 3000){
		Flag->t3s = 1;
		Flag->t3s_count = 0;
	}
#endif

// Timer 4s
#ifdef FLAG_4s
	if(Flag->t4s_count == 4000){
		Flag->t4s = 1;
		Flag->t4s_count = 0;
	}
#endif

// Timer 5s
#ifdef FLAG_5s
	if(Flag->t5s_count == 5000){
		Flag->t5s = 1;
		Flag->t5s_count = 0;
	}
#endif

// Timer 6s
#ifdef FLAG_6s
	if(Flag->t6s_count == 6000){
		Flag->t6s = 1;
		Flag->t6s_count = 0;
	}
#endif

// Timer 10s
#ifdef FLAG_10s
	if(Flag->t10s_count == 10000){
		Flag->t10s = 1;
		Flag->t10s_count = 0;
	}
#endif

// Timer 15s
#ifdef FLAG_15s
	if(Flag->t15s_count == 15000){
		Flag->t15s = 1;
		Flag->t15s_count = 0;
	}
#endif

// Timer 30s
#ifdef FLAG_30s
	if(Flag->t30s_count == 30000){
		Flag->t30s = 1;
		Flag->t30s_count = 0;
	}
#endif

// Timer 35s
#ifdef FLAG_35s
	if(Flag->t35s_count == 35000){
		Flag->t35s = 1;
		Flag->t35s_count = 0;
	}
#endif

// Timer 15p
#ifdef FLAG_15p
	if(Flag->t15p_count == 900000){
		Flag->t15p = 1;
		Flag->t15p_count = 0;
	}
#endif

// Timer 30p
#ifdef FLAG_30p
	if(Flag->t30p_count == 1800000){
		Flag->t30p = 1;
		Flag->t30p_count = 0;
	}
#endif
}




