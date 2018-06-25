cmake_minimum_required(VERSION 3.5)
project(versig)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)

add_executable(versig
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
        versig.cpp)
