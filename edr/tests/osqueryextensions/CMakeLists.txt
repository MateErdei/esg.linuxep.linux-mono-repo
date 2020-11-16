SophosAddTest(TestOsqueryExtension
        TestResultSender.cpp
        TestSophosServerTable.cpp
        MockQueryContext.h
        PROJECTS OsqueryExtensions pluginimpl
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} gmock gtest pluginimpl
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestOsqueryExtension copy_libs)

SET_TARGET_PROPERTIES(TestOsqueryExtension
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

execute_process(COMMAND mkdir -p ${PROJECT_BINARY_DIR}/samples/crashes/)
execute_process(COMMAND mkdir -p ${PROJECT_BINARY_DIR}/samples/hangs/)

configure_file(samples/crashes/id:000000,sig:06,src:000000,op:flip1,pos:0 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/crashes/id:000001,sig:06,src:000000,op:flip1,pos:1 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/crashes/id:000002,sig:06,src:000000,op:int8,pos:0,val:+32 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/crashes/id:000003,sig:06,src:000000,op:havoc,rep:32 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/crashes/id:000004,sig:06,src:000000,op:havoc,rep:16 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/crashes/id:000005,sig:06,src:000000,op:havoc,rep:128 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/crashes/id:000006,sig:06,src:000000,op:havoc,rep:8 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/crashes/id:000007,sig:06,src:000000,op:havoc,rep:16 ${PROJECT_BINARY_DIR}/samples/crashes/ COPYONLY)
configure_file(samples/hangs/id:000000,src:000020,op:havoc,rep:8 ${PROJECT_BINARY_DIR}/samples/hangs/ COPYONLY)
