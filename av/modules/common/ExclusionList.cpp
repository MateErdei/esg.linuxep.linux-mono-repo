// Copyright 2023 Sophos Limited. All rights reserved.
#include "ExclusionList.h"

#include "Logger.h"

#include <algorithm>

namespace
{
    common::ExclusionList::list_type convertExclusions(const std::vector<std::string>& exclusions)
    {
        common::ExclusionList::list_type results;
        for (const auto& exclusion : exclusions)
        {
            results.emplace_back(exclusion);
        }
        return results;
    }
}

common::ExclusionList::ExclusionList(const std::vector<std::string>& exclusions)
    : ExclusionList(convertExclusions(exclusions), 0)
{
}

common::ExclusionList::ExclusionList(list_type exclusions, int)
{
    // Convert globs to multi-glob
    exclusions_.reserve(exclusions.size());
    list_type globExclusions;
    for (const auto& excl : exclusions)
    {
        if (excl.isGlobExclusion())
        {
            globExclusions.emplace_back(std::move(excl));
            if (globExclusions.size() >= 200)
            {
                exclusions_.emplace_back(std::move(globExclusions)); // Create combined exclusion
                globExclusions.clear();
            }
        }
        else
        {
            exclusions_.emplace_back(std::move(excl));
        }
    }
    if (!globExclusions.empty())
    {
        if (globExclusions.size() == 1)
        {
            exclusions_.emplace_back(globExclusions.at(0));
        }
        else
        {
            exclusions_.emplace_back(std::move(globExclusions)); // Create combined exclusion
        }
    }
    std::sort(exclusions_.begin(), exclusions_.end());
}

bool common::ExclusionList::appliesToPath(const common::CachedPath& p, bool isDirectory, bool isFile) const
{
    for (const auto& exclusion : exclusions_)
    {
        if (exclusion.appliesToPath(p, isDirectory, isFile))
        {
            LOGTRACE("Path " << p.c_str() << " will not be scanned due to exclusion: "  << exclusion.displayPath());
            return true;
        }
    }
    return false;
}

auto common::ExclusionList::appliesToPath(const fs::path& path, bool isDirectory, bool isFile) const -> bool
{
    CachedPath p{path};
    return appliesToPath(p, isDirectory, isFile);
}

bool common::ExclusionList::operator==(const common::ExclusionList& rhs) const
{
    return exclusions_ == rhs.exclusions_;
}

bool common::ExclusionList::empty() const
{
    return exclusions_.empty();
}

std::string common::ExclusionList::printable() const
{
    std::stringstream printableExclusions;
    for(const auto &exclusion: exclusions_)
    {
        // We want to print the actual globs we are using, not the original form here.
        printableExclusions << "[\"" << exclusion.path() << "\"] ";
    }
    return printableExclusions.str();
}

bool common::ExclusionList::operator!=(const common::ExclusionList& rhs) const
{
    return !(*this == rhs);
}

common::ExclusionList::size_type common::ExclusionList::size() const
{
    return exclusions_.size();
}

const common::ExclusionList::value_type& common::ExclusionList::operator[](list_type::size_type pos) const
{
    return exclusions_[pos];
}

const common::ExclusionList::value_type& common::ExclusionList::at(list_type::size_type pos) const
{
    return exclusions_.at(pos);
}
