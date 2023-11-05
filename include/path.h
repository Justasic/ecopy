#include <string.h>

struct path
{
    // The actual path elements from the above path array
    char **elements;
    // Lenth of the elements array
    size_t length;
    // Allocated number of array pointers.
    size_t capacity;
    // Current iterator position in the elements array
    size_t iterator;
    // The original path as a null-terminated array
    char path[1];
};

extern void free_path(struct path *path);
extern struct path *split_path(const char *path);
extern char *construct_path(struct path *path);
extern char *path_normalize(const char *path);
