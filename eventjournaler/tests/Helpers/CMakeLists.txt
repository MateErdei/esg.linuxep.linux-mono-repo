include(GoogleTest)

sophos_add_library(helpers SHARED
        FakePopper.cpp
        FakePopper.h
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib}
        )
