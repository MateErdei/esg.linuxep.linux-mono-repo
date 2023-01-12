// Copyright 2023 Sophos Limited. All rights reserved.

#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/PolicyProcessor.h"
#include "pluginimpl/TaskQueue.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#include <fstream>
#include <string>

#define ERROR(x) std::cerr << x << '\n'
#define PRINT(x) std::cout << x << '\n'

#ifdef USING_LIBFUZZER

static int fuzzCorePolicy(const uint8_t *Data, size_t Size)
{
    std::string fuzzString(reinterpret_cast<const char*>(Data), Size);
    auto parsedFuzzString = Common::XmlUtilities::parseXml(fuzzString);
    auto taskQueue = std::make_shared<Plugin::TaskQueue>();
    Plugin::PolicyProcessor policyProcessor(taskQueue);
    policyProcessor.processCOREpolicy(parsedFuzzString);

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    return fuzzCorePolicy(Data, Size);
}
#else

static void initializeLogging()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

int main(int argc, char* argv[])
{
    initializeLogging();
    PRINT("arguments: " << argc);
    if( argc < 2 )
    {
        ERROR("missing arg");
        return 1;
    }

    if (sophos_filesystem::is_regular_file(argv[1]))
    {
        std::string fuzzString = Common::FileSystem::fileSystem()->readFile(argv[1]);
        auto parsedFuzzString = Common::XmlUtilities::parseXml(fuzzString);
        auto taskQueue = std::make_shared<Plugin::TaskQueue>();
        Plugin::PolicyProcessor policyProcessor(taskQueue);
        policyProcessor.processCOREpolicy(parsedFuzzString);
    }
    else
    {
        ERROR("Error: not a file ");
        return 1;
    }

    return 0;
}

#endif