/***********************************************************************************************

Copyright 2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "SDDS3Repository.h"

namespace SulDownloader
{
    std::string createSUSRequestBody(const SUSRequestParameters& parameters);
    void parseSUSResponse(const std::string& response, std::set<std::string>& suites, std::set<std::string>& releaseGroups);
    void removeSDDS3Cache();
} // namespace SulDownloader
