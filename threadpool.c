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

/*
 * Struct for holding all the info needed for a created thread. As of now it holds just
 * the worker id, and the actual thread (worker) doing the work
 */
static struct thread_local_info {
	int worker_id;					// Id number for the thread
	pthread_t thread;				// The thread actually doing the work
	struct list workerqueue;		// The local task list of the thread
	int worker_state;				// 0 represents worker is sleeping, 1 represents the worker is busy
	struct thread_pool *bigpool;	// The threadpool that holds this thread
	pthread_mutex_t local_lock;		// The local queue mutex
};

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
		pool->thread_info[i].worker_id = i + 1;
		pool->thread_info[i].bigpool = pool;
		pool->thread_info[i].local_lock = PTHREAD_MUTEX_INITIALIZER;
		list_init(&pool->thread_info[i].workerqueue);		
		pthread_create(&pool->thread_info[i].thread, NULL, worker, &pool->thread_info[i]);		
	}
	
	return pool;
}


/*
 * Helper method for threads that helps determine what tasks, or "jobs", to work
 * on. Looks for jobs by stealing jobs, looking for jobs in queue or global queue,
 * or simply sleeps until a job is avaible.
 */
static void * thread_helper(struct thread_local_info * info)
{
	struct future *newTask;
	// Pop from its own queue if there are tasks there
	if (!list_empty(&info->workerqueue))
	{
		pthread_mutex_lock(&info->local_lock);
		newTask = list_entry(list_pop_back(&info->workerqueue), struct future, elem);
		pthread_mutex_unlock(&info->local_lock);
	}
	
	else if (!list_empty(&info->bigpool->subdeque)) {
		//Get the task from the global queue if the global queue is not empty			
			
		// Lock the mutex
		pthread_mutex_lock(&info->bigpool->lock);
		newTask = list_entry(list_pop_front(&info->bigpool->subdeque), struct future, elem);
		pthread_mutex_unlock(&info->bigpool->lock);
		
		}
	else { // Get task from one of other worker
		for (int i = 1; i <= info->bigpool->N; i++) {
			// If it is not itself
			if (i != info->worker_id) {
				if (!list_empty(&info->bigpool->thread_info[i]->workerqueue)) {
					pthread_mutex_lock(&info->bigpool->thread_info[i]->local_lock);
					newTask = list_entry(list_pop_front(&info->bigpool->thread_info[i]->workerqueue), struct future, elem);
					pthread_mutex_unlock(&info->bigpool->thread_info[i]->local_lock);
					break;
				}
			}
		}
	}	
	return future_get(newTask);
}

/*
 * The thread function for each worker */ 
static void *worker(void *vargp)
{
	current_thread_info = (struct thread_local_info *)vargp;
	while (1)
		return thread_helper(current_thread_info);
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
	/*Order in how to stop things:
	 *	1.) Stop the workers and free all futures
	 *	2.) free all workers
	 *	3.) Once all the workers have been freed start
	 *	freeing the threadpool struct
	 *	4.) Everything has been freed so we can exit 
	 */
	int totalThreads = pool->N;
	// Go through all the threads
	for (int i = 0; i < totalThreads; i++)
	{		
		//Free all futures still in the worker's local
		//job queue.
		while(!list_empty(&pool->thread_info[i].wokerqueue))
		{
			//pop off elements at the back and repeat
			//until empty
			struct future *oldTask =list_entry(list_pop_back(&pool->thread_info[i].wokerqueue), struct future, elem);
			// call future free to destroy the future
			future_free(oldTask);
		}
		// all futures freed
		// free the worker
		free(pool->threadinfo[i]);
	}
	// All workers freed
	// free the worker list
	free(pool->threadinfo);
	// All that's left is the pool.
	free(pool);
	// All the memory allocated has been freed so we
	// can exit
	exit(0);
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
	/* Allocating a new Future struct for the task */
	struct future *newFuture = malloc(sizeof *newFuture);
	
	/* Initialzing the menbers of the Future */
	newFuture->task = task;
	newFuture->data = data;
	newFuture->result = NULL;
	newFuture->mutex = PTHREAD_MUTEX_INITIALIZER;
	newFuture->runState = 0;			// State 0 represents that the task has not been excuted yet
	&newFuture->elem = malloc(sizeof *elem);
	
	/* If current thread is the main thread, submit the task to global deque */
	if (current_thread_info == NULL) {
	
		/* Push the future into the global deque */
		pthread_mutex_lock(&pool->lock);
		list_push_back(&pool->subdeque, &newFuture->elem);
		pthread_mutex_unlock(&pool->lock);
	}
	/* Otherwise submit the task to a random sleeping worker, if all workers are busy then submit 
	 * to a random workers queue */
	else {
		//int count;
		//if (pool->N == 1) 									// The case when there is only one thread
		{
			pthread_mutex_lock(&current_thread_info->local_lock);
			list_push_back(&current_thread_info->workerqueue, &newFuture->elem);
			pthread_mutex_unlock(&current_thread_info->local_lock);
		}
		/*else if (current_thread_info->worker_id == pool->N) // Current worker is the last worker in the pool
		{
			count = 1;
			while (count != current_thread_info->worker_id) {
				if (pool->thread_info[count].worker_state == 0)
				{
					// Submit the task to this sleeping worker
					list_push_back(&pool->thread_info[count].workerqueue, &newFuture->elem);
					break;
				}
				count++;				
			}
			list_push_back(&pool->thread_info[count - 1].workerqueue, &newFuture->elem);			
		}
		else 											
		// Current worker is not the last worker than search starting from the next worker.
		{
			count = current_thread_info->worker_id + 1;
			while (count != current_thread_info->worker_id) {
				if (count == pool->N) count = 0;
				if (pool->thread_info[count].worker_state == 0)
				{
					// Submit the task to this sleeping worker
					list_push_back(&pool->thread_info[count].workerqueue, &newFuture->elem);
					break;
				}
				count++;				
			}
			list_push_back(&pool->thread_info[count + 1].workerqueue, &newFuture->elem);
		}*/		
	}
	
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
	/*
	 * NOTE: Should be called when 
	 * 	a task has been completed
	 * 	and when the threadpool is told to stop
	 */
	struct future *oldFuture = future;
	// free the elem
	free(&oldFuture->elem);
	// free the future
	free(oldFuture);
}


