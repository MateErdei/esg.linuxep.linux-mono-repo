sophos_add_exe(eventpubsubtests
        eventpubsubtests.cpp
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
        )

target_link_libraries(eventpubsubtests PRIVATE
        eventqueuelib
        subscriberlib
        eventwriterlib
        ${LOG4CPLUS_LIBRARY}
        )

SET_TARGET_PROPERTIES( eventpubsubtests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

target_include_directories(eventpubsubtests PUBLIC ${CMAKE_SOURCE_DIR}/modules ${LOG4CPLUS_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
