/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Common
{
    namespace DirectoryWatcher
    {
        class IDirectoryWatcherListener
        {
        public:
            virtual ~IDirectoryWatcherListener() = default;

            /**
             * @return Returns the absolute path used by Directory watcher to specify
             * the path that the listener is interested in.
             */
            virtual std::string getPath() const = 0;

            /**
             * Directory watcher informs the listener through this method
             * of the name of a file moved into the directory that the listener
             * is listening to. Receives just the filename.
             */
            virtual void fileMoved(const std::string& filename) = 0;

            /**
             * Directory watcher informs the listener through this method
             * if the directory is being actively watched.
             * True is sent on startWatch and on addition of a listener if
             * watcher is already started
             * False is sent on removal of listener from watcher
             */
            virtual void watcherActive(bool) = 0;
        };
    } // namespace DirectoryWatcher
} // namespace Common
