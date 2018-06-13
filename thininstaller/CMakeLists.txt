cmake_minimum_required(VERSION 3.10)
project(thininstaller)

set(CMAKE_CXX_STANDARD 11)

#/redist/binaries/linux10/input/
set(REDIST /redist)
set(BINARIES_DIR ${REDIST}/binaries)
set(INPUT ${BINARIES_DIR}/linux10/input)

set (LIBS ${PROJECT_SOURCE_DIR}/libs)

## Copy inputs
#SUL
file(COPY ${INPUT}/SUL DESTINATION ${LIBS})
#SUL dependencies
file(COPY ${INPUT}/boost DESTINATION ${LIBS})
file(COPY ${INPUT}/expat DESTINATION ${LIBS})
file(COPY ${INPUT}/openssl DESTINATION ${LIBS})
file(COPY ${INPUT}/zlib DESTINATION ${LIBS})
#CURL
file(COPY ${INPUT}/curl DESTINATION ${LIBS})
#glibc ?

#SUL Library
set(SUL_INPUT ${LIBS}/SUL)
set(BOOST_INPUT ${LIBS}/boost)
set(EXPAT_INPUT ${LIBS}/expat)
set(OPENSSL_INPUT ${LIBS}/openssl)
set(ZLIB_INPUT ${LIBS}/zlib)
set(SUL_LIBRARY_DIR ${SUL_INPUT}/lib64)
set(SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64:${EXPAT_INPUT}/lib64:${OPENSSL_INPUT}/lib64:${ZLIB_INPUT}/lib64)
set(SUL_LIBRARY -L${SUL_LIBRARY_DIR} -lSUL)

#curl Library
set(CURL_INPUT ./libs/curl)
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