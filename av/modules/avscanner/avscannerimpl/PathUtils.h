/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace avscanner::avscannerimpl
{
    class PathUtils
    {
    public:
        /**
         * Return true if a is longer than b.
         * @param a
         * @param b
         * @return
         */
        static bool longer(const fs::path& a, const fs::path& b)
        {
            return a.string().size() > b.string().size();
        }

        static bool startswith(const fs::path& p, const fs::path& value)
        {
            return p.string().rfind(value.string(), 0) == 0;
        }

        static bool startswithany(const std::vector<std::string>& paths, const fs::path& p)
        {
            for (auto & exclusion : paths)
            {
                if (PathUtils::startswith(p, exclusion))
                {
                    return true;
                }
            }
            return false;
        }
    };
}



