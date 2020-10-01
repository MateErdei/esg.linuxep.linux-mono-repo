/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <fstream>
#include "OutputToLog.h"

namespace Tests
{
    void OutputToLog::writeToUnitTestLog(std::string content) {
        std::ofstream outfile;

        outfile.open("/tmp/unitTest.log", std::ios_base::app);
        outfile << content << std::endl;

    }
}