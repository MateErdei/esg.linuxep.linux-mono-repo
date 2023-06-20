// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Drive.h"

#include "Common/SystemCallWrapper/SystemCallWrapperFactory.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <stdexcept>

using namespace mount_monitor::mountinfoimpl;

static Common::SystemCallWrapper::ISystemCallWrapperSharedPtr createSystemCallWrapper()
{
    static auto factory = std::make_shared<Common::SystemCallWrapper::SystemCallWrapperFactory>();
    return factory->createSystemCallWrapper();
}

static DeviceUtilSharedPtr getDeviceUtil()
{
    static auto util = std::make_shared<DeviceUtil>(createSystemCallWrapper());
    return util;
}

/**
 *
 * @param device
 * @param mountPoint
 * @param type
 */
Drive::Drive(std::string device, std::string mountPoint, std::string type, bool isDirectory, bool isReadOnly) :
    m_deviceUtil(getDeviceUtil()),
    m_mountPoint(std::move(mountPoint)),
    m_device(std::move(device)),
    m_fileSystem(std::move(type)),
    m_isDirectory(isDirectory),
    m_isReadOnly(isReadOnly)
{
}

std::string Drive::mountPoint() const
{
    return m_mountPoint;
}

std::string Drive::device() const
{
    return m_device;
}

std::string Drive::filesystemType() const
{
    return m_fileSystem;
}

bool Drive::isHardDisc() const
{
    return m_deviceUtil->isLocalFixed(device(), mountPoint(), filesystemType());
}

bool Drive::isNetwork() const
{
    return m_deviceUtil->isNetwork(device(), mountPoint(), filesystemType());
}

bool Drive::isOptical() const
{
    return m_deviceUtil->isOptical(device(), mountPoint(), filesystemType());
}

bool Drive::isRemovable() const
{
    return m_deviceUtil->isRemovable(device(), mountPoint(), filesystemType());
}

/**
 * @return true if this is a special filesystem mount that we should avoid
 * scanning.
 */
bool Drive::isSpecial() const
{
    return
        (m_deviceUtil->isSystem(device(), mountPoint(), filesystemType()) ||
        m_deviceUtil->isNotSupported(filesystemType()));
}

bool Drive::isDirectory() const
{
    return m_isDirectory;
}
bool Drive::isReadOnly() const
{
    return m_isReadOnly;
}