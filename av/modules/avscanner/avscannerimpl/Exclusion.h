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
        std::string convertGlobToRegex(const std::string& glob);
        void escapeRegexMetaCharacters(std::string& text);

        std::string m_exclusionPath;
        ExclusionType m_type;
    };
}
