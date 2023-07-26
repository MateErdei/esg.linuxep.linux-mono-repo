// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include <SulDownloader/sdds3/SDDS3Repository.h>

namespace SulDownloader
{
    std::string createSUSRequestBody(const SUSRequestParameters& parameters);
    bool validateUrl(const std::string& url);
    bool isValidChar(char c);
    void parseSUSResponse(const std::string& response, std::set<std::string>& suites, std::set<std::string>& releaseGroups);
    void removeSDDS3Cache();
} // namespace SulDownloader
