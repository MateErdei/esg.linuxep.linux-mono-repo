/******************************************************************************************************

Copyright 2019-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Plugin
{
    std::string osqueryBinaryName();
    std::string osqueryPath();
    std::string osqueryFlagsFilePath();
    std::string osqueryConfigFilePath();
    std::string osqueryConfigDirectoryPath();
    std::string osqueryXDRConfigFilePath();
    std::string osqueryMTRConfigFilePath();
    std::string osqueryXDRResultSenderIntermediaryFilePath();
    std::string osqueryXDROutputDatafeedFilePath();
    std::string osqueryCustomConfigFilePath();
    std::string osquerySocket();
    std::string lockFilePath();
    std::string osqueryPidFile();
    std::string syslogPipe();
    std::string osQueryLogDirectoryPath();
    std::string osQueryExtensionsPath();
    std::string osQueryExtensionsDirectory();
    std::string osQueryResultsLogPath();
    std::string osQueryDataBasePath();
    std::string edrConfigFilePath();
    std::string getVersionIniFilePath();
    std::string systemctlPath();
    std::string servicePath();
    std::string livequeryExecutable();
    std::string getJRLPath();
    std::string osQueryLensesPath();
    std::string livequeryResponsePath();
    std::string etcDir();
    std::string varDir();
    std::string mtrFlagsFile();
    std::string edrBinaryPath();
} // namespace Plugin
