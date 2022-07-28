/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
//TO DO: move this somewhere common
#pragma once

#include "scan_messages/ServerThreatDetected.h"

#include <vector>
#include <string>


namespace pluginimpl
{
    std::string generateThreatDetectedXml(const scan_messages::ServerThreatDetected& detection);
    std::string generateThreatDetectedJson(const scan_messages::ServerThreatDetected& detection);
    std::string generateOnAccessConfig(const std::string&,
                                       const std::vector<std::string>& exclusionList,
                                       const std::string& excludeRemoteFiles);
    long getThreatStatus();
}