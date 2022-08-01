/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigurationUtils.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

static void initializeLogging()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

static int fuzzConfigProcessor(const uint8_t *Data, size_t Size)
{
    initializeLogging();
    std::string fuzzString(reinterpret_cast<const char*>(Data), Size);
    sophos_on_access_process::OnAccessConfig::parseOnAccessSettingsFromJson(fuzzString);

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    return fuzzConfigProcessor(Data, Size);
}