cmake_minimum_required(VERSION 3.5)
project(versig)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)

set( REDIST /redist )
set( BINARIES_DIR ${REDIST}/binaries )
set( INPUT ${BINARIES_DIR}/linux10/input )
set( OPENSSL_DIR ${INPUT}/openssl )

set( OPENSSL_LIBRARY_DIR ${OPENSSL_DIR}/lib64 )

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${OPENSSL_LIBRARY_DIR} )

include_directories(BEFORE ${OPENSSL_DIR}/include64)

add_library(versig_lib
        CertificateTracker.cpp
        CertificateTracker.h
        crypto_utils.cpp
        crypto_utils.h
        digest_body.cpp
        digest_body.h
        digest_body_checker.cpp
        digest_buffer.cpp
        digest_buffer.h
        digest_buffer_checker.cpp
        digest_buffer_checker.h
        iostr_utils.h
        manifest_file.cpp
        manifest_file.h
        signed_file.cpp
        signed_file.h
        verify_exceptions.cpp
        verify_exceptions.h
        versig.cpp
        versig.h)

add_executable(versig versig_main.cpp versig.h)

target_link_libraries(versig PUBLIC versig_lib -L${OPENSSL_LIBRARY_DIR} ssl crypto )
