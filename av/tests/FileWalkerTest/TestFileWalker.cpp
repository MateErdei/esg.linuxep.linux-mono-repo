/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <filewalker/FileWalker.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <string>
#include <fstream>

#include <unistd.h>

namespace fs = sophos_filesystem;

static Common::Logging::ConsoleLoggingSetup consoleLoggingSetup; // NOLINT(cert-err58-cpp)

TEST(TestFileWalker, excludeDirectory) // NOLINT
{
    fs::create_directories("TestFileWalker");
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    class Callbacks : public filewalker::IFileWalkCallbacks
    {
    public:
        Callbacks() = default;
        void processFile(const sophos_filesystem::path& filepath) override
        {
            FAIL() << "Managed to get a file" << filepath;
        }
        bool includeDirectory(const sophos_filesystem::path& filepath) override
        {
            return !(filepath.filename() == "b");
        }
    };

    Callbacks callbacks;
    filewalker::walk("sandbox", callbacks);
    fs::remove_all("TestFileWalker");
}