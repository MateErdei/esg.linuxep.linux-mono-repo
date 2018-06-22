add_library(directorywatcherimpl SHARED
        ../DirectoryWatcher/IDirectoryWatcher.h
        DirectoryWatcherImpl.cpp
        DirectoryWatcherImpl.h)
target_include_directories(directorywatcherimpl PUBLIC ../DirectoryWatcher ../../Common)
target_link_libraries(directorywatcherimpl PUBLIC threads zeromqwrapperimpl)
add_dependencies(directorywatcherimpl threads zeromqwrapperimpl)