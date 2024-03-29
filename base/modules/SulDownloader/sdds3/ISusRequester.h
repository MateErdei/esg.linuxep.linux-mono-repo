// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "SusRequestParameters.h"

#include <set>
#include <string>

namespace SulDownloader::SDDS3
{
    struct SusData
    {
        std::set<std::string> suites;
        std::set<std::string> releaseGroups;
        std::string reason;
        std::string error;
        std::string code;
    };

    struct SusResponse
    {
        SusData data;
        bool success = false;
        bool persistentError = false;
        std::string error;
    };

    class ISusRequester
    {
    public:
        virtual ~ISusRequester() = default;
        virtual SusResponse request(const SUSRequestParameters& parameters) = 0;
    };
} // namespace SulDownloader::SDDS3