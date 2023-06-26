// Copyright 2023 Sophos Limited. All rights reserved.

#include "ALCPolicy.h"

#include "Logger.h"
#include "PolicyParseException.h"
#include "ProductSubscription.h"
#include "Proxy.h"
#include "ProxyCredentials.h"
#include "UpdateSettings.h"

#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/OSUtilities/IIPUtils.h"
#include "Common/ObfuscationImpl/Obfuscate.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/XmlUtilities/AttributesMap.h"

using namespace Common::Policy;

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

ALCPolicy::ALCPolicy(const std::string& xmlPolicy)
{
    const std::string FixedVersion{ "FixedVersion" };

    Common::XmlUtilities::AttributesMap attributesMap{{}, {}};
    try
    {
        attributesMap = Common::XmlUtilities::parseXml(xmlPolicy);
    }
    catch (const Common::XmlUtilities::XmlUtilitiesException& ex)
    {
        LOGERROR("Failed to parse policy: " << ex.what());
        std::throw_with_nested(Common::Policy::PolicyParseException(ex.what()));
    }

    auto cscComp = attributesMap.lookup("AUConfigurations/csc:Comp");
    if (cscComp.value("policyType") != "1")
    {
        throw PolicyParseException(LOCATION, "Update Policy type incorrect");
    }
    revID_ = cscComp.value("RevID");

    // sdds_id comes from SDDS2 user
    auto primaryLocation = attributesMap.lookup("AUConfigurations/AUConfig/primary_location/server");
    std::string user{ primaryLocation.value("UserName") };

    if (user.empty())
    {
        throw std::invalid_argument("Invalid policy: Username is empty ");
    }

    UpdateSettings settings;
    sdds_id_ = user;

    // Update Caches
    auto updateCacheEntities =
        attributesMap.entitiesThatContainPath("AUConfigurations/update_cache/locations/location");

    std::vector<UpdateCache> updateCaches;
    std::string certificateFileContent;
    if (!updateCacheEntities.empty())
    {
        for (auto& updateCache : updateCacheEntities)
        {
            auto attributes = attributesMap.lookup(updateCache);
            std::string hostname = attributes.value("hostname");
            std::string priority = attributes.value("priority");
            std::string id = attributes.value("id");
            updateCaches.emplace_back(UpdateCache{ hostname, priority, id });
        }

        updateCaches_ = sortUpdateCaches(updateCaches);
        std::vector<std::string> updateCacheHosts;
        updateCacheHosts.reserve(updateCaches_.size());
        for (auto& cache : updateCaches_)
        {
            updateCacheHosts.emplace_back(cache.hostname);
        }

        settings.setLocalUpdateCacheHosts(updateCacheHosts);

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
                Common::UtilityImpl::StringUtils::replaceAll(attributes.contents(), "&#13;", "");
        }
    }

    auto delayUpdating = attributesMap.lookup("AUConfigurations/AUConfig/delay_updating");
    std::string delayUpdatingDay = delayUpdating.value("Day");
    std::string delayUpdatingTime = delayUpdating.value("Time");
    WeekDayAndTimeForDelay weeklySchedule{};

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

        updateSettings_.setPolicyProxy(Proxy{
            proxyAddress,
            ProxyCredentials{ proxyUser, proxyPassword, proxyType } });
    }

    auto cloudSubscriptions =
        attributesMap.entitiesThatContainPath("AUConfigurations/AUConfig/cloud_subscriptions/subscription");
    std::vector<ProductSubscription> productsSubscription;

    bool ssplBaseIncluded = false;

    subscriptions_.clear();
    for (const auto& cloudSubscription : cloudSubscriptions)
    {
        auto subscriptionDetails = attributesMap.lookup(cloudSubscription);
        std::string rigidName = subscriptionDetails.value("RigidName");
        std::string tag = subscriptionDetails.value("Tag");
        std::string fixedVersion = subscriptionDetails.value(FixedVersion);

        ProductSubscription sub{
            rigidName,
            subscriptionDetails.value("BaseVersion"),
            tag,
            fixedVersion
        };
        subscriptions_.emplace_back(sub);
        if (rigidName != SSPLBaseName)
        {
            productsSubscription.emplace_back(sub);
        }
        else
        {
            updateSettings_.setPrimarySubscription({ rigidName,
                                            subscriptionDetails.value("BaseVersion"),
                                            subscriptionDetails.value("Tag"),
                                            fixedVersion });

            ssplBaseIncluded = true;
        }
    }

    updateSettings_.setProductsSubscription(productsSubscription);

    if (!ssplBaseIncluded)
    {
        LOGERROR(
            "SSPL base product name : " << SSPLBaseName
                                        << " not in the subscription of the policy.");
    }

    std::vector<std::string> allFeatures;
    bool includesCore = false;
    for (const auto& featureDetails : attributesMap.lookupMultiple("AUConfigurations/Features/Feature"))
    {
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

    updateSettings_.setFeatures(allFeatures);

    updateSettings_.setInstallArguments({ "--instdir", Common::ApplicationConfiguration::applicationPathManager().sophosInstall() });

    // manifest file name which must exist.
    updateSettings_.setManifestNames({ "manifest.dat" });

    // To add optional manifest file names call here
    updateSettings_.setOptionalManifestNames({"flags_manifest.dat", "sophos-scheduled-query-pack.manifest.dat"});

    // Slow supplements - no longer available - SDDS2 feature
    auto delay_supplements = attributesMap.lookup("AUConfigurations/AUConfig/delay_supplements");
    updateSettings_.setUseSlowSupplements(get_attr_bool(delay_supplements.value("enabled", "false")));

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
    updatePeriodMinutes_ = periodInt;
}


std::vector<UpdateCache> ALCPolicy::sortUpdateCaches(const std::vector<UpdateCache>& caches)
{
    // requirement: update caches candidates must be sorted by the following criteria:
    //  1. priority
    //  2. ip-proximity
    std::vector<std::string> cacheUrls;
    for (auto& candidate : caches)
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

    std::vector<UpdateCache> orderedCaches;
    for (auto& entry : report.servers)
    {
        orderedCaches.emplace_back(caches.at(entry.originalIndex));
    }

    std::stable_sort(orderedCaches.begin(), orderedCaches.end(), [](const UpdateCache& a, const UpdateCache& b) {
                         return a.priority < b.priority;
                     });
    return orderedCaches;
}