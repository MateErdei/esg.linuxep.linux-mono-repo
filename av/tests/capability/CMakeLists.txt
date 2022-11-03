SophosAddTest(TestPassOnCapability
        TestPassOnCapability.cpp
        ${CMAKE_SOURCE_DIR}/products/capability/PassOnCapability.c
        INC_DIRS
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/products
        LIBS cap
        )

