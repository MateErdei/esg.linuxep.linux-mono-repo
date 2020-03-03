/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

enum ExclusionType
{
    STEM,
    FULLPATH,
    GLOB,
    SUFFIX,
    INVALID
};

namespace avscanner::avscannerimpl
{
    class Exclusion
    {
    public:
        explicit Exclusion(const std::string& path);

        bool appliesToPath(const std::string& path) const;
        std::string path() const;
        ExclusionType type() const;

    private:
        std::string m_exclusionPath;
        ExclusionType m_type;
    };
}
