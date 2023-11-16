# Find all the C sources in this folder (and it's subdirs)
file(GLOB_RECURSE C_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR} "*.c")

# Additional files we must link for the tests to compile.
list(APPEND C_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/../src/path.c
)

# Set the source files as C sources.
set_source_files_properties(${C_SOURCE_FILES} 
	PROPERTIES 
		LANGUAGE C
		COMPILE_DEFINITIONS
			_GNU_SOURCE=1
)

# Add our target executable
add_executable(tests ${C_SOURCE_FILES})

# Set the include directories this tagret will use.
target_include_directories(tests
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Set the target as a C binary using C17 standard.
set_target_properties(tests 
	PROPERTIES 
		LINKER_LANGUAGE C 
		C_STANDARD 17
		C_STANDARD_REQUIRED TRUE
		C_EXTENSIONS FALSE
)

# Set compile options for the target.
target_compile_options(tests
  PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -pedantic
    -Werror=shadow
    -Werror=implicit-function-declaration
	-Werror=format
    -Wimplicit-fallthrough
    -Wno-unused-parameter
	-fPIC
)

# Add the testing executable
add_test(
	NAME main_tests
	COMMAND $<TARGET_FILE:tests>
)
