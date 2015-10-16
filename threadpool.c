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
	struct list subdeque;			/* Global task list */
	//pthread_t *threads;			/* An array of worker threads' tids */
	struct thread_local_info *thread_info; /* An array of worker threads' info */
	sem_t semaphore;			/* Semaphore fo the threadpool */
	int is_shutdown;			/* A flag to denote when the threadpool is shut down */
	/* Additional menbers may be needed */
};


static struct future {
	fork_join_task_t task;		/* Task */
	void * data;				/* Argument for the task */
	void * result;				/* Result of the task */
	pthread_mutex_t mutex;		/* Mutex */
	int runState; 				/* Represents the state the future is in  by number */
	struct list_elem elem;   	/* Link element for the list */
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
	struct list workerqueue;	// The local task list of the thread
};
//---------------------------------------------------------------------------------

static __thread struct thread_local_info *current_thread_info = NULL;

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
	thread_info = malloc(nthreads * sizeof(struct thread_local_info) );
	pool->N = nthreads;
	pool->lock = PTHREAD_MUTEX_INITIALIZER;
	sem_init(&pool->semaphore, 0, 0);
	list_init(&pool->subdeque);
	
	/* Spwan the worker threads */
	for (int i = 0; i < nthreads; i++) {
		thread_info[i].worker_id = i + 1;
		list_init(&thread_info[i].workerqueue);
		pthread_create(&thread_info[i].thread, NULL, worker, &thread_info[i]);		
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
	current_thread_info = (struct thread_local_info *)vargp;
	thread_helper();
	return NULL;
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
	pthread_t tid = pthread_self();
	/* Allocating a new Future struct for the task */
	struct future *newFuture = malloc(sizeof *newFuture);
	
	/* Initialzing the menbers of the Future */
	newFuture->task = task;
	newFuture->data = data;
	newFuture->result = NULL;
	newFuture->mutex = PTHREAD_MUTEX_INITIALIZER;
	newFuture->runState = 0;			// State 0 represents that the task has not been excuted yet
	&newFuture->elem = malloc(sizeof *elem);
	
	/* Push the future into the global deque */
	list_push_back(&pool->subdeque, &newFuture->elem);
	
	return newFuture;
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


