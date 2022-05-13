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

macro(SophosAddTest TARGET)
    set(multiValueArgs PROJECTS LIBS INC_DIRS )

    if (ARGC LESS 1)
        message(FATAL_ERROR "No arguments supplied to gtest_add_tests()")
    endif()

    cmake_parse_arguments(AddTest "" ""
            "${multiValueArgs}" ${ARGN} )

    if( NOT AddTest_UNPARSED_ARGUMENTS )
        message(FATAL_ERROR "No files supplied to SophosAddTest()")
    endif()
    set( INPUTFILES ${AddTest_UNPARSED_ARGUMENTS})

    # setup the google test target
    add_executable(${TARGET} ${INPUTFILES})

    target_include_directories(${TARGET} SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE})
    target_link_libraries(${TARGET}  PUBLIC ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY})

    if ( AddTest_PROJECTS )
        target_link_libraries(${TARGET} PUBLIC ${AddTest_PROJECTS})
    endif()

    if ( AddTest_INC_DIRS )
        target_include_directories(${TARGET} PUBLIC ${AddTest_INC_DIRS})
    endif()

    if ( AddTest_LIBS )
        target_link_libraries(${TARGET} PUBLIC ${AddTest_LIBS})
    endif()
    gtest_discover_tests(${TARGET} )

endmacro()
