/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include <filewalker/FileWalker.h>
#include <datatypes/Print.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <string>
#include <fstream>
#include <cerrno>

#include <unistd.h>

namespace fs = sophos_filesystem;

static Common::Logging::ConsoleLoggingSetup consoleLoggingSetup; // NOLINT(cert-err58-cpp)

namespace
{
    class FakeCallbacks : public filewalker::IFileWalkCallbacks
    {
    public:
        FakeCallbacks() = default;

        void processFile(const sophos_filesystem::path& filepath, bool /*symlinkTarget*/) override
        {
            FAIL() << "Managed to get a file" << filepath;
        }

        bool includeDirectory(const sophos_filesystem::path& filepath) override
        {
            return !(filepath.filename() == "b");
        }
    };

    class CollectCallbacks : public filewalker::IFileWalkCallbacks
    {
    public:
        CollectCallbacks() = default;
        std::vector<fs::path> m_paths;
        void processFile(const sophos_filesystem::path& filepath, bool /*symlinkTarget*/) override
        {
            m_paths.emplace_back(filepath);
        }

        bool includeDirectory(const sophos_filesystem::path&) override
        {
            return true;
        }


    };
}

TEST(TestFileWalker, includeFiles) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    CollectCallbacks callbacks;
    filewalker::walk("sandbox", callbacks);
    fs::remove_all("sandbox");

    ASSERT_EQ(callbacks.m_paths.size(), 1);
    EXPECT_EQ(callbacks.m_paths.at(0), "sandbox/a/b/file1.txt");
}


TEST(TestFileWalker, absoluteIncludePath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingpoint = fs::absolute("sandbox");

    CollectCallbacks callbacks;
    filewalker::walk(startingpoint, callbacks);
    fs::remove_all("sandbox");

    ASSERT_EQ(callbacks.m_paths.size(), 1);
    std::string firstPath = callbacks.m_paths.at(0);
//    PRINT(firstPath);
    EXPECT_EQ(firstPath, startingpoint / "a/b/file1.txt");
}


TEST(TestFileWalker, excludeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    FakeCallbacks callbacks;
    filewalker::walk("sandbox", callbacks);
    fs::remove_all("sandbox");
}



TEST(TestFileWalker, scanFileThatDoesNotExist) // NOLINT
{
    FakeCallbacks callbacks;
    try
    {
        filewalker::walk("FileThatDoesNotExist", callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(e.what(),
                std::string("filesystem error: File/Folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST(TestFileWalker, hugeFilePathStartFromPathRoot) // NOLINT
{
    const fs::path& startingPath = fs::current_path();
    fs::create_directories("TestHugePathFileWalker");
    fs::current_path("TestHugePathFileWalker");

    for (int depth = 0; depth < 100; ++depth)
    {
        fs::create_directories("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
    }

    fs::current_path(startingPath);
    FakeCallbacks callbacks;
    filewalker::walk("TestHugePathFileWalker", callbacks);

    auto traverse_and_delete_huge_directory = [](const sophos_filesystem::path& startingPath, int targetDirectory)
    {
        fs::current_path("TestHugePathFileWalker");
        for (int depth = 0; depth < targetDirectory; ++depth)
        {
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
        fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        fs::current_path(startingPath);
    };

    traverse_and_delete_huge_directory(startingPath, 70);
    traverse_and_delete_huge_directory(startingPath, 35);
    fs::remove_all("TestHugePathFileWalker");
}

TEST(TestFileWalker, hugeStartingFilePath) // NOLINT
{
    const fs::path& startingPath = fs::current_path();
    fs::create_directories("TestHugePathFileWalker");
    fs::current_path("TestHugePathFileWalker");

    for (int depth = 0; depth < 100; ++depth)
    {
        fs::create_directories("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
    }

    const fs::path& pathToScan = fs::current_path();
    fs::current_path(startingPath);
    FakeCallbacks callbacks;

    try
    {
        filewalker::walk(pathToScan, callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(e.what(), std::string("filesystem error: Starting Path too long: File name too long"));
        EXPECT_EQ(e.code().value(), ENAMETOOLONG);
    }

    auto traverse_and_delete_huge_directory = [](const sophos_filesystem::path& startingPath, int targetDirectory)
    {
        fs::current_path("TestHugePathFileWalker");
        for (int depth = 0; depth < targetDirectory; ++depth)
        {
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
        fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        fs::current_path(startingPath);
    };

    traverse_and_delete_huge_directory(startingPath, 70);
    traverse_and_delete_huge_directory(startingPath, 35);
    fs::remove_all("TestHugePathFileWalker");
}