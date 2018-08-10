cmake_minimum_required(VERSION 3.5)
project(thininstaller CXX)

set(CMAKE_CXX_STANDARD 11)

# This is called by the buildLinux.sh script which should have setup all dependencies in ./redist


if(NOT DEFINED REDIST)
    message("Defaulting redist to  REDIST /redist/binaries/linux11/input")
    set(REDIST /redist/binaries/linux11/input)
endif()

#SUL Library
set(SUL_INPUT ${REDIST}/SUL)
set(BOOST_INPUT ${REDIST}/boost)
set(EXPAT_INPUT ${REDIST}/expat)
set(OPENSSL_INPUT ${REDIST}/openssl)
set(ZLIB_INPUT ${REDIST}/zlib)
set(SUL_LIBRARY_DIR ${SUL_INPUT}/lib64)
set(SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64:${EXPAT_INPUT}/lib64:${OPENSSL_INPUT}/lib64:${ZLIB_INPUT}/lib64)
set(SUL_LIBRARY -L${SUL_LIBRARY_DIR} -lSUL)

#curl Library
set(CURL_INPUT ${REDIST}/curl)
set(CURL_LIBRARY_DIR ${CURL_INPUT}/lib64)
set(CURL_LIBRARY -L${CURL_LIBRARY_DIR}  -lcurl)

#Include dirs
include_directories(BEFORE ${SUL_INPUT}/include ${CURL_INPUT}/include64)

# Set flags
set(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,\$ORIGIN/../libs,-rpath-link,${SUL_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

add_executable(thininstaller installer.cpp)

target_link_libraries(thininstaller PUBLIC ${SUL_LIBRARY} ${CURL_LIBRARY})
