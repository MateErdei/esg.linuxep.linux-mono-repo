/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapperFactory.h"
#include "SystemCallWrapper.h"

#include <memory>
#include <string>

namespace avscanner::avscannerimpl
{
    class DeviceUtil
    {
    public:
        DeviceUtil(std::shared_ptr<ISystemCallWrapperFactory> systemCallWrapperFactory);

        /**
         * Determine if the device specified is a floppy drive.
         *
         * @return True if the device is a floppy drive.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         * @param systemCallWrapper Optional way to override ioctl call
         */
        bool isFloppy(
            const std::string& devicePath,
            const std::string& mountPoint = "",
            const std::string& filesystemType = "");

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
            const std::string& mountPoint = "",
            const std::string& filesystemType = "");

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
            const std::string& mountPoint = "",
            const std::string& filesystemType = "");

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
            const std::string& mountPoint = "",
            const std::string& filesystemType = "");

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
            const std::string& mountPoint = "",
            const std::string& filesystemType = "");

        /**
         * Determine if the device specified is a system/pseudo filesystem.
         *
         * @return True if the device is a system filesystem.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         * @param systemCallWrapper Optional way to override ioctl call
         */
        bool isSystem(
            const std::string& devicePath,
            const std::string& mountPoint = "",
            const std::string& filesystemType = "");

    private:
        std::shared_ptr<ISystemCallWrapper> m_systemCallWrapper;
    };
}