// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "Exclusion.h"

#include <vector>

namespace common
{
    class ExclusionList
    {
    public:
        using value_type = Exclusion;
        using list_type = std::vector<value_type>;
        using size_type = list_type::size_type;
        using const_reference = const value_type&;
        ExclusionList() = default;
        explicit ExclusionList(const std::vector<std::string>& exclusions);
        /**
         * Construct an ExclusionList from a list of exclusions.
         * @param exclusions
         * @param int - extra argument added to disambiguate empty {} construction.
         */
        explicit ExclusionList(list_type exclusions, int);

        bool operator==(const ExclusionList& rhs) const;
        bool operator!=(const ExclusionList& rhs) const;

        [[nodiscard]] bool appliesToPath(const CachedPath&, bool isDirectory, bool isFile) const;
        [[nodiscard]] bool appliesToPath(const fs::path&, bool isDirectory, bool isFile) const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::string printable() const;
        [[nodiscard]] size_type size() const;
        [[nodiscard]] const_reference operator[](size_type) const;
        [[nodiscard]] const_reference at(size_type) const;
    private:
        list_type exclusions_;
    };
}
