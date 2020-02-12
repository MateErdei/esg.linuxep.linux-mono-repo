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

TEST(TestFileWalker, hugeFilePath) // NOLINT
{
    const sophos_filesystem::path& startingPath = fs::current_path();
    fs::create_directories("TestHugePathFileWalker");
    fs::current_path("TestHugePathFileWalker");

    for (int depth = 0; depth < 100; ++depth)
    {
        fs::create_directories("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
    }

    fs::current_path(startingPath);
    const sophos_filesystem::path& HaveiChangedDirectories = fs::current_path();

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
    filewalker::walk("TestHugePathFileWalker", callbacks);
    fs::current_path("TestHugePathFileWalker");
    for (int depth = 0; depth < 70; ++depth)
    {
        fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
    }
    fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
    fs::current_path(startingPath);

    fs::current_path("TestHugePathFileWalker");
    for (int depth = 0; depth < 35; ++depth)
    {
        fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
    }
    fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
    fs::current_path(startingPath);

    fs::remove_all("TestHugePathFileWalker");
}