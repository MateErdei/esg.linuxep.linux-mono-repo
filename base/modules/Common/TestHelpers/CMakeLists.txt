add_library(testhelpersobject OBJECT
        TempDir.cpp
        TempDir.h
        TestExecutionSynchronizer.cpp
        TestExecutionSynchronizer.h)


add_library(testhelpers SHARED $<TARGET_OBJECTS:testhelpersobject> )

target_link_libraries(testhelpers PUBLIC filesystemimpl)

SET_TARGET_PROPERTIES( testhelpers
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS testhelpers
        LIBRARY DESTINATION files/base/lib64)
