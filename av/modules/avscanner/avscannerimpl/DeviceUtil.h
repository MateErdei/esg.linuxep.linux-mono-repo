/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SystemCallWrapper.h"

#include <memory>
#include <string>

namespace avscanner::avscannerimpl
{
    class DeviceUtil
    {
    public:
        DeviceUtil() = delete;

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
        static bool isFloppy(
            const std::string& devicePath,
            const std::string& mountPoint = "",
            const std::string& filesystemType = "",
            std::shared_ptr<ISystemCallWrapper> systemCallWrapper = std::make_shared<SystemCallWrapper>());

        /**
         * Determine if the device specified is a local fixed (hard) drive.
         *
         * @return True if the device is a hard drive.
         *
         * @param devicePath
         * @param mountPoint    Optional mount point of the device.
         * @param filesystemType    Optional type of device's filesystem.
         */
        static bool isLocalFixed(
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
        static bool isNetwork(
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
        static bool isOptical(
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
        static bool isRemovable(
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
        static bool isSystem(
            const std::string& devicePath,
            const std::string& mountPoint = "",
            const std::string& filesystemType = "",
            std::shared_ptr<ISystemCallWrapper> systemCallWrapper = std::make_shared<SystemCallWrapper>());
    };
}