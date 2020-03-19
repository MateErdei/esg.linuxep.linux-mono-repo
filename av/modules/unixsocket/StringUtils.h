/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
//TO DO: move this somewhere common
#pragma once

#include "scan_messages/ServerThreatDetected.h"

#include <string>


namespace unixsocket
{
    void escapeControlCharacters(std::string& text);

    std::string generateThreatDetectedXml(const scan_messages::ServerThreatDetected& detection);

    std::string toUtf8(const std::string& str);
}