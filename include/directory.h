#include <ftw.h>
#include <liburing.h>

// Destination directory we're copying to, set in int main.
extern const char *destination;
extern struct io_uring *ring;

enum state_type
{
	ST_COPY,
	ST_LINK,
	ST_DIR
};

// Contains the state of an on-going copy being performed
struct copystate
{
	// It's open file descriptor for copying
	int fd;
	// Current copied amount (offset into the file)
	size_t offset;
	// Total size of the file.
	size_t total_size;
	// Size of the copy buffer.
	size_t copybufsz;
	// Copy buffer.
	char buffer[1];
};

// used to track what kind of state this is.
struct state
{
	// What kind of state this is.
	enum state_type type;
	// What we need to do next with regards to our ring state.
	int ring_state;
	// The source path of the file.
	char *source;
	// The destination path of the file.
	char *destination;
	// Stat of the source file.
	struct statx source_stat;
	// if this is a regular file, this is copied.
	// if it's a directory, then we keep track of what 
	// part of the recursive directory creation we're in.
	union {
		struct copystate *copystate;
		struct path *pathstate;
	};
};

extern void write_regular_file(struct io_uring *uring, struct state *state);

extern void descend_directory(const char *fpath);
