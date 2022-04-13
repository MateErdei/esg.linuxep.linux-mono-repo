/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PlatformUtils.h"
#include "MACinfo.h"
#include "LocalIPImpl.h"

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "OSUtilities/IPlatformUtilsException.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <ctype.h>
#include <iostream>
#include <json.hpp>
#include <map>
#include <unistd.h>

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

            const std::array<std::string, 7> distroCheckFiles = {
                lsbReleasePath,
                "/etc/issue",
                "/etc/centos-release",
                "/etc/oracle-release",
                "/etc/redhat-release",
                "/etc/system-release",
                "/etc/miraclelinux-release"
            };

            std::map<std::string, std::string> distroNames = {
                std::make_pair("redhat", "redhat"),
                std::make_pair("ubuntu", "ubuntu"),
                std::make_pair("centos", "centos"),
                std::make_pair("amazonlinux", "amazon"),
                std::make_pair("oracle", "oracle"),
                std::make_pair("miracle", "miracle")
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

                        std::vector<std::string> distroParts = Common::UtilityImpl::StringUtils::splitString(distro, "=");
                        if(distroParts.size() ==2)
                        {
                            distro = distroParts[1];
                        }

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
                m_osName = Common::UtilityImpl::StringUtils::replaceAll(
                    UtilityImpl::StringUtils::extractValueFromConfigFile(lsbReleasePath, "DISTRIB_DESCRIPTION"),
                    "\"", "");
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
            std::string value = PlatformUtils::getUtsname().sysname;
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            return value;
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
            std::vector<std::string> ip4Addresses = getIp4Addresses();
            if(ip4Addresses.empty())
            {
                return "";
            }
            return ip4Addresses[0];
        }

        std::vector<std::string> PlatformUtils::getIp4Addresses() const
        {
            Common::OSUtilitiesImpl::LocalIPImpl localIp;
            Common::OSUtilities::IPs ips = localIp.getLocalIPs();
            std::vector<std::string> ip4Addresses;
            if (!ips.ip4collection.empty())
            {
                for(auto ipAddress : ips.ip4collection)
                {
                    ip4Addresses.push_back(ipAddress.stringAddress());
                }
            }
            return ip4Addresses;
        }

        std::string PlatformUtils::getIp6Address() const
        {
            std::vector<std::string> ip6Addresses = getIp6Addresses();
            if(ip6Addresses.empty())
            {
                return "";
            }
            return ip6Addresses[0];
        }

        std::vector<std::string> PlatformUtils::getIp6Addresses() const
        {
            Common::OSUtilitiesImpl::LocalIPImpl localIp;
            Common::OSUtilities::IPs ips = localIp.getLocalIPs();
            std::vector<std::string> ip6Addresses;
            if (!ips.ip6collection.empty())
            {
                for(auto ipAddress : ips.ip6collection)
                {
                    ip6Addresses.push_back(ipAddress.stringAddress());
                }
            }
            return ip6Addresses;
        }

        std::vector<std::string> PlatformUtils::getMacAddresses() const
        {
            return Common::OSUtilitiesImpl::sortedSystemMACs();
        }

        std::string PlatformUtils::getCloudPlatformMetadata(Common::HttpRequests::IHttpRequester* client) const
        {
            std::string metadata;
            metadata = PlatformUtils::getAwsMetadata(client);
            if(!metadata.empty()) { return metadata; }

            metadata = PlatformUtils::getGcpMetadata(client);
            if(!metadata.empty()) { return metadata; }

            metadata = PlatformUtils::getOracleMetadata(client);
            if(!metadata.empty()) { return metadata; }

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
            if (!curlResponseIsOk200(response))
            {
                return "";
            }

            std::string secondUrl = "http://169.254.169.254/latest/dynamic/instance-identity/document";
            Common::HttpRequests::Headers secondHeaders({{"X-aws-ec2-metadata-token", response.body}});
            response = client->put(buildCloudMetadataRequest(secondUrl, secondHeaders));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }

            nlohmann::json awsInfoJson = nlohmann::json::parse(response.body);
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
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            std::string id = response.body;

            std::string zoneUrl = "http://metadata.google.internal/computeMetadata/v1/instance/zone";
            response = client->put(buildCloudMetadataRequest(zoneUrl, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            std::string zone = response.body;

            std::string hostnameUrl = "http://metadata.google.internal/computeMetadata/v1/instance/hostname";
            response = client->put(buildCloudMetadataRequest(hostnameUrl, headers));
            if (!curlResponseIsOk200(response))
            {
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
            if (!curlResponseIsOk200(response))
            {
                return "";
            }

            nlohmann::json oracleInfoJson = nlohmann::json::parse(response.body);
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
            if (!curlResponseIsOk200(response))
            {
                return "";
            }

            nlohmann::json firstResponseBody = nlohmann::json::parse(response.body);
            std::string latestAzureApiVersion = firstResponseBody["apiVersions"][-1];

            std::string secondUrl = "http://169.254.169.254/metadata/instance?api-version=" + latestAzureApiVersion;
            response = client->put(buildCloudMetadataRequest(secondUrl, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }

            nlohmann::json azureInfoJson = nlohmann::json::parse(response.body);
            std::stringstream result;
            result  << "<azure>"
                    << "<vmId>" << azureInfoJson["compute"]["vmId"].get<std::string>() << "</vmId>"
                    << "<vmName>" << azureInfoJson["compute"]["name"].get<std::string>() << "</vmName>"
                    << "<resourceGroupName>" << azureInfoJson["compute"]["resourceGroupName"].get<std::string>() << "</resourceGroupName>"
                    << "<subscriptionId>" << azureInfoJson["instanceId"]["subscriptionId"].get<std::string>() << "</subscriptionId>"
                    <<"</azure>";

            return result.str();
        }

        Common::HttpRequests::RequestConfig PlatformUtils::buildCloudMetadataRequest(std::string url, Common::HttpRequests::Headers headers) const
        {
            Common::HttpRequests::RequestConfig request;
            request.url = url;
            request.headers = headers;
            request.timeout = 1L;
            return request;
        }

        bool PlatformUtils::curlResponseIsOk200(Common::HttpRequests::Response& response) const
        {
            if (response.errorCode == HttpRequests::OK)
            {
                if(response.status != 200)
                {
                    return false;
                }
            }
            else
            {
                // Error with Curl gets us here, log what went wrong with response.error
                return false;
            }
            return true;
        }
    }
}