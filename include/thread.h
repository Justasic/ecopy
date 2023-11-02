#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

struct thread_state
{
	// Handle to the thread object itself.
	thrd_t handle;
	// Whether this thread is running
	atomic_bool running;
	// The uring object
	struct io_uring *ring;
	// Linked list of other threads.
	struct thread_state *next;
};

extern struct thread_state *thread_head, *thread_tail;
extern void start_process_thread(struct io_uring *uring);
