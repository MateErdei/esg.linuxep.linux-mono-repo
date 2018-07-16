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
set(CURL_INPUT ${LIBS}/curl)
set(CURL_LIBRARY_DIR ${CURL_INPUT}/lib64)
set(CURL_LIBRARY -L${CURL_LIBRARY_DIR}  -lcurl)

#Include dirs
include_directories(BEFORE  ${SUL_INPUT}/include ${CURL_INPUT}/include64)

#set(CMAKE_INSTALL_RPATH "garbage/lib")

# Set flags
#set(LIBRARY_LOCATIONS \$ORIGIN/../libs/boost/lib64:\$ORIGIN/../libs/curl/lib64:\$ORIGIN/../libs/expat/lib64:\$ORIGIN/../libs/openssl/lib64:\$ORIGIN/../libs/SUL/lib64:\$ORIGIN/../libs/zlib/lib64)
#set(LIBRARY_LOCATIONS \$ORIGIN/../libs)
#todo currently when debugging the C++ part of the thininstaller we have libs in seperate folders but when the installer is built and archived we unpack it to bin/ and libs/ and the rpath used is ../libs/, should we change it to match or keep these seperate?
set(LIBRARY_LOCATIONS \$ORIGIN/../libs:\$ORIGIN/libs/boost/lib64:\$ORIGIN/libs/curl/lib64:\$ORIGIN/libs/expat/lib64:\$ORIGIN/libs/openssl/lib64:\$ORIGIN/libs/SUL/lib64:\$ORIGIN/libs/zlib/lib64)

set(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath=${LIBRARY_LOCATIONS},-rpath-link,${SUL_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

add_executable(thininstaller
        installer.cpp)

target_link_libraries(thininstaller PUBLIC ${SUL_LIBRARY} ${CURL_LIBRARY})
