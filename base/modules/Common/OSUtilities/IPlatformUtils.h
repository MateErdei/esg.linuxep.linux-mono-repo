// Copyright 2022-2023 Sophos Limited. All rights reserved.
#pragma once

#include <string>
#include <Common/HttpRequests/IHttpRequester.h>

namespace Common
{
    namespace OSUtilities
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
            virtual std::string getIp4Address() const = 0;
            virtual std::vector<std::string> getIp4Addresses() const = 0;
            virtual std::string getIp6Address() const = 0;
            virtual std::vector<std::string> getIp6Addresses() const = 0;
            virtual std::string getCloudPlatformMetadata(std::shared_ptr<Common::HttpRequests::IHttpRequester> client) const = 0;
            virtual std::vector<std::string> getMacAddresses() const = 0;
        };



    } // namespace OSUtilities
} // namespace Common