// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/ZipUtilities/ZipUtils.h"

#include <iostream>

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    if (argc == 3)
    {
        return Common::ZipUtilities::ZipUtils::unzip(argv[1], argv[2]);
    }
    else if (argc == 4)
    {
        return Common::ZipUtilities::ZipUtils::unzip(argv[1], argv[2], true, argv[3]);
    }
    else
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <zip file> <unpack destination>" << std::endl;
        return EINVAL;
    }
}