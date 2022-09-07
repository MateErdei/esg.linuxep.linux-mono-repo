if (NOT DEFINED ENV{USE_LIBFUZZER})

add_subdirectory(fanotifyhandler)
add_subdirectory(onaccessimpl)

include(GoogleTest)

SophosAddTest(TestSophosOnAccessProcessImpl
        ../common/LogInitializedTests.cpp
        TestOnAccessConfigUtils.cpp
        PROJECTS soapdbootstrap
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/tests
        ${pluginapiinclude}
        ${testhelpersinclude}
        LIBS pthread ${pluginapilib} ${testhelperslib}
        )

SET_TARGET_PROPERTIES(TestSophosOnAccessProcessImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

endif (NOT DEFINED ENV{USE_LIBFUZZER})

add_executable(OnAccessConfigReaderFuzzer
        OnAccessConfigReaderFuzzer.cpp)

target_link_libraries(OnAccessConfigReaderFuzzer  PUBLIC
        onAccessConfigMonitor
        datatypes
        )

target_include_directories(OnAccessConfigReaderFuzzer PUBLIC ${CMAKE_SOURCE_DIR}/modules)

if (DEFINED ENV{USE_LIBFUZZER})
SET_TARGET_PROPERTIES( OnAccessConfigReaderFuzzer
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../../../base-sdds/files/base/lib64/"
        )
install(TARGETS OnAccessConfigReaderFuzzer
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )
endif (DEFINED ENV{USE_LIBFUZZER})