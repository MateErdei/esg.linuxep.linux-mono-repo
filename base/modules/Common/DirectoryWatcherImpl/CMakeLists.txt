add_library(directorywatcherimpl SHARED
        ../DirectoryWatcher/IDirectoryWatcher.h
        ../DirectoryWatcher/IDirectoryWatcherException.h
        DirectoryWatcherImpl.cpp
        DirectoryWatcherImpl.h
        Logger.h Logger.cpp)
target_include_directories(directorywatcherimpl PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_link_libraries(directorywatcherimpl PUBLIC threads zeromqwrapperimpl  ${LOG4CPLUS_LIBRARY})
add_dependencies(directorywatcherimpl threads zeromqwrapperimpl)

SET_TARGET_PROPERTIES( directorywatcherimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS directorywatcherimpl
        LIBRARY DESTINATION files/base/lib64)
