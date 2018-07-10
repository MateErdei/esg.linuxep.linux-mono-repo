cmake_minimum_required(VERSION 3.5)
project(everest_base CXX)

option(test "Build all tests." ON) # Makes boolean 'test' available.

set(CMAKE_CXX_STANDARD 11)
#this is necessary right now because the centos 7 default gcc does not produce __cxx11 abi strings.
# and the cento7 gcc is building 3rdparty libraries.
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -D_GLIBCXX_USE_CXX11_ABI=0")
enable_testing()

find_package(PkgConfig)
FIND_PACKAGE ( Threads REQUIRED )

## use pkg-config to get hints for 0mq locations
#pkg_check_modules(PC_ZeroMQ QUIET zmq)

## load in pkg-config support
## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.h
        PATHS /redist/binaries/everest/zeromq/include
        )

## use the hint from about to find the location of libzmq
find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS /redist/binaries/everest/zeromq/lib
        )

if(NOT DEFINED REDIST)
    set( REDIST /redist )
endif()
if(NOT DEFINED BINARIES_DIR)
    set( BINARIES_DIR ${REDIST}/binaries )
endif()
if(NOT DEFINED INPUT)
    set( INPUT ${BINARIES_DIR}/linux11/input )
endif()

set( SUL_INPUT ${INPUT}/SUL )
set( BOOST_INPUT ${INPUT}/boost )
set( CURL_INPUT ${INPUT}/curl )
set( OPENSSL_INPUT ${INPUT}/openssl )
set( SUL_LIBRARY_DIR ${SUL_INPUT}/lib64 )
set( SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64:${CURL_INPUT}/lib64:${OPENSSL_INPUT}/lib64)

set(Protobuf_root /redist/binaries/everest/protobuf)
set(Protobuf_INCLUDE_DIR ${Protobuf_root}/include)
set(PROTOBUF_PROTOC_EXECUTABLE ${Protobuf_root}/bin/protoc)
set(Protobuf_PROTOC_LIBRARY ${Protobuf_root}/lib/libprotoc.so ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_LITE_LIBRARY  ${Protobuf_root}/lib/libprotobuf-lite.so )
set(Protobuf_LIBRARY ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_VERSION "3.5.1")
set( ENV{LD_LIBRARY_PATH} ${Protobuf_root}/lib:$ENV{LD_LIBRARY_PATH}  )
INCLUDE(FindProtobuf)
#FIND_PACKAGE(Protobuf REQUIRED )

include_directories(modules)

set(REL_DIST distribution)
set(DIST "${CMAKE_BINARY_DIR}/${REL_DIST}")
set(CMAKE_INSTALL_PREFIX "${REL_DIST}")

add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
