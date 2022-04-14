/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "OSUtilities/IPlatformUtils.h"
#include <Common/HttpRequests/IHttpRequester.h>
#include <sys/utsname.h>

namespace Common
{
    namespace OSUtilitiesImpl
    {
        class PlatformUtils : public Common::OSUtilities::IPlatformUtils
        {
        public:
            PlatformUtils();
            ~ PlatformUtils() = default;

            [[nodiscard]] std::string getHostname() const override;
            [[nodiscard]] std::string getPlatform() const override;
            [[nodiscard]] std::string getVendor() const override;
            [[nodiscard]] std::string getArchitecture() const override;
            [[nodiscard]] std::string getOsName() const override;
            [[nodiscard]] std::string getKernelVersion() const override;
            [[nodiscard]] std::string getOsMajorVersion() const override;
            [[nodiscard]] std::string getOsMinorVersion() const override;
            [[nodiscard]] std::string getDomainname() const override;
            [[nodiscard]] std::string getIp4Address() const override;
            [[nodiscard]] std::vector<std::string> getIp4Addresses() const override;
            [[nodiscard]] std::string getIp6Address() const override;
            [[nodiscard]] std::vector<std::string> getIp6Addresses() const override;
            [[nodiscard]] std::string getCloudPlatformMetadata(Common::HttpRequests::IHttpRequester* client) const override;
            [[nodiscard]] std::vector<std::string> getMacAddresses() const override;

        private:
            /**
             * Gets the basic platform information
             * @return a utsname struct containing platform information
             */
            [[nodiscard]] utsname getUtsname() const;
            void populateVendorDetails();
            [[nodiscard]] static std::string extractDistroFromFile(const std::string& filePath);
            [[nodiscard]] std::string getAwsMetadata(Common::HttpRequests::IHttpRequester* client) const;
            [[nodiscard]] std::string getGcpMetadata(Common::HttpRequests::IHttpRequester* client) const;
            [[nodiscard]] std::string getOracleMetadata(Common::HttpRequests::IHttpRequester* client) const;
            [[nodiscard]] std::string getAzureMetadata(Common::HttpRequests::IHttpRequester* client) const;
            [[nodiscard]] Common::HttpRequests::RequestConfig buildCloudMetadataRequest(std::string url, Common::HttpRequests::Headers headers) const;
            [[nodiscard]] bool curlResponseIsOk200(Common::HttpRequests::Response& response) const;

            std::string m_vendor;
            std::string m_osName;
            std::string m_osMajorVersion;
            std::string m_osMinorVersion;
        };

    } // namespace OSUtilitiesImpl
} // namespace Common