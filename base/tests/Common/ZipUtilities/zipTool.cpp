// Copyright 2023 Sophos Limited. All rights reserved.

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ZipUtilities/ZipUtils.h>

#include <iostream>

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    Common::ZipUtilities::ZipUtils zipUtils;
    if (argc == 3)
    {
        zipUtils.zip(argv[1], argv[2]);
    }
    else if (argc == 4)
    {
        zipUtils.zip(argv[1], argv[2], argv[3]);
    }
    else
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <target directory> <zip file>" << std::endl;
        return 1;
    }
    return 0;
}