// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    using pua_exclusion_t = std::vector<std::string>;

    /**
     * Test PUA exclusions are good.
     *
     * @param exclusions
     * @return True if any PUA exclusion is bad, false if they are all good
     */
    bool badPuaExclusion(const pua_exclusion_t& exclusions);
}
