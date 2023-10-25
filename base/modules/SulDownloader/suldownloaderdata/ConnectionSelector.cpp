// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ConnectionSelector.h"

#include "ConfigurationData.h"
#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/StringUtils.h"

using namespace Common::Policy;
using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

namespace
{
    bool isValidChar(char c)
    {
        if (std::isalnum(c))
        {
            return true;
        }
        switch (c)
        {
            case L'.':
            case L'-':
            case L'_':
                return true;
            default:
                return false;
        }
    }
} // namespace

bool ConnectionSelector::validateUrl(const std::string& url)
{
    std::string strippedHostname;
    strippedHostname = Common::UtilityImpl::StringUtils::replaceAll(url, "https://", "");
    strippedHostname = Common::UtilityImpl::StringUtils::replaceAll(strippedHostname, "http://", "");
    strippedHostname = Common::UtilityImpl::StringUtils::splitString(strippedHostname, "/")[0];
    strippedHostname = Common::UtilityImpl::StringUtils::splitString(strippedHostname, ":")[0];
    if (strippedHostname.empty() || strippedHostname.size() > 1024)
    {
        LOGWARN("Hostname is empty in ALC policy");
        return false;
    }

    if (std::find_if_not(std::cbegin(strippedHostname), std::cend(strippedHostname), isValidChar) !=
        std::cend(strippedHostname))
    {
        return false;
    }
    return true;
}

std::vector<ConnectionSetup> ConnectionSelector::getSDDS3Candidates(
    std::vector<Proxy>& proxies,
    std::vector<std::string>& urls,
    const std::string& key,
    std::vector<std::string>& caches)
{
    std::vector<ConnectionSetup> candidates;
    std::vector<std::string> validUrls = {};

    // Requirement: With update cache no proxy url must be given but the credentials are still necessary.
    // if the proxy is set then, then we only pass the credentials data for the proxy to the update cache proxy
    // settings. If no credentials are required for proxy then empty strings are passed  - this is ok.
    Proxy proxyForUpdateCache("noproxy:", proxies[0].getCredentials());

    std::string sdds3OverrideSettingsFile =
        Common::ApplicationConfiguration::applicationPathManager().getSdds3OverrideSettingsFile();
    std::string overrideValue;
    if (Common::FileSystem::fileSystem()->isFile(sdds3OverrideSettingsFile))
    {
        overrideValue = Common::UtilityImpl::StringUtils::extractValueFromIniFile(sdds3OverrideSettingsFile, key);
    }

    if (!overrideValue.empty())
    {
        LOGWARN("Overriding Sophos Update Service address list with " << overrideValue);
        urls = Common::UtilityImpl::StringUtils::splitString(overrideValue, ",");
    }

    for (const auto& url : urls)
    {
        if (validateUrl(url))
        {
            validUrls.emplace_back(url);
        }
        else
        {
            LOGWARN("Discarding url: " << url << " as a connectionCandiate as it failed validation");
        }
    }

    for (const auto& cache : caches)
    {
        if (key == "CDN_URL")
        {
            candidates.emplace_back(cache, true, proxyForUpdateCache);
            LOGDEBUG("Adding connection candidate, Update cache: " << cache);
        }
        else
        {
            for (const auto& url : validUrls)
            {
                Proxy messageRelay{ cache };
                candidates.emplace_back(url, false, messageRelay);
                LOGDEBUG("Adding connection candidate, URL: " << url << ", Message Relay: " << messageRelay.getUrl());
            }
        }
    }

    for (auto& proxy : proxies)
    {
        for (const auto& url : validUrls)
        {
            candidates.emplace_back(url, false, proxy);
            LOGDEBUG("Adding connection candidate, URL: " << url << ", proxy: " << proxy.getUrl());
        }
    }

    return candidates;
}

std::pair<std::vector<ConnectionSetup>, std::vector<ConnectionSetup>> ConnectionSelector::getConnectionCandidates(
    const UpdateSettings& updateSettings)
{
    std::vector<Proxy> proxies = SulDownloader::suldownloaderdata::ConfigurationData::proxiesList(updateSettings);
    std::vector<std::string> UpdateCaches = updateSettings.getLocalUpdateCacheHosts();
    std::vector<std::string> MessageRelays = updateSettings.getLocalMessageRelayHosts();

    std::vector<std::string> susUrl = { updateSettings.getSophosSusURL() };
    std::vector<std::string> cdnUrls = updateSettings.getSophosCDNURLs();

    std::vector<ConnectionSetup> susCandidates = getSDDS3Candidates(proxies, susUrl, "URLS", MessageRelays);
    std::vector<ConnectionSetup> cdnCandidates = getSDDS3Candidates(proxies, cdnUrls, "CDN_URL", UpdateCaches);

    return std::make_pair(susCandidates, cdnCandidates);
}