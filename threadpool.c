/**
 * threadpool.c
 *
 * A work-stealing, fork-join thread pool.
 */

#include <stdlib.h>
#include <pthread.h>
#include <smaphore.h>
#include "list.h"

/* 
 * Opaque forward declarations. The actual definitions of these 
 * types will be local to your threadpool.c implementation.
 */
static struct thread_pool {
	int N;						/* Number of workers in the threadpool */
	pthread_mutex_t lock;		/* Mutex for the threadpool*/
	struct list deque;			/* Global task list */
	pthread_t *threads;			/* An array of worker threads' tids */
	sem_t semaphore;			/* Semaphore fo the threadpool */
	/* Additional menbers may be needed */
};


static struct future {
	fork_join_task_t task;		/* Task */
	void * data;				/* Argument for the task */
	void * result;				/* Result of the task */
	pthread_mutex_t mutex;		/* Mutex */
	int runState; 				/* Represents the state the future is in  by number */
	//sem_t sem;
};

//---------------------------------------------------------------------------------
//ANDREW (10/15/15; 2:45)
/*
 * Struct for holding all the info needed for a created thread. As of now it holds just
 * the worker id, and the actual thread (worker) doing the work
 */
static struct thread_local_info {
	int worker_id;		// Id number for the thread
	pthread_t thread;	// The thread actually doing the work
};
//---------------------------------------------------------------------------------

/* 
 * Forward declaration of worker threads
 */
static void *worker(void *vargp);
	

/* Create a new thread pool with no more than n threads. */
struct thread_pool * thread_pool_new(int nthreads)
{
	/* Creating a new thread pool */
	struct thread_pool *pool = malloc(sizeof *pool);
	
	/* Initializing its menbers */
	pool->N = nthreads;
	pool->threads = malloc(sizeof(pthread_t) * nthreads);	
	pool->lock = PTHREAD_MUTEX_INITIALIZER;
	sem_init(&pool->semaphore, 0, 0);
	list_init(&pool->deque);
	
	/* Spwan the worker threads */
	for (int i = 0; i < nthreads; i++) {
		pthread_create(threads + i, NULL, worker, NULL);
	}
	
	return pool;
}

//------------------------------------------------------------------------------
// Andrew Knittle (10/15/15; 4:00)
/*
 * Helper method for threads that helps determine what tasks, or "jobs", to work
 * on. Looks for jobs by stealing jobs, looking for jobs in queue or global queue,
 * or simply sleeps until a job is avaible.
 */
static void thread_helper()
{
	// NOTE: THIS METHOD MUST BE UPDATED. MOST CODE ARE PLACEHOLDERS 
}

/*
 * The thread function for each worker */ 
static void *worker(void *vargp)
{
	struct list queue; 				// Task list inside each worker thread
}

/* 
 * Shutdown this thread pool in an orderly fashion.  
 * Tasks that have been submitted but not executed may or
 * may not be executed.
 *
 * Deallocate the thread pool object before returning. 
 */
void thread_pool_shutdown_and_destroy(struct thread_pool *)
{
}


/* 
 * Submit a fork join task to the thread pool and return a
 * future.  The returned future can be used in future_get()
 * to obtain the result.
 * 'pool' - the pool to which to submit
 * 'task' - the task to be submitted.
 * 'data' - data to be passed to the task's function
 *
 * Returns a future representing this computation.
 */
struct future * thread_pool_submit(
        struct thread_pool *pool, 
        fork_join_task_t task, 
        void * data)
{
	return NULL;
}

/* Make sure that the thread pool has completed the execution
 * of the fork join task this future represents.
 *
 * Returns the value returned by this task.
 */
void * future_get(struct future *)
{
}

/* Deallocate this future.  Must be called after future_get() */
void future_free(struct future *)
{
}

/* Helper function checking all workers to see whether they are busy */
//static int 


