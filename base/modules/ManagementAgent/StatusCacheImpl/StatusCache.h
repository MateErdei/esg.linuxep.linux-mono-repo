// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ManagementAgent/StatusCache/IStatusCache.h"

#include <map>

namespace ManagementAgent
{
    namespace StatusCacheImpl
    {
        class StatusCache : public virtual ManagementAgent::StatusCache::IStatusCache
        {
        public:
            StatusCache() = default;
            StatusCache(const StatusCache&) = delete;
            ~StatusCache() override = default;
            bool statusChanged(const std::string& appid, const std::string& statusForComparison) override;
            void loadCacheFromDisk() override;

        private:
            /**
             * Possibly need to convert to on-disk, or use on-disk as well
             */
            std::map<std::string, std::string> m_statusCache;

            /**
             * Update m_statusCache and return true.
             * @param appid
             * @param statusForComparison
             */
            void updateStatus(const std::string& appid, const std::string& statusForComparison);
        };
    } // namespace StatusCacheImpl
} // namespace ManagementAgent
