/*
 * my_lib.h
 *
 *  Created on: Oct 27, 2015
 *      Author: parallels
 */

#ifndef THREAD_LIB_H_
#define THREAD_LIB_H_

typedef struct timespec TimeType;

struct task_par_ {
	void* arg; 		/* task argument */
	void* exit;		/* exit condition */
	long wcet; 		/* in microseconds */
	int period; 	/* in milliseconds */
	int deadline; 	/* relative (ms) */
	int priority; 	/* in [0,99] */
	int dmiss; 		/* no. of misses */
	TimeType at; 	/* next activ time */
	TimeType dl; 	/* abs. deadline */
};

typedef struct task_par_ TaskPar;


void time_copy(TimeType *ta, TimeType *par);

void time_add_ms(TimeType *t, int ms);

int time_cmp(TimeType *t1, TimeType *t2);
/* the function returns
 * 	0 if t1 == t2
 * 	1 if t1 > t2
 * 	-1 if t1 < t2
 */
void time_print(TimeType *t);

/* methods to manage Thread (ex ptask) -------------------------------*/

void set_taskpar(TaskPar *, void*,void*, int, int, int);

void set_period(TaskPar *tp);

void wait_for_period(TaskPar *tp);

int deadline_miss(TaskPar *tp);
/* the function returns 1 if the deadline is miss, 0 otherwise */
TimeType time_passed(TimeType *t_start, TimeType *t_end);

/* postpone the current deadline of millisec milliseconds */
void postpone_deadline(TaskPar *tp, int millisec);

#endif /* THREAD_LIB_H_ */
