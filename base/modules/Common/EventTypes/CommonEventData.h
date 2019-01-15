/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Common
{
    namespace EventTypes
    {
        struct UserSid
        {
        public:
            std::string username;
            std::string sid;
            std::string domain;

        };

        struct NetworkAddress
        {
        public:
            std::string address;
        };

        struct SocketAddress
        {
        public:
            std::string address;
            unsigned int port;
        };

        struct IpFlow
        {
        public:
            SocketAddress sourceAddress;
            SocketAddress destinationAddress;
            unsigned short protocol;

        };

        struct OptionalUInt64
        {
        public:
            std::uint64_t value;
        };

        struct TextOffsetLength
        {
        public:
            std::uint32_t length;
            std::uint32_t offset;
        };

        struct Pathname
        {
        public:
            std::uint16_t flags;
            std::uint32_t fileSystemType;
            std::uint8_t driveLetter;
            std::string pathname;
            TextOffsetLength openName;
            TextOffsetLength volumeName;
            TextOffsetLength shareName;
            TextOffsetLength extensionName;
            TextOffsetLength streamName;
            TextOffsetLength finalComponentName;
            TextOffsetLength parentDirName;
        };
    };
}