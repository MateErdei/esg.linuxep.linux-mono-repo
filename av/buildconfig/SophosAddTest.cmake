# usage: SophosAddTest( <NameOfTheTargetToGenerate>
#        <ListOfSourceCode>
#        PROJECTS <nameOfLibrariesGeneratedByThisProject>
#        LIBS <PathOrNameOfThirdPartyLibrariesToLinkAgainst>
#        INC_DIRS <PathOfThirdPartyIncludes>
#       )
#  NameOfTheTargetToGenerate: required. Example: MyGoogleTest
#  ListOfSourceCode: required. Example: test1.cpp test2.cpp mymock.h
#  nameOfLibrariesGeneratedByThisProject: optional. Example: common filesystemimpl
#  PathOrNameOfThirdPartyLibrariesToLinkAgainst: optional. Example: ${pluginapilib} ${log4cpluslib}
#  PathOfThirdPartyIncludes: optional. Example: ${pluginapiinclude}
#

include(GoogleTest)

macro(SophosAddTest TARGET)
    set(multiValueArgs PROJECTS LIBS INC_DIRS)

    if (ARGC LESS 1)
        message(FATAL_ERROR "No arguments supplied to gtest_add_tests()")
    endif ()

    cmake_parse_arguments(AddTest "" ""
            "${multiValueArgs}" ${ARGN})

    if (NOT AddTest_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "No files supplied to SophosAddTest()")
    endif ()
    set(INPUT_FILES ${AddTest_UNPARSED_ARGUMENTS})

    # setup the google test target
    add_executable(${TARGET} ${INPUT_FILES})

    target_include_directories(${TARGET}
            PUBLIC
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/modules
            ${CMAKE_SOURCE_DIR}/tests
            )

    target_include_directories(${TARGET}
            SYSTEM BEFORE PUBLIC
            ${GTEST_INCLUDE}
            ${GMOCK_INCLUDE}
            )

    target_link_libraries(${TARGET}
            PUBLIC
            ${GTEST_LIBRARY}
            ${GTEST_MAIN_LIBRARY}
            ${GMOCK_LIBRARY}
            pthread
            )

    if (AddTest_PROJECTS)
        target_link_libraries(${TARGET} PUBLIC ${AddTest_PROJECTS})
    endif ()

    if (AddTest_INC_DIRS)
        target_include_directories(${TARGET} PUBLIC ${AddTest_INC_DIRS})
    endif ()

    if (AddTest_LIBS)
        target_link_libraries(${TARGET} PUBLIC ${AddTest_LIBS})
    endif ()

    gtest_discover_tests(${TARGET})

    add_dependencies(${TARGET} copy_libs)

    SET_TARGET_PROPERTIES(${TARGET}
            PROPERTIES
            BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
            #INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
            )
endmacro()
