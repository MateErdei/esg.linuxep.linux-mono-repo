add_executable(zmqchecker
        main.cpp
        Client.cpp
        Client.h
        Server.h
        Server.cpp
        ZmqCheckerMessageHandler.cpp
        ZmqCheckerMessageHandler.h)
add_dependencies(zmqchecker zmqwrapperapiimpl)
target_link_libraries(zmqchecker zmqwrapperapiimpl)


install(TARGETS zmqchecker
        DESTINATION  ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

