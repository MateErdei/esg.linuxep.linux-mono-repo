cmake_minimum_required(VERSION 3.10)
project(thininstaller)

set(CMAKE_CXX_STANDARD 11)

#/redist/binaries/linux10/input/
set(REDIST /redist)
set(BINARIES_DIR ${REDIST}/binaries)
set(INPUT ${BINARIES_DIR}/linux10/input)

#SUL Library
set(SUL_INPUT ${INPUT}/SUL)
set(BOOST_INPUT ${INPUT}/boost)
set(SUL_LIBRARY_DIR ${SUL_INPUT}/lib64)
set(SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64)
set( SUL_LIBRARY -L${SUL_LIBRARY_DIR}  -lSUL)

#curl Library
set(CURL_INPUT ${INPUT}/curl)
set(CURL_LIBRARY_DIR ${CURL_INPUT}/lib64)
set(CURL_LIBRARY -L${CURL_LIBRARY_DIR}  -lcurl)

#Include dirs
include_directories(BEFORE  ${SUL_INPUT}/include ${CURL_INPUT}/include64)

# Set flags
set(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

add_executable(thininstaller
        installer.cpp)

target_link_libraries(thininstaller PUBLIC ${SUL_LIBRARY} ${CURL_LIBRARY})