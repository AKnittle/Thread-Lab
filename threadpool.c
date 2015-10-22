/**
 * threadpool.c
 *
 * A work-stealing, fork-join thread pool.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "list.h"
#include "threadpool.h"

/* 
 * Opaque forward declarations. The actual definitions of these 
 * types will be local to your threadpool.c implementation.
 */
struct thread_pool{
	int N;						/* Number of workers in the threadpool */
	pthread_mutex_t lock;		/* Mutex for the threadpool*/
	struct list subdeque;			/* Global task list */
	//pthread_t *threads;			/* An array of worker threads' tids */
	struct thread_local_info *thread_info; /* An array of worker threads' info */
	sem_t semaphore;			/* Semaphore fo the threadpool */
	int is_shutdown;			/* A flag to denote when the threadpool is shut down */
	/* Additional menbers may be needed */
};


struct future{
	fork_join_task_t task;		/* Task function*/
	void * data;				/* Argument for the task */
	void * result;				/* Result of the task */
	pthread_mutex_t mutex;		/* Mutex */
	int runState; 				/* Represents the state the future is in  by number 
									0 represents the task has not been executed,
									1 represents the task is currently running,
									2 represents the task already has a result aviliable */
	struct list_elem elem;   	/* Link element for the list */
	int mylist;					/* Flag to show which list this future is in
									-1 if it is not in a list
									0 if it in global queue
									positive numbers represent worker_id otherwise */
	sem_t signal;
};

/*
 * Struct for holding all the info needed for a created thread. As of now it holds just
 * the worker id, and the actual thread (worker) doing the work
 */
struct thread_local_info{
	int worker_id;					// Id number for the thread
	pthread_t thread;				// The thread actually doing the work
	struct list workerqueue;		// The local task list of the thread
	int worker_state;				// 0 represents worker is sleeping, 1 represents the worker is busy
	struct thread_pool *bigpool;	// The threadpool that holds this thread
	pthread_mutex_t local_lock;		// The local queue mutex
};

static __thread struct thread_local_info *current_thread_info = NULL;
static inline bool
is_interior (struct list_elem *elem);
//static bool check_workers(struct future *);

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
	pool->thread_info = malloc(nthreads * sizeof(struct thread_local_info) );
	pool->N = nthreads;
	pool->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	pool->is_shutdown = 0;
	sem_init(&pool->semaphore, 0, 0);
	list_init(&pool->subdeque);
	
	/* Spwan the worker threads */
	int i = 0;
	for (; i < nthreads; i++) {
		pool->thread_info[i].worker_id = i + 1;
		pool->thread_info[i].bigpool = pool;
		pool->thread_info[i].worker_state = 0;
		pool->thread_info[i].local_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
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
static void *thread_helper(struct thread_local_info * info)
{
	int in_myqueue = 0;
	int in_global = 0;
	struct future *newTask = NULL;
	// Pop from its own queue if there are tasks there
	pthread_mutex_lock(&current_thread_info->local_lock);
	//printf("Locking local list in worker: %d with addr: %p\n", current_thread_info->worker_id, &current_thread_info->local_lock);
	if (!list_empty(&current_thread_info->workerqueue))
	{
		in_myqueue = 1;
		struct list_elem *back = list_back (&current_thread_info->workerqueue);
		if (is_interior(back))
		newTask = list_entry(list_pop_back(&current_thread_info->workerqueue), struct future, elem);
		//printf("Pop task out from worker: %d\n", current_thread_info->worker_id);
	}
	pthread_mutex_unlock(&current_thread_info->local_lock);
	//printf("Unlocking local list in worker: %d\n", current_thread_info->worker_id);
	//pthread_mutex_lock(&info->bigpool->lock);
	if (in_myqueue == 0)
	{ 
		pthread_mutex_lock(&info->bigpool->lock);
		if( !list_empty(&info->bigpool->subdeque)) 
		{
			//Get the task from the global queue if the global queue is not empty
			in_global = 1;
			struct list_elem *front = list_front (&info->bigpool->subdeque);
			if (is_interior(front))
			newTask = list_entry(list_pop_front(&info->bigpool->subdeque), struct future, elem);		
		}
		pthread_mutex_unlock(&info->bigpool->lock);
	}
	if (in_myqueue == 0 && in_global == 0) { // Get task from one of other worker
		int i = 1;
		for (; i <= info->bigpool->N; i++) {
			// If it is not itself
			if (i != current_thread_info->worker_id) {
				//printf("Locking worker list in worker: %d with addr: %p\n", i - 1, &info->bigpool->thread_info[i - 1].local_lock);
	
				pthread_mutex_lock(&info->bigpool->thread_info[i - 1].local_lock);
				if (!list_empty(&info->bigpool->thread_info[i - 1].workerqueue)) {
					// List assertion failed here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					// WHY???????????????????????????
					struct list_elem *front = list_front (&info->bigpool->thread_info[i - 1].workerqueue);
					if (is_interior(front)) {
					newTask = list_entry(list_pop_front(&info->bigpool->thread_info[i - 1].workerqueue), struct future, elem);
					pthread_mutex_unlock(&info->bigpool->thread_info[i - 1].local_lock);
					//printf("Pop task out from worker: %d\n", i - 1);
					break;
				}
				}
				pthread_mutex_unlock(&info->bigpool->thread_info[i - 1].local_lock);
				//printf("Unlocking worker list in worker: %d with addr: %p\n", i - 1, &info->bigpool->thread_info[i - 1].local_lock);
			}
		}
	}
	// Strat executing the task function and put the result into the future
	if (newTask == NULL) {
		return NULL;
	}
	if (newTask->task == NULL) {
		return NULL;
	}
	pthread_mutex_lock(&newTask->mutex);
	newTask->elem.next = NULL;
	newTask->elem.prev = NULL;
	newTask->mylist = -1;
	fork_join_task_t task = newTask->task;
	info->worker_state = 1;								// Set the state of the worker to be busy
	newTask->runState = 1;								// Set the runstate to be 1 when task is in progress
	pthread_mutex_unlock(&newTask->mutex);
	newTask->result = task(info->bigpool, newTask->data);
	pthread_mutex_lock(&newTask->mutex);
	newTask->runState = 2;								// Set the runstate to be 2 when the result is aviliable
	info->worker_state = 0;								// Set the state of the worker to be aviliable
	
	// Signal that this future has the value
	pthread_mutex_unlock(&newTask->mutex);
	sem_post(&newTask->signal);
	return newTask->result;
}

/*
 * The thread function for each worker */ 
static void *worker(void *vargp)
{
	int shutdown;
	current_thread_info = (struct thread_local_info *)vargp;
	//printf("I'm this thread: %d\n", current_thread_info->worker_id);
	while (1) {
		//Wait until a future is ready or it is about to shutdown
		sem_wait(&current_thread_info->bigpool->semaphore);
		
		pthread_mutex_lock(&current_thread_info->bigpool->lock);
		shutdown = current_thread_info->bigpool->is_shutdown;
		//pthread_mutex_unlock(&current_thread_info->bigpool->lock);
		
		if (shutdown == 0) {
			pthread_mutex_unlock(&current_thread_info->bigpool->lock);
			thread_helper(current_thread_info);
		}
		else {
			pthread_mutex_unlock(&current_thread_info->bigpool->lock);
			break;
		}
	}
	return NULL;
}

/* 
 * Shutdown this thread pool in an orderly fashion.  
 * Tasks that have been submitted but not executed may or
 * may not be executed.
 *
 * Deallocate the thread pool object before returning.
 */
void thread_pool_shutdown_and_destroy(struct thread_pool * pool)
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
	int j = 0;
	pthread_mutex_lock(&pool->lock);
	pool->is_shutdown = 1;
	pthread_mutex_unlock(&pool->lock);
	for (; j < totalThreads; j++)
	{
		sem_post(&pool->semaphore);
		//printf("unlocking");	
	}
	int i = 0;
	for (; i < totalThreads; i++)
	{
		//Must now join all threads.
		pthread_join(pool->thread_info[i].thread, NULL);
	}
	// All workers freed
	// free the worker list
	free(pool->thread_info);
	// All that's left is the pool.
	free(pool);
	// All the memory allocated has been freed so we
	// can exit
	//exit(0);
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
	
	/* Allocating a new future for the task */
	struct future *myFuture = malloc(sizeof(struct future));
	/* Initialzing the menbers of the Future */
	myFuture->task = task;	
	myFuture->data = data;
	myFuture->result = NULL;
	sem_init(&myFuture->signal, 0, 0);
	myFuture->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	myFuture->runState = 0;			// State 0 represents that the task has not been excuted yet

	
	/* If current thread is the main thread, submit the task to global deque */
	if (current_thread_info == NULL) {
		pthread_mutex_lock(&pool->lock);
		myFuture->mylist = 0;
		/* Push the future into the global deque */
		list_push_back(&pool->subdeque, &myFuture->elem);
		pthread_mutex_unlock(&pool->lock);
	}
	/* Otherwise submit the task to its own deque */
	else {			
		pthread_mutex_lock(&current_thread_info->local_lock);						
		myFuture->mylist = current_thread_info->worker_id;
		list_push_back(&current_thread_info->workerqueue, &myFuture->elem);
		pthread_mutex_unlock(&current_thread_info->local_lock);
	}
	/* Signal the workers there is a future submitted */
	sem_post(&pool->semaphore);
	return myFuture;
}

/* Make sure that the thread pool has completed the execution
 * of the fork join task this future represents.
 *
 * Returns the value returned by this task.
 */
void * future_get(struct future * givenFuture)
{
	
	//If the current thread is not main thread, check the future state
	if (current_thread_info != NULL) {
		
		pthread_mutex_lock(&givenFuture->mutex);
		int myList = givenFuture->mylist;
		
		
		// The given future is still pending
		if (myList > 0) {
				// Remove this future from its list when trying to executing it
			//printf("Locking worker list in worker: %d with addr: %p\n", myList - 1, &current_thread_info->bigpool->thread_info[myList - 1].local_lock);
	
			pthread_mutex_lock(&current_thread_info->bigpool->thread_info[myList - 1].local_lock);
			if (!is_interior (&givenFuture->elem)) {
				sem_wait(&givenFuture->signal);
				return givenFuture->result;
			}
			list_remove(&givenFuture->elem);
			givenFuture->elem.next = NULL;
			givenFuture->elem.prev = NULL;
			pthread_mutex_unlock(&current_thread_info->bigpool->thread_info[myList - 1].local_lock);
			//printf("Unlocking worker list in worker: %d with addr: %p\n", myList - 1, &current_thread_info->bigpool->thread_info[myList - 1].local_lock);

					//pthread_mutex_lock(&givenFuture->mutex);
			givenFuture->mylist = -1;
			fork_join_task_t task = givenFuture->task;
			current_thread_info->worker_state = 1;					// Set the state of the worker to be busy
			givenFuture->runState = 1;								// Set the runstate to be 1 when task is in progress
			pthread_mutex_unlock(&givenFuture->mutex);
			givenFuture->result = task(current_thread_info->bigpool, givenFuture->data);
			pthread_mutex_lock(&givenFuture->mutex);
			givenFuture->runState = 2;								// Set the runstate to be 2 when the result is aviliable
			current_thread_info->worker_state = 0;					// Set the state of the worker to be aviliable
			pthread_mutex_unlock(&givenFuture->mutex);
			sem_post(&givenFuture->signal);
			sem_wait(&givenFuture->signal);
		}
		else {
			pthread_mutex_unlock(&givenFuture->mutex);
			sem_wait(&givenFuture->signal);
		}
	}
	else { 
		sem_wait(&givenFuture->signal);
	}
	return givenFuture->result;
}


/* Deallocate this future.  Must be called after future_get() */
void future_free(struct future * givenFuture)
{
	if (givenFuture != NULL) {
		//sem_wait(&givenFuture->signal);
		struct future *oldFuture = givenFuture;
		free(oldFuture);
	}
}

/* Returns true if ELEM is an interior element,
   false otherwise. */
static inline bool
is_interior (struct list_elem *elem)
{
  return elem != NULL && elem->prev != NULL && elem->next != NULL;
}


