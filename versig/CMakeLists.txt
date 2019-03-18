cmake_minimum_required(VERSION 3.5)
project(versig)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif (NOT DEFINED CMAKE_CXX_STANDARD)

include_directories(.)

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/bin/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )
get_filename_component(libstdcpp_directory "${libstdcpp_path}" DIRECTORY )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/bin/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )
message( "${libgcc_s_path}" )

if (NOT DEFINED REDIST)
    set( REDIST /redist )
endif (NOT DEFINED REDIST)

set( BINARIES_DIR ${REDIST}/binaries )

if (NOT DEFINED INPUT)
    set( INPUT ${BINARIES_DIR}/linux10/input )
endif (NOT DEFINED INPUT)

set( OPENSSL_DIR ${INPUT}/openssl )

file( READ ${OPENSSL_DIR}/LIBVERSION OPENSSL_LIBVERSION)
string(STRIP ${OPENSSL_LIBVERSION} OPENSSL_LIBVERSION )

set( OPENSSL_LIBRARY_DIR ${OPENSSL_DIR}/lib64 )

set(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,\$ORIGIN/../lib64,-rpath-link,${OPENSSL_LIBRARY_DIR} )

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
        src/SophosCppStandard.h
        src/print.h
        src/libcrypto-compat.h
        src/libcrypto-compat.cpp)

add_executable(versig src/versig_main.cpp src/versig.h)

target_link_libraries(versig PUBLIC versig_lib -L${OPENSSL_LIBRARY_DIR} crypto )

install(TARGETS versig
        DESTINATION bin)

install(FILES
            ${OPENSSL_LIBRARY_DIR}/libcrypto.so.${OPENSSL_LIBVERSION}
            ${libstdcpp_path}
            ${libstdcpp_directory}/libstdc++.so.6
            ${libgcc_s_path}
        DESTINATION lib64)

include(GoogleTest)
enable_testing()

add_subdirectory(tests/googletest EXCLUDE_FROM_ALL)

add_executable(versig_test
        tests/test_versig.cpp
        src/versig.h)
target_link_libraries(versig_test PUBLIC versig_lib -L${OPENSSL_LIBRARY_DIR} ssl crypto gtest gtest_main)

target_include_directories(versig_test SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(versig_test BEFORE PUBLIC "${gtest_SOURCE_DIR}" src)

gtest_discover_tests(versig_test)
