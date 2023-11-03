#include <liburing.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "directory.h"
#include "thread.h"

struct thread_state *thread_head = NULL, *thread_tail = NULL;

// Process completions in a separate thread.
int process_completions(void *value)
{
	struct thread_state *tstate = (struct thread_state*)value;
	while (tstate->running)
	{
		struct io_uring_cqe *cqe_head;
		struct __kernel_timespec ts;
		ts.tv_nsec = 2000;
		ts.tv_sec = 0;
		int res = io_uring_wait_cqes(tstate->ring, &cqe_head, 5, &ts, NULL);

		if (res < 0 && res != -ETIME)
		{
			fprintf(stderr, "Error waiting for completion events: %s\n", strerror(-res));
			exit(EXIT_FAILURE);
		}

		unsigned int head, i = 0;
		struct io_uring_cqe *cqe;

		io_uring_for_each_cqe(tstate->ring, head, cqe)
		{
			if (cqe->res < 0)
			{
				if (cqe->user_data != -1ull)
					fprintf(stderr, "Failed to copy file asynchronously: %s\n", strerror(-cqe->res));
				else
					fprintf(stderr, "Failed to perform asynchronous operation: %s\n", strerror(-cqe->res));
			}

			// If it's a file copy, re-call the write function to update the next stage.
			if (cqe->user_data != -1ull)
			{
				// It's a regular file copy operation (or an invalid pointer, either way we'll find out)
				struct state *state = (struct state*)cqe->user_data;
				write_regular_file(tstate->ring, state);
			}
			++i;
		}

		io_uring_cq_advance(tstate->ring, i);
	}

	return 0;
}

void start_process_thread(struct io_uring *uring)
{
	struct thread_state *tstate = malloc(sizeof(struct thread_state));
	bzero(tstate, sizeof(struct thread_state));
	tstate->ring = uring;
	tstate->running = true;

	errno = 0;
	int res = thrd_create(&tstate->handle, process_completions, tstate);
	if (res == thrd_nomem)
	{
		fprintf(stderr, "Insufficient memory to create a new thread\n");
		exit(EXIT_FAILURE);
	}
	else if (res == thrd_error)
	{
		fprintf(stderr, "Error creating a new thread: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!thread_head)
		thread_head = thread_tail = tstate;
	else
	{
		thread_tail->next = tstate;
		thread_tail = tstate;
	}
}
