// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <vector>
#include <tuple>

namespace Plugin
{
    class IsolationExclusion
    {
    public:
        enum Direction
        {
            IN,
            OUT,
            BOTH
        };

        [[nodiscard]] Direction direction() const;
        void setDirection(Direction direction);

        using address_list_t = std::vector<std::string>;

        /**
         *
         * Empty vector means the exclusion applies to all remote addresses
         * Contents can be:
         * 1. IPv4 address
         * 2. IPv4 CIDR
         * 3. IPv6 address
         * 4. IPv6 CIDR
         *
         * @return The list of remote addresses this exclusion applies to.
         */
        [[nodiscard]] address_list_t remoteAddresses() const;
        void setRemoteAddresses(address_list_t);

        using port_list_t = std::vector<std::string>;

        /**
         * List of local ports to allow.
         * Empty means allow all local ports.
         * @return
         */
        [[nodiscard]] port_list_t localPorts() const;
        void setLocalPorts(port_list_t);
        /**
         * List of remote ports to allow.
         * Empty means allow all remote ports.
         * @return
         */
        [[nodiscard]] port_list_t remotePorts() const;
        void setRemotePorts(port_list_t);

        friend bool operator==(const IsolationExclusion& lhs, const IsolationExclusion& rhs)
        {
            return std::tie(lhs.direction_, lhs.remoteAddresses_, lhs.localPorts_, lhs.remotePorts_)
                == std::tie(rhs.direction_, rhs.remoteAddresses_, rhs.localPorts_, rhs.remotePorts_);
        }
    protected:
        Direction direction_{Direction::BOTH};
        address_list_t remoteAddresses_;
        port_list_t localPorts_;
        port_list_t remotePorts_;
    };
}
