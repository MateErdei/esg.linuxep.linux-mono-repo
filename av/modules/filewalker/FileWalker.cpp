/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalker.h"

#include "datatypes/Print.h"

#include <utility>

namespace fs = sophos_filesystem;

using namespace filewalker;

namespace
{
    class FileWalker
    {
    public:
        /**
         * Construct a new file walker
         *
         * @param starting_point
         * @param callbacks BORROWED reference to callbacks
         */
        FileWalker(sophos_filesystem::path starting_point, IFileWalkCallbacks& callbacks)
                : m_starting_path(std::move(starting_point)), m_callback(callbacks)
        {}

        void run();
    private:
        const sophos_filesystem::path m_starting_path;
        IFileWalkCallbacks& m_callback;
    };
}

void FileWalker::run()
{
    for(const auto& p: fs::recursive_directory_iterator(m_starting_path))
    {
        if (fs::is_regular_file(p.status()))
        {
            m_callback.processFile(p);
        }
        else
        {
            PRINT("Not calling with " << p);
        }
    }
}

void filewalker::walk(sophos_filesystem::path starting_point, IFileWalkCallbacks& callbacks)
{
    FileWalker f(std::move(starting_point), callbacks);
    f.run();
}
