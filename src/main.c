#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <liburing.h>
#include <threads.h>
#include <unistd.h>

#include "directory.h"
#include "thread.h"

const char *destination;
struct io_uring *ring;

__attribute__((noreturn))
void usage(int argc, const char **argv) 
{
	// Handle weird usecase where argv[0] may be null.
	if (argc < 1)
		fprintf(stderr, "Usage: ecopy SRC [SRC ...] DEST\n");
	else
		fprintf(stderr, "Usage: %s SRC [SRC ...] DEST\n", argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, const char **argv)
{
	// TODO: parse command line arguments.
	// TODO: Warn/error about not running as root?
	int euid = geteuid();

	if (argc < 3)
		usage(argc, argv);

	destination = argv[argc-1];

	// ring parameters.
	struct io_uring_params ring_params;
	bzero(&ring_params, sizeof(ring_params));

	int ret;
	struct rlimit64 nofile_limit;
	ret = getrlimit64(RLIMIT_NOFILE, &nofile_limit);

	if (ret < 0)
		fprintf(stderr, "Failed to check file descriptor limits: %s (%d)\n", strerror(errno), errno);
	else
	{
		if (nofile_limit.rlim_cur != nofile_limit.rlim_max && euid)
		{
			fprintf(stderr, "Can only open %ld file descriptors of %ld, attempting to set to %ld...\n", nofile_limit.rlim_cur, nofile_limit.rlim_max, nofile_limit.rlim_max);
			nofile_limit.rlim_cur = nofile_limit.rlim_max;
			ret = setrlimit64(RLIMIT_NOFILE, &nofile_limit);
			if (ret < 0)
				fprintf(stderr, "Failed to set file descriptor soft limit: %s\nNOTE: this may cause copying to be slowed...\n", strerror(errno));
		}
		else if (nofile_limit.rlim_cur != nofile_limit.rlim_max && !euid)
		{
			rlim64_t fdmax = nofile_limit.rlim_max;
			fprintf(stderr, "Can only open %ld file descriptors as root, attempting to set infinite...\n", nofile_limit.rlim_cur);
			nofile_limit.rlim_cur = nofile_limit.rlim_max = RLIM_INFINITY;
			ret = setrlimit64(RLIMIT_NOFILE, &nofile_limit);
			if (ret < 0)
			{
				// Attempt to set it to the max we can go.
				fprintf(stderr, "Failed to set infinite file descriptor limit, attempting to set current limit to maximum of %ld\n", fdmax);
				nofile_limit.rlim_cur = nofile_limit.rlim_max = fdmax;
				ret = setrlimit64(RLIMIT_NOFILE, &nofile_limit);
				if (ret < 0)
					fprintf(stderr, "Failed to raise file descriptor limits: %s\nNOTE: this may cause copying to be slowed...\n", strerror(errno));
			}

			// Verify our rlimits now, print a warning if they're too low.
			ret = getrlimit64(RLIMIT_NOFILE, &nofile_limit);
			if (ret < 0)
				fprintf(stderr, "Failed to get file descriptor limits: %s\n", strerror(errno));
			else
			{
				// TODO: Find an appropriate limit?
				if (nofile_limit.rlim_cur < 4096)
					fprintf(stderr, "A file descriptor limit below 4096 is likely to cause slowdowns, please consider adjusting the limits and re-running this program.\n");
			}
		}
	}

	// TODO: probe kernel for supported capabilities?
	
	// Setup a kernel thread to pull for the submission queue
	ring_params.flags = IORING_SETUP_SQPOLL;
	// How long the thread should busy-wait before we have to 
	// wake it with a syscall.
	ring_params.sq_thread_idle = 2000;

	struct io_uring local_ring;
	// stack-allocate the ring, avoiding the heap.
	ring = &local_ring;
	// NOTE: not sure if 8 entries is enough, may need to be more?
	ret = io_uring_queue_init_params(8, ring, &ring_params);
	if (ret < 0)
	{
		fprintf(stderr, "Failed to initialize io_uring: %s (%d)\n", strerror(-ret), -ret);
		return EXIT_FAILURE;
	}

	// Start some threads.
	for (int i = 0; i < 3; ++i)
		start_process_thread(&local_ring);

	// Enter io loop to begin recursive descent.
	
	// Iterate each argument of argv, skipping the first element (our own name)
	// and the last element (the destination path)
	for (int i = 1; i < argc-1; ++i)
	{
		const char *path = argv[i];
		if (!strcasecmp(path, "-h") || !strcasecmp(path, "--help"))
			usage(argc, argv);

		// Perform a recursive descent into each path given.
		// Walk the directory, FTW_MOUNT keeps it in the same filesystem
		// FTW_PHYS means symbolic links won't be dereferenced.
		ret = nftw64(path, descend_directory64, nofile_limit.rlim_cur, FTW_MOUNT | FTW_PHYS);
		if (ret < 0)
			fprintf(stderr, "Failed to read %s: %s\n", path, strerror(errno));
	}

	// Request all the threads terminate.
	for (struct thread_state *iter = thread_head; iter; iter = iter->next)
	{
		iter->running = false;
		int res;
		thrd_join(iter->handle, &res);
	}

	// Close the queue, we're done.
	io_uring_queue_exit(ring);

	return EXIT_SUCCESS;
}
