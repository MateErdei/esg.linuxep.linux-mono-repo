/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Mounts.h"

#include "DeviceUtil.h"
#include "Logger.h"

// Standard C++
#include <cstdlib>
#include <sstream>
#include <fstream>

// Standard C
#include <fstab.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace avscanner::avscannerimpl;

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
                buf += ch; // store characters in case it turns out not to be an octal escape
                if (ch < '0' || ch > '7')
                {
                    break;
                }
                val = val * 8 + (ch - '0');
            }
            if (i == 3) // looks like it was a full octal sequence
            {
                out += (char)val;
            }
            else // does not look like an octal sequence
            {
                out += buf;
            }
        }
        else
        {
            out += ch;
        }
    }

    return out;
}

/**
 * constructor
 */
Mounts::Mounts()
{
    parseProcMounts();
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

        //LOGDEBUG("line: " << line);

        if (!parseLinuxProcMountsLine(line, device, mountPoint, type))
        {
            continue;
        }

        device = realMountPoint(device);
        //LOGDEBUG("dev " << device << " on " << mountPoint << " type " << type);
        m_devices.push_back(std::make_shared<Drive>(device, mountPoint, type));
    }

    if (device("/").empty())
    {
        // Check the root device is included, because there's no chance it isn't mounted.
        // FIXME: getfsfile is obsolete - if we really want to have this block getmntent should be used.
        struct fstab* mountpoint = getfsfile("/");


        if ((mountpoint != nullptr) && (mountpoint->fs_spec != nullptr))
        {
            m_devices.push_back(std::make_shared<Drive>(
                        fixDeviceWithMount(mountpoint->fs_spec),
                        mountpoint->fs_file,
                        mountpoint->fs_type));
        }
    }
}

/**
 * Determines which device is mounted on a particular mountpoint.
 *
 * @param mountPoint
 */
std::string Mounts::device(const std::string& mountPoint) const
{
    for (auto & it : m_devices)
    {
        if (it->mountPoint() == mountPoint)
        {
            return it->device();
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
Mounts::Drive::Drive(std::string device, std::string mountPoint, std::string type)
: m_mountPoint(std::move(mountPoint))
, m_device(std::move(device))
, m_fileSystem(std::move(type))
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

/**
 * Run a command and collect the output.
 * @return an empty string on error.  This is obviously ambiguous with
 * successfully running a command which returns no output; however for our needs
 * that behaviour is equivalent to a failure.
 *
 * @param path    Command to run
 * @param args    arguments.
 */
std::string Mounts::scrape(const std::string& path, const std::vector<std::string>& args)
{
    std::string result;


    if (access(path.c_str(), X_OK) == 0)
    {
        // We have permission to execute the command
        int fd[2];
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


                    for (auto & it : args)
                    {
                        argv[index] = new char[it.size() + 1];
                        memcpy(argv[index], it.c_str(), it.size() + 1);
                        index++;
                    }
                    argv[index] = nullptr;

                    execv(path.c_str(), argv);
                    // never returns
                    LOGERROR("execv failed");
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
                        LOGWARN(path << " failed to execute");
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

bool Mounts::parseLinuxProcMountsLine(const std::string& line, std::string& device, std::string& mountpoint, std::string& filesystem)
{
    std::istringstream ist(line);


    ist >> device >> mountpoint >> filesystem;

    // Whitespace is octal escaped in /proc/mounts so we have to handle it
    device = octalUnescape(device);
    mountpoint = octalUnescape(mountpoint);

    if (device.empty() || mountpoint.empty() || filesystem.empty())
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
    size_t equals = device.find('=');
    std::string result = device;


    if (equals != std::string::npos)
    {
        std::string prefix = device.substr(0, equals);
        if (prefix == "LABEL" || prefix == "UUID")
        {
            std::vector<std::string> args;
            args.emplace_back("findfs");
            args.push_back(device);

            // result is "" if findfs doesn't exist.
            result = Mounts::scrape("/sbin/findfs", args);

            if (result.empty())
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
                args.clear();
                args.emplace_back("mount -f -n -v");
                args.push_back(device);
                args.emplace_back("/");


                std::string output = Mounts::scrape("/bin/mount", args);
                // output is probably going to be "" if user is not root.  But
                // its worth a shot.
                if (output.empty())
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
 * Iterator for the list of mount points.
 */
std::vector<std::shared_ptr<IMountPoint>> Mounts::mountPoints()
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

