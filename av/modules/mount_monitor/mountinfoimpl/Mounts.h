/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "DeviceUtil.h"

#include "mount_monitor/mountinfo/IMountInfo.h"
#include "mount_monitor/mountinfo/ISystemPaths.h"

#include <memory>

#include <mntent.h>

namespace mount_monitor::mountinfoimpl
{
    class Mounts : virtual public mountinfo::IMountInfo
    {

    public:
        /**
         * Object which represents a mounted device
         * @author William Waghorn
         * @version 1.0
         * @updated 04-Feb-2008 15:41:14
         */
        class Drive : virtual public mountinfo::IMountPoint
        {

        public:
            /**
             *
             * @param device
             * @param mountPoint
             * @param type
             */
            Drive(std::string device, std::string mountPoint, std::string type, bool isDirectory);

            /**
             * Constructs Drive of mount point nearest to childPath.
             * Throws if unable to access /proc/mounts or no parent found (should always find "/").
             *
             * @param childPath
             */
            explicit Drive(const std::string& childPath);

            ~Drive() override = default;

            [[nodiscard]] std::string mountPoint() const override;

            [[nodiscard]] std::string device() const override;

            [[nodiscard]] std::string filesystemType() const override;

            [[nodiscard]] bool isHardDisc() const override;

            [[nodiscard]] bool isNetwork() const override;

            [[nodiscard]] bool isOptical() const override;

            [[nodiscard]] bool isRemovable() const override;

            /**
             * @return true if this is a special filesystem mount that we should avoid
             * scanning.
             */
            [[nodiscard]] bool isSpecial() const override;

            [[nodiscard]] bool isDirectory() const override;

            [[nodiscard]] bool isReadOnly() const override;

        private:
            mountinfoimpl::DeviceUtilSharedPtr m_deviceUtil;
            std::string m_mountPoint;
            std::string m_device;
            std::string m_fileSystem;
            bool m_isDirectory;
            bool m_isReadOnly;
        };

        /**
         * constructor
         */
        explicit Mounts(mountinfo::ISystemPathsSharedPtr systemPaths);
        Mounts(const Mounts&) = delete;


        /**
         * destructor
         */
        ~Mounts() override = default;

        /**
         * Determines which device is mounted on a particular mountpoint.
         *
         * @param mountPoint
         */
        [[nodiscard]] std::string device(const std::string& mountPoint) const;


        /**
         * Iterator for the list of mount points.
         */
        mountinfo::IMountPointSharedVector mountPoints() override;

    private:

        /**
         * Run a command and collect the output.
         * @return an empty string on error.  This is obviously ambiguous with
         * successfully running a command which returns no output; however for our needs
         * that behaviour is equivalent to a failure.
         *
         * @param path    Command to run
         * @param args    arguments.
         */
        std::string scrape(const std::string& path, const std::vector<std::string>& args);


        mountinfo::ISystemPathsSharedPtr m_systemPaths;
        mountinfo::IMountPointSharedVector m_devices;

        /**
         * Returns the path to the real mount point for a listing in /proc/mounts
         * @param device    a line from /proc/mounts.
         *
         */
        std::string realMountPoint(const std::string& device);

        /**
         * Use mount -f -n -v to determine the real mount point if the
         * device begins with 'LABEL=' or 'UUID='.
         */
        std::string fixDeviceWithMount(const std::string& device);

        /**
         * Parse one line of /proc/mounts on Linux.
         */
        bool parseLinuxProcMountsLine(const std::string& line, std::string& device, std::string& mountpoint,
                                             std::string& filesystem);

        /**
         * Try and parse /proc/mounts.
         */
        void parseProcMounts();
    };
}