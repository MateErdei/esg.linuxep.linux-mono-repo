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

        using socket_port_t = uint16_t;

        struct SocketAddress
        {
        public:
            std::string address;
            socket_port_t port;
        };

        using sophos_ip_flow_protocol_t = std::uint8_t;

        struct IpFlow
        {
        public:
            SocketAddress sourceAddress;
            SocketAddress destinationAddress;
            sophos_ip_flow_protocol_t protocol;
        };

        struct OptionalUInt64
        {
        public:
            std::uint64_t value;
        };

        struct TextOffsetLength
        {
        public:
            std::uint32_t length = 0;
            std::uint32_t offset = 0;
        };

        struct Pathname
        {
        public:
            std::uint32_t fileSystemType = 0;
            std::string pathname;
            TextOffsetLength openName;
            TextOffsetLength volumeName;
            TextOffsetLength shareName;
            TextOffsetLength extensionName;
            TextOffsetLength streamName;
            TextOffsetLength finalComponentName;
            TextOffsetLength parentDirName;
            std::uint16_t flags = 0;
            std::uint8_t driveLetter = 0;
        };

        using windows_timestamp_t = std::uint64_t;
        using sophos_pid_t = std::uint64_t;
        using sophos_tid_t = std::uint64_t;

        struct SophosPid
        {
        public:
            // Process ID
            sophos_pid_t pid;

            // Timestamp the process was started
            windows_timestamp_t timestamp;
        };

        struct SophosTid
        {
        public:
            // Thread ID
            sophos_tid_t tid;

            // Timestamp the thread was started
            windows_timestamp_t timestamp;
        };

    }; // namespace EventTypes
} // namespace Common