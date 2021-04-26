
include(GoogleTest)

SophosAddTest(TestPassOnCapability
        TestPassOnCapability.cpp
        ${CMAKE_SOURCE_DIR}/products/capability/PassOnCapability.c
        INC_DIRS
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/products
        LIBS cap
        )
SET_TARGET_PROPERTIES(TestPassOnCapability
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
