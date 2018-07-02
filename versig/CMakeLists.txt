cmake_minimum_required(VERSION 3.5)
project(versig)

if (NOT DEFINED CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD 11)
endif (NOT DEFINED CMAKE_CXX_STANDARD)

include_directories(.)

set( REDIST /redist )
set( BINARIES_DIR ${REDIST}/binaries )
set( INPUT ${BINARIES_DIR}/linux10/input )
set( OPENSSL_DIR ${INPUT}/openssl )

set( OPENSSL_LIBRARY_DIR ${OPENSSL_DIR}/lib64 )

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${OPENSSL_LIBRARY_DIR} )

include_directories(BEFORE ${OPENSSL_DIR}/include64)

add_library(versig_lib
        src/CertificateTracker.cpp
        src/CertificateTracker.h
        src/crypto_utils.cpp
        src/crypto_utils.h
        src/digest_body.cpp
        src/digest_body.h
        src/digest_body_checker.cpp
        src/digest_buffer.cpp
        src/digest_buffer.h
        src/digest_buffer_checker.cpp
        src/digest_buffer_checker.h
        src/iostr_utils.h
        src/manifest_file.cpp
        src/manifest_file.h
        src/signed_file.cpp
        src/signed_file.h
        src/verify_exceptions.cpp
        src/verify_exceptions.h
        src/versig.cpp
        src/versig.h
        src/SophosCppStandard.h)

add_executable(versig src/versig_main.cpp src/versig.h)

target_link_libraries(versig PUBLIC versig_lib -L${OPENSSL_LIBRARY_DIR} ssl crypto )

add_subdirectory(tests/googletest)

add_executable(versig_test
        tests/test_versig.cpp
        src/versig.h)
target_link_libraries(versig_test PUBLIC versig_lib -L${OPENSSL_LIBRARY_DIR} ssl crypto gtest gtest_main)

target_include_directories(versig_test SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(versig_test BEFORE PUBLIC "${gtest_SOURCE_DIR}" src)

add_test(versig_test versig_test)
