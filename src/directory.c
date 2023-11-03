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

	switch (state->copystate->ring_state)
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
void iterate_directory_set(const char *fdir)
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
			char symbuffer[8192] = {0}; 

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
			bufptr = NULL;

			printf("path: %s -> %s\n", fpath, buffer);
			int type = DT_UNKNOWN;

			// Attempt to be cheap about syscalls here and use the type.
			switch (dir->d_type)
			{
				case DT_BLK: __attribute__((fallthrough)); // Block device, skip.
				case DT_CHR: __attribute__((fallthrough)); // Character device, skip.
				case DT_FIFO:__attribute__((fallthrough)); // FIFO device, skip.
				case DT_SOCK: continue;                    // UNIX socket, skip.
				case DT_DIR: __attribute__((fallthrough)); // Directory, note for later.
				case DT_LNK: __attribute__((fallthrough)); // Symbolic link, note for later.
				case DT_REG:                               // Regular file, note for later.
					type = dir->d_type;
					break;
				case DT_UNKNOWN: // Unknown type, we're forced to make a syscall
				{
					printf("Unknown file %s, falling back to lstat...\n", dir->d_name);
					struct stat64 sb;
					if (lstat64(buffer, &sb) < 0)
					{
						fprintf(stderr, "Failed to open %s: %s\n", buffer, strerror(errno));
						continue;
					}

					switch (sb.st_mode & S_IFMT)
					{
						case S_IFDIR: type = DT_DIR; break; // Directory.
						case S_IFLNK: type = DT_LNK; break; // Symlink.
						case S_IFREG: type = DT_REG; break; // Regular file.
						default: // Skip anything else, waste of a syscall.
							continue;
					}
					break;
				}
				default:
					printf("You've reached an unreachable part of the program? wtf.\n");
					continue;
			}

			// Logic for dispatching the creation of whatever we're doing.
			struct state *state = malloc(sizeof(struct state));
			bzero(state, sizeof(struct state));
			switch (type)
			{
				case DT_DIR:
				{
					printf("Directory: %s -> %s\n", fpath, buffer);
					state->type = ST_DIR;
					struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
					io_uring_sqe_set_data(sqe, state);
					io_uring_prep_mkdir(sqe, buffer, sb->st_mode);
					break;
				}
				case DT_REG:
				{
					state->type = ST_COPY;

					printf("File: %s -> %s\n", fpath, buffer);

					// TODO: Expensive allocations, can we mitigate somehow?
					struct copystate *cp = state->copystate = malloc(sizeof(struct copystate) + 8192);
					state->type     = ST_COPY;
					// Buffer size determined from this talk: https://www.youtube.com/watch?v=qbKGw8MQ0i8
					cp->copybufsz   = 8192;
					cp->ring_state  = IORING_OP_OPENAT;
					cp->total_size  = sb->st_size;

					// prepare the first SQE
					struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
					io_uring_prep_openat(sqe, AT_FDCWD, state->source, 0, sb->st_mode);
					io_uring_sqe_set_data(sqe, state);

					break;
				}
				case DT_LNK:
					state->type = ST_LINK;

					// Unfortunate syscall here.
					ssize_t pathlen = readlink(fpath, symbuffer, sizeof(buffer));
					// Add the null byte
					if (pathlen == sizeof(buffer))
						symbuffer[pathlen-1] = 0;
					else
						symbuffer[pathlen] = 0;

					printf("Symlink: %s -> %s\n", buffer, symbuffer);
					struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
					io_uring_sqe_set_data(sqe, state);
					io_uring_prep_symlink(sqe, buffer, symbuffer);

					break;
				default:
					fprintf(stderr, "BUG! You've encountered an unreachable part of the program!\n");
					free(state);
					break;
			}

		}
		closedir(d);
	}
	else
		fprintf(stderr, "Failed to open %s: %s\n", fdir, strerror(errno));
}
