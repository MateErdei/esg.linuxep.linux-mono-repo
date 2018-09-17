cmake_minimum_required(VERSION 3.8)
project(everest_base CXX)

option(test "Build all tests." ON) # Makes boolean 'test' available.

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig")
enable_testing()

find_package(PkgConfig)
FIND_PACKAGE ( Threads REQUIRED )

if(NOT DEFINED REDIST)
    set( REDIST /redist )
endif()
if(NOT DEFINED BINARIES_DIR)
    set( BINARIES_DIR ${REDIST}/binaries )
endif()
if(NOT DEFINED INPUT)
    set( INPUT ${BINARIES_DIR}/linux11/input )
endif()

## Find 3rd party libraries
set( BOOST_INPUT ${INPUT}/boost )
set( CURL_INPUT ${INPUT}/curl )
set( OPENSSL_INPUT ${INPUT}/openssl )
set( EXPAT_INPUT ${INPUT}/expat )
set( LOG4CPLUS_INPUT ${INPUT}/log4cplus )

set( BOOST_INCLUDE ${BOOST_INPUT}/include )

if(NOT DEFINED VERSIG_INPUT)
    set( VERSIG_INPUT ${INPUT}/versig)
endif()

## Allow this to be redirected for SUL development
if(NOT DEFINED SUL_INPUT)
    set( SUL_INPUT ${INPUT}/SUL )
endif()

set( SUL_LIBRARY_DIR ${SUL_INPUT}/lib64 )
set( SUL_LIBRARY_DIRS STRING ${CMAKE_BINARY_DIR}/libs:${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64:${CURL_INPUT}/lib64:${OPENSSL_INPUT}/lib64 FORCE)

set( PROTOBUF_INPUT ${INPUT}/protobuf )
set(Protobuf_root ${PROTOBUF_INPUT}/install64)
set(Protobuf_INCLUDE_DIR ${Protobuf_root}/include)
set(PROTOBUF_PROTOC_EXECUTABLE ${Protobuf_root}/bin/protoc)
set(Protobuf_PROTOC_LIBRARY ${Protobuf_root}/lib/libprotoc.so ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_LITE_LIBRARY  ${Protobuf_root}/lib/libprotobuf-lite.so )
set(Protobuf_LIBRARY ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_DEBUG 1)
set(Protobuf_LD_LIBRARY_PATH ${Protobuf_root}/lib:$ENV{LD_LIBRARY_PATH} )
set( ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH} )
INCLUDE(FindProtobuf)
#FIND_PACKAGE(Protobuf REQUIRED )
configure_file(build/protoc.sh.in protoc.sh)

set( ZEROMQ_INPUT ${INPUT}/zeromq )
## use pkg-config to get hints for 0mq locations
#pkg_check_modules(PC_ZeroMQ QUIET zmq)

## load in pkg-config support
## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.h
        PATHS ${ZEROMQ_INPUT}/include
        )

## use the hint from above to find the location of libzmq
find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${ZEROMQ_INPUT}/lib
        )

find_path(EXPAT_INCLUDE_DIR
        NAMES expat.h
        PATHS ${EXPAT_INPUT}/include
        NO_DEFAULT_PATH
        )
find_library(EXPAT_LIBRARY
        NAMES expat
        PATHS ${EXPAT_INPUT}/lib64
        NO_DEFAULT_PATH
        )

set( LOG4CPLUS_INCLUDE_DIR ${LOG4CPLUS_INPUT}/include )

find_library(LOG4CPLUS_LIBRARY
        NAMES log4cplus
        PATHS ${LOG4CPLUS_INPUT}/lib
        )
set( OPENSSL_INCLUDE_DIR  ${OPENSSL_INPUT}/include64)
find_library(OPENSSL_LIBRARY
        NAMES ssl
        PATHS ${OPENSSL_INPUT}/lib64
        NO_DEFAULT_PATH
        )
find_library(OPENSSLCRYPTO_LIBRARY
        NAMES crypto
        PATHS ${OPENSSL_INPUT}/lib64
        NO_DEFAULT_PATH
        )


message("openssl"  ${OPENSSLCRYPTO_LIBRARY})


include_directories(modules)

set(REL_DIST distribution)
set(DIST "${CMAKE_BINARY_DIR}/${REL_DIST}")
set(CMAKE_INSTALL_PREFIX "${REL_DIST}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
