/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
//TO DO: move this somewhere common
#pragma once

#include "scan_messages/ServerThreatDetected.h"

#include <string>


namespace pluginimpl
{
    std::string generateThreatDetectedXml(const scan_messages::ServerThreatDetected& detection);
    std::string generateThreatDetectedJson(const std::string& threatName, const std::string& threatPath);
}