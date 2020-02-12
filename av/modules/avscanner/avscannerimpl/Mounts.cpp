/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Mounts.h"

#include "DeviceUtil.h"
#include "datatypes/Print.h"

// Standard C++
#include <sstream>
#include <fstream>

// Standard C
#include <fstab.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


std::string octalUnescape(const std::string& input)
{
    char *ptr;
    std::string out;


    ptr = const_cast<char *>(input.c_str()); // We just want to read
    while (*ptr != 0)
    {
        char ch = *ptr++;
        if (ch == '\\')
        {
            std::string buf("\\");
            size_t i;
            unsigned int val = 0;


            for (i = 0; i < 3; i++, ptr++)
            {
                ch = *ptr;
                buf = buf + ch; // store characters in case it turns out not to be an octal escape
                if (ch == 0 || ch < '0' || ch > '7')
                {
                    break;
                }
                val = val * 8 + (ch - '0');
            }
            if (i == 3) // looks like it was a full octal sequence
            {
                out = out + (char)val;
            }
            else // does not look like an octal sequence
            {
                out = out + buf;
            }
        }
        else
        {
            out = out + ch;
        }
    }

    return out;
}

/**
 * constructor
 */
Mounts::Mounts()
{
    std::string mountCommand="/bin/mount";
    StringSet args(mountCommand);

    std::string mount = scrape(mountCommand, args);


    if (mount == "")
    {
        // /proc/mounts is only available on Linux
        parseProcMounts();
        return;
    }

    // otherwise parse our scrape
    std::istringstream ist(mount);
    std::string line;


    while (!std::getline(ist, line).eof())
    {
        std::string device;
        std::string mountpoint;
        std::string filesystem;



        // TODO: It could be better to use getmntent_r here, but it would mean refactoring everything for Linux.
        if (!parseLinuxMountsLine(line, device, mountpoint, filesystem))
        {
            PRINT("Failed to parse: " << line.c_str());
            continue;
        }

        PRINT("dev " << device << " on " << mountpoint << " type " << filesystem);
        m_devices.push_back(new Drive(device, mountpoint, filesystem));
    }
}

/**
 * destructor
 */
Mounts::~Mounts()
{
    for (std::vector<IMountPoint*>::iterator it = m_devices.begin(); it != m_devices.end(); it++)
    {
        delete *it;
    }

    m_devices.clear();
    return;
}

/**
 * Try and parse /proc/mounts.
 */
void Mounts::parseProcMounts()
{
    std::ifstream mountstream("/proc/mounts");
    std::string line;


    if (!mountstream)
    {
        throw std::runtime_error("Unable to access /proc/mounts");
    }

    while (!mountstream.eof())
    {
        std::string device;
        std::string type;
        std::string mountPoint;


        std::getline(mountstream, line);

        //DBGOUT("line: " << line);

        if (!parseLinuxProcMountsLine(line, device, mountPoint, type))
        {
            PRINT("Failed to parse: " << line);
            continue;
        }

        device = realMountPoint(device);
        PRINT("dev " << device << " on " << mountPoint << " type " << type);
        m_devices.push_back(new Drive(device, mountPoint, type));
    }

    if (device("/") == "")
    {
        // Check the root device is included, because there's no chance it isn't mounted.
        // FIXME: getfsfile is obsolete - if we really want to have this block getmntent should be used.
        struct fstab* mountpoint = getfsfile("/");


        if ((mountpoint != 0) && (mountpoint->fs_spec != 0))
        {
            m_devices.push_back(new Drive(
                        fixDeviceWithMount(mountpoint->fs_spec),
                        mountpoint->fs_file,
                        mountpoint->fs_type));
        }
    }
}

/**
 * Returns a list of mounted filesystems.
 */
StringSet Mounts::devices() const
{
    StringSet result;

    for (std::vector<IMountPoint*>::const_iterator it = m_devices.begin(); it != m_devices.end(); it++)
    {
        result.push_back((*it)->device());
    }
    return result;
}

/**
 * Indicates where the device is mounted
 * @return mountpoint for the given device, or an empty string if the device is
 * not mounted.
 *
 * @param device
 */
std::string Mounts::mountPoint(const std::string& device) const
{
    for (std::vector<IMountPoint*>::const_iterator it = m_devices.begin(); it != m_devices.end(); it++)
    {
        if ((*it)->device() == device)
        {
            return (*it)->mountPoint();
        }
    }
    return "";
}

/**
 * Indicates the filesystem type for the mounted device
 * @return The filesystem type for the given device
 *
 * @param device
 */
std::string Mounts::fileSystem(const std::string& device) const
{
    for (std::vector<IMountPoint*>::const_iterator it = m_devices.begin(); it != m_devices.end(); it++)
    {
        if ((*it)->device() == device)
        {
            return (*it)->filesystemType();
        }
    }
    return "";
}

/**
 * Determines which device is mounted on a particular mountpoint.
 *
 * @param mountPoint
 */
std::string Mounts::device(const std::string& mountPoint) const
{
    for (std::vector<IMountPoint*>::const_iterator it = m_devices.begin(); it != m_devices.end(); it++)
    {
        if ((*it)->mountPoint() == mountPoint)
        {
            return (*it)->device();
        }
    }
    return "";
}

/**
 *
 * @param device
 * @param mountPoint
 * @param type
 */
Mounts::Drive::Drive(const std::string& device, const std::string& mountPoint, const std::string& type)
        : m_mountPoint(mountPoint), m_device(device), m_fileSystem(type)
{
    return;
}

Mounts::Drive::~Drive()
{
}

std::string Mounts::Drive::mountPoint() const
{
    return m_mountPoint;
}

std::string Mounts::Drive::device() const
{
    return m_device;
}

const std::string& Mounts::Drive::fileSystem() const
{
    return m_fileSystem;
}

/**
 * Run a command and collect the output.
 * @return an empty string on error.  This is obviously ambiguous with
 * successfully running a command which returns no output; however for our needs
 * that behaviour is equivalent to a failure.
 *
 * @param path    Command to run
 * @param args    arguments.
 */
std::string Mounts::scrape(const std::string& path, const StringSet& args)
{
    //PRINT("scrape(" << path << ", " << args << ")");
    std::string result;
    int fd[2];


    if (access(path.c_str(), X_OK) == 0)
    {
        // We have permission to execute the command
        if (pipe(fd) != -1)
        {
            pid_t child = fork();

            switch (child)
            {
                case -1:
                    // unable to fork.
                    close(fd[0]);
                    close(fd[1]);
                    break;

                case 0:
                    // child
                {
                    // connect stdout from mount to fd[1]
                    dup2(fd[1], 1);
                    // also collect output from stderr otherwise it'll end
                    // up splattered all over the console.
                    dup2(fd[1], 2);
                    close(fd[0]);
                    close(fd[1]);

                    char** argv = new char*[args.size() + 1];
                    int index = 0;


                    for (StringSet::const_iterator it = args.begin(); it != args.end(); ++it)
                    {
                        argv[index] = new char[it->size() + 1];
                        memcpy(argv[index], it->c_str(), it->size() + 1);
                        index++;
                    }
                    argv[index] = 0;

                    execv(path.c_str(), argv);
                    // never returns
                    PRINT("execv failed");
                    _exit(1);
                }
                default:
                {
                    int status;


                    close(fd[1]);

                    {
                        char buf[64];
                        ssize_t bytes;

                        do
                        {
                            bytes = ::read(fd[0], buf, 64);


                            if (bytes > 0)
                            {
                                result.append(buf, bytes);
                            }
                        } while (waitpid(child, &status, WNOHANG) != child);


                        // Need to read remaining output...
                        do
                        {
                            bytes = read(fd[0], buf, 64);

                            if (bytes > 0)
                            {
                                result.append(buf, bytes);
                            }
                        } while (bytes > 0);
                    }


                    close(fd[0]);

                    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
                    {
                        PRINT(path << " failed to execute");
                        result = "";
                    }
                    break;
                }
            }
        }
    }
    return result;
}

/**
 * Returns the path to the real mount point for a listing in /proc/mounts
 *
 * @param device    a line from /proc/mounts.
 */
std::string Mounts::realMountPoint(const std::string& device)
{
    struct stat st;


    if (stat(device.c_str(), &st) == -1)
    {
        if (device == "/dev/root" || device == "rootfs")
        {
            // SPECIAL CASE
            std::ifstream cmdlinestream("/proc/cmdline");
            std::string entry;


            PRINT("realMountPoint(" << device << ")");
            while (!cmdlinestream.eof())
            {
                cmdlinestream >> entry;

                if (entry.substr(0, 5) == "root=")
                {
                    return fixDeviceWithMount(entry.substr(5));
                }
            }
        }
    }

    return device;
}

bool Mounts::parseLinuxMountsLine(const std::string& line, std::string& device, std::string& mountpoint, std::string& filesystem)
{
    size_t on, type, space;


    on = line.find(" on ");
    type = line.find(" type ");

    if (on == std::string::npos || type == std::string::npos)
    {
        return false;
    }

    space = line.find(' ', type + 6);
    if (space == std::string::npos)
    {
        return false;
    }

    device = line.substr(0, on);
    mountpoint = line.substr(on + 4, type - on - 4);
    filesystem = line.substr(type + 6, space - type - 6);

    return true;
}

bool Mounts::parseLinuxProcMountsLine(const std::string& line, std::string& device, std::string& mountpoint, std::string& filesystem)
{
    std::istringstream ist(line);


    ist >> device >> mountpoint >> filesystem;

    // Whitespace is octal escaped in /proc/mounts so we have to handle it
    device = octalUnescape(device);
    mountpoint = octalUnescape(mountpoint);

    if (device == "" || mountpoint == "" || filesystem == "")
    {
        return false;
    }

    return true;
}

/**
 * Use mount -f -n -v to determine the real mount point if the
 * device begins with 'LABEL=' or 'UUID='.
 */
std::string Mounts::fixDeviceWithMount(const std::string& device)
{
    PRINT("fixDeviceWithMount(" << device << ")");
    size_t equals = device.find('=');
    std::string result = device;


    if (equals != std::string::npos)
    {
        std::string prefix = device.substr(0, equals);
        if (prefix == "LABEL" || prefix == "UUID")
        {
            StringSet args("findfs");


            args.push_back(device);

            // result is "" if findfs doesn't exist.
            result = Mounts::scrape("/sbin/findfs", args);

            if (result != "")
            {
                // first line only please.
                equals = result.find('\n');
                if (equals != std::string::npos)
                {
                    result = result.substr(0, equals);
                }
            }
            else
            {
                args = StringSet("mount -f -n -v", ' ');


                args.push_back(device);
                args.push_back("/");


                std::string output = Mounts::scrape("/bin/mount", args);
                // output is probably going to be "" if user is not root.  But
                // its worth a shot.
                if (output != "")
                {
                    equals = output.find(' ');

                    if (equals != std::string::npos)
                    {
                        result = output.substr(0 ,equals);
                    }
                }
            }
        }
    }
    return result;
}

/**
 * Given a device ID, return the major device name.
 *
 * @param deviceID
 */
std::string Mounts::majorName(dev_t deviceID)
{
    std::string line;
    std::ifstream ifst("/proc/devices");

    //DBGOUT("  looking up major name for device " << deviceID);

    if (!ifst)
    {
        throw std::runtime_error("Unable to open /proc/devices");
    }

    while (!std::getline(ifst, line).eof())
    {
        if (line == "")
        {
            // Character devices are followed by a blank line, and then by
            // block devices.  Checking for the blank line.
            break;
        }
    }
    if (line != "")
    {
        throw std::runtime_error("Unable to find block devices in /proc/devices");
    }

    while(!std::getline(ifst, line).eof())
    {
        std::istringstream  ist(line);
        unsigned            int id = 0;
        std::string         name;


        ist >> id >> name;
        if (static_cast<int>(id) == static_cast<int>(major(deviceID)))
        {
            return name;
        }
    }

    throw std::runtime_error("Unable to find requested block device in /proc/devices");
}

/**
 * Iterator for the list of mount points.
 */
std::vector<IMountPoint*> Mounts::mountPoints()
{
    return m_devices;
}

std::string Mounts::Drive::filesystemType() const
{
    return m_fileSystem;
}

bool Mounts::Drive::isHardDisc() const
{
    return DeviceUtil::isLocalFixed(device(),mountPoint(),filesystemType());
}

bool Mounts::Drive::isNetwork() const
{
    return DeviceUtil::isNetwork(device(),mountPoint(),filesystemType());
}

bool Mounts::Drive::isOptical() const
{
    return DeviceUtil::isOptical(device(),mountPoint(),filesystemType());
}

bool Mounts::Drive::isRemovable() const
{
    return DeviceUtil::isRemovable(device(),mountPoint(),filesystemType());
}

/**
 * @return true if this is a special filesystem mount that we should avoid
 * scanning.
 */
bool Mounts::Drive::isSpecial() const
{
    return DeviceUtil::isSystem(device(),mountPoint(),filesystemType());
}

