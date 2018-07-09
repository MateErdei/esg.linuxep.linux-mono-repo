add_library(directorywatcherimpl SHARED
        ../DirectoryWatcher/IDirectoryWatcher.h
        ../DirectoryWatcher/IDirectoryWatcherException.h
        DirectoryWatcherImpl.cpp
        DirectoryWatcherImpl.h
        Logger.h)
target_include_directories(directorywatcherimpl PUBLIC ../DirectoryWatcher ../../Common)
target_link_libraries(directorywatcherimpl PUBLIC threads zeromqwrapperimpl)
add_dependencies(directorywatcherimpl threads zeromqwrapperimpl)

install(TARGETS directorywatcherimpl
        LIBRARY DESTINATION files/base/lib64)
