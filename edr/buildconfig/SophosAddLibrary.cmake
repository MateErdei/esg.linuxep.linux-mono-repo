function(sophos_add_library target  )
    set(multiValueArgs EXTRA_PROJECTS EXTRA_LIBS EXTRA_INCLUDES )
    set(librayKindOptions SHARED OBJECT)
    if (ARGC LESS 1)
        message(FATAL_ERROR "No arguments supplied to gtest_add_tests()")
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

    # decompose the inputs between the Target and the supplied files
    if( NOT Args_UNPARSED_ARGUMENTS )
            message(FATAL_ERROR "No target supplied to sophos_add_library()")
    endif()

    if( NOT Args_UNPARSED_ARGUMENTS )
        message(FATAL_ERROR "No files supplied to sophos_add_library()")
    endif()
    set( INPUTFILES ${Args_UNPARSED_ARGUMENTS})

    add_library(${target} SHARED
            ${INPUTFILES}
    )

    target_include_directories(${target} PUBLIC ${pluginapiinclude}  ${CMAKE_SOURCE_DIR} ${Args_EXTRA_INCLUDES})


    target_link_libraries(${target}  PUBLIC ${Args_EXTRA_PROJECTS} ${pluginapilib} ${Args_EXTRA_LIBS})

    add_dependencies(${target} copy_libs)

    SET_TARGET_PROPERTIES( ${target}
            PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_RPATH "$ORIGIN")

    install(TARGETS ${target}
            LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
endfunction()
