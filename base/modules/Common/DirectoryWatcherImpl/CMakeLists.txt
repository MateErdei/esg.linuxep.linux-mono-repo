
add_library(directorywatcherimpl SHARED
        DirectoryWatcherImpl.cpp
        DirectoryWatcherImpl.h)
target_include_directories(directorywatcherimpl PUBLIC ../DirectoryWatcher ../../Common)
target_link_libraries(directorywatcherimpl PUBLIC threads zeromqwrapperimpl)
add_dependencies(directorywatcherimpl threads zeromqwrapperimpl)
add_executable(directorywatcher
        DirectoryWatcherImpl.cpp DirectoryWatcherImpl.h)
target_link_libraries(directorywatcher  PUBLIC threads zeromqwrapperimpl)