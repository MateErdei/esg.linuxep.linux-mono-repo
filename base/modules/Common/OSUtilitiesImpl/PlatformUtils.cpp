/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PlatformUtils.h"

#include "LocalIPImpl.h"

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "OSUtilities/IPlatformUtilsException.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <ctype.h>
#include <map>
#include <unistd.h>
#include <json.hpp>

namespace Common
{
    namespace OSUtilitiesImpl
    {
        PlatformUtils::PlatformUtils()
            : m_vendor("UNKNOWN")
            , m_osName("")
            , m_osMajorVersion("")
            , m_osMinorVersion("")
        {
            populateVendorDetails();
        }
        
        void PlatformUtils::populateVendorDetails()
        {
            const std::string lsbReleasePath = "/etc/lsb-release";

            const std::array<std::string, 5> distroCheckFiles = {
                lsbReleasePath,
                "/etc/issue",
                "/etc/centos-release",
                "/etc/redhat-release",
                "/etc/system-release"
            };

            std::map<std::string, std::string> distroNames = {
                std::make_pair("redhat", "redhat"),
                std::make_pair("ubuntu", "ubuntu"),
                std::make_pair("centos", "centos"),
                std::make_pair("amazonlinux", "amazon")
            };
            
            auto fs = FileSystem::fileSystem();
            for(auto& path : distroCheckFiles)
            {
                if(fs->isFile(path))
                {
                    std::string distro;
                    std::vector<std::string> fileContents = fs->readLines(path);
                    if(!fileContents.empty())
                    {
                        distro = fileContents[0];
                        distro = UtilityImpl::StringUtils::replaceAll(distro, " ", "");
                        distro = UtilityImpl::StringUtils::replaceAll(distro, "/", "_");
                        std::transform(distro.begin(), distro.end(), distro.begin(),
                                       [](unsigned char c){ return std::tolower(c); });
                        std::string tempDistro = distroNames[distro];
                        if(!tempDistro.empty())
                        {
                            m_vendor = tempDistro;
                            break;
                        }
                    }
                }
            }

            if(fs->isFile(lsbReleasePath))
            {
                m_osName = UtilityImpl::StringUtils::extractValueFromConfigFile(lsbReleasePath, "DISTRIB_DESCRIPTION");
                std::string version = UtilityImpl::StringUtils::extractValueFromConfigFile(lsbReleasePath, "DISTRIB_RELEASE");
                std::vector<std::string> majorAndMinor = UtilityImpl::StringUtils::splitString(version, ".");
                if(majorAndMinor.size() >= 2)
                {
                    m_osMajorVersion = majorAndMinor[0];
                    m_osMinorVersion = majorAndMinor[1];
                }
            }
        }

        std::string PlatformUtils::getHostname() const
        {
            return PlatformUtils::getUtsname().nodename;
        }

        std::string PlatformUtils::getPlatform() const
        {
            return PlatformUtils::getUtsname().sysname;
        }

        std::string PlatformUtils::getVendor() const
        {
            return m_vendor;
        }

        std::string PlatformUtils::getArchitecture() const
        {
            // example machine value is x86_64
            return PlatformUtils::getUtsname().machine;
        }

        std::string PlatformUtils::getOsName() const
        {
            return m_osName;
        }

        std::string PlatformUtils::getKernelVersion() const
        {
            return PlatformUtils::getUtsname().release;
        }

        std::string PlatformUtils::getOsMajorVersion() const
        {
            return m_osMajorVersion;
        }

        std::string PlatformUtils::getOsMinorVersion() const
        {
            return m_osMinorVersion;
        }

        std::string PlatformUtils::getDomainname() const
        {
            return "UNKNOWN";
        }

        std::string PlatformUtils::getIp4Address() const
        {
            Common::OSUtilitiesImpl::LocalIPImpl localIp;
            Common::OSUtilities::IPs ips = localIp.getLocalIPs();
            std::string ip4Address("");
            if (!ips.ip4collection.empty())
            {
                ip4Address = ips.ip4collection[0].stringAddress();
            }

            return ip4Address;
        }

        std::string PlatformUtils::getIp6Address() const
        {
            Common::OSUtilitiesImpl::LocalIPImpl localIp;
            Common::OSUtilities::IPs ips = localIp.getLocalIPs();
            std::string ip6Address("");

            if (!ips.ip6collection.empty())
            {
                ip6Address = ips.ip6collection[0].stringAddress();
                unsigned long pos = ip6Address.find("%");

                if(pos != std::string::npos)
                {
                    ip6Address = ip6Address.substr(0, pos);
                }
            }

            return ip6Address;
        }

        std::string PlatformUtils::getCloudPlatformMetadata(Common::HttpRequests::IHttpRequester* client) const
        {
            std::string metadata;
            metadata = PlatformUtils::getAwsMetadata(client);
            if(!metadata.empty()) return metadata;

            metadata = PlatformUtils::getGcpMetadata(client);
            if(!metadata.empty()) return metadata;

            metadata = PlatformUtils::getOracleMetadata(client);
            if(!metadata.empty()) return metadata;

            metadata = PlatformUtils::getAzureMetadata(client);
            return metadata;
        }

        utsname PlatformUtils::getUtsname() const
        {
            utsname platformInfo;
            errno = 0;
            if (uname(&platformInfo) < 0)
            {
                perror("uname");
                throw OSUtilities::IPlatformUtilsException("uname call failed: " + std::string(strerror(errno)));
            }
            return platformInfo;
        }

        std::string PlatformUtils::getAwsMetadata(Common::HttpRequests::IHttpRequester* client) const
        {
            std::string initialUrl = "http://169.254.169.254/latest/api/token";
            Common::HttpRequests::Headers initialHeaders({{"X-aws-ec2-metadata-token-ttl-seconds", "21600"}});

            Common::HttpRequests::Response response = client->put(buildCloudMetadataRequest(initialUrl, initialHeaders));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }

            std::string secondUrl = "http://169.254.169.254/latest/dynamic/instance-identity/document";
            Common::HttpRequests::Headers secondHeaders({{"X-aws-ec2-metadata-token", response.body}});
            response = client->put(buildCloudMetadataRequest(secondUrl, secondHeaders));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }

            nlohmann::json awsInfoJson = response.body;
            std::stringstream result;
            result  << "<aws>"
                    << "<region>" << awsInfoJson["region"].get<std::string>() << "</region>"
                    << "<accountId>" << awsInfoJson["accountId"].get<std::string>() << "</accountId>"
                    << "<instanceId>" << awsInfoJson["instanceId"].get<std::string>() << "</instanceId>"
                    << "</aws>";

            return result.str();
        }

        std::string PlatformUtils::getGcpMetadata(Common::HttpRequests::IHttpRequester* client) const
        {
            Common::HttpRequests::Headers headers({{"Metadata-Flavor", "Google"}});

            std::string idUrl = "http://metadata.google.internal/computeMetadata/v1/instance/id";
            Common::HttpRequests::Response response = client->put(buildCloudMetadataRequest(idUrl, headers));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }
            std::string id = response.body;

            std::string zoneUrl = "http://metadata.google.internal/computeMetadata/v1/instance/zone";
            response = client->put(buildCloudMetadataRequest(zoneUrl, headers));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }
            std::string zone = response.body;

            std::string hostnameUrl = "http://metadata.google.internal/computeMetadata/v1/instance/hostname";
            response = client->put(buildCloudMetadataRequest(hostnameUrl, headers));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }
            std::string hostname = response.body;

            std::stringstream result;
            result  << "<google>"
                    << "<hostname>" << hostname << "</hostname>"
                    << "<id>" << id << "</id>"
                    << "<zone>" << zone << "</zone>"
                    << "</google>";

            return result.str();
        }

        std::string PlatformUtils::getOracleMetadata(Common::HttpRequests::IHttpRequester* client) const
        {
            std::string url = "http://169.254.169.254/opc/v2/instance/";
            Common::HttpRequests::Headers headers({{"Authorization", "Bearer Oracle"}});
            Common::HttpRequests::Response response = client->put(buildCloudMetadataRequest(url, headers));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }

            nlohmann::json oracleInfoJson = response.body;
            std::stringstream result;
            result  << "<oracle>"
                    << "<region>" << oracleInfoJson["region"].get<std::string>() << "</region>"
                    << "<availabilityDomain>" << oracleInfoJson["availabilityDomain"].get<std::string>() << "</availabilityDomain>"
                    << "<compartmentId>" << oracleInfoJson["compartmentId"].get<std::string>() << "</compartmentId>"
                    << "<displayName>" << oracleInfoJson["displayName"].get<std::string>() << "</displayName>"
                    << "<hostname>" << oracleInfoJson["hostname"].get<std::string>() << "</hostname>"
                    << "<state>" << oracleInfoJson["state"].get<std::string>() << "</state>"
                    << "<instanceId>" << oracleInfoJson["instanceId"].get<std::string>() << "</instanceId>"
                    <<"</oracle>";

            return result.str();
        }

        std::string PlatformUtils::getAzureMetadata(Common::HttpRequests::IHttpRequester* client) const
        {
            std::string initialUrl = "http://169.254.169.254/metadata/versions";
            Common::HttpRequests::Headers headers({{"Metadata", "True"}}); // uncertain about this True
            Common::HttpRequests::Response response = client->put(buildCloudMetadataRequest(initialUrl, headers));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }

            nlohmann::json firstResponseBody = response.body;
            std::string latestAzureApiVersion = firstResponseBody["apiVersions"][-1];

            std::string secondUrl = "http://169.254.169.254/metadata/instance?api-version=" + latestAzureApiVersion;
            response = client->put(buildCloudMetadataRequest(secondUrl, headers));
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return "";
                }
                // Error with Curl gets us here, log what went wrong with response.error
                return "";
            }

            nlohmann::json azureInfoJson = response.body;
            std::stringstream result;
            result  << "<azure>"
                    << "<vmId>" << azureInfoJson["compute"]["vmId"].get<std::string>() << "</vmId>"
                    << "<vmName>" << azureInfoJson["compute"]["name"].get<std::string>() << "</vmName>"
                    << "<resourceGroupName>" << azureInfoJson["compute"]["resourceGroupName"].get<std::string>() << "</resourceGroupName>"
                    << "<subscriptionId>" << azureInfoJson["instanceId"]["subscriptionId"].get<std::string>() << "</subscriptionId>"
                    <<"</azure>";

            return result.str();
        }

        void PlatformUtils::setProxyConfig(std::map<std::string, std::string> proxyConfig)
        {
            m_proxyConfig = proxyConfig;
        }

        Common::HttpRequests::RequestConfig PlatformUtils::buildCloudMetadataRequest(std::string url, Common::HttpRequests::Headers headers) const
        {
            Common::HttpRequests::RequestConfig request;
            auto proxyConfig = m_proxyConfig;
            request.proxy = proxyConfig["proxy"];
            request.proxyUsername = proxyConfig["proxyUsername"];
            request.proxyPassword = proxyConfig["proxyPassword"];
            request.url = url;
            request.headers = headers;
            return request;
        }
    }
}