/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PlatformUtils.h"
#include "SystemUtils.h"
#include "MACinfo.h"
#include "LocalIPImpl.h"

#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "OSUtilities/IPlatformUtilsException.h"
#include "OSUtilitiesImpl/CloudMetadataConverters.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <json.hpp>
#include <map>


namespace Common::OSUtilitiesImpl
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
            ::OSUtilitiesImpl::SystemUtils systemUtils;
            std::string lsbReleasePath = systemUtils.getEnvironmentVariable("LSB_ETC_LSB_RELEASE");
            if (lsbReleasePath.empty())
            {
                lsbReleasePath = "/etc/lsb-release";
            }

            const std::array<std::string, 6> distroCheckFiles = {
                "/etc/issue",
                "/etc/centos-release",
                "/etc/oracle-release",
                "/etc/redhat-release",
                "/etc/system-release",
                "/etc/miraclelinux-release"
            };
            
            auto *fs = FileSystem::fileSystem();

            if (fs->isFile(lsbReleasePath))
            {
                std::string distro = extractDistroFromFile(lsbReleasePath);
                if (!distro.empty())
                {
                    m_vendor = distro;
                }
                m_osName = Common::UtilityImpl::StringUtils::replaceAll(
                    UtilityImpl::StringUtils::extractValueFromConfigFile(lsbReleasePath, "DISTRIB_DESCRIPTION"),
                    "\"", "");
                std::string version = UtilityImpl::StringUtils::extractValueFromConfigFile(lsbReleasePath, "DISTRIB_RELEASE");
                std::vector<std::string> majorAndMinor = UtilityImpl::StringUtils::splitString(version, ".");
                if (majorAndMinor.size() >= 2)
                {
                    m_osMajorVersion = majorAndMinor[0];
                    m_osMinorVersion = majorAndMinor[1];
                }
                return;
            }

            for (const auto& path : distroCheckFiles)
            {
                if (fs->isFile(path))
                {
                    std::string distro = extractDistroFromFile(path);
                    if (!distro.empty())
                    {
                        m_vendor = distro;
                        break;
                    }
                }
            }
        }

        std::string PlatformUtils::extractDistroFromFile(const std::string& filePath)
        {
            std::map<std::string, std::string> distroNames = {
                std::make_pair("redhat", "redhat"),
                std::make_pair("ubuntu", "ubuntu"),
                std::make_pair("centos", "centos"),
                std::make_pair("amazonlinux", "amazon"),
                std::make_pair("oracle", "oracle"),
                std::make_pair("miracle", "miracle")
            };

            auto *fs = FileSystem::fileSystem();
            std::string distro;
            std::vector<std::string> fileContents = fs->readLines(filePath);
            if (!fileContents.empty())
            {
                distro = fileContents[0];
                distro = UtilityImpl::StringUtils::replaceAll(distro, " ", "");
                distro = UtilityImpl::StringUtils::replaceAll(distro, "/", "_");

                std::vector<std::string> distroParts = Common::UtilityImpl::StringUtils::splitString(distro, "=");
                if (distroParts.size() ==2)
                {
                    distro = distroParts[1];
                }

                UtilityImpl::StringUtils::toLower(distro);

                for (const auto&[key, value] : distroNames)
                {
                    if (UtilityImpl::StringUtils::isSubstring(distro, key))
                    {
                        return value;
                    }
                }
            }
            return "";
        }

        std::string PlatformUtils::getHostname() const
        {
            return PlatformUtils::getUtsname().nodename;
        }

        std::string PlatformUtils::getPlatform() const
        {
            std::string value = PlatformUtils::getUtsname().sysname;
            UtilityImpl::StringUtils::toLower(value);
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

        std::string PlatformUtils::getFirstIpAddress(const std::vector<std::string>& ipAddresses) const
        {
            if (ipAddresses.empty())
            {
                return "";
            }
            return ipAddresses[0];
        }

        std::vector<std::string> PlatformUtils::getIp4Addresses(const std::vector<Common::OSUtilities::Interface>& interfaces) const
        {
            std::vector<std::string> ip4Addresses;

            for (const auto& interface : interfaces)
            {
                for (const auto& ip4: interface.ipAddresses.ip4collection)
                {
                    ip4Addresses.emplace_back(ip4.stringAddress());
                }
            }

            return ip4Addresses;
        }

        std::vector<std::string> PlatformUtils::getIp6Addresses(const std::vector<Common::OSUtilities::Interface>& interfaces) const
        {
            std::vector<std::string> ip6Addresses;

            for (const auto& interface : interfaces)
            {
                for (const auto& ip6: interface.ipAddresses.ip6collection)
                {
                    ip6Addresses.emplace_back(ip6.stringAddress());
                }
            }

            return ip6Addresses;
        }

        InterfaceCharacteristic PlatformUtils::getInterfaceCharacteristic(const Common::OSUtilities::Interface& interface)
        {
            if (Common::UtilityImpl::StringUtils::startswith(interface.name, "eth"))
            {
                return InterfaceCharacteristic::IS_ETH;
            }
            else if (Common::UtilityImpl::StringUtils::startswith(interface.name, "em"))
            {
                return InterfaceCharacteristic::IS_EM;
            }
            else if (Common::UtilityImpl::StringUtils::startswith(interface.name, "docker"))
            {
                return InterfaceCharacteristic::IS_DOCKER;
            }
            else
            {
                for (const auto& ip4: interface.ipAddresses.ip4collection)
                {
                    if (Common::UtilityImpl::StringUtils::startswith(ip4.stringAddress(), "172."))
                    {
                        return InterfaceCharacteristic::IS_PRIVATE;
                    }
                }

                return InterfaceCharacteristic::IS_OTHER;
            }
        }

        void PlatformUtils::sortInterfaces(std::vector<Common::OSUtilities::Interface>& interfaces) const
        {
            if (!interfaces.empty())
            {
                std::sort(interfaces.begin(), interfaces.end(), [](const Common::OSUtilities::Interface& lhs, const Common::OSUtilities::Interface& rhs)
                    {
                        return static_cast<int>(getInterfaceCharacteristic(lhs)) < static_cast<int>(getInterfaceCharacteristic(rhs));
                    });
            }
        }

        std::vector<std::string> PlatformUtils::getMacAddresses() const
        {
            return Common::OSUtilitiesImpl::sortedSystemMACs();
        }

        std::string PlatformUtils::getCloudPlatformMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
        {
            std::string metadata;
            metadata = PlatformUtils::getAwsMetadata(client);
            if (!metadata.empty()) { return metadata; }

            metadata = PlatformUtils::getGcpMetadata(client);
            if (!metadata.empty()) { return metadata; }

            metadata = PlatformUtils::getOracleMetadata(client);
            if (!metadata.empty()) { return metadata; }

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

        std::string PlatformUtils::getAwsMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
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

            return CloudMetadataConverters::parseAwsMetadataJson(response.body);
        }

        std::string PlatformUtils::getGcpMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
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

            std::map<std::string, std::string> metadataValues {
                {"id", id},
                {"zone", zone},
                {"hostname", hostname}
            };

            return CloudMetadataConverters::parseGcpMetadata(metadataValues);
        }

        std::string PlatformUtils::getOracleMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
        {
            std::string url = "http://169.254.169.254/opc/v2/instance/";
            Common::HttpRequests::Headers headers({{"Authorization", "Bearer Oracle"}});
            Common::HttpRequests::Response response = client->put(buildCloudMetadataRequest(url, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }

            return CloudMetadataConverters::parseOracleMetadataJson(response.body);
        }

        std::string PlatformUtils::getAzureMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
        {
            std::string initialUrl = "http://169.254.169.254/metadata/versions";
            Common::HttpRequests::Headers headers({{"Metadata", "True"}});
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

            return CloudMetadataConverters::parseAzureMetadataJson(response.body);
        }

        Common::HttpRequests::RequestConfig PlatformUtils::buildCloudMetadataRequest(const std::string& url, const Common::HttpRequests::Headers& headers) const
        {
            Common::HttpRequests::RequestConfig request;
            request.url = url;
            request.headers = headers;
            request.timeout = 1L;
            return request;
        }

        bool PlatformUtils::curlResponseIsOk200(const Common::HttpRequests::Response& response) const
        {
            if (response.errorCode == HttpRequests::OK)
            {
                if (response.status != 200)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
            return true;
        }
    }