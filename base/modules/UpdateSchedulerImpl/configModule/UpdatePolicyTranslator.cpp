// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "UpdatePolicyTranslator.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Policy/PolicyParseException.h"
#include "Common/SslImpl/Digest.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "UpdateSchedulerImpl/common/Logger.h"

#include <algorithm>

using namespace Common::Policy;

namespace UpdateSchedulerImpl::configModule
{
    using namespace Common::ApplicationConfiguration;
    using namespace Common::FileSystem;

    SettingsHolder UpdatePolicyTranslator::translatePolicy(const std::string& policyXml)
    {
        static const std::string error{ "Failed to parse policy" };
        try
        {
            return _translatePolicy(policyXml);
        }
        catch (std::invalid_argument& ex)
        {
            LOGERROR("Failed to parse policy (invalid argument): " << ex.what());
            std::throw_with_nested(std::runtime_error(error));
        }
    }

    SettingsHolder UpdatePolicyTranslator::_translatePolicy(const std::string& policyXml)
    {
        try
        {
            updatePolicy_ = std::make_shared<Common::Policy::ALCPolicy>(policyXml);
            auto settings = updatePolicy_->getUpdateSettings();
            settings.setInstallArguments({ "--instdir", applicationPathManager().sophosInstall() });
            settings.setLogLevel(Common::Policy::UpdateSettings::LogLevel::VERBOSE);

            updatePolicyTelemetry_.updateSubscriptions(
                updatePolicy_->getSubscriptions(), updatePolicy_->getUpdateSettings().getEsmVersion());
            updatePolicyTelemetry_.resetTelemetry(Common::Telemetry::TelemetryHelper::getInstance());

            return { .configurationData = settings,
                     .updateCacheCertificatesContent = updatePolicy_->getUpdateCertificatesContent(),
                     .schedulerPeriod = updatePolicy_->getUpdatePeriod(),
                     .weeklySchedule = updatePolicy_->getWeeklySchedule() };
        }
        catch (const Common::Policy::PolicyParseException& ex)
        {
            LOGERROR("Failed to parse policy");
            std::throw_with_nested(Common::Exceptions::IException(LOCATION, ex.what()));
        }
    }

    std::string UpdatePolicyTranslator::cacheID(const std::string& hostname) const
    {
        assert(updatePolicy_);
        return updatePolicy_->cacheID(hostname);
    }

    std::string UpdatePolicyTranslator::revID() const
    {
        assert(updatePolicy_);
        return updatePolicy_->revID();
    }

    UpdatePolicyTranslator::UpdatePolicyTranslator() : updatePolicyTelemetry_{}
    {
        Common::Telemetry::TelemetryHelper::getInstance().registerResetCallback(
            "subscriptions",
            [this](Common::Telemetry::TelemetryHelper& telemetry)
            { updatePolicyTelemetry_.setSubscriptions(telemetry); });
    }

    UpdatePolicyTranslator::~UpdatePolicyTranslator()
    {
        Common::Telemetry::TelemetryHelper::getInstance().unregisterResetCallback("subscriptions");
    }
} // namespace UpdateSchedulerImpl::configModule

namespace UpdateSchedulerImpl
{
    void UpdatePolicyTelemetry::resetTelemetry(Common::Telemetry::TelemetryHelper& telemetryToSet)
    {
        Common::Telemetry::TelemetryObject updateTelemetry;
        setSubscriptions(telemetryToSet);
    }

    void UpdatePolicyTelemetry::setSubscriptions(Common::Telemetry::TelemetryHelper& telemetryToSet)
    {
        UpdatePolicyTelemetry::CombinedVersionInfo versionInfo;
        {
            auto locked = warehouseTelemetry_.m_subscriptions.lock();
            versionInfo = *locked;
        }
        bool usingESM = versionInfo.esmVersion.isEnabled();

        std::string esmName = usingESM ? versionInfo.esmVersion.name() : "";
        std::string esmToken = usingESM ? versionInfo.esmVersion.token() : "";

        telemetryToSet.set("esmName", esmName);
        telemetryToSet.set("esmToken", esmToken);

        for (const auto& subscription : versionInfo.subscriptionVector)
        {
            std::string key = "subscriptions-";
            key += subscription.rigidName();

            std::string value;
            if (usingESM)
            {
                value = esmName;
            }
            else
            {
                value = subscription.fixedVersion();
                if (subscription.fixedVersion().empty())
                {
                    value = subscription.tag();
                }
            }

            telemetryToSet.set(key, value);
        }
    }

    void UpdatePolicyTelemetry::updateSubscriptions(SubscriptionVector subscriptions, ESMVersion esmVersion)
    {
        auto locked = warehouseTelemetry_.m_subscriptions.lock();
        locked->subscriptionVector.swap(subscriptions);
        locked->esmVersion = std::move(esmVersion);
    }
} // namespace UpdateSchedulerImpl
