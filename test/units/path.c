#include "path.h"
#include "nala.h"

// Test whether paths can be normalized.
TEST(NormalizePaths)
{
	// Sanity check
	ASSERT_EQ(path_normalize("/path/to/a"), "/path/to/a");
	// Whether /path//to//////a is normalized to /path/to/a 
	ASSERT_EQ(path_normalize("/path//to///////a"), "/path/to/a");
	// Whether periods are removed.
	ASSERT_EQ(path_normalize("/path/to/./a"), "/path/to/a");
	// Whether several periods are removed.
	ASSERT_EQ(path_normalize("/path/./to/./a"), "/path/to/a");
	// Whether backtracking works.
	ASSERT_EQ(path_normalize("/path/to/b/../a"), "/path/to/a");
	// Whether multiple backtracks work.
	ASSERT_EQ(path_normalize("/path/to/b/../../a"), "/path/a");
}
