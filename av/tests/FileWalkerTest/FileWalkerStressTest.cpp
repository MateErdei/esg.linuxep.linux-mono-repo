/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/Logging/ConsoleLoggingSetup.h"

#include "filewalker/FileWalker.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

namespace fs = sophos_filesystem;

namespace
{
    using pathVector = std::vector<fs::path>;

    class CallbackImpl : public filewalker::IFileWalkCallbacks
    {
    private:
        pathVector m_walkedPaths;

    public:
        CallbackImpl() = default;
        void processFile(const fs::path& p, bool /*symlinkTarget*/) override
        {
            m_walkedPaths.emplace_back(p);
        }
        bool includeDirectory(const fs::path& p) override
        {
            std::cout << "DIR:" << p << '\n';
            return true;
        }
        bool userDefinedExclusionCheck(const fs::path& p, bool /*isSymlink*/) override
        {
            std::cout << "DIR:" << p << '\n';
            return false;
        }
        void registerError(const std::ostringstream &errorString) override
        {
            std::cout << errorString.str() << '\n';
        }
        void clear()
        {
            m_walkedPaths.clear();
        }
        pathVector walked()
        {
            return m_walkedPaths;
        }
    };

    pathVector createFilesInDir(const sophos_filesystem::path& dir)
    {
        namespace fs = sophos_filesystem;
        fs::create_directories(dir);
        pathVector result;

        for (int i=0; i<200; i++)
        {
            fs::path f = dir / std::to_string(i);
            result.emplace_back(f);
            std::ofstream output;
            output.open(f);
            output << i << '\n';
            output.close();
            if (!fs::is_regular_file(f))
            {
                PRINT("Failed to create: "<< f);
            }
        }

        return result;
    }

    pathVector create_static_test_files(const fs::path& starting_point)
    {
        namespace fs = sophos_filesystem;

        fs::path starting_point_path(starting_point);
        fs::path static_dir = starting_point_path / "static";

        return createFilesInDir(static_dir);

    }

    bool contains(const pathVector& actual, const fs::path& expected)
    {
#ifdef USE_BOOST_FILESYSTEM
        return std::find(actual.begin(), actual.end(), expected) != actual.end();
#else
        for (const auto& path : actual)
        {
            if (path.compare(expected) == 0)
            {
                return true;
            }
        }
        return false;
#endif
    }

    void addRemoveTempFiles(const fs::path& startingPoint)
    {
        using namespace std::chrono_literals;

        PRINT("Starting addRemoveTempFiles in " << startingPoint);
        namespace fs = sophos_filesystem;

        fs::path tempDir = fs::path(startingPoint) / "temp";

        for(int i=0; i<500; i++)
        {
            auto created = createFilesInDir(tempDir);
            PRINT(i << ": Created files in " << tempDir);
//            std::this_thread::sleep_for(1ms);
            fs::remove_all(tempDir);
            PRINT(i << ": Removed files from " << tempDir);
//            std::this_thread::sleep_for(1ms);

        }
        auto created = createFilesInDir(tempDir);
        PRINT("Finished addRemoveTempFiles");
    }
}

int main(int argc, char* argv[])
{
    using namespace std::chrono_literals;

    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
    CallbackImpl callbacks;

    if (argc < 2)
    {
        PRINT("Specify starting_point arg");
        return 2;
    }
    fs::path starting_point = argv[1];

    auto expectedPaths = create_static_test_files(starting_point);

    std::thread backgroundThread(addRemoveTempFiles, starting_point);
    backgroundThread.detach();


    for(int i=0; i<2000; i++)
    {
        PRINT(i << ": Starting file walk");
        filewalker::FileWalker walker(callbacks);
        walker.walk(starting_point);
        auto walked = callbacks.walked();
        PRINT(i << ": Scanned " << walked.size() << " files");

        // Check that expected files are present
        for (const auto& p : expectedPaths)
        {
            if (!contains(walked, p))
            {
                PRINT(p << " was not scanned");
                return 1;
            }
        }

//        std::this_thread::sleep_for(1ms);
        callbacks.clear();
    }
}