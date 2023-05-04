// Copyright 2023 Sophos Limited. All rights reserved.

#include "PuaExclusions.h"

#include "Logger.h"

namespace avscanner::avscannerimpl
{
    bool badPuaExclusion(const pua_exclusion_t& puaExclusions)
    {
        for (const auto& exclusion : puaExclusions)
        {
            if (exclusion.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_/-+.,") != std::string::npos)
            {
                LOGERROR("PUA Exclusion " << exclusion << " is invalid.");
                return true;
            }
        }
        return false;

    }
}
