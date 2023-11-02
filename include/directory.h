#include <ftw.h>
#include <liburing.h>

// Destination directory we're copying to, set in int main.
extern const char *destination;
extern struct io_uring *ring;

extern int descend_directory64(const char *fpath, const struct stat64 *sb, int typeflag, struct FTW *ftwbuf);
