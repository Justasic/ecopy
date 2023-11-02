#include "directory.h"
#include <ftw.h>
#include <stdio.h>

int descend_directory64(const char *fpath, const struct stat64 *sb, int typeflag, struct FTW *ftwbuf)
{
	switch (typeflag)
	{
		case FTW_D: // Directory
			printf("Dir: %s\n", fpath);
			break;
		case FTW_F: // Regular file
			printf("File: %s\n", fpath);
			break;
		case FTW_SL: // Symbolic Link
			printf("Symlink: %s\n", fpath);
			break;
		case FTW_SLN: // Symbolic Link to non-existing file
			printf("Broken symlink: %s\b", fpath);
			break;
		case FTW_NS:
		case FTW_DNR:
			fprintf(stderr, "Cannot read %s\n", fpath);
			break;
	}

	return 0;
}
