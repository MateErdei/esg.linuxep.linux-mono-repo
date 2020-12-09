
add_library(directorywatcherimplobject OBJECT
        ../DirectoryWatcher/IDirectoryWatcher.h
        ../DirectoryWatcher/IDirectoryWatcherListener.h
        ../DirectoryWatcher/IDirectoryWatcherException.h
        ../DirectoryWatcher/IiNotifyWrapper.h
        DirectoryWatcherImpl.cpp
        DirectoryWatcherImpl.h
        Logger.h Logger.cpp)
target_include_directories(directorywatcherimplobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
add_library(directorywatcherimpl SHARED $<TARGET_OBJECTS:directorywatcherimplobject>)

target_link_libraries(directorywatcherimpl PUBLIC threads zeromqwrapperimpl ${LOG4CPLUS_LIBRARY})
add_dependencies(directorywatcherimpl threads zeromqwrapperimpl)

SET_TARGET_PROPERTIES(directorywatcherimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS directorywatcherimpl
        LIBRARY DESTINATION files/base/lib64)
