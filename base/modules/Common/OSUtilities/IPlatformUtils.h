/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
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
            virtual std::string getPlatform() const = 0;
            virtual std::string getVendor() const = 0;
            virtual std::string getArchitecture() const = 0;
            virtual std::string getOsName() const = 0;
            virtual std::string getKernelVersion() const = 0;
            virtual std::string getOsMajorVersion() const = 0;
            virtual std::string getOsMinorVersion() const = 0;
            virtual std::string getDomainname() const = 0;
            virtual std::string getIp4Address() const = 0;
            virtual std::string getIp6Address() const = 0;
            virtual std::string getCloudPlatformMetadata(Common::HttpRequests::IHttpRequester* client) const = 0;
            virtual void setProxyConfig(std::map<std::string, std::string> proxyConfig) = 0;
        };



    } // namespace OSUtilities
} // namespace Common