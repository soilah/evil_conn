#ifndef __CONN_H__
#define __CONN_H__


#include <pthread.h>
#include "structs.h"

struct connection {
	/* A file descriptor returned
	 * by the accept system call */
	int fd;

	/* Pointer to the next connection */
	struct connection *next;
};

struct conn_queue {
	/* head of the queue */
	struct connection *head;

	/* tail of the queue */
	struct connection *tail;

	/* number of elements in the queue */
	int size;

	/* a pair of a pthread mutex and condition variable
	 * to allow blocking when trying to dequeue from an
	 * empty queue */
	pthread_mutex_t mutex;
	pthread_cond_t cond_var;
};

/* creates a new connection queue */
struct conn_queue *conn_queue_create(void);

/* creates a new connection object. fd is a
 * file descriptor previously returned by accept */
struct connection *conn_create(int fd);

/* enqueues a new connection in the queue and wakes up one
 * waiting thread */
void conn_enqueue(struct conn_queue *queue,int conn_fd);

/* returns the next file descriptor in the queue. This will
 * block if the queue is found empty, until an enqueue is
 * performed */
struct connection *conn_dequeue(struct conn_queue *queue,struct server_thread_args *);

/* returns true if the queue is empty */
int conn_empty(struct conn_queue *queue);

/* Send broadcast signal to threads */

void wake_up_threads(struct server_thread_args *args);

#endif /* __CONN_H__ */
