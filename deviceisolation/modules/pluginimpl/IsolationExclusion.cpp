// Copyright 2023 Sophos Limited. All rights reserved.

#include "IsolationExclusion.h"

using namespace Plugin;

IsolationExclusion::Direction IsolationExclusion::direction() const
{
    return direction_;
}

void IsolationExclusion::setDirection(IsolationExclusion::Direction direction)
{
    direction_ = direction;
}

IsolationExclusion::address_iptype_list_t IsolationExclusion::remoteAddressesAndIpTypes() const
{
    return remoteAddressesAndIpTypes_;
}

void IsolationExclusion::setRemoteAddressesAndIpTypes(IsolationExclusion::address_iptype_list_t remoteAddressesAndIpTypes)
{
    remoteAddressesAndIpTypes_ = std::move(remoteAddressesAndIpTypes);
}

IsolationExclusion::port_list_t IsolationExclusion::localPorts() const
{
    return localPorts_;
}

void IsolationExclusion::setLocalPorts(IsolationExclusion::port_list_t localPorts)
{
    localPorts_ = std::move(localPorts);
}

IsolationExclusion::port_list_t IsolationExclusion::remotePorts() const
{
    return remotePorts_;
}

void IsolationExclusion::setRemotePorts(IsolationExclusion::port_list_t remotePorts)
{
    remotePorts_ = std::move(remotePorts);
}