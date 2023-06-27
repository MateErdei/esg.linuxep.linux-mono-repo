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

                SulDownloader::suldownloaderdata::ConfigurationData config{settings};
                config.setInstallArguments({ "--instdir", applicationPathManager().sophosInstall() });
                config.setLogLevel(SulDownloader::suldownloaderdata::ConfigurationData::LogLevel::VERBOSE);

                updatePolicyTelemetry_.setSDDSid(updatePolicy_->getSddsID());
                updatePolicyTelemetry_.updateSubscriptions(updatePolicy_->getSubscriptions());
                updatePolicyTelemetry_.resetTelemetry(Common::Telemetry::TelemetryHelper::getInstance());

                return {.configurationData=config,
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


  /*          const std::string FixedVersion{ "FixedVersion" };
            m_Caches.clear();

            Common::XmlUtilities::AttributesMap attributesMap = parseXml(policyXml);

            auto cscComp = attributesMap.lookup("AUConfigurations/csc:Comp");
            if (cscComp.value("policyType") != "1")
            {
                throw std::runtime_error("Update Policy type incorrect");
            }
            m_revID = cscComp.value("RevID");

            auto primaryLocation = attributesMap.lookup("AUConfigurations/AUConfig/primary_location/server");

            std::string connectionAddress;

            if (fileSystem()->isFile(applicationPathManager().getSophosAliasFilePath()))
            {
                connectionAddress = fileSystem()->readFile(applicationPathManager().getSophosAliasFilePath());
                LOGINFO("Using connection address provided by sophos_alias.txt file.");
            }
            else
            {
                connectionAddress = primaryLocation.value("ConnectionAddress");
            }

            std::vector<std::string> defaultLocations{
                SulDownloader::suldownloaderdata::ConfigurationData::DefaultSophosLocationsURL
            };

            if (!connectionAddress.empty())
            {
                defaultLocations.insert(begin(defaultLocations), connectionAddress);
            }

            SulDownloader::suldownloaderdata::ConfigurationData config(defaultLocations);
            std::string user{ primaryLocation.value("UserName") };
            std::string pass{ primaryLocation.value("UserPassword") };
            std::string algorithm{ primaryLocation.value("Algorithm") };
            bool requireObfuscation = true;

            if (algorithm == "AES256")
            {
                pass = Common::ObfuscationImpl::SECDeobfuscate(pass);
            }
            else if (user.size() == 32 && user == pass)
            {
                requireObfuscation = false;
            }

            if (requireObfuscation)
            {
                std::string obfuscated = calculateSulObfuscated(user, pass);
                config.setCredentials(SulDownloader::suldownloaderdata::Credentials{ obfuscated, obfuscated });
            }
            else
            {
                config.setCredentials(SulDownloader::suldownloaderdata::Credentials{ user, pass });
            }
            updatePolicyTelemetry_.setSDDSid(user);

            auto updateCacheEntities =
                attributesMap.entitiesThatContainPath("AUConfigurations/update_cache/locations/location");

            std::string certificateFileContent;
            if (!updateCacheEntities.empty())
            {
                for (auto& updateCache : updateCacheEntities)
                {
                    auto attributes = attributesMap.lookup(updateCache);
                    std::string hostname = attributes.value("hostname");
                    std::string priority = attributes.value("priority");
                    std::string id = attributes.value("id");
                    m_Caches.emplace_back(Cache{ hostname, priority, id });
                }

                m_Caches = sortUpdateCaches(m_Caches);
                std::vector<std::string> updateCaches;
                for (auto& cache : m_Caches)
                {
                    updateCaches.emplace_back(cache.hostname);
                }

                config.setLocalUpdateCacheUrls(updateCaches);

                auto cacheCertificates = attributesMap.entitiesThatContainPath(
                    "AUConfigurations/update_cache/intermediate_certificates/intermediate_certificate");

                for (auto& certificate : cacheCertificates)
                {
                    auto attributes = attributesMap.lookup(certificate);
                    // Remove line endings from certificate
                    if (!certificateFileContent.empty())
                    {
                        certificateFileContent += "\n";
                    }
                    certificateFileContent +=
                        Common::UtilityImpl::StringUtils::replaceAll(attributes.value(attributes.TextId), "&#13;", "");
                }
                std::string cacheCertificatePath = applicationPathManager().getUpdateCacheCertificateFilePath();
            }

            auto delayUpdating = attributesMap.lookup("AUConfigurations/AUConfig/delay_updating");
            std::string delayUpdatingDay = delayUpdating.value("Day");
            std::string delayUpdatingTime = delayUpdating.value("Time");
            SettingsHolder::WeekDayAndTimeForDelay weeklySchedule{};

            if (!delayUpdatingDay.empty() && !delayUpdatingTime.empty())
            {
                std::string delayUpdatingDayAndTime = delayUpdatingDay + "," + delayUpdatingTime;
                std::tm scheduledUpdateTime{};
                if (strptime(delayUpdatingDayAndTime.c_str(), "%a,%H:%M:%S", &scheduledUpdateTime))
                {
                    weeklySchedule = {
                          .enabled = true,
                          .weekDay = scheduledUpdateTime.tm_wday,
                          .hour = scheduledUpdateTime.tm_hour,
                          .minute = scheduledUpdateTime.tm_min };
                }
                else
                {
                    LOGERROR("Could not parse update schedule from policy, leaving update schedule as default");
                }
            }

            auto primaryProxy = attributesMap.lookup("AUConfigurations/AUConfig/primary_location/proxy");
            std::string proxyAddress = primaryProxy.value("ProxyAddress");
            if (!proxyAddress.empty())
            {
                std::string proxyPort = primaryProxy.value("ProxyPortNumber");
                std::string proxyUser = primaryProxy.value("ProxyUserName");
                std::string proxyPassword = primaryProxy.value("ProxyUserPassword");
                std::string proxyType = primaryProxy.value("ProxyType");
                if (!proxyPort.empty())
                {
                    proxyAddress += ":" + proxyPort;
                }

                config.setPolicyProxy(SulDownloader::suldownloaderdata::Proxy{
                    proxyAddress,
                    SulDownloader::suldownloaderdata::ProxyCredentials{ proxyUser, proxyPassword, proxyType } });
            }

            auto cloudSubscriptions =
                attributesMap.entitiesThatContainPath("AUConfigurations/AUConfig/cloud_subscriptions/subscription");
            std::vector<SulDownloader::suldownloaderdata::ProductSubscription> productsSubscription;

            bool ssplBaseIncluded = false;

            std::vector<std::tuple<std::string, std::string, std::string>> subscriptions;
            for (const auto& cloudSubscription : cloudSubscriptions)
            {
                auto subscriptionDetails = attributesMap.lookup(cloudSubscription);
                std::string rigidName = subscriptionDetails.value("RigidName");
                std::string tag = subscriptionDetails.value("Tag");
                std::string fixedVersion = subscriptionDetails.value(FixedVersion);

                subscriptions.emplace_back(rigidName, tag, fixedVersion);
                if (rigidName != SulDownloader::suldownloaderdata::SSPLBaseName)
                {
                    productsSubscription.emplace_back(SulDownloader::suldownloaderdata::ProductSubscription(
                        rigidName,
                        subscriptionDetails.value("BaseVersion"),
                        subscriptionDetails.value("Tag"),
                        fixedVersion));
                }
                else
                {
                    config.setPrimarySubscription({ rigidName,
                                                    subscriptionDetails.value("BaseVersion"),
                                                    subscriptionDetails.value("Tag"),
                                                    fixedVersion });

                    ssplBaseIncluded = true;
                }
            }
            updatePolicyTelemetry_.updateSubscriptions(subscriptions);
            updatePolicyTelemetry_.resetTelemetry(Common::Telemetry::TelemetryHelper::getInstance());
            config.setProductsSubscription(productsSubscription);

            if (!ssplBaseIncluded)
            {
                LOGERROR(
                    "SSPL base product name : " << SulDownloader::suldownloaderdata::SSPLBaseName
                                                << " not in the subscription of the policy.");
            }

            auto features = attributesMap.entitiesThatContainPath("AUConfigurations/Features/Feature");
            std::vector<std::string> allFeatures;
            bool includesCore = false;
            for (const auto& feature : features)
            {
                auto featureDetails = attributesMap.lookup(feature);
                std::string featureName = featureDetails.value("id");
                if (featureName.empty())
                {
                    continue;
                }
                if (featureName == "CORE")
                {
                    includesCore = true;
                }
                allFeatures.emplace_back(featureName);
            }

            if (!includesCore)
            {
                LOGERROR("CORE not in the features of the policy.");
            }

            config.setFeatures(allFeatures);

            config.setInstallArguments({ "--instdir", applicationPathManager().sophosInstall() });
            config.setLogLevel(SulDownloader::suldownloaderdata::ConfigurationData::LogLevel::VERBOSE);

            // manifest file name which must exist.
            config.setManifestNames({ "manifest.dat" });

            // To add optional manifest file names call here
            config.setOptionalManifestNames({"flags_manifest.dat", "sophos-scheduled-query-pack.manifest.dat"});


            auto delay_supplements = attributesMap.lookup("AUConfigurations/AUConfig/delay_supplements");
            config.setUseSlowSupplements(get_attr_bool(delay_supplements.value("enabled", "false")));

            std::string period = attributesMap.lookup("AUConfigurations/AUConfig/schedule").value("Frequency");
            int periodInt = 60;
            if (!period.empty())
            {
                std::pair<int, std::string> value = Common::UtilityImpl::StringUtils::stringToInt(period);
                if (value.second.empty())
                {
                    periodInt = value.first;
                }
            }

            return SettingsHolder{ config, certificateFileContent, std::chrono::minutes(periodInt), weeklySchedule};*/
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

/*
        std::vector<Common::Policy::Cache> UpdatePolicyTranslator::sortUpdateCaches(
            const std::vector<Common::Policy::Cache>& */
/*caches*//*
)
        {
            // requirement: update caches candidates must be sorted by the following criteria:
            //  1. priority
            //  2. ip-proximity
            std::vector<std::string> cacheUrls;
            for (auto& candidate : m_Caches)
            {
                cacheUrls.emplace_back(Common::OSUtilities::tryExtractServerFromHttpURL(candidate.hostname));
            }

            auto start = std::chrono::steady_clock::now();
            Common::OSUtilities::SortServersReport report =
                Common::OSUtilities::indexOfSortedURIsByIPProximity(cacheUrls);
            auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            std::string logReport = reportToString(report);
            LOGSUPPORT("Sort update cache took " << elapsed << " ms. Entries: " << logReport);
            std::vector<int> sortedIndex = Common::OSUtilities::sortedIndexes(report);

            std::vector<Cache> orderedCaches;
            for (auto& entry : report.servers)
            {
                orderedCaches.emplace_back(m_Caches.at(entry.originalIndex));
            }

            std::stable_sort(orderedCaches.begin(), orderedCaches.end(), [](const Cache& a, const Cache& b) {
                return a.priority < b.priority;
            });
            return orderedCaches;
        }
*/

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
        warehouseTelemetry.m_sddsid = sdds;
    }

    void UpdatePolicyTelemetry::resetTelemetry(Common::Telemetry::TelemetryHelper& telemetryToSet)
    {
        Common::Telemetry::TelemetryObject updateTelemetry;
        Common::Telemetry::TelemetryValue ssd(warehouseTelemetry.m_sddsid);
        updateTelemetry.set("sddsid", ssd);
        telemetryToSet.set("warehouse", updateTelemetry, true);

        setSubscriptions(telemetryToSet);

    }

    void UpdatePolicyTelemetry::setSubscriptions(Common::Telemetry::TelemetryHelper& telemetryToSet)
    {
        for (auto& subscription : warehouseTelemetry.m_subscriptions)
        {
            if (subscription.fixedVersion().empty())
            {
                telemetryToSet.set("subscriptions-"+subscription.rigidName(), subscription.tag());
            }
            else
            {
                telemetryToSet.set("subscriptions-"+subscription.rigidName(), subscription.fixedVersion());
            }
        }
    }


    void UpdatePolicyTelemetry::updateSubscriptions(std::vector<Common::Policy::ProductSubscription> subscriptions)
    {
        warehouseTelemetry.m_subscriptions = std::move(subscriptions);
    }


} // namespace UpdateSchedulerImpl
