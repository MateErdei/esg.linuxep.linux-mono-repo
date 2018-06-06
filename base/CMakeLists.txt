cmake_minimum_required(VERSION 3.5)
project(everest_base CXX)

option(test "Build all tests." ON) # Makes boolean 'test' available.

set(CMAKE_CXX_STANDARD 11)

enable_testing()

find_package(PkgConfig)
FIND_PACKAGE ( Threads REQUIRED )
## use pkg-config to get hints for 0mq locations
pkg_check_modules(PC_ZeroMQ QUIET zmq)

## load in pkg-config support
## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.h
        PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
        )

## use the hint from about to find the location of libzmq
find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
        )

include_directories(modules)

add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
