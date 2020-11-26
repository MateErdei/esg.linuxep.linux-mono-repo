
include(GoogleTest)

SET (BOOST_LIBRARYDIR  ${BOOSTPATH}/boost/lib64)
SET (BOOST_INCLUDEDIR ${BOOSTPATH}/boost/include)
SET (BOOST_ROOT ${BOOSTPATH}/boost)

find_package( Boost REQUIRED COMPONENTS system locale)

SophosAddTest(TestCommon
        ../common/LogInitializedTests.cpp
        TestPathUtils.cpp
        TestStringUtils.cpp
        PROJECTS common
        INC_DIRS
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        LIBS ${Boost_LIBRARIES}
        )
SET_TARGET_PROPERTIES(TestCommon
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
