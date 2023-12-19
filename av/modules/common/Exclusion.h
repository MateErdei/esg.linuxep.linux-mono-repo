// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "PathUtils.h"

#include "datatypes/sophos_filesystem.h"

#define EXCLUSION_USE_RE2

#ifdef EXCLUSION_USE_RE2
#include "re2/re2.h"
#endif /* EXCLUSION_USE_RE2 */

#include <memory>
#include <string>
#ifndef EXCLUSION_USE_RE2
#include <regex>
#endif /* !EXCLUSION_USE_RE2 */

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
    SUFFIX,
    GLOB,
    RELATIVE_GLOB,
    MULTI_GLOB,
    INVALID
};

namespace common
{
    class ExclusionBase
    {
    public:
        explicit ExclusionBase(const std::string& path);
        [[nodiscard]] std::string path() const;
        [[nodiscard]] std::string displayPath() const;
        [[nodiscard]] ExclusionType type() const;
    protected:
        CachedPath m_exclusionPath;
        std::string m_exclusionDisplayPath;
        ExclusionType m_type;
    };

    class Exclusion : public ExclusionBase
    {
    public:
#ifdef EXCLUSION_USE_RE2
        using regex_t = std::shared_ptr<RE2>;
#else /* ! EXCLUSION_USE_RE2 */
        using regex_t = std::regex;
#endif /* EXCLUSION_USE_RE2 */
        explicit Exclusion(const std::string& path);

        /**
         * Create a replacement exclusion that combines many glob exclusions.
         * @param exclusions
         */
        explicit Exclusion(const std::vector<Exclusion>& exclusions);

        [[nodiscard]] bool appliesToPath(const fs::path&, bool isDirectory, bool isFile) const;
        [[nodiscard]] bool appliesToPath(const CachedPath&, bool isDirectory, bool isFile) const;
        [[nodiscard]] bool appliesToPath(const fs::path&, bool isDirectory=false) const;

        bool operator==(const Exclusion& rhs) const;
        bool operator<(const Exclusion& rhs) const;

        bool isGlobExclusion() const;

    protected:
        static std::string convertGlobToRegexString(const CachedPath& glob);

    private:

        regex_t convertGlobToRegex(const CachedPath& glob);

        regex_t m_pathRegex;
        std::string pathRegexString_;
    };

}
