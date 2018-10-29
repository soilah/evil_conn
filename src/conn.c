#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "conn.h"

/* Locks the queue, using the underlying lock */
static void _conn_queue_lock(struct conn_queue *queue)
{
	int ret;

	ret = pthread_mutex_lock(&queue->mutex);
	if (ret) {
		perror("Could not lock queue");
		exit(1);
	}
}

/* Unlocks the queue */
static void _conn_queue_unlock(struct conn_queue *queue)
{
	int ret;

	ret = pthread_mutex_unlock(&queue->mutex);
	if (ret) {
		perror("Could not unlock queue");
		exit(1);
	}
}

/* Signals the conditional variable allowing one thread
 * to wake up and get the newly inserted connection */
static void _conn_wakeup_blocked(struct conn_queue *queue)
{
	int ret;

	ret = pthread_cond_signal(&queue->cond_var);
	if (ret) {
		perror("Error waking up thread in queue");
		exit(1);
	}
}

/* Blocks the thread that tries to dequeue. This assumes
 * that the caller holds the corresponding mutex */
static void _conn_block(struct conn_queue *queue)
{
	/* pthread_cond_wait never returns an error */
	pthread_cond_wait(&queue->cond_var, &queue->mutex);
}

/* returns true if the queue is empty. It assumes that
 * the caller holds the queue's mutex */
static inline int _conn_empty(struct conn_queue *queue)
{
	return queue->head == NULL;
}

/* Enqueues one connection. This assumes that the
 * caller holds the queue's mutex */
static inline void _conn_enqueue(struct conn_queue *queue, struct connection *conn)
{
	conn->next = queue->tail;
	queue->tail = conn;
	if (!queue->head)
		queue->head = conn;

	queue->size++;
}

/* Dequeues and returns a connection. This assumes that the
 * queue is not empty and that the caller holds the queue's
 * lock */
static inline struct connection *_conn_deque(struct conn_queue *queue)
{
	struct connection *ret;

	assert(!_conn_empty(queue));

	ret = queue->head;
	queue->head = ret->next;
	if (queue->tail == ret) {
		assert(queue->head == NULL);
		queue->tail = NULL;
	}

	queue->size--;
	ret->next = NULL;

	return ret;
}

static struct connection *_conn_deque_or_sleep(struct conn_queue *queue,struct server_thread_args *args)
{
	struct connection *ret;

	/* We take the mutex before checking. _conn_block is guaranteed
	 * to unlock the mutex before actually blocking and re-taking it
	 * before it returns */
	_conn_queue_lock(queue);
	while (_conn_empty(queue) && args->status ==1){
		_conn_block(queue);
    }
    _conn_queue_unlock(queue);
    if (!args->status){
        pthread_exit(NULL);
    }

	/* At this point the queue cannot be empty */
	assert(!_conn_empty(queue));

	ret = _conn_deque(queue);

	/* We should have a valid connection */
	assert(ret != NULL);

	return ret;
}

struct conn_queue *conn_queue_create(void)
{
	struct conn_queue *ret;

	ret = malloc(sizeof(struct conn_queue));
	if (!ret) {
		perror("Could not allocate memory for new connection queue");
		exit(1);
	}

	ret->head = NULL;
	ret->tail = NULL;
	ret->size = 0;
	pthread_mutex_init(&ret->mutex, NULL);
	pthread_cond_init(&ret->cond_var, NULL);

	return ret;
}

struct connection *conn_create(int fd)
{
	struct connection *ret;

	ret = malloc(sizeof(struct connection));
	if (!ret) {
		perror("Could not allocate memory for new connection");
		exit(1);
	}

	ret->fd = fd;
	ret->next = NULL;

	return ret;
}

void conn_enqueue(struct conn_queue *queue, int conn_fd)
{

	assert(queue != NULL);

    struct connection *conn = conn_create(conn_fd);

	_conn_queue_lock(queue);
	_conn_enqueue(queue, conn);
	_conn_wakeup_blocked(queue);
	_conn_queue_unlock(queue);
}


struct connection *conn_dequeue(struct conn_queue *queue,struct server_thread_args *args)
{
	assert(queue != NULL);

	return _conn_deque_or_sleep(queue,args);
}

void wake_up_threads(struct server_thread_args *args){
    pthread_cond_broadcast(&(args->fd_queue->cond_var));
}
