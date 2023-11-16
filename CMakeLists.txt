cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

# TODO: maybe search for clang?

# Enable color output when using generators that may not allow cmake to enable it (like Ninja)
set(CMAKE_COLOR_DIAGNOSTICS TRUE)
# Output compile commands json for language servers in editors
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Add our cmake module path to the search paths cmake can use.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Finally, test the compiler and create the project.
project(ecopy
  VERSION 0.0.1
  DESCRIPTION "Worlds Fastest Copy Command"
  LANGUAGES C
)

# Various CMake options.
option(USE_MSAN "Use clang's memory sanitizer" OFF)
option(USE_ASAN "Use clang's address sanitizer" OFF)
option(USE_TSAN "Use clang's thread sanitizer" OFF)
# option(BUILD_STATIC_EXECUTABLES "Build a statically linked executable" OFF)
#
# # https://cmake.org/pipermail/cmake/2010-September/039850.html
# if(BUILD_STATIC_EXECUTABLES)
# 	set(CMAKE_EXE_LINKER_FLAGS -static)
# 	set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
# 	set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
# 	# set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
# 	set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
# 	# set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
# 	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
# 	# set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
# 	# Maybe this works as well, haven't tried yet.
# 	# set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
# else(BUILD_STATIC_EXECUTABLES)
# 	# Set RPATH to use for installed targets; append linker search path
# 	set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LOFAR_LIBDIR}")
# 	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# endif(BUILD_STATIC_EXECUTABLES)

# Find liburing
find_package(liburing REQUIRED)

# Generate our configure file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sysconf.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/sysconf.h)

# Gather all our C files to compile.
# FIXME: this will become problematic in the future
file(GLOB_RECURSE C_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR} "src/*.c")

# Set source properties.
set_source_files_properties(${C_SOURCE_FILES}
  PROPERTIES
    LANGUAGE C
	COMPILE_DEFINITIONS
		_GNU_SOURCE=1
)

# Create the target
add_executable(${PROJECT_NAME} ${C_SOURCE_FILES})

# Set target options.
set_target_properties(${PROJECT_NAME}
  PROPERTIES
    LINKER_LANGUAGE C
    C_STANDARD 17
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS FALSE
)

# Set compile options for the target.
target_compile_options(${PROJECT_NAME}
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

if (USE_MSAN)
	message(STATUS "Using memory sanitizer...")
	# Specify the compile option
	target_compile_options(${PROJECT_NAME}
		PRIVATE
			-fsanitize=memory
	)
	# Specify the link option
	target_link_options(${PROJECT_NAME}
		PRIVATE
			-fsanitize=memory
	)
endif (USE_MSAN)

if (USE_ASAN)
	message(STATUS "Using address sanitizer...")
	# Specify the compile option, also enable ubsan
	target_compile_options(${PROJECT_NAME}
		PRIVATE
			-fsanitize=address,undefined
	)
	# Specify the link option
	target_link_options(${PROJECT_NAME}
		PRIVATE
			-fsanitize=address,undefined
	)
endif (USE_ASAN)

if (USE_TSAN)
	message(STATUS "Using thread sanitizer...")
	# Specify the compile option
	target_compile_options(${PROJECT_NAME}
		PRIVATE
			-fsanitize=thread
	)
	# Specify the link option
	target_link_options(${PROJECT_NAME}
		PRIVATE
			-fsanitize=thread
	)
endif (USE_TSAN)

# Add include directories
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
	${LIBURING_INCLUDE_DIR}
)

# Set linker flags (if any)
#target_link_options(${PROJECT_NAME}
#  PRIVATE
#)

# Add libraries to link against (if any)
target_link_libraries(${PROJECT_NAME} ${LIBURING_LIBRARY})

# Enable the cmake testing framework
enable_testing()

# Add tests subdir.
add_subdirectory(test)
