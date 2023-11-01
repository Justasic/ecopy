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

# Find liburing
find_package(liburing REQUIRED)

# Generate our configure file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/sysconf.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/sysconf.h)

# Gather all our C files to compile.
# FIXME: this will become problematic in the future
file(GLOB_RECURSE C_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR} "src/*.c")

message(STATUS "SOURCES: ${C_SOURCE_FILES}")

# Set source properties.
set_source_files_properties(${C_SOURCE_FILES}
  PROPERTIES
    LANGUAGE C
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
    -Wimplicit-fallthrough
    -Wno-unused-parameter
    -fPIC
)

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
