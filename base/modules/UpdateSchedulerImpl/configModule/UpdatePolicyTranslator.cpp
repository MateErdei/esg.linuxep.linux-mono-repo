// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "UpdatePolicyTranslator.h"

#include "../Logger.h"
#include "Policy/ALCPolicy.h"
#include "Policy/PolicyParseException.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/SslImpl/Digest.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>

#include <algorithm>

namespace UpdateSchedulerImpl
{
    namespace configModule
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
            catch (SulDownloader::suldownloaderdata::SulDownloaderException& ex)
            {
                LOGERROR("Failed to parse policy: " << ex.what());
                std::throw_with_nested(std::runtime_error(error));
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


                updatePolicyTelemetry_.setSDDSid(updatePolicy_->getSddsID());
                updatePolicyTelemetry_.updateSubscriptions(updatePolicy_->getSubscriptions());
                updatePolicyTelemetry_.resetTelemetry(Common::Telemetry::TelemetryHelper::getInstance());

                return {.configurationData=settings,
                        .updateCacheCertificatesContent=updatePolicy_->getUpdateCertificatesContent(),
                        .schedulerPeriod=updatePolicy_->getUpdatePeriod(),
                        .weeklySchedule=updatePolicy_->getWeeklySchedule()
                };
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

        std::string UpdatePolicyTranslator::calculateSulObfuscated(const std::string& user, const std::string& pass)
        {
            return Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, user + ':' + pass);
        }

        UpdatePolicyTranslator::UpdatePolicyTranslator() :
            updatePolicyTelemetry_{ }
        {
            Common::Telemetry::TelemetryHelper::getInstance().registerResetCallback("subscriptions", [this](Common::Telemetry::TelemetryHelper& telemetry){ updatePolicyTelemetry_.setSubscriptions(telemetry); });
        }

        UpdatePolicyTranslator::~UpdatePolicyTranslator()
        {
            Common::Telemetry::TelemetryHelper::getInstance().unregisterResetCallback("subscriptions");
        }
    } // namespace configModule

    void UpdatePolicyTelemetry::setSDDSid(const std::string& sdds)
    {
        warehouseTelemetry_.m_sddsid = sdds;
    }

    void UpdatePolicyTelemetry::resetTelemetry(Common::Telemetry::TelemetryHelper& telemetryToSet)
    {
        Common::Telemetry::TelemetryObject updateTelemetry;
        Common::Telemetry::TelemetryValue ssd(warehouseTelemetry_.m_sddsid);
        updateTelemetry.set("sddsid", ssd);
        telemetryToSet.set("warehouse", updateTelemetry, true);

        setSubscriptions(telemetryToSet);

    }

    void UpdatePolicyTelemetry::setSubscriptions(Common::Telemetry::TelemetryHelper& telemetryToSet)
    {
        SubscriptionVector subs;
        {
            auto locked = warehouseTelemetry_.m_subscriptions.lock();
            subs = *locked;
        }
        for (const auto& subscription : subs)
        {
            std::string key = "subscriptions-";
            key += subscription.rigidName();

            std::string value = subscription.fixedVersion();
            if (subscription.fixedVersion().empty())
            {
                value = subscription.tag();
            }
            telemetryToSet.set(key, value);
        }
    }

    void UpdatePolicyTelemetry::updateSubscriptions(SubscriptionVector subscriptions)
    {
        auto locked = warehouseTelemetry_.m_subscriptions.lock();
        locked->swap(subscriptions);
    }


} // namespace UpdateSchedulerImpl
