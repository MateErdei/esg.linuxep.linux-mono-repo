/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../../../modules/osqueryextensions/ResultsSender.h"

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <iostream>

void callback()
{
    // Do nothing.
}

int main()
{
    std::string mainDir = "/tmp/fuzz-result-sender";
    std::string intermediaryFile = mainDir + "/xdr-inter";
    std::string datafeedOutputDir = mainDir + "/datafeed";
    std::string pluginVarDir = mainDir + "/var";
    std::string queryPack = mainDir + "/querypack.conf";
    std::string mtrQueryPack = mainDir + "/querypack.mtr.conf";
    const uint DATA_LIMIT_BYTES = 1000000000; // 1GB, don't really care about this.
    const uint PERIOD_SECONDS = 1000000000;   // A long time, don't really care about this.

    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();

    std::cout << "Starting";
    ResultsSender resultsSender(
        intermediaryFile, datafeedOutputDir, queryPack,mtrQueryPack, pluginVarDir, DATA_LIMIT_BYTES, PERIOD_SECONDS, callback);

    std::string input;
    std::cin >> input;

    resultsSender.Add(input);
}