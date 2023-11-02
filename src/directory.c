#include "directory.h"
#include <fcntl.h>
#include <ftw.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void write_regular_file(struct io_uring *uring, struct copystate *state)
{
	// Function can be called multiple times, call flow should work like:
	// IORING_OP_OPENAT
	// IORING_OP_FADVISE
	// IORING_OP_FALLOCATE
	// IORING_OP_READ_FIXED
	// IORING_OP_WRITE_FIXED
	// IORING_OP_CLOSE

	switch (state->ring_state)
	{
		case IORING_OP_OPENAT:
			break;
		case IORING_OP_FADVISE:
			break;
		case IORING_OP_FALLOCATE:
			break;
		case IORING_OP_READ_FIXED:
			break;
		case IORING_OP_WRITE_FIXED:
			break;
		case IORING_OP_CLOSE:
		{
			// deallocate the object, it's no longer needed.
			free(state->source);
			free(state->destination);
			free(state);
			break;
		}
	}
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
			sqe->user_data = -1;
			io_uring_prep_mkdir(sqe, buffer, sb->st_mode);
			io_uring_submit(ring);
			break;
		}
		case FTW_F: // Regular file
		{
			printf("File: %s\n", buffer);

			// TODO: Expensive allocations, can we mitigate somehow?
			struct copystate *cp = malloc(sizeof(struct copystate) + sb->st_blksize);
			bzero(cp, sizeof(struct copystate));

			cp->copybufsz   = sb->st_blksize;
			cp->destination = strdup(buffer);
			cp->source      = strdup(fpath);
			cp->ring_state  = IORING_OP_OPENAT;
			cp->total_size  = sb->st_size;

			// prepare the first SQE
			struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
			io_uring_prep_openat(sqe, AT_FDCWD, cp->source, 0, sb->st_mode);
			io_uring_sqe_set_data(sqe, cp);
			io_uring_submit(ring);

			break;
		}
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
			sqe->user_data = -1;
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

	return 0;
}
