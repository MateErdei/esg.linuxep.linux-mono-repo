/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <fstream>
#include <iostream>
#include "OutputToLog.h"

namespace Tests
{
    void OutputToLog::writeToUnitTestLog(std::string content) {
        std::cout << "Writing file"  << std::endl;
        std::cout << content  << std::endl;
        std::ofstream outfile;
        outfile.open("/tmp/unitTest.log", std::ios_base::app);
        outfile << content << std::endl;
        outfile.close();

    }
}