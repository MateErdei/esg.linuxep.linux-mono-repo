/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
//TO DO: move this somewhere common
#pragma once

#include <string>
#include <ThreatDetected.capnp.h>

namespace unixsocket
{
    void escapeControlCharacters(std::string& text);

    std::string generateThreatDetectedXml(const Sophos::ssplav::ThreatDetected::Reader& detection);
}