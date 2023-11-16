#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "directory.h"
#include "path.h"
#include "thread.h"

char *names[] = {
	"IORING_OP_NOP",
	"IORING_OP_READV",
	"IORING_OP_WRITEV",
	"IORING_OP_FSYNC",
	"IORING_OP_READ_FIXED",
	"IORING_OP_WRITE_FIXED",
	"IORING_OP_POLL_ADD",
	"IORING_OP_POLL_REMOVE",
	"IORING_OP_SYNC_FILE_RANGE",
	"IORING_OP_SENDMSG",
	"IORING_OP_RECVMSG",
	"IORING_OP_TIMEOUT",
	"IORING_OP_TIMEOUT_REMOVE",
	"IORING_OP_ACCEPT",
	"IORING_OP_ASYNC_CANCEL",
	"IORING_OP_LINK_TIMEOUT",
	"IORING_OP_CONNECT",
	"IORING_OP_FALLOCATE",
	"IORING_OP_OPENAT",
	"IORING_OP_CLOSE",
	"IORING_OP_FILES_UPDATE",
	"IORING_OP_STATX",
	"IORING_OP_READ",
	"IORING_OP_WRITE",
	"IORING_OP_FADVISE",
	"IORING_OP_MADVISE",
	"IORING_OP_SEND",
	"IORING_OP_RECV",
	"IORING_OP_OPENAT2",
	"IORING_OP_EPOLL_CTL",
	"IORING_OP_SPLICE",
	"IORING_OP_PROVIDE_BUFFERS",
	"IORING_OP_REMOVE_BUFFERS",
	"IORING_OP_TEE",
	"IORING_OP_SHUTDOWN",
	"IORING_OP_RENAMEAT",
	"IORING_OP_UNLINKAT",
	"IORING_OP_MKDIRAT",
	"IORING_OP_SYMLINKAT",
	"IORING_OP_LINKAT",
	"IORING_OP_MSG_RING",
	"IORING_OP_FSETXATTR",
	"IORING_OP_SETXATTR",
	"IORING_OP_FGETXATTR",
	"IORING_OP_GETXATTR",
	"IORING_OP_SOCKET",
	"IORING_OP_URING_CMD",
	"IORING_OP_SEND_ZC",
	"IORING_OP_SENDMSG_ZC",
	/* this goes last, obviously */
	"IORING_OP_LAST"
};

struct thread_state *thread_head = NULL, *thread_tail = NULL;

void process_operation(struct io_uring *uring, struct io_uring_cqe *cqe)
{
	struct state *state = (struct state*)cqe->user_data;

	switch (state->ring_state)
	{
		case IORING_OP_STATX:
		{
			// The statx buffer should now be filled in.
			switch (state->source_stat.stx_mode & S_IFMT)
			{
				case S_IFDIR: // Directory.
				{
					state->type = ST_DIR;
					printf("Directory: %s -> %s\n", state->source, state->destination);

					// First, normalize the path we need to create.
					char *normalized = path_normalize(state->destination);
					// next, split the path into components.
					struct path *p = split_path(normalized);

					// Begin concatenating parts of the path so we can mkdir them.
					state->pathstate = p;
					char *nextpath = construct_path(p);

					printf("Initial mkdir for %s\n", nextpath);
					printf(" - source: %s\n", state->source);
					printf(" - destination: %s\n", state->destination);

					struct io_uring_sqe *sqe = NULL;

					size_t count = 0;
					while (!(sqe = io_uring_get_sqe(ring)) || ++count >= 10000)
						io_uring_submit(ring);

					if (!sqe)
					{
						fprintf(stderr, "FATAL: Cannot acquire a new submission request, ring is full! This is a bug.\n");
						exit(EXIT_FAILURE);
					}
					io_uring_sqe_set_data(sqe, state);
					io_uring_prep_mkdir(sqe, nextpath, state->source_stat.stx_mode);
					state->ring_state = IORING_OP_MKDIRAT;
					break;
				}
				case S_IFLNK: // Symlink.
				{
#if 0
					state->type = ST_LINK; 
					char symbuffer[8192] = {0};

					// Unfortunate syscall here.
					ssize_t pathlen = readlink(state->source, symbuffer, sizeof(symbuffer));
					// Add the null byte
					if (pathlen == sizeof(symbuffer))
						symbuffer[pathlen-1] = 0;
					else
						symbuffer[pathlen] = 0;

					printf("Symlink: %s -> %s\n", state->destination, symbuffer);
					struct io_uring_sqe *sqe = NULL;

					size_t count = 0;
					while (!(sqe = io_uring_get_sqe(ring)) || ++count >= 10000)
						io_uring_submit(ring);

					if (!sqe)
					{
						fprintf(stderr, "FATAL: Cannot acquire a new submission request, ring is full! This is a bug.\n");
						exit(EXIT_FAILURE);
					}
					io_uring_sqe_set_data(sqe, state);
					io_uring_prep_symlink(sqe, state->destination, symbuffer);
					state->ring_state = IORING_OP_SYMLINKAT;
#endif
					break;
				}
				case S_IFREG: // Regular file.
#if 0
					state->type = ST_COPY; 

					printf("File: %s -> %s\n", state->source, state->destination);

					// TODO: Expensive allocations, can we mitigate somehow?
					struct copystate *cp = state->copystate = malloc(sizeof(struct copystate) + 8192);
					state->type       = ST_COPY;
					state->ring_state = IORING_OP_OPENAT;
					// Buffer size determined from this talk: https://www.youtube.com/watch?v=qbKGw8MQ0i8
					cp->copybufsz   = 8192;
					cp->total_size  = state->source_stat.stx_size;

					// prepare the first SQE
					struct io_uring_sqe *sqe = NULL;

					size_t count = 0;
					while (!(sqe = io_uring_get_sqe(ring)) || ++count >= 10000)
						io_uring_submit(ring);

					if (!sqe)
					{
						fprintf(stderr, "FATAL: Cannot acquire a new submission request, ring is full! This is a bug.\n");
						exit(EXIT_FAILURE);
					}
					io_uring_prep_openat(sqe, AT_FDCWD, state->source, 0, state->source_stat.stx_mode);
					io_uring_sqe_set_data(sqe, state);
#endif
					break;
				default: // Skip anything else, waste of a syscall.
					return;
			}

			io_uring_submit(ring);

			break;
		}
		case IORING_OP_MKDIRAT:
		{
			// Descend down the next directory
			if (cqe->res >= 0)
			{

				// Descend the directory, we've made the path now.
				if (state->pathstate->iterator == state->pathstate->length)
				{
					printf("Attempted to deallocate %s, %zu == %zu\n", state->destination, state->pathstate->iterator, state->pathstate->length);
					// printf("Setting path state to null for %s\n", state->destination);
					// // we can deallocate our path info.
					// free_path(state->pathstate);
					// state->pathstate = NULL;
					// descend_directory(state->source);
				}
				else
				{
					// Path is incomplete, continue forming the next set of paths.
					char *nextpath = construct_path(state->pathstate);
					struct io_uring_sqe *sqe = NULL;

					size_t count = 0;
					while (!(sqe = io_uring_get_sqe(ring)) || ++count >= 10000)
						io_uring_submit(ring);

					if (!sqe)
					{
						fprintf(stderr, "FATAL: Cannot acquire a new submission request, ring is full! This is a bug.\n");
						exit(EXIT_FAILURE);
					}
					io_uring_sqe_set_data(sqe, state);
					io_uring_prep_mkdir(sqe, nextpath, state->source_stat.stx_mode);
					state->ring_state = IORING_OP_MKDIRAT;
					printf("Next path creation: %s\n", nextpath);
					io_uring_submit(ring);
				}
			}
			break;
		}
		default:
			printf("Not implimented: %s\n", names[state->ring_state]);
			break;
	}

	// write_regular_file(tstate->ring, state);
}

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
			struct state *state = (struct state*)cqe->user_data;
			if (cqe->res < 0)
				fprintf(stderr, "Failed %s from %s to %s: %s\n", names[state->ring_state], state->source, state->destination, strerror(-cqe->res));
			
			process_operation(tstate->ring, cqe);
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
