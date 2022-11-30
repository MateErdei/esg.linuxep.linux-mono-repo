// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/ISystemCallWrapperFactory.h"
#include "datatypes/SystemCallWrapper.h"
#include "mount_monitor/mountinfo/IDeviceUtil.h"

#include <memory>
#include <string>

namespace mount_monitor::mountinfoimpl
{
    class DeviceUtil : public mountinfo::IDeviceUtil
    {
    public:
        explicit DeviceUtil(const datatypes::ISystemCallWrapperFactorySharedPtr& systemCallWrapperFactory);
        explicit DeviceUtil(datatypes::ISystemCallWrapperSharedPtr systemCallWrapper);

        /**
         * Determine if the device specified is a floppy drive.
         *
         * @return True if the device is a floppy drive.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         */
        bool isFloppy(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) override;

        /**
         * Determine if the device specified is a local fixed (hard) drive.
         *
         * @return True if the device is a hard drive.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         */
        bool isLocalFixed(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) override;

        /**
         * Determine if the device is a network drive.
         *
         * @return True if the device is a network drive.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         */
        bool isNetwork(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) override;

        /**
         * Determine if the device specified is a CD/DVD to our best ability.
         *
         * @return True if the device is optical.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         */
        bool isOptical(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) override;

        /**
         * Determine if the device specified is removable.
         *
         * @return True if the device is removable.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         */
        bool isRemovable(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) override;

        /**
         * Determine if the device specified is a system/pseudo filesystem.
         *
         * @return True if the device is a system filesystem.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         */

        bool isSystem(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) override;

        /**
         * Determine if the device specified is not a supported filesystem for On Access and On Demand.
         * There is a separate list for On Access only exclusions
         *
         * @return True if the device is not a supported filesystem
         *
         * @param filesystemType    Optional type of device's filesystem.
         */

        bool isNotSupported( const std::string& filesystemType) override;

        /**
         * Determine if the file descriptor specified is on a filesystem that we can safely cache for
         *
         * @return True if the fd is on a safe filesystem
         *
         * @param fd
         */
         bool isCachable(int fd) override;

    private:
        datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };

    using DeviceUtilSharedPtr = std::shared_ptr<DeviceUtil>;
}