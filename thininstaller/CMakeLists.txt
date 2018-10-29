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
set(VERSIG_INPUT ${REDIST}/versig)
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
set(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,\$ORIGIN/../lib64,-rpath-link,${SUL_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

add_executable(thininstaller installer.cpp)

target_link_libraries(thininstaller PUBLIC ${SUL_LIBRARY} ${CURL_LIBRARY})

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build-scripts/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

get_filename_component(libstcpp_name "${libstdcpp_path}" NAME)

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build-scripts/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )


if (NOT DEFINED OUTPUT)
    set(OUTPUT  ${CMAKE_BINARY_DIR}/output)
endif()

if (NOT DEFINED INSTALLERDIR)
    set(INSTALLERDIR ${CMAKE_BINARY_DIR}/installer)
endif()

set(bin_dir ${INSTALLERDIR}/bin)
set(libs_dir ${INSTALLERDIR}/lib64)

add_custom_target(
        copyInstaller
        COMMAND mkdir -p "${bin_dir}"
        COMMAND cp thininstaller ${bin_dir}/installer
        COMMAND cp -a ${VERSIG_INPUT}/bin64/versig ${bin_dir}/
        COMMAND strip ${bin_dir}/*
        COMMAND chmod 700 ${bin_dir}/*
        COMMAND mkdir -p "${libs_dir}"
        COMMAND cp -a ${SUL_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${CURL_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${BOOST_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${OPENSSL_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${EXPAT_INPUT}//lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${ZLIB_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${libstdcpp_path} ${libs_dir}/
        COMMAND cp -a ${libgcc_s_path} ${libs_dir}/
        COMMAND strip ${libs_dir}/*
        COMMAND chmod 700 ${libs_dir}/*
        COMMAND ln -snf ${libstcpp_name} ${libs_dir}/libstdc++.so.6
        COMMAND cp ${CMAKE_SOURCE_DIR}/rootca.crt ${INSTALLERDIR}/
        COMMAND cp ${CMAKE_SOURCE_DIR}/ps_rootca.crt ${INSTALLERDIR}/
        COMMAND cp ${CMAKE_SOURCE_DIR}/sdds_https_rootca.crt ${INSTALLERDIR}/
)

