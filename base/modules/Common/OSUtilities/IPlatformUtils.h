// Copyright 2022-2023 Sophos Limited. All rights reserved.
#pragma once

#include "IIPUtils.h"

#include "Common/HttpRequests/IHttpRequester.h"

#include <string>

namespace Common::OSUtilities
{
    class IPlatformUtils
    {
    public:
        virtual ~IPlatformUtils() = default;

        virtual std::string getHostname() const = 0;
        virtual std::string getFQDN() const = 0;
        virtual std::string getPlatform() const = 0;
        virtual std::string getVendor() const = 0;
        virtual std::string getArchitecture() const = 0;
        virtual std::string getOsName() const = 0;
        virtual std::string getKernelVersion() const = 0;
        virtual std::string getOsMajorVersion() const = 0;
        virtual std::string getOsMinorVersion() const = 0;
        virtual std::string getDomainname() const = 0;
        virtual std::string getFirstIpAddress(const std::vector<std::string>& ipAddresses) const = 0;
        virtual std::vector<std::string> getIp4Addresses(
            const std::vector<Common::OSUtilities::Interface>& interfaces) const = 0;
        virtual std::vector<std::string> getIp6Addresses(
            const std::vector<Common::OSUtilities::Interface>& interfaces) const = 0;
        virtual void sortInterfaces(std::vector<Common::OSUtilities::Interface>& interfaces) const = 0;
        virtual std::string getCloudPlatformMetadata(
            std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const = 0;
        virtual std::vector<std::string> getMacAddresses() const = 0;
    };
} // namespace Common::OSUtilities