// Copyright 2018-2022, Sophos Limited. All rights reserved.

#include "UpdatePolicyTranslator.h"

#include "../Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/OSUtilities/IIPUtils.h>
#include <Common/ObfuscationImpl/Obfuscate.h>
#include <Common/SslImpl/Digest.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>

#include <algorithm>

namespace
{
    template<typename IpCollection>
    std::string ipCollectionToString(const IpCollection& ipCollection)
    {
        if (ipCollection.empty())
        {
            return std::string{};
        }
        std::stringstream s;
        bool first = true;
        for (auto& ip : ipCollection)
        {
            if (first)
            {
                s << "{";
            }
            else
            {
                s << ", ";
            }
            s << ip.stringAddress();
            first = false;
        }
        s << "}";
        return s.str();
    }

    std::string ipToString(const Common::OSUtilities::IPs& ips)
    {
        std::stringstream s;
        bool hasip4 = false;
        if (!ips.ip4collection.empty())
        {
            s << ipCollectionToString(ips.ip4collection);
            hasip4 = true;
        }

        if (!ips.ip6collection.empty())
        {
            if (hasip4)
            {
                s << "\n";
            }
            s << ipCollectionToString(ips.ip6collection);
        }
        return s.str();
    }

    std::string reportToString(const Common::OSUtilities::SortServersReport& report)
    {
        std::stringstream s;
        s << "Local ips: \n"
          << ipToString(report.localIps) << "\n"
          << "Servers: \n";
        for (auto& server : report.servers)
        {
            s << server.uri << '\n';
            if (!server.error.empty())
            {
                s << server.error << '\n';
            }
            else
            {
                s << ipToString(server.ips) << '\n';
            }
            s << "distance associated: ";
            if (server.MaxDistance == server.associatedMinDistance)
            {
                s << "Max\n";
            }
            else
            {
                s << server.associatedMinDistance << " bits\n";
            }
        }
        return s.str();
    }

    /**
     * Cover the possible false values for an attribute
     * from SAU:src/Utilities/PugiXmlHelpers.h:pugi_get_attr_bool
     * @param attr
     * @return
     */
    static inline bool get_attr_bool(const std::string& attr)
    {
        if (attr == "false" || attr == "0" || attr == "no" || attr.empty())
            return false;
        return true;
    }
} // namespace

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        using namespace Common::XmlUtilities;
        using namespace Common::ApplicationConfiguration;
        using namespace Common::FileSystem;

        SettingsHolder UpdatePolicyTranslator::translatePolicy(const std::string& policyXml)
        {
            static std::string error{ "Failed to parse policy" };
            try
            {
                return _translatePolicy(policyXml);
            }
            catch (SulDownloader::suldownloaderdata::SulDownloaderException& ex)
            {
                LOGERROR(ex.what());
                throw std::runtime_error(error);
            }
            catch (std::invalid_argument& ex)
            {
                LOGERROR(ex.what());
                throw std::runtime_error(error);
            }
        }

        SettingsHolder UpdatePolicyTranslator::_translatePolicy(const std::string& policyXml)
        {
            const std::string FixedVersion{ "FixedVersion" };
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
            m_updatePolicy.setSDDSid(user);

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
            m_updatePolicy.updateSubscriptions(subscriptions);
            m_updatePolicy.resetTelemetry(Common::Telemetry::TelemetryHelper::getInstance());
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

            return SettingsHolder{ config, certificateFileContent, std::chrono::minutes(periodInt), weeklySchedule};
        }

        std::string UpdatePolicyTranslator::cacheID(const std::string& hostname) const
        {
            std::string strippedHostname;
            strippedHostname = Common::UtilityImpl::StringUtils::replaceAll(hostname, "https://", "");
            if (Common::UtilityImpl::StringUtils::endswith(strippedHostname, "/v3"))
            {
                strippedHostname = Common::UtilityImpl::StringUtils::replaceAll(strippedHostname, "/v3", "");
            }
            for (auto& cache : m_Caches)
            {
                if (cache.hostname == strippedHostname)
                {
                    return cache.id;
                }
            }
            // Could not find the cache
            return std::string();
        }

        std::string UpdatePolicyTranslator::revID() const { return m_revID; }

        std::vector<UpdatePolicyTranslator::Cache> UpdatePolicyTranslator::sortUpdateCaches(
            const std::vector<UpdatePolicyTranslator::Cache>& /*caches*/)
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

        std::string UpdatePolicyTranslator::calculateSulObfuscated(const std::string& user, const std::string& pass)
        {
            return Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, user + ':' + pass);
        }

        UpdatePolicyTranslator::UpdatePolicyTranslator() :
            m_Caches{},
            m_revID{},
            m_updatePolicy{ }
        {
            Common::Telemetry::TelemetryHelper::getInstance().registerResetCallback("subscriptions", [this](Common::Telemetry::TelemetryHelper& telemetry){ m_updatePolicy.setSubscriptions(telemetry); });
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
            if (std::get<2>(subscription).empty())
            {
                telemetryToSet.set("subscriptions-"+std::get<0>(subscription), std::get<1>(subscription));
            }
            else
            {
                telemetryToSet.set("subscriptions-"+std::get<0>(subscription), std::get<2>(subscription));
            }

        }
    }


    void UpdatePolicyTelemetry::updateSubscriptions(std::vector<std::tuple<std::string, std::string, std::string>> subscriptions)
    {
        warehouseTelemetry.m_subscriptions = subscriptions;
    }


} // namespace UpdateSchedulerImpl
