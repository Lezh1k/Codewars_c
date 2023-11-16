cmake_minimum_required(VERSION 3.16)

project(codewars_c)            # this sets the project name

###############################################################################
## file globbing ##############################################################
###############################################################################

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables 
# `sources` and `data`.
file(GLOB_RECURSE sources      src/*.c inc/*.h)
file(GLOB_RECURSE sources_test tests/*.c)
file(GLOB_RECURSE data resources/*)
# You can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically.

###############################################################################
## target definitions #########################################################
###############################################################################

find_package(PkgConfig REQUIRED)

add_executable(codewars_c ${sources} ${data})

# This allows to include files relative to the root of the src directory with a <> pair
target_include_directories(codewars_c PRIVATE inc)
# Just for codewars_c add some compiler flags.

set(warning_level -Wall -Wextra -pedantic)
set(cflags -pipe -O0 -g -std=gnu18)
target_compile_options(codewars_c PUBLIC ${warning_level} ${cflags})
target_link_libraries(codewars_c m) # math library
# This copies all resource files in the build directory.
# We need this, because we want to work with paths relative to the executable.
file(COPY ${data} DESTINATION resources)

###############################################################################
## dependencies ###############################################################
###############################################################################

# This defines the variables Boost_LIBRARIES that containts all library names
# that we need to link into the program.
# find_package(Boost REQUIRED COMPONENTS program_options)
#
# target_link_libraries(codewars_c PUBLIC
#   ${Boost_LIBRARIES}
# )

###############################################################################
## testing ####################################################################
###############################################################################

# This is for our testing framework,
# we don't add REQUIRED because it's just for testing.
# People who might want to build the project to use it should not be required
# to install testing dependencies.
# find_package(GTest)
#
# if(GTEST_FOUND)
#   add_executable(unit_tests ${sources_test} ${sources})
#   target_include_directories(unit_tests PRIVATE inc)
#
#   # This define is added to prevent collision with the main.
#   # It might be better solved by not adding the source with the main to the
#   # testing target.
#   target_compile_definitions(unit_tests PUBLIC _UNIT_TESTS_)
#
#   # This allows us to use the executable as a link library, and inherit all 
#   # linker options and library dependencies from it, by simply adding it as dependency.
#   set_target_properties(codewars_c PROPERTIES ENABLE_EXPORTS on)
#
#   target_link_libraries(unit_tests PUBLIC
#     ${GTEST_BOTH_LIBRARIES}
#     codewars_c
#   )
#
#   target_include_directories(unit_tests PUBLIC
#     ${GTEST_INCLUDE_DIRS} # doesn't do anything on linux
#   )
#  
# endif()

###############################################################################
## packaging ##################################################################
###############################################################################

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS codewars_c DESTINATION mlm_data_generator_destination)

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
