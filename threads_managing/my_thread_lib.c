
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>

#include "my_thread_lib.h"


void time_copy(TimeType *ta, TimeType *par)
{
	(*ta).tv_nsec = par->tv_nsec;
	(*ta).tv_sec = par->tv_sec;
}

void time_add_ms(TimeType *t, int ms)
{
	t->tv_sec += ms/1000;
	t->tv_nsec += ms * 1000000;
	t->tv_sec += t->tv_nsec / 1000000000;
	t->tv_nsec = t->tv_nsec % 1000000000;
}

TimeType time_passed(TimeType* t_start, TimeType* t_end)
{
TimeType t;

	t.tv_sec = t_start->tv_sec - t_end->tv_sec;
	t.tv_nsec = (t_start->tv_nsec - t_end->tv_nsec) % 1000000000;

	if (t_end->tv_nsec > t_start->tv_nsec) t.tv_sec--;

	return t;
}

int time_cmp(TimeType *t1, TimeType *t2)
{
	if (t1->tv_sec > t2->tv_sec) return 1;
	if (t1->tv_sec < t2->tv_sec) return -1;
	if (t1->tv_nsec > t2->tv_nsec) return 1;
	if (t1->tv_nsec < t2->tv_nsec) return -1;

	return 0;
}

void time_print(TimeType *t)
{
	printf("%d s \t %d ns \n",(int)t->tv_sec,(int) t->tv_nsec);
}

/* methods to manage Thread (ex ptask) -------------------------------*/

void set_taskpar(TaskPar *tp, void* arg,void* exit, int period, int deadline, int priority)
{
	tp->arg = arg;
	tp->exit = exit;
	tp->period = period;
	tp->deadline = deadline;
	tp->priority = priority;
	tp->dmiss = 0;
}


void set_period(TaskPar *tp)
{
TimeType t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	/* I initialize at and dt with the current time */
	time_copy(&(tp->at), &t);
	time_copy(&(tp->dl), &t);
	/* calculate next activity time and deadline */
	time_add_ms(&(tp->at), tp->period);
	time_add_ms(&(tp->dl), tp->deadline);

}

void wait_for_period(TaskPar *tp)
{
	/* calculate next activation time */
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(tp->at), NULL);

	/* calculate next activity time and deadline */
	time_add_ms(&(tp->at), tp->period);
	time_add_ms(&(tp->dl), tp->deadline);
}

int deadline_miss(TaskPar *tp)
{
TimeType now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (time_cmp(&now, &(tp->dl))>0)
	{
		tp->dmiss++;
		return 1;
	}
	return 0;
}

void postpone_deadline(TaskPar *tp, int millisec)
{
	time_add_ms(&(tp->at), millisec);
	time_add_ms(&(tp->dl), millisec);
}
