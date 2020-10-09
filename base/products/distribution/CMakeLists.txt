

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

set(third_party_libs ${libstdcpp_path} ${libgcc_s_path})

set(DISTRIBUTION_TOOL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(STRIP_EXECUTABLES ${DISTRIBUTION_TOOL_DIR}/stripExecutables.py)
set(ADD_SYMBOLS_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/addSymbols.sh)
set(SYMBOLS_DIR ${CMAKE_BINARY_DIR}/symbols)
set(GENERATE_SDDS_IMPORT_MANIFEST_DAT ${DISTRIBUTION_TOOL_DIR}/generateSDDSImportAndManifestDat.py )

add_custom_command(OUTPUT ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python3 ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR} ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
        )

add_custom_target(
        dist
        COMMAND python3 ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR} ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
)


add_custom_target(
        forcedist
        COMMAND rm -f ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python3 ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR} ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
)

INCLUDE(FindLibraryFullPath)

function(add_third_party_library LIB LIBRARY_PATH)
    FindLibraryFullPath(FOUND_LIB_ABSOLUTE ${LIB} ${LIBRARY_PATH})
    set(third_party_libs ${third_party_libs} ${FOUND_LIB_ABSOLUTE} PARENT_SCOPE)

endfunction(add_third_party_library)

foreach (LIB boost_atomic boost_chrono boost_date_time boost_filesystem boost_regex boost_system boost_thread)
    add_third_party_library(${LIB} ${BOOST_INPUT}/lib64)
endforeach(LIB)

add_third_party_library(curl ${CURL_INPUT}/lib64)

add_third_party_library(expat ${EXPAT_INPUT}/lib64)

add_third_party_library(z  ${ZLIB_INPUT}/lib64)

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
        DESTINATION files/base/lib64)

install(FILES ${VERSIG_INPUT}/bin64/versig
        DESTINATION files/base/update)

add_custom_target(
        copy_libs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/libs
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/copy_libs.py ${CMAKE_BINARY_DIR}/libs ${third_party_libs}
)

execute_process(
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/getReleasePackageVersion.py ${CMAKE_SOURCE_DIR}/build/release-package.xml
        OUTPUT_VARIABLE RELEASE_PACKAGE_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/computeAutoVersion.py ${CMAKE_SOURCE_DIR} ${RELEASE_PACKAGE_VERSION} CIJenkinsfile
        OUTPUT_VARIABLE PRODUCT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND date +%Y-%m-%d
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(VERSION.ini.in VERSION.ini @ONLY)

install(FILES ../../build64/products/distribution/VERSION.ini
        DESTINATION files/base/)

install(FILES datafeed-config-scheduled_query.json
        DESTINATION files/base/etc/)
