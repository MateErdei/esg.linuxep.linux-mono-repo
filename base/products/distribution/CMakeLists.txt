
configure_file(PRODUCT_NAME.in PRODUCT_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )
configure_file(DEFAULT_HOME_FOLDER.in DEFAULT_HOME_FOLDER @ONLY )
configure_file(FEATURE_LIST.in FEATURE_LIST @ONLY )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libatomic.so.1
        OUTPUT_VARIABLE libatomic_path)
string(STRIP "${libatomic_path}" libatomic_path )

set(third_party_libs ${libstdcpp_path} ${libgcc_s_path} ${libatomic_path} ${ZLIB_LIBRARY})

INCLUDE(FindLibraryFullPath)

function(add_third_party_library LIB LIBRARY_PATH)
    FindLibraryFullPath(FOUND_LIB_ABSOLUTE ${LIB} ${LIBRARY_PATH})
    set(third_party_libs ${third_party_libs} ${FOUND_LIB_ABSOLUTE} PARENT_SCOPE)

endfunction(add_third_party_library)

foreach (LIB boost_atomic boost_chrono boost_date_time boost_filesystem boost_regex boost_system boost_thread)
    add_third_party_library(${LIB} ${BOOST_INPUT}/lib64)
endforeach(LIB)

add_third_party_library(bzip2 ${BZ2_LIBRARY_DIR})
add_third_party_library(liblzma ${LZMA_LIBRARY_DIR})
add_third_party_library(minizip ${MINIZIP_INPUT})
add_third_party_library(curl ${CURL_INPUT}/lib64)
add_third_party_library(expat ${EXPAT_INPUT})
add_third_party_library(zlib  ${ZLIB_INPUT})
add_third_party_library(zmq ${ZEROMQ_INPUT}/lib)

foreach (LIB crypto ssl)
    add_third_party_library(${LIB} ${OPENSSL_INPUT}/lib64)
endforeach(LIB)

foreach (LIB SUL retailer_lib xmlcpp aesclib saucrypto)
    add_third_party_library(${LIB} ${SUL_LIBRARY_DIR})
endforeach(LIB)

add_third_party_library(protobuf ${Protobuf_root}/lib)
add_third_party_library(log4cplus ${LOG4CPLUS_INPUT}/lib)

install(FILES ${third_party_libs}
        DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(FILES ${VERSIG_INPUT}/bin64/versig
        DESTINATION ${BASE_DIST_PREFIX}/files/base/update)

# libasan and libubsan are only used during debug builds and needed for unit tests to run but we do not want to add
# them to the shipped third party library list, so create a separate list of build time only libs
execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libasan.so
        OUTPUT_VARIABLE libasan_path)
string(STRIP "${libasan_path}" libasan_path)
execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libubsan.so
        OUTPUT_VARIABLE libubsan_path)
string(STRIP "${libubsan_path}" libubsan_path )
set(build_time_only_libs ${libasan_path} ${libubsan_path})

# Put all needed libs into <build dir>/libs so that unit tests can link against them without setting LD_LIBARY_PATH.
add_custom_target(
        copy_libs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/libs
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/copy_libs.py ${CMAKE_BINARY_DIR}/libs ${third_party_libs} ${build_time_only_libs}
)

configure_file(VERSION.ini.in VERSION.ini @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION ${BASE_DIST_PREFIX}/files/base/)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION ${BASE_DIST_PREFIX})

install(FILES datafeed-config-scheduled_query.json
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc/)

if (DEFINED ENV{O_SIX_O})
    install(FILES ../../faker_lib.so.2.23.999
            DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64/)

    install(FILES ../../fake_lib.so.1.66.999
            DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64/)

    install(FILES ../../also_a_fake_lib.so.5.86.999
            DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64/)
endif()
