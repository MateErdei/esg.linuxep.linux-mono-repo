// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Drive.h"

#include "Logger.h"

#include "datatypes/SystemCallWrapperFactory.h"

#include "Common/UtilityImpl/StringUtils.h"

#include <climits>
#include <stdexcept>

#include <mntent.h>

using namespace mount_monitor::mountinfoimpl;

static datatypes::ISystemCallWrapperSharedPtr createSystemCallWrapper()
{
    static auto factory = std::make_shared<datatypes::SystemCallWrapperFactory>();
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
Drive::Drive(std::string device, std::string mountPoint, std::string type, bool isDirectory) :
    m_deviceUtil(getDeviceUtil()),
    m_mountPoint(std::move(mountPoint)),
    m_device(std::move(device)),
    m_fileSystem(std::move(type)),
    m_isDirectory(isDirectory),
    m_isReadOnly(false) // placeholder until option unpacking added to DeviceUtil
{
}

Drive::Drive(const std::string& childPath) :
    m_deviceUtil(getDeviceUtil())
{
//    LOGDEBUG("Searching for nearest parent mount of: " << childPath);

    FILE* f = nullptr;
    f = setmntent("/proc/mounts", "r"); // open file for describing the mounted filesystems
    mntent* mount;
    ulong parentPathSize = 0UL;

    mntent mnt_buf {};
    char buf[PATH_MAX * 3] {};

    if (!f)
    {
        throw std::runtime_error("Could not access /proc/mounts to find parent mount of: " + childPath);
    }

    while ((mount = getmntent_r(f, &mnt_buf, buf, sizeof(buf)))) // read next line
    {
        std::string mountPoint = mount->mnt_dir;
        if (Common::UtilityImpl::StringUtils::startswith(childPath, mountPoint) &&
            (parentPathSize < mountPoint.size()))
        {
            m_mountPoint = mount->mnt_dir;
            m_device = mount->mnt_fsname;
            m_fileSystem = mount->mnt_type;
            if (childPath == m_mountPoint)
            {
                m_isDirectory = false;
            }
            else
            {
                m_isDirectory = true;
            }
            m_isReadOnly = hasmntopt(mount, "ro");
            parentPathSize = m_mountPoint.size();
            LOGDEBUG("Found potential parent: " << m_device << " -- at path: " << m_mountPoint);
        }
    }
    endmntent(f); // close file for describing the mounted filesystems
    if (parentPathSize == 0UL)
    {
        throw std::runtime_error("No parent mounts found for path: " + childPath);
    }
    LOGDEBUG("Best fit parent found: " << m_device << " -- at path: " << m_mountPoint);
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