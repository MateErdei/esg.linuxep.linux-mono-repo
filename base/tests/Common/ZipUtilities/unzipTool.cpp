// Copyright 2023 Sophos Limited. All rights reserved.

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ZipUtilities/ZipUtils.h>

#include <iostream>

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    if (argc != 3)
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <zip file> <unpack destination>" << std::endl;
        return 1;
    }

    Common::ZipUtilities::ZipUtils zipUtils;
    zipUtils.unzip(argv[1], argv[2]);

    return 0;
}