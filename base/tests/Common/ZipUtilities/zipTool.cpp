// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/ZipUtilities/ZipUtils.h"

#include <iostream>


#define PRINT(x) std::cerr << x << '\n'

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    if (argc == 3)
    {
        PRINT("Creating zip file " << argv[2] << " without password");
        return Common::ZipUtilities::zipUtils().zip(argv[1], argv[2], true);
    }
    else if (argc == 4)
    {
        PRINT("Creating zip file " << argv[2] << " with password");
        return Common::ZipUtilities::zipUtils().zip(argv[1], argv[2], true, true, argv[3]);
    }
    else
    {
        PRINT("Invalid argument. Usage: " << argv[0] << " <target directory> <zip file>");
        return EINVAL;
    }
    return 0;
}
