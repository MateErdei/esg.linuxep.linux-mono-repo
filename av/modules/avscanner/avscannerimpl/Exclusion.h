/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <regex>

enum ExclusionType
{
    STEM,
    FULLPATH,
    GLOB,
    FILENAME,
    RELATIVE_PATH,
    RELATIVE_STEM,
    RELATIVE_GLOB,
    INVALID
};

namespace avscanner::avscannerimpl
{
    class Exclusion
    {
    public:
        explicit Exclusion(const std::string& path);

        [[nodiscard]] bool appliesToPath(const std::string& path) const;
        [[nodiscard]] std::string path() const;
        [[nodiscard]] ExclusionType type() const;

    private:
        static std::regex convertGlobToRegex(const std::string& glob);
        static void escapeRegexMetaCharacters(std::string& text);

        std::string m_exclusionPath;
        std::regex m_pathRegex;
        ExclusionType m_type;
    };
}
