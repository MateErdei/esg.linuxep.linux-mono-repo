// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "Mounts.h"

#include "Drive.h"
#include "Logger.h"

#include "Common/UtilityImpl/StringUtils.h"

// Standard C++
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

// Standard C
#include <fstab.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace mount_monitor::mountinfo;
using namespace mount_monitor::mountinfoimpl;

std::string octalUnescape(const std::string& input)
{
    std::string out;

    char* ptr = const_cast<char*>(input.c_str()); // We just want to read
    while (*ptr != 0)
    {
        char ch = *ptr++;
        if (ch == '\\')
        {
            std::string buf("\\");
            size_t i; // NOLINT(cppcoreguidelines-init-variables)
            unsigned int val = 0;

            for (i = 0; i < 3; i++)
            {
                ch = *ptr++;
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
Mounts::Mounts(ISystemPathsSharedPtr systemPaths) : m_systemPaths(std::move(systemPaths))
{
    assert(m_systemPaths);
    parseProcMounts();
}

/**
 * Try and parse /proc/mounts.
 */
void Mounts::parseProcMounts()
{
    std::string mountInfoFilePath = m_systemPaths->mountInfoFilePath();
    if (mountInfoFilePath.empty())
    {
        throw std::runtime_error("SystemPaths return empty string for mountInfoFilePath!");
    }
    std::ifstream mountstream(mountInfoFilePath);

    if (!mountstream)
    {
        throw std::system_error(errno, std::system_category(), "Unable to access /proc/mounts, reason: ");
    }

    while (!mountstream.eof())
    {
        std::string device;
        std::string type;
        std::string mountPoint;

        std::string line;
        std::getline(mountstream, line);

        // LOGDEBUG("line: " << line);

        if (!parseLinuxProcMountsLine(line, device, mountPoint, type))
        {
            continue;
        }

        device = realMountPoint(device);
        // LOGDEBUG("dev " << device << " on " << mountPoint << " type " << type);
        try
        {
            bool isDir = std::filesystem::is_directory(mountPoint);
            m_devices[mountPoint] = std::make_shared<Drive>(device, mountPoint, type, isDir);
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            LOGDEBUG("Failed to determine if " << mountPoint << " is a directory or not, skipping: " << e.what());
        }
    }

    if (device("/").empty())
    {
        // Check the root device is included, because there's no chance it isn't mounted.
        // FIXME: getfsfile is obsolete - if we really want to have this block getmntent should be used.
        struct fstab* mountpoint = getfsfile("/");

        if ((mountpoint != nullptr) && (mountpoint->fs_spec != nullptr))
        {
            std::string devicePath = fixDeviceWithMount(mountpoint->fs_spec);
            try
            {
                bool isDir = std::filesystem::is_directory( mountpoint->fs_file);
                m_devices[mountpoint->fs_file] = 
                    std::make_shared<Drive>(devicePath, mountpoint->fs_file, mountpoint->fs_type, isDir);
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                LOGDEBUG("Failed to determine if " << devicePath << " is a directory or not, skipping: " << e.what());
            }
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
    if ( m_devices.find(mountPoint) != m_devices.end() )
    {
        return m_devices.at(mountPoint)->device();
    }
    else
    {
        return "";
    }
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
            // pre-process arguments for child process
            char** argv = new char*[args.size() + 1];
            int index = 0;

            for (const auto& it : args)
            {
                argv[index] = new char[it.size() + 1];
                memcpy(argv[index], it.c_str(), it.size() + 1);
                index++;
            }
            argv[index] = nullptr;
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

                        execv(path.c_str(), argv);
                        // never returns
                        _exit(1);
                    }
                default:
                {
                    int status = 0;

                    close(fd[1]);

                    {
                        char buf[64];
                        ssize_t bytes; // NOLINT(cppcoreguidelines-init-variables)
                        do
                        {
                            bytes = ::read(fd[0], buf, 64);

                            if (bytes > 0)
                            {
                                // remove trailing newline
                                result.append(buf, bytes - 1);
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

                    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) // NOLINT(hicpp-signed-bitwise)
                    {
                        LOGERROR("Running " << path << " failed");
                        result = "";
                    }
                    break;
                }
            }
            // clean up manually allocated memory
            for (; index >= 0; index--)
            {
                delete[] argv[index];
            }
            delete[] argv;
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
    struct stat st
    {
    };

    if (stat(device.c_str(), &st) == -1)
    {
        if (device == "/dev/root" || device == "rootfs")
        {
            // SPECIAL CASE
            std::string cmdlineInfoFilePath = m_systemPaths->cmdlineInfoFilePath();
            if (cmdlineInfoFilePath.empty())
            {
                throw std::runtime_error("System Paths had empty cmdline Info file Path!");
            }

            std::ifstream cmdlinestream(cmdlineInfoFilePath);
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

bool Mounts::parseLinuxProcMountsLine(
    const std::string& line,
    std::string& device,
    std::string& mountpoint,
    std::string& filesystem)
{
    std::istringstream ist(line);

    ist >> device >> mountpoint >> filesystem;

    // Whitespace is octal escaped in /proc/mounts so we have to handle it
    device = octalUnescape(device);
    mountpoint = octalUnescape(mountpoint);

    return !(device.empty() || mountpoint.empty() || filesystem.empty());
}

/**
 * Use findfs or mount -f -n -v to determine the real mount point if the
 * device begins with 'LABEL=' or 'UUID='.
 */
std::string Mounts::fixDeviceWithMount(const std::string& device)
{
    auto equals = device.find('=');
    if (equals == std::string::npos)
    {
        // not key=value
        return device;
    }

    std::string prefix = device.substr(0, equals);
    if (prefix != "LABEL" && prefix != "UUID")
    {
        // Not actually LABEL= or UUID=
        return device;
    }

    // Try findfs first
    std::vector<std::string> args;
    args.emplace_back("findfs");
    args.emplace_back(device);

    // result is "" if findfs doesn't exist.
    std::string result = Mounts::scrape(m_systemPaths->findfsCmdPath(), args);

    if (!result.empty())
    {
        // first line only please.
        auto newline = result.find('\n');
        if (newline != std::string::npos)
        {
            result = result.substr(0, newline);
        }
        return result;
    }

    // findfs failed - try mount instead
    assert(result.empty()); // Only want to do this if findfs failed
    args.clear();
    args.emplace_back("mount");
    args.emplace_back("-fnv");
    args.emplace_back(device);
    args.emplace_back("/");

    std::string output = Mounts::scrape(m_systemPaths->mountCmdPath(), args);
    // "mount: /dev/rootfs mounted on /." on Ubuntu 18.04
    // output is probably going to be "" if user is not root.
    // But it's worth a shot.
    auto first_space = output.find(' ');
    if (first_space != std::string::npos)
    {
        first_space += 1;
        auto second_space = output.find(' ', first_space);
        assert(first_space != second_space);
        if (second_space != std::string::npos)
        {
            return output.substr(first_space, second_space - first_space); // only return if we got something useful.
        }
    }

    return device;
}

/**
 * Iterator for the list of mount points.
 */
IMountPointSharedVector Mounts::mountPoints()
{
    IMountPointSharedVector mountPoints;
    for (auto const& [mount, device] : m_devices)
    {
        mountPoints.push_back(device);
    }
    return mountPoints;
}

IMountPointSharedPtr Mounts::getMountFromPath(const std::string& childPath)
{
    // /proc/mounts is ordered by successive mount layers and therefore the last match is the best one
    IMountPointSharedPtr lastMatchingMountDir = nullptr;
    for (auto const& [mount, device] : m_devices)
    {
        std::string mountDir = mount;
        if (mountDir.back() != '/')
        {
            mountDir += '/';
        }
        if (Common::UtilityImpl::StringUtils::startswith(childPath, mountDir))
        {
            LOGDEBUG("Found potential parent: " << device->device() << " -- at path: " << mountDir);
            lastMatchingMountDir = device;
        }
    }
    if (lastMatchingMountDir == nullptr)
    {
        throw std::runtime_error("No parent mounts found for path: " + childPath);
    }
    LOGDEBUG("Best fit parent found: " << lastMatchingMountDir->device() << " -- at path: " << lastMatchingMountDir->mountPoint());
    return lastMatchingMountDir;
}
