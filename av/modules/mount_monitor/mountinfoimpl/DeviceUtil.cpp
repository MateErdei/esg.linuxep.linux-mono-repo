/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DeviceUtil.h"

// Standard C++
#include <algorithm>
#include <cstring>

// Standard C
#include <sys/vfs.h>
#include <fcntl.h>
#include <unistd.h>

extern "C"
{
#include <linux/cdrom.h>
#include <linux/fd.h>
#include <linux/hdreg.h>
}

using namespace mount_monitor::mountinfoimpl;
using namespace datatypes;

DeviceUtil::DeviceUtil(const ISystemCallWrapperFactorySharedPtr& systemCallWrapperFactory)
: DeviceUtil(systemCallWrapperFactory->createSystemCallWrapper())
{
}

DeviceUtil::DeviceUtil(ISystemCallWrapperSharedPtr systemCallWrapper)
    : m_systemCallWrapper(std::move(systemCallWrapper))
{
}

/**
 * Determine if the device specified is a floppy drive.
 *
 * @return True if the device is a floppy drive.
 *
 * @param devicePath
 * @param mountPoint    Optional mount point of the device.
 * @param filesystemType    Optional type of device's filesystem.
 */
bool DeviceUtil::isFloppy(
    const std::string& devicePath,
    const std::string& mountPoint,
    const std::string& filesystemType)
{
    bool result = false;
    static_cast<void>(devicePath);
    static_cast<void>(mountPoint);
    static_cast<void>(filesystemType);

    int fd = m_systemCallWrapper->_open(devicePath.c_str(), O_RDONLY | O_NONBLOCK, 0644);

    if (fd != -1)
    {
        // something is there.  Determine whether this is a floppy drive
        floppy_drive_name name {};

        if (m_systemCallWrapper->_ioctl(fd, FDGETDRVTYP, name) != -1) // NOLINT(hicpp-signed-bitwise)
        {
            // this is a floppy drive.
            if (strncmp("(null)", name, sizeof(name)) != 0)
            {
                // and there really is actual floppy hardware present
                result = true;
            }
        }
        ::close(fd);
    }

    return result;
}

/**
 * Determine if the device specified is a local fixed (hard) drive.
 *
 * @return True if the device is a hard drive.
 *
 * @param devicePath
 * @param mountPoint    Optional mount point of the device.
 * @param filesystemType    Optional type of device's filesystem.
 */
bool DeviceUtil::isLocalFixed(const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType)
{
    static_cast<void>(devicePath);
    static_cast<void>(mountPoint);
    static_cast<void>(filesystemType);

    if (isRemovable(devicePath,mountPoint,filesystemType) ||
        isNetwork(devicePath,mountPoint,filesystemType) ||
        isSystem(devicePath,mountPoint,filesystemType))
    {
        return false;
    }

    return true;
}

/**
 * Determine if the device is a network drive.
 *
 * @return True if the device is a network drive.
 *
 * @param devicePath
 * @param mountPoint    Optional mount point of the device.
 * @param filesystemType    Optional type of device's filesystem.
 */
bool DeviceUtil::isNetwork(const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType)
{
    static_cast<void>(devicePath);
    static_cast<void>(mountPoint);
    static_cast<void>(filesystemType);

    if (filesystemType == "nfs" ||
        filesystemType == "cifs" ||
        filesystemType == "smbfs" ||
        filesystemType == "smb" ||
        filesystemType == "9p" ||
        filesystemType == "ncpfs" ||
        filesystemType == "afs" ||
        filesystemType == "coda" ||
        filesystemType == "nfs4" ||
        filesystemType == "nfs3")
    {
        return true;
    }

    // Also look at the device path
    if (devicePath[0] != '/' && devicePath.find_first_of(':') != std::string::npos)
    {
        return true;
    }

    return false;
}

/**
 * Determine if the device specified is a CD/DVD to our best ability.
 *
 * @return True if the device is optical.
 *
 * @param devicePath
 * @param mountPoint    Optional mount point of the device.
 * @param filesystemType    Optional type of device's filesystem.
 */
bool DeviceUtil::isOptical(const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType)
{
    static_cast<void>(devicePath);
    static_cast<void>(mountPoint);
    static_cast<void>(filesystemType);
    bool result = false;

    int fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK, 0644);


    if (fd != -1)
    {
        struct cdrom_volctrl vol{};

        // I'd like to use CDROMREADTOCHDR and assume success or EIO means its
        // a CDROM drive.  Unfortuantely, on Redhat 8.0 (see [FML1384]) all
        // hard drives generate EIO on this ioctl.
        if (ioctl(fd, CDROMVOLREAD, &vol) != -1)
        {
            result = true;
        }
        close(fd);
    }

    if (filesystemType == "isofs" ||
        filesystemType == "udf" ||
        filesystemType == "iso9660" ||
        filesystemType == "hsfs") // Solaris CD
    {
        result = true;
    }

    return result;
}

/**
 * Determine if the device specified is removable.
 *
 * @return True if the device is removable.
 *
 * @param devicePath
 * @param mountPoint    Optional mount point of the device.
 * @param filesystemType    Optional type of device's filesystem.
 */
bool DeviceUtil::isRemovable(const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType)
{
    static_cast<void>(devicePath);
    static_cast<void>(mountPoint);
    static_cast<void>(filesystemType);

    // TODO: Need to also handle removable harddrives - maybe look at device path? LINUXDAR-2678

    return isFloppy(devicePath,mountPoint,filesystemType) ||
           isOptical(devicePath,mountPoint,filesystemType);
}

/**
 * Determine if the device specified is a system/pseudo filesystem.
 *
 * @return True if the device is a system filesystem.
 *
 * @param devicePath
 * @param mountPoint    Optional mount point of the device.
 * @param filesystemType    Optional type of device's filesystem.
 */
bool DeviceUtil::isSystem(
    const std::string& devicePath,
    const std::string& mountPoint,
    const std::string& filesystemType)
{
    static_cast<void>(devicePath);
    static_cast<void>(mountPoint);
    static_cast<void>(filesystemType);

    if (filesystemType == "binfmt_misc" ||
        filesystemType == "configfs" ||
        filesystemType == "debugfs" ||
        filesystemType == "devfs" ||
        filesystemType == "devpts" ||
        filesystemType == "nfsd" ||
        filesystemType == "proc" ||
        filesystemType == "securityfs" ||
        filesystemType == "selinuxfs" ||
        filesystemType == "mqueue" ||
        filesystemType == "cgroup" ||
        filesystemType == "cgroup2" ||
        filesystemType == "rpc_pipefs" ||
        filesystemType == "hugetlbfs" ||
        filesystemType == "sysfs" ||
        filesystemType == "fusectl" ||
        filesystemType == "pipefs" ||
        filesystemType == "sockfs" ||
        filesystemType == "usbfs" ||
        filesystemType == "tracefs" ||
        filesystemType == "fuse.lxcfs" ||
        filesystemType == "fuse.gvfsd-fuse"
    )
    {
        return true;
    }
    else if (filesystemType == "none" || filesystemType.empty())
    {
        struct statfs sfs{};

        int ret = m_systemCallWrapper->_statfs(mountPoint.c_str(), &sfs);
        if (ret == 0)
        {
            auto sb_type = static_cast<unsigned long>(sfs.f_type);

            if (sb_type == 0x9fa0 || // proc
                sb_type == 0x62656572 || // sysfs
                sb_type == 0x64626720 || // debugfs
                sb_type == 0x73636673 || // securityfs
                sb_type == 0xf97cff8c || // selinux
                sb_type == 0x9fa2 || // usbdevice
                sb_type == 0x1cd1 || // devpts
                sb_type == 0x62656570 || // configfs
                sb_type == 0x65735543 || // fusectl
                sb_type == 0x42494e4d || // binfmt_misc
                sb_type == 0x534F434B || // sockfs
                sb_type == 0x50495045 || // pipefs
                sb_type == 0x6e667364 || // nfsd
                sb_type == 0x19800202 || // mqueue
                sb_type == 0x27e0eb || // cgroup
                sb_type == 0x63677270 || // cgroup2
                sb_type == 0x67596969 || // rpc_pipefs
                sb_type == 0x958458f6 || // hugetlbfs
                sb_type == 0x1373 || // devfs
                sb_type == 0x74726163) // tracefs
            {
                return true;
            }
        }
    }

    return false;
}