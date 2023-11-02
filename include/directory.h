#include <ftw.h>
#include <liburing.h>

// Destination directory we're copying to, set in int main.
extern const char *destination;
extern struct io_uring *ring;

// Contains the state of an on-going copy being performed
struct copystate
{
	// Where the source file is from
	char *source;
	// Where it's going
	char *destination;
	// What we need to do next with regards to our ring state.
	int ring_state;
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

extern void write_regular_file(struct io_uring *uring, struct copystate *state);

extern int descend_directory64(const char *fpath, const struct stat64 *sb, int typeflag, struct FTW *ftwbuf);
