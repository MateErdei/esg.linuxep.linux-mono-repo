# usage: sophos_add_library( <NameOfTheTargetToGenerate> SHARED
#        <ListOfSourceCode>
#        EXTRA_PROJECTS <nameOfLibrariesGeneratedByThisProject>
#        EXTRA_LIBS <PathOrNameOfThirdPartyLibrariesToLinkAgainst>
#        EXTRA_INCLUDES <PathOfThirdPartyIncludes>
#       )
#  NameOfTheTargetToGenerate: required. Example: MyLibrary
#  ListOfSourceCode: required. Example: source1.cpp source1.h
#  nameOfLibrariesGeneratedByThisProject: optional. Example: common filesystemimpl
#  PathOrNameOfThirdPartyLibrariesToLinkAgainst: optional. Example: ${pluginapilib} ${log4cpluslib}
#  PathOfThirdPartyIncludes: optional. Example: ${pluginapiinclude}
#

function(sophos_add_library TARGET  )
    set(multiValueArgs EXTRA_PROJECTS EXTRA_LIBS EXTRA_INCLUDES )
    set(librayKindOptions SHARED OBJECT)
    if (ARGC LESS 1)
        message(FATAL_ERROR "No arguments supplied to sophos_add_library()")
    endif()

    cmake_parse_arguments(Args "${librayKindOptions}" ""
            "${multiValueArgs}" ${ARGN} )

    set( OptionArg )
    if ( Args_SHARED )
        set( OptionArg "SHARED")
    elseif( Args_OBJECT)
        set( OptionArg "OBJECT")
    endif()

    if ( NOT OptionArg )
        message(FATAL_ERROR "Invalid library option.")
    endif()

    if( NOT Args_UNPARSED_ARGUMENTS )
            message(FATAL_ERROR "No target supplied to sophos_add_library()")
    endif()

    set( INPUTFILES ${Args_UNPARSED_ARGUMENTS})

    add_library(${TARGET} SHARED
            ${INPUTFILES}
    )

    target_include_directories(${TARGET} PUBLIC ${pluginapiinclude}  ${CMAKE_SOURCE_DIR} ${Args_EXTRA_INCLUDES})


    target_link_libraries(${TARGET}  PUBLIC ${Args_EXTRA_PROJECTS} ${pluginapilib} ${Args_EXTRA_LIBS})

    add_dependencies(${TARGET} copy_libs)

    SET_TARGET_PROPERTIES( ${TARGET}
            PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_RPATH "$ORIGIN")

    install(TARGETS ${TARGET}
            LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
endfunction()
