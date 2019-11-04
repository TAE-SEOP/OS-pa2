/**********************************************************************
 * Copyright (c) 2019
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter  is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}



#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the next process to run */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};


/***********************************************************************
 * SJF scheduler
 ***********************************************************************/

static int sjf_initialize(void)
{
	return 0;
}

static void sjf_finalize(void)
{
}



static struct process *sjf_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process *next = NULL;


	if (!current || current->status == PROCESS_WAIT) {
		goto skip;
	}

	if (current->age < current->lifespan) {
		return current;
	}

skip:
	if (!list_empty(&readyqueue)) {
		current = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry(next, &readyqueue, list) {
			if (current->lifespan > next->lifespan) current = next;
		}
		list_del_init(&current->list);
		return current;
	}
	return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release,/* Use the default FCFS release() */
	.initialize = sjf_initialize,
	.finalize = sjf_finalize,
	.schedule = sjf_schedule,		 /* TODO: Assign sjf_schedule()
								to this function pointer to activate
								SJF in the system */
};


/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/

static int srtf_initialize(void)
{
	return 0;
}

static void srtf_finalize(void)
{
}


static struct process *srtf_schedule(void) 
{
	struct process *init = NULL;
	struct process *next = NULL;


	if (!list_empty(&readyqueue)) {  //readyqueue가 비어있지 않을 경우
		init = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry(next, &readyqueue, list) {
			if (init->lifespan - init->age > next->lifespan - next->age) init = next;
		}
		if (!current || current->status == PROCESS_WAIT) {   // current가 없을 경우 (readyqueue는 비어있지 않음)
			list_del_init(&init->list);
			return init;
		}
		else {    // current가 있을 경우 (readyqueue는 비어있지 않음)
			if (current->age < current->lifespan) { // current가 더 일해야 한다면 
				if (init->lifespan - init->age < current->lifespan - current->age) {   //init이 더 짧으면 init을 list에서 삭제하고 current를 list에 추가하고 init반환
					list_del_init(&init->list);
					list_add(&current->list, &readyqueue);
					return init;
				}
				else {    // current가 더 짧을 경우
					return current;
				}
			}
			else {  // current가 더 일안해도 되면 init반환
				list_del_init(&init->list);
				return init;
			}
		}
	}
	if (current != NULL || current->status == PROCESS_RUNNING) { // readyqueue가 비어있을 때 current가 있다면 current가 일을 더 해야하면 current를, 안 해도 된다면 NULL을 반환
		if (current->age < current->lifespan)  
			return current;
	}

	return init;   
}



struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release,
	.schedule = srtf_schedule,/* Use the default FCFS release() */
	.initialize = srtf_initialize,  /* You need to check the newly created processes to implement SRTF.*/
	.finalize = srtf_finalize,    /* Use @forked() callback to mark newly created processes */
	                          /* Obviously, you should implement srtf_schedule() and attach it here */
};


/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	                         /* Obviously, you should implement rr_schedule() and attach it here */
    

};


/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
struct scheduler prio_scheduler = {
	.name = "Priority",
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};


/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
struct scheduler pip_scheduler = {
	.name = "Priority + Priority Inheritance Protocol",
	/**
	 * Implement your own acqure/release function too to make priority
	 * scheduler correct.
	 */
	/* It goes without saying to implement your own pip_schedule() */
};
