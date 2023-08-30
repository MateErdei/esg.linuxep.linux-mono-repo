// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace Common::DownloadReport
{
    struct ProductInfo
    {
        std::string m_rigidName;
        std::string m_productName;
        std::string m_version;
        std::string m_installedVersion;

        [[nodiscard]] bool operator==(const ProductInfo& other) const
        {
            // clang-format off
            return m_rigidName == other.m_rigidName &&
                   m_productName == other.m_productName &&
                   m_version == other.m_version &&
                   m_installedVersion == other.m_installedVersion;
            // clang-format on
        }
    };
} // namespace Common::DownloadReport