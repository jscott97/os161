/* Process Header File */

#ifndef _PROCESS_H_
#define _PROCESS_H_

#define INIT_PROCESS 2

#include <types.h>
#include <thread.h>
#include <processlist.h>

/* Process Structure */
struct process {
	char *p_name;
	pid_t p_id;

	/* Internal Stuff */
	struct processlistnode p_listnode; /* Link for run/sleep/zombie lists */

	/*For waitpid()*/
	struct semaphore *p_waitsem;
	// struct cv *p_waitcv;
	// struct lock *p_waitlock;
	//struct processlist p_waiters;

	pid_t p_parentpid;
	//struct processlist p_children;
	struct thread *p_thread;

	/* For fork() */
	struct semaphore *p_forksem;
};

/* States a process can be in. */
typedef enum {
	P_FREE,		/* available (no process) */
	P_USED,		/* unavalable (process running) */
	P_ZOMBIE,	/* zombie (process exited) */
	P_INVALID	/* Invalid PID */
} pidstate_t;

struct process *init_process_create(const char*);
int process_create(const char*, pid_t parent, struct process**);

void process_exit(pid_t pid, int exitcode);
void process_destroy(pid_t pid);
int process_wait(pid_t pidToWait, pid_t pidToWaitFor);
/* Call once during system startup to allocate data structures */
void processtable_bootstrap(void);

void processtable_biglock_acquire(void);
void processtable_biglock_release(void);
bool processtable_biglock_do_i_hold(void);

int allocate_pid(pid_t* allocated_pid);
void release_pid(int);

struct process* get_process(pid_t);
pidstate_t get_pid_state(pid_t);
pid_t get_process_parent(pid_t);

int get_process_exitcode(pid_t);
void set_process_parent(pid_t,pid_t);
void abandon_children(pid_t);
void collect_children(void);

#endif /* _PROCESS_H_ */