// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "SulDownloader/suldownloaderdata/ConfigurationData.h"

#include "UpdateScheduler/IMapHostCacheId.h"

#include "Common/Exceptions/IException.h"
#include "Common/Policy/ALCPolicy.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/Threads/LockableData.h"

#include <chrono>
#include <memory>

namespace UpdateSchedulerImpl
{
    class UpdatePolicyTelemetry
    {
    public:
        using SubscriptionVector = std::vector<Common::Policy::ProductSubscription>;
        /**
         * Called while parsing a policy to update current product subscriptions
         * @param subscriptions
         */
        void updateSubscriptions(SubscriptionVector subscriptions);
        /**
         * Called while parsing a policy to update SDDS-ID
         */
        void setSDDSid(const std::string & );
        /**
         *  Called while parsing a policy, after above setting methods to update telemetry
         */
        void resetTelemetry(Common::Telemetry::TelemetryHelper& );
        /**
         * Called by resetTelemetry
         * and after generating telemetry - by reset callback
         */
        void setSubscriptions(Common::Telemetry::TelemetryHelper& );
    private:
        struct WarehouseTelemetry
        {
            using locked_subscription_vector_t = Common::Threads::LockableData<SubscriptionVector>;
            locked_subscription_vector_t m_subscriptions;
            std::string m_sddsid;
        };
        WarehouseTelemetry warehouseTelemetry_;
    };


    namespace configModule
    {
        struct SettingsHolder
        {
            using WeekDayAndTimeForDelay = Common::Policy::WeekDayAndTimeForDelay;
            using SulDownloaderConfigurationData = SulDownloader::suldownloaderdata::ConfigurationData;

            SulDownloaderConfigurationData configurationData;
            std::string updateCacheCertificatesContent;
            std::chrono::minutes schedulerPeriod;
            WeekDayAndTimeForDelay weeklySchedule;
        };

        class PolicyValidationException : public Common::Exceptions::IException
        {
        public:
            using Common::Exceptions::IException::IException;
        };

        class UpdatePolicyTranslator : public virtual UpdateScheduler::IMapHostCacheId
        {
        public:
            // may throw PolicyValidationException if the policy does not pass the validation criteria.
            SettingsHolder translatePolicy(const std::string& policyXml);
            static std::string calculateSulObfuscated(const std::string& user, const std::string& pass);

            std::string cacheID(const std::string& hostname) const override;

            std::string revID() const;
            UpdatePolicyTranslator();
            ~UpdatePolicyTranslator();
        private:
            SettingsHolder _translatePolicy(const std::string& policyXml);
            std::shared_ptr<Common::Policy::ALCPolicy> updatePolicy_;

            UpdatePolicyTelemetry updatePolicyTelemetry_;
        };
    } // namespace configModule
} // namespace UpdateSchedulerImpl
