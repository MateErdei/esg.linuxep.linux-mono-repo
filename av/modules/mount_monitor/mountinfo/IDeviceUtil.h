// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>
#include <string>

namespace mount_monitor::mountinfo
{
    class IDeviceUtil
    {
    public:
        inline IDeviceUtil() = default;
        inline virtual ~IDeviceUtil() = default;

        /**
         * Determine if the device specified is a floppy drive.
         *
         * @return True if the device is a floppy drive.
         *
         * @param devicePath
         * @param mountPoint
         * @param filesystemType
         */
        virtual bool isFloppy(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) = 0;

        /**
         * Determine if the device specified is a local fixed (hard) drive.
         *
         * @return True if the device is a hard drive.
         *
         * @param devicePath
         * @param mountPoint
         * @param filesystemType
         */
        virtual bool isLocalFixed(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) = 0;

        /**
         * Determine if the device is a network drive.
         *
         * @return True if the device is a network drive.
         *
         * @param devicePath
         * @param mountPoint
         * @param filesystemType
         */
        virtual bool isNetwork(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) = 0;

        /**
         * Determine if the device specified is a CD/DVD to our best ability.
         *
         * @return True if the device is optical.
         *
         * @param devicePath
         * @param mountPoint
         * @param filesystemType
         */
        virtual bool isOptical(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) = 0;

        /**
         * Determine if the device specified is removable.
         *
         * @return True if the device is removable.
         *
         * @param devicePath
         * @param mountPoint
         * @param filesystemType
         */
        virtual bool isRemovable(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) = 0;

        /**
         * Determine if the device specified is a system/pseudo filesystem.
         *
         * @return True if the device is a system filesystem.
         *
         * @param devicePath
         * @param mountPoint
         * @param filesystemType
         */
        virtual bool isSystem(
            const std::string& devicePath,
            const std::string& mountPoint,
            const std::string& filesystemType) = 0;

        /**
         * Determine if the file descriptor specified is on a filesystem that we can safely cache for
         *
         * @return True if the fd is on a safe filesystem
         *
         * @param fd
         */
        virtual bool isCachable(int fd) = 0;

    };

    using IDeviceUtilSharedPtr = std::shared_ptr<IDeviceUtil>;
}