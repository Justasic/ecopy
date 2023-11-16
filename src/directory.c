#include "directory.h"
#include <fcntl.h>
#include <ftw.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

void write_regular_file(struct io_uring *uring, struct state *state)
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
			free(state->copystate);
			free(state->source);
			free(state->destination);
			free(state);
			break;
		}
	}
}

/**
 * Iterate a directory, this will "recursively" iterate child directories.
 */
void descend_directory(const char *fdir)
{
	DIR *d = opendir(fdir);
	if (d) 
	{
		struct dirent *dir;
		while ((dir = readdir(d)) != NULL) 
		{
			// Skip . and ..
			if ((dir->d_name[0] == '.' && !dir->d_name[1]) || 
					(dir->d_name[0] == '.' && dir->d_name[1] == '.' && !dir->d_name[2]))
			{
				continue;
			}

			char fpath[8192] = {0};
			char buffer[8192] = {0};

			// Normalize the source path.
			// FIXME: these use unsafe stpcpy instead of specifying buffer length!
			char *bufptr = fpath;
			if (!((fdir[0] == '.' && !fdir[1]) || (fdir[0] == '.' && fdir[1] == '.' && !fdir[2])))
			{
				bufptr = stpcpy(fpath, fdir);
				bufptr = stpcpy(bufptr, "/");
			}
			bufptr = stpcpy(bufptr, dir->d_name);

			// Create the destination path.
			bufptr = buffer;
			bufptr = stpcpy(bufptr, destination);
			bufptr = stpcpy(bufptr, "/");
			bufptr = stpcpy(bufptr, fpath);

			printf("path: %s -> %s\n", fpath, buffer);

			// Allocate a new object.
			struct state *state = malloc(sizeof(struct state));
			bzero(state, sizeof(struct state));
			state->ring_state  = IORING_OP_STATX;
			state->source      = strdup(fpath);
			state->destination = strdup(buffer);

			// Perform a stat on the file, we need size and type info.
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
			io_uring_prep_statx(sqe, AT_FDCWD, state->source, AT_SYMLINK_NOFOLLOW, STATX_SIZE|STATX_TYPE, &state->source_stat);
			io_uring_submit(ring);
		}
		closedir(d);
	}
	else
		fprintf(stderr, "Failed to open %s: %s\n", fdir, strerror(errno));
}
