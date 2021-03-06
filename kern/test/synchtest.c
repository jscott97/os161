/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
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
 * Synchronization test code.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#define NSEMLOOPS     63
#define NLOCKLOOPS    120
#define NCVLOOPS      5
#define NTHREADS      5

static volatile unsigned long testval1;
static volatile unsigned long testval2;
static volatile unsigned long testval3;
static struct semaphore *testsem;
static struct lock *testlock;
static struct cv *testcv;
static struct semaphore *donesem;
static struct rwlock *rwlock;
static int numThreads = 40;

static
void
inititems(void)
{
	if (testsem==NULL) {
		testsem = sem_create("testsem", 2);
		if (testsem == NULL) {
			panic("synchtest: sem_create failed\n");
		}
	}
	if (testlock==NULL) {
		testlock = lock_create("testlock");
		if (testlock == NULL) {
			panic("synchtest: lock_create failed\n");
		}
	}
	if (testcv==NULL) {
		testcv = cv_create("testlock");
		if (testcv == NULL) {
			panic("synchtest: cv_create failed\n");
		}
	}
	if (donesem==NULL) {
		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("synchtest: sem_create failed\n");
		}
	}
	if(rwlock == NULL) {
		rwlock = rwlock_create("rwlock");
		if(rwlock == NULL) {
			panic("synchtest: rwlock_create failed\n");
		}
	}
}

static
void
semtestthread(void *junk, unsigned long num)
{
	int i;
	(void)junk;

	/*
	 * Only one of these should print at a time.
	 */
	P(testsem);
	kprintf("Thread %2lu: ", num);
	for (i=0; i<NSEMLOOPS; i++) {
		kprintf("%c", (int)num+64);
	}
	kprintf("\n");
	V(donesem);
}

int
semtest(int nargs, char **args)
{
	int i, result;

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting semaphore test...\n");
	kprintf("If this hangs, it's broken: ");
	P(testsem);
	P(testsem);
	kprintf("ok\n");

	for (i=0; i<NTHREADS; i++) {
		result = thread_fork("semtest", semtestthread, NULL, i, NULL);
		if (result) {
			panic("semtest: thread_fork failed: %s\n", 
			      strerror(result));
		}
	}

	for (i=0; i<NTHREADS; i++) {
		V(testsem);
		P(donesem);
	}

	/* so we can run it again */
	V(testsem);
	V(testsem);

	kprintf("Semaphore test done.\n");
	return 0;
}

static
void
fail(unsigned long num, const char *msg)
{
	kprintf("thread %lu: Mismatch on %s\n", num, msg);
	kprintf("Test failed\n");

	lock_release(testlock);

	V(donesem);
	thread_exit();
}

static
void
locktestthread(void *junk, unsigned long num)
{
	int i;
	(void)junk;

	for (i=0; i<NLOCKLOOPS; i++) {
		lock_acquire(testlock);
		testval1 = num;
		testval2 = num*num;
		testval3 = num%3;

		if (testval2 != testval1*testval1) {
			fail(num, "testval2/testval1");
		}

		if (testval2%3 != (testval3*testval3)%3) {
			fail(num, "testval2/testval3");
		}

		if (testval3 != testval1%3) {
			fail(num, "testval3/testval1");
		}

		if (testval1 != num) {
			fail(num, "testval1/num");
		}

		if (testval2 != num*num) {
			fail(num, "testval2/num");
		}

		if (testval3 != num%3) {
			fail(num, "testval3/num");
		}

		lock_release(testlock);
	}
	V(donesem);
}


int
locktest(int nargs, char **args)
{
	int i, result;

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting lock test...\n");

	for (i=0; i<NTHREADS; i++) {
		result = thread_fork("synchtest", locktestthread, NULL, i,
				     NULL);
		if (result) {
			panic("locktest: thread_fork failed: %s\n",
			      strerror(result));
		}
	}
	for (i=0; i<NTHREADS; i++) {
		P(donesem);
	}

	kprintf("Lock test done.\n");

	return 0;
}

static
void
cvtestthread(void *junk, unsigned long num)
{
	int i;
	volatile int j;
	time_t secs1, secs2;
	uint32_t nsecs1, nsecs2;

	(void)junk;

	for (i=0; i<NCVLOOPS; i++) {
		lock_acquire(testlock);
		while (testval1 != num) {
			gettime(&secs1, &nsecs1);
			cv_wait(testcv, testlock);
			gettime(&secs2, &nsecs2);

			if (nsecs2 < nsecs1) {
				secs2--;
				nsecs2 += 1000000000;
			}
			
			nsecs2 -= nsecs1;
			secs2 -= secs1;

			/* Require at least 2000 cpu cycles (we're 25mhz) */
			if (secs2==0 && nsecs2 < 40*2000) {
				kprintf("cv_wait took only %u ns\n", nsecs2);
				kprintf("That's too fast... you must be "
					"busy-looping\n");
				V(donesem);
				thread_exit();
			}

		}
		kprintf("Thread %lu\n", num);
		testval1 = (testval1 + NTHREADS - 1)%NTHREADS;

		/*
		 * loop a little while to make sure we can measure the
		 * time waiting on the cv.
		 */
		for (j=0; j<3000; j++);

		cv_broadcast(testcv, testlock);
		lock_release(testlock);
	}
	V(donesem);
}

int
cvtest(int nargs, char **args)
{

	int i, result;

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting CV test...\n");
	kprintf("Threads should print out in reverse order.\n");

	testval1 = NTHREADS-1;

	for (i=0; i<NTHREADS; i++) {
		result = thread_fork("synchtest", cvtestthread, NULL, i,
				      NULL);
		if (result) {
			panic("cvtest: thread_fork failed: %s\n",
			      strerror(result));
		}
	}
	for (i=0; i<NTHREADS; i++) {
		P(donesem);
	}

	kprintf("CV test done\n");

	return 0;
}

static
void
cvtest2thread(void *junk, unsigned long num)
{
	int i;
	(void)junk;

	for (i=0; i<NCVLOOPS; i++) {
		lock_acquire(testlock);
		while (testval1 != num) {
      testval2 = 0;
			cv_wait(testcv, testlock);
      testval2 = 0xFFFFFFFF;
		}
		testval2 = num;
		cv_broadcast(testcv, testlock);
		thread_yield();
		kprintf("Thread %lu\n", testval2);
		testval1 = (testval1 + NTHREADS - 1)%NTHREADS;
		lock_release(testlock);
	}
	V(donesem);
}

int
cvtest2(int nargs, char **args)
{
	int i, result;

	(void)nargs;
	(void)args;

	inititems();
	kprintf("Starting new CV test...\n");
	kprintf("Threads should print out in reverse order.\n");
	
  testval1 = NTHREADS-1;
	
	for (i=0; i<NTHREADS; i++) {
		result = thread_fork("synchtest", cvtest2thread, NULL, i,
				      NULL);
		if (result) {
			panic("cvtest: thread_fork failed: %s\n",
			      strerror(result));
		}
	}
	for (int j=0; j<NTHREADS; j++) {
		P(donesem);
	}

	kprintf("CV test done\n");

	return 0;
}

static
void
rwlocktest1thread(void *junk, unsigned long num)
{
	(void)junk;
	(void)num;
	rwlock_acquire_read(rwlock);
	kprintf("Thread %lur Acquiring Read\n",num);
	thread_yield();
	kprintf("Thread %lur Releasing Read\n", num);
	rwlock_release_read(rwlock);
	V(donesem);
}

static
void
rwlocktest2thread(void *junk, unsigned long num)
{
	(void)junk;
	kprintf("Thread %luw Acquiring Write...",num);
	rwlock_acquire_write(rwlock);
	kprintf("%luw Done\n",num);
	thread_yield();
	kprintf("Thread %luw Releasing Write...", num);
	rwlock_release_write(rwlock);
	kprintf("%luw Done\n",num);
	V(donesem);
}

int
rwlocktest(int nargs, char **args)
{
	(void)nargs;
	(void)args;
	inititems();
	//rwlocktest1(nargs, args);
	rwlocktest2(nargs, args);
	kprintf("RWL Test Done.\n");

	return 0;
}

int
rwlocktest1(int nargs, char **args)
{
	kprintf("Starting RWL Test 1...\n");
	(void) nargs;
	(void) args;

	int result;

	for(int i=0;i<numThreads;i++)
	{
		result = thread_fork("synchtest", rwlocktest1thread, NULL, i ,NULL);
		if (result) {
			panic("rwlocktest1: thread_fork failed: %s\n",
			      strerror(result));
		}
	}

	for (int i=0; i<numThreads; i++) {
		P(donesem);
	}


	return 0;
}

int
rwlocktest2(int nargs, char **args)
{
	kprintf("Starting RWL Test 2...\n");
	(void) nargs;
	(void) args;

	int result;

	for(int i=0;i<numThreads;i++)
	{
		result = thread_fork("synchtest", rwlocktest1thread, NULL, i ,NULL);
		if (result) {
			panic("rwlocktest2: thread_fork failed: %s\n",
			      strerror(result));
		}
		result = thread_fork("synchtest", rwlocktest2thread, NULL, i ,NULL);
		if (result) {
			panic("rwlocktest2: thread_fork failed: %s\n",
			      strerror(result));
		}
	}

	for (int i=0; i<numThreads * 2; i++) {
		P(donesem);
	}


	return 0;
}
