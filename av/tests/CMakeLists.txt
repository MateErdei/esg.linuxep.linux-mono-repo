if( NOT EXISTS ${CMAKE_SOURCE_DIR}/tests/googletest )
    message( FATAL_ERROR "googletest directory not present. \nPlease, run ${CMAKE_SOURCE_DIR}/build.sh which will setup the directory")
endif()
add_subdirectory(googletest EXCLUDE_FROM_ALL)
add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
add_subdirectory(avscanner)
add_subdirectory(commonTest)
add_subdirectory(datatypes)
add_subdirectory(FDTransfer)
add_subdirectory(FileWalkerTest)
add_subdirectory(installer)
add_subdirectory(manager)
add_subdirectory(plugin)
add_subdirectory(scan_messages)
add_subdirectory(threat_scanner)
add_subdirectory(unixsocket)
