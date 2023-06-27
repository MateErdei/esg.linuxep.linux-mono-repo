// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "UpdateSettings.h"
#include "WeekDayAndTimeForDelay.h"

#include <chrono>
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

    class ALCPolicy
    {
    public:
        explicit ALCPolicy(const std::string& xmlPolicy);
        [[nodiscard]] UpdateSettings getUpdateSettings() const
        {
            return updateSettings_;
        }

        [[nodiscard]] std::string getSddsID() const { return sdds_id_; }
        [[nodiscard]] std::vector<ProductSubscription> getSubscriptions() const { return subscriptions_; }

        [[nodiscard]] std::chrono::minutes getUpdatePeriod() const
        {
            return std::chrono::minutes{updatePeriodMinutes_};
        }

        [[nodiscard]] std::string getUpdateCertificatesContent() const
        {
            return update_certificates_content_;
        }

        [[nodiscard]] WeekDayAndTimeForDelay getWeeklySchedule() const
        {
            return weeklySchedule_;
        }

    private:
        std::string revID_;
        std::string sdds_id_;
        std::string update_certificates_content_;
        std::vector<ProductSubscription> subscriptions_;
        std::vector<UpdateCache> sortUpdateCaches(const std::vector<UpdateCache>& caches);
        std::vector<UpdateCache> updateCaches_;
        UpdateSettings updateSettings_;
        WeekDayAndTimeForDelay weeklySchedule_;
        int updatePeriodMinutes_ = 60;
    };
}
