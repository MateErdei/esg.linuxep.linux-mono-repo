/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include "SophosString.h"

namespace Common
{
    namespace EventTypes
    {
        struct UserSid
        {
        public:
            SophosString username;
            SophosString sid;
            SophosString domain;
            SophosString machineid;
            std::uint32_t userid = 0xFFFFFFFF; // Unset
        };

        struct NetworkAddress
        {
        public:
            SophosString address;
        };

        using socket_port_t = uint16_t;

        struct SocketAddress
        {
        public:
            SophosString address;
            socket_port_t port = 0;
        };

        using sophos_ip_flow_protocol_t = std::uint8_t;

        struct IpFlow
        {
        public:
            SocketAddress sourceAddress;
            SocketAddress destinationAddress;
            sophos_ip_flow_protocol_t protocol = 0;
        };

        struct OptionalUInt64
        {
        public:
            std::uint64_t value = 0;
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
            SophosString pathname;
            TextOffsetLength openName;
            TextOffsetLength volumeName;
            TextOffsetLength shareName;
            TextOffsetLength extensionName;
            TextOffsetLength streamName;
            TextOffsetLength finalComponentName;
            TextOffsetLength parentDirName;
            std::uint32_t fileSystemType = 0;
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
            sophos_pid_t pid = 0;

            // Timestamp the process was started
            windows_timestamp_t timestamp = 0;
        };

        struct SophosTid
        {
        public:
            // Thread ID
            sophos_tid_t tid = 0;

            // Timestamp the thread was started
            windows_timestamp_t timestamp = 0;
        };

    }; // namespace EventTypes
} // namespace Common