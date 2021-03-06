/*
 * Copyright (c) 2001, 2002, 2009
 *  The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

static volatile int males = 0;
static volatile int females = 0;
static struct lock *wm_lock;

static struct cv *wm_mcv;
static struct cv *wm_fcv;
static struct cv *wm_mmcv;

static struct semaphore *inter_sem;

static struct lock *locks[4];

void whalemating_init() {
  wm_lock = lock_create("whales");

  wm_mcv = cv_create("males");
  wm_fcv = cv_create("females");
  wm_mmcv = cv_create("matchmakers");
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void whalemating_cleanup() {
  lock_destroy(wm_lock);
  cv_destroy(wm_mcv);
  cv_destroy(wm_fcv);
  cv_destroy(wm_mmcv);
  return;
}

void
male(void *p, unsigned long which)
{
  struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  male_start();
  kprintf("SStarting Male %lu\n",which);
  lock_acquire(wm_lock);
  males++;
  cv_signal(wm_mmcv,wm_lock);
  cv_wait(wm_mcv,wm_lock);
  kprintf("Releasing Male");
  lock_release(wm_lock);
  male_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{
  struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  female_start();
  kprintf("SStarting Female %lu\n",which);
  lock_acquire(wm_lock);
  females++;
  cv_signal(wm_mmcv,wm_lock);
  cv_wait(wm_mcv,wm_lock);
  kprintf("Releasing Female");
  lock_release(wm_lock);
  female_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{
  struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  matchmaker_start();
  lock_acquire(wm_lock);
  while(1)
  {
    cv_wait(wm_mmcv,wm_lock);
    lock_acquire(wm_lock);
    if(males > 0 && females > 0)
    {
      kprintf("Ready to match!");
      break;
    }
  }
  cv_signal(wm_mcv,wm_lock);
  cv_signal(wm_fcv,wm_lock);
  males--;
  females--;
  lock_release(wm_lock);
  matchmaker_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 | 
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

void stoplight_init() {
    locks[0] = lock_create("q0 lock");
    locks[1] = lock_create("q1 lock");
    locks[2] = lock_create("q2 lock");
    locks[3] = lock_create("q3 lock");

    inter_sem = sem_create("inter sem",3); 
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {
  return;
}

void
gostraight(void *p, unsigned long direction)
{
  struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;

  //Enter intersection
  P(inter_sem);
  int firstQuad = direction;
  int secondQuad = (direction + 3) % 4;
  //Try to enter X
  lock_acquire(locks[firstQuad]);
  //Enter X
  inQuadrant(firstQuad);
  //Try to enter (X+3) % 4
  lock_acquire(locks[secondQuad]);
  //Leave X, enter (x+3) % 4
  inQuadrant(secondQuad);
  lock_release(locks[firstQuad]);
  //Leave (x+3) % 4 (thereby leaving the intersection)
  leaveIntersection();
  lock_release(locks[secondQuad]);
  V(inter_sem);
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnleft(void *p, unsigned long direction)
{
  struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;

  //Enter intersection
  P(inter_sem);
  int firstQuad = direction;
  int secondQuad = (direction + 3) % 4;
  int thirdQuad = (direction + 2) % 4;
  //Try to enter X
  lock_acquire(locks[firstQuad]);
  //Enter X
  inQuadrant(firstQuad);
  //Try to enter (X+3) % 4
  lock_acquire(locks[secondQuad]);
  //Leave X, enter (x+3) % 4
  inQuadrant(secondQuad);
  lock_release(locks[firstQuad]);
  //Leave (x+3) % 4, enter (x+2) % 4 
  lock_acquire(locks[thirdQuad]);
  inQuadrant(thirdQuad);
  lock_release(locks[secondQuad]);
  //Leave intersection
  leaveIntersection();
  lock_release(locks[thirdQuad]);
  V(inter_sem);
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnright(void *p, unsigned long direction)
{
  struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  
  //Enter intersection
  P(inter_sem);
  int firstQuad = direction;
  //Try to enter X
  lock_acquire(locks[firstQuad]);
  //Enter X
  inQuadrant(firstQuad);
  //Leave intersection
  leaveIntersection();
  lock_release(locks[firstQuad]);
  V(inter_sem);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}
