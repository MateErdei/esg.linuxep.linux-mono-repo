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

macro(SophosAddTest )
    set(multiValueArgs PROJECTS LIBS INC_DIRS )

    if (ARGC LESS 1)
        message(FATAL_ERROR "No arguments supplied to gtest_add_tests()")
    endif()

    cmake_parse_arguments(AddTest "" ""
            "${multiValueArgs}" ${ARGN} )

    # decompose the inputs between the Target and the supplied files
    if( NOT AddTest_UNPARSED_ARGUMENTS )
            message(FATAL_ERROR "No target supplied to SophosAddTest()")
    endif()

    list(GET AddTest_UNPARSED_ARGUMENTS 0 TARGET)
    list(REMOVE_AT AddTest_UNPARSED_ARGUMENTS 0)

    if( NOT AddTest_UNPARSED_ARGUMENTS )
        message(FATAL_ERROR "No files supplied to SophosAddTest()")
    endif()
    set( INPUTFILES ${AddTest_UNPARSED_ARGUMENTS})

    # setup the google test target
    add_executable(${TARGET} ${INPUTFILES})

    target_include_directories(${TARGET} SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" "${gtest_SOURCE_DIR}")
    target_link_libraries(${TARGET}  PUBLIC gtest gtest_main gmock)

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
