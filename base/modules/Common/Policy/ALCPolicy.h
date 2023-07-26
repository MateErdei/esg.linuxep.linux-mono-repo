// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "UpdateSettings.h"
#include "WeekDayAndTimeForDelay.h"

#include "Common/XmlUtilities/AttributesMap.h"

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
        [[nodiscard]] UpdateSettings getUpdateSettings() const { return updateSettings_; }

        [[nodiscard]] std::vector<ProductSubscription> getSubscriptions() const { return subscriptions_; }

        [[nodiscard]] std::chrono::minutes getUpdatePeriod() const
        {
            return std::chrono::minutes{ updatePeriodMinutes_ };
        }

        [[nodiscard]] std::string getUpdateCertificatesContent() const { return update_certificates_content_; }

        [[nodiscard]] WeekDayAndTimeForDelay getWeeklySchedule() const { return weeklySchedule_; }

        [[nodiscard]] std::string cacheID(const std::string& hostname) const;

        [[nodiscard]] std::string revID() const { return revID_; }

        [[nodiscard]] std::string getTelemetryHost() const { return telemetryHost_; }

    private:
        std::vector<UpdateCache> sortUpdateCaches(const std::vector<UpdateCache>& caches);
        void extractSDDS3SusUrl(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractSDDS3SophosUrls(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractUpdateCaches(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractUpdateSchedule(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractProxyDetails(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractFeatures(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractCloudSubscriptions(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractPeriod(const Common::XmlUtilities::AttributesMap& attributesMap);
        void extractESMVersion(const Common::XmlUtilities::AttributesMap& attributesMap);
        void formatUrl(std::string& url);
        bool validateHostname(const std::string& url);
        void logVersion();

        std::string revID_;
        std::string update_certificates_content_;
        std::string telemetryHost_;
        std::vector<ProductSubscription> subscriptions_;
        std::vector<UpdateCache> updateCaches_;
        UpdateSettings updateSettings_;
        WeekDayAndTimeForDelay weeklySchedule_;
        int updatePeriodMinutes_ = 60;
    };
} // namespace Common::Policy
