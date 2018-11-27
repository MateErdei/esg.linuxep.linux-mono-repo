add_library(testhelpers SHARED
        TempDir.h
        TempDir.cpp
        TestExecutionSynchronizer.h
        TestExecutionSynchronizer.cpp)
target_link_libraries(testhelpers PUBLIC filesystemimpl)

