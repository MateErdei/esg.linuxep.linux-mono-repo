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

namespace common
{
    class Exclusion
    {
    public:
        explicit Exclusion(const std::string& path);

        [[nodiscard]] bool appliesToPath(const std::string& path, bool isDirectory, bool isFile) const;
        [[nodiscard]] bool appliesToPath(const std::string& path, bool ignoreFilenameExclusion=false) const;
        [[nodiscard]] std::string path() const;
        [[nodiscard]] std::string displayPath() const;
        [[nodiscard]] ExclusionType type() const;

        bool operator==(const Exclusion& rhs) const;

    private:
        static std::regex convertGlobToRegex(const std::string& glob);
        static void escapeRegexMetaCharacters(std::string& text);

        std::string m_exclusionPath;
        std::string m_exclusionDisplayPath;
        std::regex m_pathRegex;
        ExclusionType m_type;
    };
}
