// Copyright 2018-2023 Sophos Limited. All rights reserved.

// Contains code original in UpdatePolicyTranslator.cpp

#include "ALCPolicy.h"

#include "Logger.h"
#include "PolicyHelper.h"
#include "PolicyParseException.h"
#include "ProductSubscription.h"
#include "Proxy.h"
#include "ProxyCredentials.h"
#include "UpdateSettings.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/OSUtilities/IIPUtils.h"
#include "Common/ObfuscationImpl/Obfuscate.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/XmlUtilities/AttributesMap.h"
#include "sophlib/hostname/Validate.h"

using namespace Common::ApplicationConfiguration;
using namespace Common::Policy;

namespace
{
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
    Common::XmlUtilities::AttributesMap attributesMap{ {}, {} };
    try
    {
        attributesMap = Common::XmlUtilities::parseXml(xmlPolicy);
    }
    catch (const Common::XmlUtilities::XmlUtilitiesException& ex)
    {
        LOGERROR("Failed to parse policy: " << ex.what());
        std::throw_with_nested(Common::Policy::PolicyParseException(LOCATION, ex.what()));
    }

    auto cscComp = attributesMap.lookup("AUConfigurations/csc:Comp");
    if (cscComp.value("policyType") != "1")
    {
        throw PolicyParseException(LOCATION, "Update Policy type incorrect");
    }
    revID_ = cscComp.value("RevID");

    extractSDDS3SusUrl(attributesMap);
    extractSDDS3SophosUrls(attributesMap);
    extractUpdateCaches(attributesMap);
    extractUpdateSchedule(attributesMap);
    extractProxyDetails(attributesMap);
    extractCloudSubscriptions(attributesMap);
    extractPeriod(attributesMap);
    extractFeatures(attributesMap);
    extractESMVersion(attributesMap);

    // manifest file name which must exist.
    updateSettings_.setManifestNames({ "manifest.dat" });

    // To add optional manifest file names call here
    updateSettings_.setOptionalManifestNames({ "flags_manifest.dat", "sophos-scheduled-query-pack.manifest.dat" });

    // Slow supplements - no longer available - SDDS2 feature
    auto delay_supplements = attributesMap.lookup("AUConfigurations/AUConfig/delay_supplements");
    updateSettings_.setUseSlowSupplements(get_attr_bool(delay_supplements.value("enabled", "false")));

    const auto telemetryHosts = attributesMap.lookupMultiple("AUConfigurations/server_names/telemetry");
    if (telemetryHosts.empty())
    {
        telemetryHost_ = std::nullopt;
    }
    else if (telemetryHosts.size() == 1)
    {
        try
        {
            telemetryHost_ = telemetryHosts[0].contents();
            sophlib::hostname::validate(*telemetryHost_);
        }
        catch (const std::runtime_error& ex)
        {
            LOGERROR("Invalid telemetry host '" + *telemetryHost_ + "'");
            telemetryHost_ = "";
        }
    }
    else
    {
        throw PolicyParseException(LOCATION, "More than one telemetry host");
    }

    logVersion();
}

std::vector<UpdateCache> ALCPolicy::sortUpdateCaches(const std::vector<UpdateCache>& caches)
{
    // requirement: update caches candidates must be sorted by the following criteria:
    //  1. priority
    //  2. ip-proximity
    std::vector<std::string> cacheUrls;
    cacheUrls.reserve(caches.size());
    for (auto& candidate : caches)
    {
        cacheUrls.emplace_back(Common::OSUtilities::tryExtractServerFromHttpURL(candidate.hostname));
    }

    auto start = std::chrono::steady_clock::now();
    Common::OSUtilities::SortServersReport report = Common::OSUtilities::indexOfSortedURIsByIPProximity(cacheUrls);
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    std::string logReport = reportToString(report);
    LOGSUPPORT("Sort update cache took " << elapsed << " ms. Entries: " << logReport);
    std::vector<int> sortedIndex = Common::OSUtilities::sortedIndexes(report);

    std::vector<UpdateCache> orderedCaches;
    orderedCaches.reserve(report.servers.size());
    for (auto& entry : report.servers)
    {
        orderedCaches.emplace_back(caches.at(entry.originalIndex));
    }

    std::stable_sort(
        orderedCaches.begin(),
        orderedCaches.end(),
        [](const UpdateCache& a, const UpdateCache& b) { return a.priority < b.priority; });
    return orderedCaches;
}

std::string ALCPolicy::cacheID(const std::string& hostname) const
{
    std::string strippedHostname;
    strippedHostname = Common::UtilityImpl::StringUtils::replaceAll(hostname, "https://", "");
    if (Common::UtilityImpl::StringUtils::endswith(strippedHostname, "/v3"))
    {
        strippedHostname = Common::UtilityImpl::StringUtils::replaceAll(strippedHostname, "/v3", "");
    }
    for (const auto& cache : updateCaches_)
    {
        if (cache.hostname == strippedHostname)
        {
            return cache.id;
        }
    }
    // Could not find the cache
    return {};
}

void ALCPolicy::formatUrl(std::string& url)
{
    if (!Common::UtilityImpl::StringUtils::startswith(url, "https://"))
    {
        url = "https://" + url;
    }
}

void ALCPolicy::extractSDDS3SusUrl(const Common::XmlUtilities::AttributesMap& attributesMap)
{
    std::string susUrl = attributesMap.lookup("AUConfigurations/server_names/sdds3/sus").contents();

    std::string sophosUpdateSus = UpdateSettings::DefaultSophosSusUrl;

    if (susUrl.empty())
    {
        LOGWARN("SUS hostname is empty in ALC policy");
    }
    else if (validateHostname(susUrl))
    {
        formatUrl(susUrl);
        sophosUpdateSus = susUrl;
    }

    updateSettings_.setSophosSusURL(sophosUpdateSus);
}

void ALCPolicy::extractSDDS3SophosUrls(const Common::XmlUtilities::AttributesMap& attributesMap)
{
    auto cdns = attributesMap.entitiesThatContainPath("AUConfigurations/server_names/sdds3/content_servers/server");

    if (cdns.empty())
    {
        LOGWARN("No CDN urls in in ALC policy");
        updateSettings_.setSophosCDNURLs(UpdateSettings::DefaultSophosCDNUrls);
        return;
    }

    std::vector<std::string> cdnUrls;
    for (const auto& url : cdns)
    {
        auto attributes = attributesMap.lookup(url);
        std::string cdnurl = attributes.contents();
        if (cdnurl.empty())
        {
            LOGWARN("CDN hostname is empty in ALC policy");
            continue;
        }
        if (validateHostname(cdnurl))
        {
            formatUrl(cdnurl);
            cdnUrls.emplace_back(cdnurl);
        }
    }
    if (cdnUrls.empty())
    {
        updateSettings_.setSophosCDNURLs(UpdateSettings::DefaultSophosCDNUrls);
        return;
    }
    updateSettings_.setSophosCDNURLs(cdnUrls);
}

bool ALCPolicy::validateHostname(const std::string& url)
{
    std::vector<std::string> parts = Common::UtilityImpl::StringUtils::splitString(url, ":");
    if (parts.size() > 2)
    {
        LOGWARN("Malformed url '" << url << "' in ALC policy");

        return false;
    }
    else if (parts.size() == 2)
    {
        if (!Common::UtilityImpl::StringUtils::isPositiveInteger(parts[1]))
        {
            LOGWARN("Invalid url '" << url << "' port is not a number in ALC policy");
        }
    }
    try
    {
        sophlib::hostname::validate(parts[0]);
    }
    catch (const std::runtime_error& ex)
    {
        LOGWARN("Invalid host '" << parts[0] << "' in ALC policy: " << ex.what());

        return false;
    }

    return true;
}

void ALCPolicy::extractUpdateCaches(const Common::XmlUtilities::AttributesMap& attributesMap)
{
    // Update Caches
    auto updateCacheEntities =
        attributesMap.entitiesThatContainPath("AUConfigurations/update_cache/locations/location");

    if (updateCacheEntities.empty())
    {
        return;
    }

    std::vector<UpdateCache> updateCaches;
    std::string certificateFileContent;

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

    updateSettings_.setLocalUpdateCacheHosts(updateCacheHosts);

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
        std::string certString = attributes.contents();
        // The UC certs are generated on a Windows machine, so we have to fix them up before using them.
        // Remove any xml escaped carriage returns
        certString = UtilityImpl::StringUtils::replaceAll(certString, "&#13;", "");
        // Replace any non-escaped carriage returns
        certString = UtilityImpl::StringUtils::replaceAll(certString, "\r", "");
        // Fix up double new line in certs. We don't want 2 lines between the cert data and "-----END CERTIFICATE-----"
        certString = UtilityImpl::StringUtils::replaceAll(certString, "\n\n", "\n");
        certificateFileContent += certString;
    }
    update_certificates_content_ = certificateFileContent;
}

void ALCPolicy::extractUpdateSchedule(const Common::XmlUtilities::AttributesMap& attributesMap)
{
    auto delayUpdating = attributesMap.lookup("AUConfigurations/AUConfig/delay_updating");
    std::string delayUpdatingDay = delayUpdating.value("Day");
    std::string delayUpdatingTime = delayUpdating.value("Time");

    if (!delayUpdatingDay.empty() && !delayUpdatingTime.empty())
    {
        std::string delayUpdatingDayAndTime = delayUpdatingDay + "," + delayUpdatingTime;
        std::tm scheduledUpdateTime{};
        if (strptime(delayUpdatingDayAndTime.c_str(), "%a,%H:%M:%S", &scheduledUpdateTime))
        {
            weeklySchedule_ = { .enabled = true,
                                .weekDay = scheduledUpdateTime.tm_wday,
                                .hour = scheduledUpdateTime.tm_hour,
                                .minute = scheduledUpdateTime.tm_min };
        }
        else
        {
            LOGERROR("Could not parse update schedule from policy, leaving update schedule as default");
        }
    }
}

void ALCPolicy::extractProxyDetails(const Common::XmlUtilities::AttributesMap& attributesMap)
{
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

        updateSettings_.setPolicyProxy(Proxy{ proxyAddress, ProxyCredentials{ proxyUser, proxyPassword, proxyType } });
    }
}

void ALCPolicy::extractFeatures(const Common::XmlUtilities::AttributesMap& attributesMap)
{
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

    updateSettings_.setFeatures(std::move(allFeatures));
}

void ALCPolicy::extractCloudSubscriptions(const Common::XmlUtilities::AttributesMap& attributesMap)
{
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
        std::string fixedVersion = subscriptionDetails.value("FixedVersion");

        ProductSubscription sub{ rigidName, subscriptionDetails.value("BaseVersion"), tag, fixedVersion };
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
        LOGERROR("SSPL base product name : " << SSPLBaseName << " not in the subscription of the policy.");
        throw PolicyParseException(LOCATION, "Invalid policy: No base subscription");
    }
}

void ALCPolicy::extractESMVersion(const Common::XmlUtilities::AttributesMap& attributesMap)
{
    auto tokenAttr = attributesMap.lookup("AUConfigurations/AUConfig/fixed_version/token");
    auto nameAttr = attributesMap.lookup("AUConfigurations/AUConfig/fixed_version/name");

    ESMVersion esmVersion(nameAttr.contents(), tokenAttr.contents());

    updateSettings_.setEsmVersion(std::move(esmVersion));
}

void ALCPolicy::extractPeriod(const Common::XmlUtilities::AttributesMap& attributesMap)
{
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

void ALCPolicy::logVersion()
{
    std::stringstream msg;
    auto esmVersion = updateSettings_.getEsmVersion();

    if (esmVersion.isEnabled())
    {
        msg << "Using FixedVersion " << esmVersion.name() << " with token " << esmVersion.token();
    }
    else
    {
        msg << "Using version " << updateSettings_.getPrimarySubscription().tag();
    }

    LOGINFO(msg.str());
}