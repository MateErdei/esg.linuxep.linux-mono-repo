/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdatePolicyTranslator.h"
#include "../Logger.h"
#include <Common/XmlUtilities/AttributesMap.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>

#include <algorithm>
#include <regex>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {

        using namespace Common::XmlUtilities;
        using namespace Common::ApplicationConfiguration;

        SettingsHolder UpdatePolicyTranslator::translatePolicy(const std::string& policyXml)
        {
            m_Caches.clear();


            Common::XmlUtilities::AttributesMap attributesMap = parseXml(policyXml);

            auto cscComp = attributesMap.lookup("AUConfigurations/csc:Comp");
            if (cscComp.value("policyType") != "1")
            {
                throw std::runtime_error("Update Policy type incorrect");
            }
            m_revID = cscComp.value("RevID");

            auto primaryLocation = attributesMap.lookup("AUConfigurations/AUConfig/primary_location/server");

            std::string connectionAddress = primaryLocation.value("ConnectionAddress");
            std::vector<std::string> defaultLocations{SulDownloader::suldownloaderdata::ConfigurationData::DefaultSophosLocationsURL};
            if (!connectionAddress.empty())
            {
                defaultLocations.insert(begin(defaultLocations), connectionAddress);
            }


            SulDownloader::suldownloaderdata::ConfigurationData config{defaultLocations};
            config.setCredentials(
                    SulDownloader::suldownloaderdata::Credentials{primaryLocation.value("UserName"), primaryLocation.value("UserPassword")}
            );


            auto updateCacheEntities = attributesMap.entitiesThatContainPath(
                    "AUConfigurations/update_cache/locations/location"
            );

            std::string certificateFileContent;
            if (!updateCacheEntities.empty())
            {
                for (auto& updateCache : updateCacheEntities)
                {
                    auto attributes = attributesMap.lookup(updateCache);
                    std::string hostname = attributes.value("hostname");
                    std::string priority = attributes.value("priority");
                    std::string id = attributes.value("id");
                    m_Caches.emplace_back(Cache{hostname, priority, id});
                }

                std::stable_sort(m_Caches.begin(), m_Caches.end(),
                                 [](const Cache& a, const Cache& b) { return a.priority < b.priority; }
                );

                std::vector<std::string> updateCaches;
                for (auto& cache : m_Caches)
                {
                    updateCaches.emplace_back(cache.hostname);
                }

                config.setLocalUpdateCacheUrls(updateCaches);

                auto cacheCertificates = attributesMap.entitiesThatContainPath(
                        "AUConfigurations/update_cache/intermediate_certificates/intermediate_certificate"
                );

                for (auto& certificate : cacheCertificates)
                {
                    auto attributes = attributesMap.lookup(certificate);
                    // Remove line endings from certificate
                    if (!certificateFileContent.empty())
                    {
                        certificateFileContent += "\n";
                    }
                    certificateFileContent += Common::UtilityImpl::StringUtils::replaceAll(
                            attributes.value(attributes.TextId), "&#13;", ""
                    );
                }
                std::string cacheCertificatePath = applicationPathManager().getUpdateCacheCertificateFilePath();
                config.setUpdateCacheSslCertificatePath(cacheCertificatePath);
            }

            ScheduledUpdate scheduledUpdate;

            auto delayUpdating = attributesMap.lookup("AUConfigurations/AUConfig/delay_updating");
            std::string delayUpdatingDay = delayUpdating.value("Day");
            std::string delayUpdatingTime = delayUpdating.value("Time");
            if (!delayUpdatingDay.empty() && !delayUpdatingDay.empty())
            {
                std::string delayUpdatingDayAndTime = delayUpdatingDay + "," + delayUpdatingTime;
                std::tm scheduledUpdateTime;
                if(strptime(delayUpdatingDayAndTime.c_str(), "%a,%H:%M:%S", &scheduledUpdateTime))
                {
                    scheduledUpdate.setScheduledTime(scheduledUpdateTime);
                    scheduledUpdate.setEnabled(true);
                }
            }

            auto primaryProxy = attributesMap.lookup("AUConfigurations/AUConfig/primary_location/proxy");
            std::string proxyAddress = primaryProxy.value("ProxyAddress");
            if (!proxyAddress.empty())
            {
                //      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
                std::string proxyPort = primaryProxy.value("ProxyPortNumber");
                std::string proxyUser = primaryProxy.value("ProxyUserName");
                std::string proxyPassword = primaryProxy.value("ProxyUserPassword");
                std::string proxyType = primaryProxy.value("ProxyType");
                if (!proxyPort.empty())
                {
                    proxyAddress += ":" + proxyPort;
                }

                config.setPolicyProxy(
                        SulDownloader::suldownloaderdata::Proxy{proxyAddress
                                                                , SulDownloader::suldownloaderdata::ProxyCredentials{
                                        proxyUser, proxyPassword, proxyType}}
                );
            }

            config.setCertificatePath(applicationPathManager().getUpdateCertificatesPath());
            config.setInstallationRootPath(applicationPathManager().sophosInstall());
            config.setSystemSslCertificatePath(":system:");

//        struct ProductGUID
//        {
//            std::string Name;
//            bool Primary;
//            bool Prefix;
//            std::string releaseTag;
//            std::string baseVersion;
//        };
            config.addProductSelection({"ServerProtectionLinux-Base", true, false, "RECOMMENDED", "0"});
            config.addProductSelection({"ServerProtectionLinux-Plugin", false, true, "RECOMMENDED", "0"});


            config.setInstallArguments({"--instdir", applicationPathManager().sophosInstall()});
            config.setLogLevel(SulDownloader::suldownloaderdata::ConfigurationData::LogLevel::VERBOSE);

            std::string period = attributesMap.lookup("AUConfigurations/AUConfig/schedule").value("Frequency");
            int periodInt = 60;
            if (!period.empty())
            {
                periodInt = std::stoi(period);
            }

            return SettingsHolder{config, certificateFileContent, std::chrono::minutes(periodInt), scheduledUpdate};
        }

        std::string UpdatePolicyTranslator::cacheID(const std::string& hostname) const
        {
            for (auto& cache : m_Caches)
            {
                if (cache.hostname == hostname)
                {
                    return cache.id;
                }
            }
            // Could not find the cache
            return std::string();
        }

        std::string UpdatePolicyTranslator::revID() const
        {
            return m_revID;
        }


        void PolicyValidationException::validateOrThrow(SettingsHolder& settingsHolder)
        {
            if( !settingsHolder.configurationData.verifySettingsAreValid())
            {
                LOGWARN("Configuration for the connection to the warehouse is invalid. Can not be used. ");
                throw PolicyValidationException( "Invalid ConfigurationData");
            }

            long updatePeriod = settingsHolder.schedulerPeriod.count();
            constexpr long year = 365 * 24 * 60;
            if( updatePeriod < 5 || updatePeriod > year)
            {
                throw PolicyValidationException( "Invalid update period given. It must be between 5 minutes and an year.");
            }
        }
    }
}
