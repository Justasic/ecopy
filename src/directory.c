#include "directory.h"
#include <ftw.h>
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Contains the state of an on-going copy being performed
struct copystate
{
	// Where the source file is from
	const char *source;
	// Where it's going
	const char *destination;
	// What we need to do next with regards to our ring state.
	int ring_state;
	// Information on the file.
	struct stat64 *sb_source;
	// It's open file descriptor for copying
	int fd;
	// Current copied amount (offset into the file)
	size_t offset;
	// Total size of the file.
	size_t total_size;
};

void write_regular_file(const char *fsource, const char *fdest, struct copystate *state)
{
	// Function can be called multiple times, call flow should work like:
	// IORING_OP_OPENAT
	// IORING_OP_FADVISE
	// IORING_OP_FALLOCATE
	// IORING_OP_READ_FIXED
	// IORING_OP_WRITE_FIXED
	// IORING_OP_CLOSE
}

int descend_directory64(const char *fpath, const struct stat64 *sb, int typeflag, struct FTW *ftwbuf)
{
	// prep the target path.
	static char buffer[8192] = {0};
	static char symbuffer[8192] = {0}; 
	strncpy(buffer, destination, sizeof(buffer));
	strncat(buffer, "/", sizeof(buffer)-1);

	// Treat . and .. as special
	if (*fpath == '.')
		strncat(buffer, fpath+2, sizeof(buffer)-1);
	else if (fpath[0] == '.' && fpath[1] == '.')
		strncat(buffer, fpath+3, sizeof(buffer)-1);
	else
		strncat(buffer, fpath, sizeof(buffer)-1);

	switch (typeflag)
	{
		case FTW_D: // Directory
		{
			printf("Dir: %s\n", buffer);
			struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
			io_uring_prep_mkdir(sqe, buffer, sb->st_mode);
			io_uring_submit(ring);
			break;
		}
		case FTW_F: // Regular file
			printf("File: %s\n", buffer);
			break;
		case FTW_SLN: // Symbolic Link to non-existing file
			__attribute__((fallthrough));
		case FTW_SL: // Symbolic Link
		{
			// Unfortunate syscall here.
			ssize_t pathlen = readlink(fpath, symbuffer, sizeof(buffer));
			// Add the null byte
			if (pathlen == sizeof(buffer))
				symbuffer[pathlen-1] = 0;
			else
				symbuffer[pathlen] = 0;

			printf("Symlink: %s -> %s\n", buffer, symbuffer);
			struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
			io_uring_prep_symlink(sqe, buffer, symbuffer);
			io_uring_submit(ring);
			break;
		}
		// case FTW_SLN: // Symbolic Link to non-existing file
		// 	printf("Broken symlink: %s\b", buffer);
		// 	break;
		case FTW_NS:
		case FTW_DNR:
			fprintf(stderr, "Cannot read %s\n", buffer);
			break;
	}

	// Wait for CQEs here and process them.
	// TODO: migrate this to it's own thread?
	// struct io_uring_cqe *cqe;
	//
	// struct __kernel_timespec ts;
	// ts.tv_sec = 0;
	// ts.tv_nsec = 200;
	//
	// int res = io_uring_wait_cqe_timeout(ring, &cqe, &ts);
	// if (res < 0)
	// {
	// 	fprintf(stderr, "Failed to get CQEs: %s\n", strerror(errno));
	// 	exit(EXIT_FAILURE);
	// }



	return 0;
}
