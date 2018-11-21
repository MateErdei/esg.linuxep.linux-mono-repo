/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdatePolicyTranslator.h"
#include "../Logger.h"
#include <Common/XmlUtilities/AttributesMap.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/OSUtilities/IIPUtils.h>
#include <sstream>
#include <algorithm>
#include <regex>

namespace
{
    template<typename IpCollection>
    std::string ipCollectionToString(const IpCollection & ipCollection)
    {
        if ( ipCollection.empty())
        {
            return std::string{};
        }
        std::stringstream s;
        bool first = true;
        for( auto & ip : ipCollection)
        {
            if ( first )
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
        if( !ips.ip4collection.empty())
        {
            s << ipCollectionToString(ips.ip4collection);
            hasip4 = true;
        }

        if( !ips.ip6collection.empty())
        {
            if( hasip4)
            {
                s<<"\n";
            }
            s << ipCollectionToString(ips.ip6collection);
        }
        return s.str();
    }

    std::string reportToString( const Common::OSUtilities::SortServersReport & report)
    {
        std::stringstream s;
        s << "Local ips: \n"
          << ipToString(report.localIps) << "\n"
          << "Servers: \n";
        for( auto & server : report.servers)
        {
            s<< server.uri << '\n';
            if( !server.error.empty())
            {
                s << server.error << '\n';
            }
            else
            {
                s << ipToString(server.ips) << '\n';
            }
            s << "distance associated: ";
            if( server.MaxDistance == server.associatedMinDistance)
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
}

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

                m_Caches = sortUpdateCaches(m_Caches);
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

            return SettingsHolder{config, certificateFileContent, std::chrono::minutes(periodInt)};
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


        std::vector<UpdatePolicyTranslator::Cache> UpdatePolicyTranslator::sortUpdateCaches(const std::vector<UpdatePolicyTranslator::Cache>& caches)
        {
            // requirement: update caches candidates must be sorted by the following criteria:
            //  1. priority
            //  2. ip-proximity
            std::vector<std::string> cacheUrls;
            for(auto & candidate: m_Caches)
            {
                cacheUrls.emplace_back( Common::OSUtilities::tryExtractServerFromHttpURL(candidate.hostname));
            }

            auto start = std::chrono::steady_clock::now();
            Common::OSUtilities::SortServersReport report = Common::OSUtilities::indexOfSortedURIsByIPProximity(cacheUrls);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            std::string logReport = reportToString(report);
            // FIXME: uncomment the line below after LINUXEP-5910
            //LOGSUPPORT( "Sort update cache took " << elapsed << " ms. Entries: " << logReport);
            std::vector<int> sortedIndex = Common::OSUtilities::sortedIndexes(report);


            std::vector<Cache> orderedCaches;
            for( auto & entry : report.servers)
            {
                orderedCaches.emplace_back( m_Caches.at(entry.originalIndex) );
            }

            std::stable_sort(orderedCaches.begin(), orderedCaches.end(),
                             [](const Cache& a, const Cache& b) { return a.priority < b.priority; }
            );
            return orderedCaches;

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
