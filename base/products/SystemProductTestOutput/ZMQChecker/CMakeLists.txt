add_executable(zmqchecker
        main.cpp
        Client.cpp
        Client.h
        Server.h
        Server.cpp
        ZmqCheckerMessageHandler.cpp
        ZmqCheckerMessageHandler.h
        $<TARGET_OBJECTS:zeromqwrapperimplobject>
        $<TARGET_OBJECTS:zmqwrapperapiimplobject>
        $<TARGET_OBJECTS:reactorimplobject>
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:threadsobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:utilityimplobject>
        )

target_link_libraries(zmqchecker ${ZeroMQ_LIBRARY} ${LOG4CPLUS_LIBRARY} pthread)
add_dependencies(zmqchecker zmqcheckerdir)

set(outputzmqcheckerpath "${CMAKE_BINARY_DIR}/SystemProductTestOutput/zmqchecker")
add_custom_target(
        zmqcheckerdir ALL
        COMMAND rm -rf ${outputzmqcheckerpath}
        COMMAND mkdir -p ${outputzmqcheckerpath}
        )

SET_TARGET_PROPERTIES( zmqchecker
        PROPERTIES INSTALL_RPATH "/opt/sophos-spl/base/lib64/" )

install(
        TARGETS zmqchecker
        DESTINATION  ${outputzmqcheckerpath} )

