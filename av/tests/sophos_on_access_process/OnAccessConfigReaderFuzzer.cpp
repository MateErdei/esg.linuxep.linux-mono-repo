/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigurationUtils.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/FileSystem/IFileSystem.h"

#include <cstring>
#include <fstream>
#include <string>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERROR(x) std::cerr << x << '\n'
#define PRINT(x) std::cout << x << '\n'



#ifdef USING_LIBFUZZER

static int fuzzConfigProcessor(const uint8_t *Data, size_t Size)
{
    std::string fuzzString(reinterpret_cast<const char*>(Data), Size);
    sophos_on_access_process::OnAccessConfig::parseOnAccessSettingsFromJson(fuzzString);

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    return fuzzConfigProcessor(Data, Size);
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
        sophos_on_access_process::OnAccessConfig::parseOnAccessSettingsFromJson(fuzzString);
    }
    else
    {
        ERROR("Error: not a file ");
        return 1;
    }

    return 0;
}

#endif