#include <stdio.h>
#include <stdlib.h>

#include "path.h"

void free_path(struct path *path)
{
	free(path->elements);
	free(path);
}

struct path *split_path(const char *path)
{
    // If the path starts with /, skip it
    if (*path == '/')
        path++;

    size_t len = strlen(path);

    struct path *p = malloc(sizeof(struct path) + len + 1);
	if (!p)
	{
		fprintf(stderr, "Insufficient memory available\n");
		exit(EXIT_FAILURE);
	}

	// strcpy here is busted as fuck, use memcpy instead.
	memcpy(p->path, path, len);
	p->path[len+1] = 0;
	p->length = 0;
    p->capacity = 8; // initial capacity so we don't call realloc a bunch.
    p->elements = calloc(p->capacity, sizeof(char*));

	if (!p->elements)
	{
		fprintf(stderr, "Insufficient memory available\n");
		exit(EXIT_FAILURE);
	}

    // Find the path separators in p->path and replace them with \0
    char *iter = p->path;
	p->elements[p->length] = p->path;
	if (!((iter[0] == '.' && !iter[1]) || (iter[0] == '.' && iter[1] == '.' && !iter[2])))
		p->length++;
	
	// FIXME: this could be made better with a do-while loop maybe?
    while (*++iter)
    {
        if (*iter == '/')
        {
            *iter = 0;
            // Terminate the loop if there's a trailing /
            if (!*(iter+1))
                break;
            
            // Resize the array (safely)
            if (p->length+1 == p->capacity)
            {
                p->capacity += 8;
                size_t result;
                if (__builtin_umull_overflow(sizeof(char*), p->capacity, &result))
                {
                    fprintf(stderr, "Capacity overflow\n");
                    exit(EXIT_FAILURE);
                }
                void *elm_ptr = realloc(p->elements, result);
                if (!elm_ptr)
                {
                    fprintf(stderr, "Insufficient memory!\n");
                    exit(EXIT_FAILURE);
                }
                p->elements = elm_ptr;
            }

            // Append the element
            p->elements[p->length] = iter+1;
			if (!((iter[0] == '.' && !iter[1]) || (iter[0] == '.' && iter[1] == '.' && !iter[2])))
				p->length++;
        }
    }

    return p;
}

char *construct_path(struct path *path)
{
	static char buffer[8192];
	bzero(buffer, sizeof(buffer));

	char *ptr = buffer;

    if (path->iterator < path->length)
        path->iterator++;
    else
        path->iterator = path->length;

	for (size_t i = 0; i < path->iterator && i < path->length; i++)
	{
		ptr = stpcpy(ptr, path->elements[i]);
		ptr = stpcpy(ptr, "/");
	}

	return buffer;
}

/**
 * Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A/B.
 * It should be understood that this may change the meaning of the path
 * if it contains symbolic links!
 *
 * Calling this function more than once will change the returned pointer.
 */
char *path_normalize(const char *path) 
{
    if (!path)
        return NULL;

    static char copy[8192];
    bzero(copy, sizeof(copy));
    strcpy(copy, path);

    char *ptr = copy;

    for (int i = 0; copy[i]; i++)
    {
        if (path[i] == '/' && path[i+1] && path[i+1] == '.')
            i += 2;

        if (path[i] == '/' && path[i+1] && path[i+1] == '.' && path[i+2] && path[i+2] == '.')
        {
            i += 3;
            // Iterate Backward to the previous slash.
			// NOTE: this doesn't properly handle `../../`
            while (*ptr != '/')
                ptr--;
        }

        *ptr++ = path[i];
        if (path[i] == '/')
        {
            i++;
            while (path[i] == '/') 
                i++;
            i--;
        }
    }

	// Null terminate
    *ptr = 0;

    return copy;
}
