// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "PlatformUtils.h"

#include "LocalIPImpl.h"
#include "MACinfo.h"
#include "SystemUtils.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/OSUtilities/IPlatformUtilsException.h"
#include "Common/OSUtilitiesImpl/CloudMetadataConverters.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <json.hpp>
#include <map>
#include <unistd.h>

namespace Common::OSUtilitiesImpl
{
    PlatformUtils::PlatformUtils() : m_vendor("UNKNOWN"), m_osName(""), m_osMajorVersion(""), m_osMinorVersion("")
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

        const std::array<std::string, 7> distroCheckFiles = { "/etc/issue",          "/etc/centos-release",
                                                              "/etc/oracle-release", "/etc/redhat-release",
                                                              "/etc/system-release", "/etc/miraclelinux-release",
                                                              "/etc/SUSE-brand" };

        auto* fs = FileSystem::fileSystem();

        if (fs->isFile(lsbReleasePath))
        {
            std::string distro = extractDistroFromFile(lsbReleasePath);
            if (!distro.empty())
            {
                m_vendor = distro;
            }
            m_osName = Common::UtilityImpl::StringUtils::replaceAll(
                UtilityImpl::StringUtils::extractValueFromConfigFile(lsbReleasePath, "DISTRIB_DESCRIPTION"), "\"", "");
            std::string version =
                UtilityImpl::StringUtils::extractValueFromConfigFile(lsbReleasePath, "DISTRIB_RELEASE");
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

        if (m_vendor == "UNKNOWN")
        {
            std::string distro = extractDistroFromOSFile();
            if (!distro.empty())
            {
                m_vendor = distro;
            }
        }
    }

    std::string PlatformUtils::extractDistroFromOSFile()
    {
        std::map<std::string, std::string> distroNames = {
            std::make_pair("redhat", "redhat"), std::make_pair("ubuntu", "ubuntu"),
            std::make_pair("centos", "centos"), std::make_pair("amazonlinux", "amazon"),
            std::make_pair("oracle", "oracle"), std::make_pair("miracle", "miracle"),
            std::make_pair("suse", "suse")
        };

        auto* fs = FileSystem::fileSystem();
        std::string distro;
        std::string path = "/etc/os-release";
        if (fs->isFile(path))
        {
            std::string pretty_name = UtilityImpl::StringUtils::extractValueFromConfigFile(path, "PRETTY_NAME");
            if (!pretty_name.empty())
            {
                pretty_name = UtilityImpl::StringUtils::replaceAll(pretty_name, " ", "");
                pretty_name = UtilityImpl::StringUtils::replaceAll(pretty_name, "/", "_");

                UtilityImpl::StringUtils::toLower(pretty_name);

                for (const auto& [key, value] : distroNames)
                {
                    if (UtilityImpl::StringUtils::isSubstring(pretty_name, key))
                    {
                        return value;
                    }
                }
            }
        }
        return "";
    }

    std::string PlatformUtils::extractDistroFromFile(const std::string& filePath)
    {
        std::map<std::string, std::string> distroNames = {
            std::make_pair("redhat", "redhat"), std::make_pair("ubuntu", "ubuntu"),
            std::make_pair("centos", "centos"), std::make_pair("amazonlinux", "amazon"),
            std::make_pair("oracle", "oracle"), std::make_pair("miracle", "miracle"),
            std::make_pair("sle", "suse")
        };

        auto* fs = FileSystem::fileSystem();
        std::string distro;
        std::vector<std::string> fileContents = fs->readLines(filePath);
        if (!fileContents.empty())
        {
            distro = fileContents[0];
            distro = UtilityImpl::StringUtils::replaceAll(distro, " ", "");
            distro = UtilityImpl::StringUtils::replaceAll(distro, "/", "_");

            std::vector<std::string> distroParts = Common::UtilityImpl::StringUtils::splitString(distro, "=");
            if (distroParts.size() == 2)
            {
                distro = distroParts[1];
            }

            UtilityImpl::StringUtils::toLower(distro);

            for (const auto& [key, value] : distroNames)
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

    static std::string gethostnameWrapper()
    {
        std::vector<char> hostname(1024);
        while (hostname.size() < 1024 * 1024)
        {
            hostname[hostname.size() - 1] = 0;
            auto result = ::gethostname(hostname.data(), hostname.size() - 1);
            int error = errno;
            if (result == 0)
            {
                return { hostname.data() };
            }
            else if (error != ENAMETOOLONG)
            {
                throw std::runtime_error("gethostname failed with unexpected error: " + std::to_string(error));
            }

            hostname.resize(hostname.size() * 2);
        }
        throw std::runtime_error("hostname is too long");
    }

    std::string PlatformUtils::getFQDN() const
    {
        auto hostname = gethostnameWrapper();
        struct addrinfo hints
        {
        };
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_CANONNAME;

        struct addrinfo* res = nullptr;
        std::string result;
        if (::getaddrinfo(hostname.c_str(), 0, &hints, &res) == 0)
        {
            // The hostname was successfully resolved.
            result = res->ai_canonname;
            freeaddrinfo(res);
        }
        else
        {
            // Not resolved, so fall back to hostname returned by OS.
            result = hostname;
        }

        if (result.find('.') != std::string::npos)
        {
            return result;
        }

        // Backup if SOPHOS_HOSTNAME_F environment variable is set
        const char* SOPHOS_HOSTNAME_F = ::getenv("SOPHOS_HOSTNAME_F");
        if (SOPHOS_HOSTNAME_F != nullptr)
        {
            auto* fs = FileSystem::fileSystem();
            if (fs->isFile(SOPHOS_HOSTNAME_F))
            {
                auto contents = fs->readFile(SOPHOS_HOSTNAME_F);
                if (contents.find('.') != std::string::npos)
                {
                    return contents;
                }
            }
        }

        // Fall back to getaddrinfo if hostname -f didn't find something better
        return result;
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

    std::vector<std::string> PlatformUtils::getIp4Addresses(
        const std::vector<Common::OSUtilities::Interface>& interfaces) const
    {
        std::vector<std::string> ip4Addresses;

        for (const auto& interface : interfaces)
        {
            for (const auto& ip4 : interface.ipAddresses.ip4collection)
            {
                ip4Addresses.emplace_back(ip4.stringAddress());
            }
        }

        return ip4Addresses;
    }

    std::vector<std::string> PlatformUtils::getIp6Addresses(
        const std::vector<Common::OSUtilities::Interface>& interfaces) const
    {
        std::vector<std::string> ip6Addresses;

        for (const auto& interface : interfaces)
        {
            for (const auto& ip6 : interface.ipAddresses.ip6collection)
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
            for (const auto& ip4 : interface.ipAddresses.ip4collection)
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
            std::sort(
                interfaces.begin(),
                interfaces.end(),
                [](const Common::OSUtilities::Interface& lhs, const Common::OSUtilities::Interface& rhs) {
                    return static_cast<int>(getInterfaceCharacteristic(lhs)) <
                           static_cast<int>(getInterfaceCharacteristic(rhs));
                });
        }
    }

    std::vector<std::string> PlatformUtils::getMacAddresses() const
    {
        try
        {
            return Common::OSUtilitiesImpl::sortedSystemMACs();
        }
        catch (const std::exception&)
        {
            return {};
        }
    }

    std::string PlatformUtils::getCloudPlatformMetadata(
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
    {
        std::string metadata;
        metadata = PlatformUtils::getAwsMetadata(client);
        if (!metadata.empty())
        {
            return metadata;
        }

        metadata = PlatformUtils::getGcpMetadata(client);
        if (!metadata.empty())
        {
            return metadata;
        }

        metadata = PlatformUtils::getOracleMetadata(client);
        if (!metadata.empty())
        {
            return metadata;
        }

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

    std::string PlatformUtils::getAwsMetadata(const std::shared_ptr<Common::HttpRequests::IHttpRequester>& client) const
    {
        std::string initialUrl = "http://169.254.169.254/latest/api/token";
        Common::HttpRequests::Headers initialHeaders({ { "X-aws-ec2-metadata-token-ttl-seconds", "21600" } });
        Common::HttpRequests::Response response;
        try
        {
            response = client->put(buildCloudMetadataRequest(initialUrl, initialHeaders));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            std::string secondUrl = "http://169.254.169.254/latest/dynamic/instance-identity/document";
            Common::HttpRequests::Headers secondHeaders({ { "X-aws-ec2-metadata-token", response.body } });
            response = client->get(buildCloudMetadataRequest(secondUrl, secondHeaders));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            return CloudMetadataConverters::parseAwsMetadataJson(response.body);
        }
        catch (const std::exception& ex)
        {
            return "";
        }
    }

    std::string PlatformUtils::getGcpMetadata(const std::shared_ptr<Common::HttpRequests::IHttpRequester>& client) const
    {
        Common::HttpRequests::Headers headers({ { "Metadata-Flavor", "Google" } });
        std::string idUrl = "http://metadata.google.internal/computeMetadata/v1/instance/id";
        Common::HttpRequests::Response response;
        try
        {
            response = client->get(buildCloudMetadataRequest(idUrl, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            if (!CloudMetadataConverters::verifyGoogleId(response.body))
            {
                return "";
            }
            std::string id = response.body;

            std::string zoneUrl = "http://metadata.google.internal/computeMetadata/v1/instance/zone";
            response = client->get(buildCloudMetadataRequest(zoneUrl, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            std::string zone = response.body;

            std::string hostnameUrl = "http://metadata.google.internal/computeMetadata/v1/instance/hostname";
            response = client->get(buildCloudMetadataRequest(hostnameUrl, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            std::string hostname = response.body;

            std::map<std::string, std::string> metadataValues{ { "id", id },
                                                               { "zone", zone },
                                                               { "hostname", hostname } };
            return CloudMetadataConverters::parseGcpMetadata(metadataValues);
        }
        catch (const std::exception& ex)
        {
            return "";
        }
    }

    std::string PlatformUtils::getOracleMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
    {
        std::string url = "http://169.254.169.254/opc/v2/instance/";
        Common::HttpRequests::Headers headers({ { "Authorization", "Bearer Oracle" } });
        Common::HttpRequests::Response response;
        try
        {
            response = client->get(buildCloudMetadataRequest(url, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            return CloudMetadataConverters::parseOracleMetadataJson(response.body);
        }
        catch (const std::exception& ex)
        {
            return "";
        }
    }

    std::string PlatformUtils::getAzureMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const
    {
        std::string initialUrl = "http://169.254.169.254/metadata/versions";
        Common::HttpRequests::Headers headers({ { "Metadata", "True" } });
        Common::HttpRequests::Response response;
        try
        {
            response = client->get(buildCloudMetadataRequest(initialUrl, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            nlohmann::json firstResponseBody = nlohmann::json::parse(response.body);
            std::string latestAzureApiVersion = firstResponseBody["apiVersions"][-1];
            std::string secondUrl = "http://169.254.169.254/metadata/instance?api-version=" + latestAzureApiVersion;
            response = client->get(buildCloudMetadataRequest(secondUrl, headers));
            if (!curlResponseIsOk200(response))
            {
                return "";
            }
            return CloudMetadataConverters::parseAzureMetadataJson(response.body);
        }
        catch (const std::exception& ex)
        {
            return "";
        }
    }

    Common::HttpRequests::RequestConfig PlatformUtils::buildCloudMetadataRequest(
        const std::string& url,
        const Common::HttpRequests::Headers& headers) const
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
} // namespace Common::OSUtilitiesImpl