// Copyright 2023 Sophos Limited. All rights reserved.

#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/PolicyProcessor.h"
#include "pluginimpl/TaskQueue.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/XmlUtilities/AttributesMap.h"

#include <fstream>
#include <string>

#define ERROR(x) std::cerr << x << '\n'
#define PRINT(x) std::cout << x << '\n'

static void setPLUGIN_INSTALL()
{
    // Set PLUGIN_INSTALL so that the code doesn't fail immediately
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/CorcPolicyProcessorFuzzer");
}

static void testFuzzString(const std::string& fuzzString)
{
    setPLUGIN_INSTALL();
    auto parsedFuzzString = Common::XmlUtilities::parseXml(fuzzString);
    auto taskQueue = std::make_shared<Plugin::TaskQueue>();
    Plugin::PolicyProcessor policyProcessor(taskQueue);
    policyProcessor.processCorcPolicy(parsedFuzzString);
}

#ifdef USING_LIBFUZZER

static int fuzzCorcPolicy(const uint8_t *Data, size_t Size)
{

    std::string fuzzString(reinterpret_cast<const char*>(Data), Size);
    try
    {
        testFuzzString(fuzzString);
    }
    catch (Common::XmlUtilities::XmlUtilitiesException& e)
    {
        // Do nothing as parsing xml exception handled
    }

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    return fuzzCorcPolicy(Data, Size);
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
        return 2;
    }

    if (sophos_filesystem::is_regular_file(argv[1]))
    {
        auto fuzzString = Common::FileSystem::fileSystem()->readFile(argv[1]);
        testFuzzString(fuzzString);
    }
    else
    {
        ERROR("Error: not a file " << argv[1]);
        return 1;
    }

    return 0;
}

#endif