cmake_minimum_required(VERSION 3.16)

project(codewars VERSION 0.1 LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# set(CMAKE_C_COMPILER clang)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables 
# `sources` and `data`.
file(GLOB_RECURSE sources      src/*.c src/*.s inc/*.h)
file(GLOB_RECURSE sources_test tests/*.c)
file(GLOB_RECURSE data resources/*)

find_package(Curses REQUIRED)

message( "curses includes: ${CURSES_INCLUDE_DIRS}" )
message( "curses libs: ${CURSES_LIBRARIES}" )

add_executable(${PROJECT_NAME} ${sources} ${data})

target_compile_definitions(${PROJECT_NAME} PRIVATE
  _POSIX_C_SOURCE=200809L
  _posix_c_source=200809l
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -msse
  -msse2
  $<$<CONFIG:Release>:-O2>
  $<$<CONFIG:Debug>:-O0 -g>
)

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CURSES_INCLUDE_DIRS}
  inc
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
  m 
  ${CURSES_LIBRARIES}
)

# This copies all resource files in the build directory.
# We need this, because we want to work with paths relative to the executable.
file(COPY ${data} DESTINATION resources)
