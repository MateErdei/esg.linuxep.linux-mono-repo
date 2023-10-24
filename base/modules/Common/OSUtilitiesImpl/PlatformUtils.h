// Copyright 2022-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/HttpRequests/IHttpRequester.h"
#include "Common/OSUtilities/IIPUtils.h"
#include "Common/OSUtilities/IPlatformUtils.h"

#include <sys/utsname.h>

namespace Common::OSUtilitiesImpl
{
    // Ordering is important, this defines the order of the interface sorting
    enum class InterfaceCharacteristic
    {
        IS_ETH,
        IS_EM,
        IS_OTHER,
        IS_PRIVATE,
        IS_DOCKER
    };

    class PlatformUtils : public Common::OSUtilities::IPlatformUtils
    {
    public:
        PlatformUtils();
        ~PlatformUtils() = default;

        [[nodiscard]] std::string getHostname() const override;
        [[nodiscard]] std::string getFQDN() const override;
        [[nodiscard]] std::string getPlatform() const override;
        [[nodiscard]] std::string getVendor() const override;
        [[nodiscard]] std::string getArchitecture() const override;
        [[nodiscard]] std::string getOsName() const override;
        [[nodiscard]] std::string getKernelVersion() const override;
        [[nodiscard]] std::string getOsMajorVersion() const override;
        [[nodiscard]] std::string getOsMinorVersion() const override;
        [[nodiscard]] std::string getDomainname() const override;
        [[nodiscard]] std::string getFirstIpAddress(const std::vector<std::string>& ipAddresses) const override;
        [[nodiscard]] std::vector<std::string> getIp4Addresses(
            const std::vector<Common::OSUtilities::Interface>& interfaces) const override;
        [[nodiscard]] std::vector<std::string> getIp6Addresses(
            const std::vector<Common::OSUtilities::Interface>& interfaces) const override;
        void sortInterfaces(std::vector<Common::OSUtilities::Interface>& interfaces) const override;
        [[nodiscard]] std::string getCloudPlatformMetadata(
            std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const override;
        [[nodiscard]] std::vector<std::string> getMacAddresses() const override;

    private:
        /**
         * Gets the basic platform information
         * @return a utsname struct containing platform information
         */
        [[nodiscard]] utsname getUtsname() const;
        void populateVendorDetails();
        std::string getValueFromOSReleaseFile(const std::string& osReleasePath, const std::string& key);
        void checkValueLengthAndTruncate(std::string& value);
        [[nodiscard]] static InterfaceCharacteristic getInterfaceCharacteristic(
            const Common::OSUtilities::Interface& interface);
        [[nodiscard]] std::string getAwsMetadata(
            const std::shared_ptr<Common::HttpRequests::IHttpRequester>& client) const;
        [[nodiscard]] std::string getGcpMetadata(
            const std::shared_ptr<Common::HttpRequests::IHttpRequester>& client) const;
        [[nodiscard]] std::string getOracleMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const;
        [[nodiscard]] std::string getAzureMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const;
        [[nodiscard]] Common::HttpRequests::RequestConfig buildCloudMetadataRequest(
            const std::string& url,
            const Common::HttpRequests::Headers& headers) const;
        [[nodiscard]] bool curlResponseIsOk200(const Common::HttpRequests::Response& response) const;

        std::string m_vendor;
        std::string m_osName;
        std::string m_osMajorVersion;
        std::string m_osMinorVersion;
        static constexpr unsigned int osInfoLengthLimit_ = 255;
    };

} // namespace  Common::OSUtilitiesImpl