/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IMountInfo.h"
#include "StringSet.h"

class Mounts : virtual public IMountInfo
{

public:
    /**
     * Object which represents a mounted device
     * @author William Waghorn
     * @version 1.0
     * @updated 04-Feb-2008 15:41:14
     */
    class Drive : virtual public IMountPoint
    {

    public:
        /**
         *
         * @param device
         * @param mountPoint
         * @param type
         */
        Drive(const std::string& device, const std::string& mountPoint, const std::string& type);

        virtual ~Drive();

        std::string mountPoint() const;


        std::string device() const;


        const std::string& fileSystem() const;
        virtual std::string filesystemType() const;
        virtual bool isHardDisc() const;
        virtual bool isNetwork() const;
        virtual bool isOptical() const;
        virtual bool isRemovable() const;
        /**
         * @return true if this is a special filesystem mount that we should avoid
         * scanning.
         */
        virtual bool isSpecial() const;

    private:
        std::string m_mountPoint;
        std::string m_device;
        std::string m_fileSystem;

    };

    /**
     * Collection of Drive objects
     * @author William Waghorn
     * @version 1.0
     * @updated 04-Feb-2008 15:41:15
     */
    typedef std::vector<Drive*> DriveSet;


    /**
     * constructor
     */
    Mounts();

    /**
     * destructor
     */
    virtual ~Mounts();

    /**
     * Returns a list of mounted filesystems.
     */
    StringSet devices() const;

    /**
     * Indicates where the device is mounted
     * @return mountpoint for the given device, or an empty string if the device is
     * not mounted.
     *
     * @param device
     */
    std::string mountPoint(const std::string& device) const;


    /**
     * Indicates the filesystem type for the mounted device
     * @return The filesystem type for the given device
     *
     * @param device
     */
    std::string fileSystem(const std::string& device) const;


    /**
     * Determines which device is mounted on a particular mountpoint.
     *
     * @param mountPoint
     */
    std::string device(const std::string& mountPoint) const;


    /**
     * Run a command and collect the output.
     * @return an empty string on error.  This is obviously ambiguous with
     * successfully running a command which returns no output; however for our needs
     * that behaviour is equivalent to a failure.
     *
     * @param path    Command to run
     * @param args    arguments.
     */
    static std::string scrape(const std::string& path, const StringSet& args);

    /**
     * Iterator for the list of mount points.
     */
    virtual std::vector<IMountPoint*> mountPoints();

private:
    std::vector<IMountPoint*> m_devices;


    /**
     * Given a device ID, return the major device name.
     *
     * @param deviceID
     */
    static std::string majorName(dev_t deviceID);
    /**
     * Returns the path to the real mount point for a listing in /proc/mounts
     * @param device    a line from /proc/mounts.
     *
     */
    static std::string realMountPoint(const std::string& device);

    /**
     * Use mount -f -n -v to determine the real mount point if the
     * device begins with 'LABEL=' or 'UUID='.
     */
    static std::string fixDeviceWithMount(const std::string& device);

    /**
     * Parse one line of /proc/mounts on Linux.
     */
    static bool parseLinuxProcMountsLine(const std::string& line, std::string& device, std::string& mountpoint, std::string& filesystem);

    /**
     * Try and parse /proc/mounts.
     */
    void parseProcMounts();
    /**
     * Parse one line of mount command output on Linux.
     */
    static bool parseLinuxMountsLine(const std::string& line, std::string& device, std::string& mountpoint, std::string& filesystem);
};
