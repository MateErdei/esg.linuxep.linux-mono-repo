/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

#include <string>
#include <regex>

namespace fs = sophos_filesystem;

/**
 * Types - ordered by cost
 */
enum ExclusionType
{
    FULLPATH,
    STEM,
    FILENAME,
    RELATIVE_PATH,
    RELATIVE_STEM,
    GLOB,
    RELATIVE_GLOB,
    INVALID
};

namespace common
{
    class Exclusion
    {
    public:
        explicit Exclusion(const std::string& path);

        [[nodiscard]] bool appliesToPath(const fs::path&, bool isDirectory, bool isFile) const;
        [[nodiscard]] bool appliesToPath(const fs::path&, bool isDirectory=false) const;
        [[nodiscard]] std::string path() const;
        [[nodiscard]] std::string displayPath() const;
        [[nodiscard]] ExclusionType type() const;

        bool operator==(const Exclusion& rhs) const;
        bool operator<(const Exclusion& rhs) const;

    private:
        static std::regex convertGlobToRegex(const std::string& glob);
        static void escapeRegexMetaCharacters(std::string& text);

        fs::path m_exclusionPath;
        std::string m_exclusionDisplayPath;
        std::regex m_pathRegex;
        ExclusionType m_type;
    };
}
