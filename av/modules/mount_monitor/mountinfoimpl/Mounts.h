// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "DeviceUtil.h"
#include "MountsList.h"

#include "mount_monitor/mountinfo/IMountInfo.h"
#include "mount_monitor/mountinfo/ISystemPaths.h"

#include <map>
#include <memory>

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

        /**
         * Get the mount point where the file at childPath is located
         */
        mountinfo::IMountPointSharedPtr getMountFromPath(const std::string& childPath) override;

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
        MountsList m_devices;

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
        bool parseLinuxProcMountsLine(
            const std::string& line,
            std::string& device,
            std::string& mountpoint,
            std::string& filesystem);

        /**
         * Try and parse /proc/mounts.
         */
        void parseProcMounts();
    };
} // namespace mount_monitor::mountinfoimpl