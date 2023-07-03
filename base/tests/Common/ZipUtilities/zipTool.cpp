// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/ZipUtilities/ZipUtils.h"

#include <iostream>

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    if (argc == 3)
    {
        return Common::ZipUtilities::zipUtils().zip(argv[1], argv[2], true);
    }
    else if (argc == 4)
    {
        return Common::ZipUtilities::zipUtils().zip(argv[1], argv[2], true, true, argv[3]);
    }
    else
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <target directory> <zip file>" << std::endl;
        return EINVAL;
    }
    return 0;
}