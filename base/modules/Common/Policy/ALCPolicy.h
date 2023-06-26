// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "UpdateSettings.h"

#include <string>
#include <vector>

namespace Common::Policy
{
    struct UpdateCache
    {
        std::string hostname;
        std::string priority;
        std::string id;
    };

    // Settings for delayed updates
    struct WeekDayAndTimeForDelay
    {
        bool enabled = false;
        int weekDay = 0;
        int hour = 0;
        int minute = 0;
        // seconds is not used when setting up delayed updates
    };

    class ALCPolicy
    {
    public:
        explicit ALCPolicy(const std::string& xmlPolicy);
        UpdateSettings getUpdateSettings() const
        {
            return updateSettings_;
        }

        [[nodiscard]] std::string getSddsID() const { return sdds_id_; }
        [[nodiscard]] std::vector<ProductSubscription> getSubscriptions() const { return subscriptions_; }

        int getUpdatePeriodMinutes() const
        {
            return updatePeriodMinutes_;
        }

    private:
        std::string revID_;
        std::string sdds_id_;
        std::vector<ProductSubscription> subscriptions_;
        std::vector<UpdateCache> sortUpdateCaches(const std::vector<UpdateCache>& caches);
        std::vector<UpdateCache> updateCaches_;
        UpdateSettings updateSettings_;
        int updatePeriodMinutes_;
    };
}
