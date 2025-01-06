cmake_minimum_required(VERSION 3.16)

project(codewars_c)
enable_language(C ASM)
###############################################################################
## file globbing ##############################################################
###############################################################################

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_C_COMPILER clang)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug build.")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  message("Release build.")
else()
  message("Some other build type. Setting up default = Release")
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables 
# `sources` and `data`.
file(GLOB_RECURSE sources      src/*.c src/*.s inc/*.h)
file(GLOB_RECURSE sources_test tests/*.c)
file(GLOB_RECURSE data resources/*)
# You can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically.
###############################################################################
## target definitions #########################################################
###############################################################################

add_executable(codewars_c ${sources} ${data})
target_include_directories(codewars_c PRIVATE inc)

set(warning_level -Wall -Wextra -pedantic)
target_compile_options(codewars_c PUBLIC ${warning_level})
target_link_libraries(codewars_c m) 

# This copies all resource files in the build directory.
# We need this, because we want to work with paths relative to the executable.
file(COPY ${data} DESTINATION resources)

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS codewars_c DESTINATION codewars_c_destination)

# This is basically a repeat of the file copy instruction that copies the
# resources in the build directory, but here we tell cmake that we want it
# in the package.
install(DIRECTORY resources DESTINATION codewars_c_destination)

# Now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here.
set(CPACK_PACKAGE_NAME "codewars_c")
set(CPACK_PACKAGE_VERSION "0.1.0")
# We don't want to split our program up into several incomplete pieces.
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
