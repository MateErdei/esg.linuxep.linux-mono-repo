/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "diagnose_main.h"

#include "GatherFiles.h"
#include "SystemCommands.h"

#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <string>

namespace
{
    std::string workOutInstallDirectory()
    {
        // Check if we have an environment variable telling us the installation location
        char* SOPHOS_INSTALL = secure_getenv("SOPHOS_INSTALL");
        if (SOPHOS_INSTALL != nullptr)
        {
            return SOPHOS_INSTALL;
        }

        // If we can't get the cwd then use a fixed string.
        return "/opt/sophos-spl";
    }
} // namespace
namespace diagnose
{
    int diagnose_main::main(int argc, char* argv[])
    {
        if (argc > 2)
        {
            std::cout << "Expecting only one parameter got " << (argc - 1) << std::endl;
            return 1;
        }

        std::string outputDir = ".";
        if (argc == 2)
        {
            std::string arg(argv[1]);
            if (arg == "--help")
            {
                std::cout << "Expected Usage: ./sophos_diagnose <path_to_output_directory>" << std::endl;
                return 0;
            }
            outputDir = arg;
        }

        try
        {
            const std::string installDir = workOutInstallDirectory();

            // Setup the file gatherer.
            GatherFiles gatherFiles;
            gatherFiles.setInstallDirectory(installDir);

            // The top level directory in the output directory structure
            Path destination = gatherFiles.createRootDir(outputDir);

            // All the base log files etc
            Path baseFilesDir = gatherFiles.createBaseFilesDir(destination);

            // All the plugin log files etc
            Path pluginFilesDir = gatherFiles.createPluginFilesDir(destination);

            // All the system files and output from system commands
            Path systemFilesDir = gatherFiles.createSystemFilesDir(destination);

            // Perform product file copying.
            gatherFiles.copyBaseFiles(baseFilesDir);
            gatherFiles.copyPluginFiles(pluginFilesDir);

            // Grab audit log files
            gatherFiles.copyAllOfInterestFromDir("/var/log/audit/", systemFilesDir);

            // Run any system commands that we cant to capture the output from.
            SystemCommands systemCommands(systemFilesDir);
            systemCommands.runCommand("df -h", "df");
            systemCommands.runCommand("top -bHn1", "top");
            systemCommands.runCommand("dstat -a -m 1 5", "dstat");
            systemCommands.runCommand("iostat 1 5", "iostat");
            systemCommands.runCommand("hostnamectl", "hostnamectl");
            systemCommands.runCommand("uname -a", "uname");
            systemCommands.runCommand("lscpu", "lscpu");
            systemCommands.runCommand("lshw", "lshw");  // Doesn't work on Amazon
            systemCommands.runCommand("ls -l /lib/systemd/system", "systemd");
            systemCommands.runCommand("ls -l /usr/lib/systemd/system", "usr-systemd");
            systemCommands.runCommand("systemctl list-unit-files", "list-unit-files");
            systemCommands.runCommand("auditctl -l", "auditctl");
            systemCommands.runCommand("systemctl status auditd", "systemctl-status-auditd");
            systemCommands.runCommand("ls /etc/audisp/plugins.d/", "plugins.d");
            systemCommands.runCommand("journalctl -u sophos-spl", "journalctl-sophos-spl");
            systemCommands.runCommand("journalctl -u auditd", "journalctl-auditd");

            // todo We need to check this one... is it correct?
            systemCommands.runCommand("journalctl_TRANSPORT=audit", "journalctl_TRANSPORT=audit");
            systemCommands.runCommand("journalctl --since yesterday | grep -v audit", "journalctl-auditd-yesterday");
            systemCommands.runCommand("ps -ef", "ps");
            systemCommands.runCommand("getenforce", "getenforce");
            systemCommands.runCommand("ldd --version", "ldd-version");
            systemCommands.runCommand("route -n", "route");
            systemCommands.runCommand("ip route", "ip-route");
            systemCommands.runCommand("dmesg", "dmesg");
            systemCommands.runCommand("env", "env");
            systemCommands.runCommand("ss -an", "ss");
            systemCommands.runCommand("uptime", "uptime");
            systemCommands.runCommand("mount", "mount");
            systemCommands.runCommand("pstree -ap", "pstree");
            systemCommands.runCommand("lsmod", "lsmod");
            systemCommands.runCommand("lspci", "lspci");
            systemCommands.runCommand("ls -alR " + installDir, "ListAllFilesInSSPLDir");
            systemCommands.runCommand("du -h " + installDir + " --max-depth=2", "DiskSpaceOfSSPL");
            systemCommands.runCommand("ifconfig -a", "ifconfig");
            systemCommands.runCommand("ip addr", "ip-addr");
            systemCommands.runCommand("sysctl -a", "sysctl");
            systemCommands.runCommand("rpm -qa", "rpm-pkgs");
            systemCommands.runCommand("dpkg --get-selections", "dpkg-pkgs");
            systemCommands.runCommand("yum -y list installed", "yum-pkgs");
            systemCommands.runCommand("zypper se  -i", "zypper-pkgs");
            systemCommands.runCommand("apt list --installed", "apt-pkgs");
            systemCommands.runCommand("ldconfig -p", "ldconfig");

            // Copy any files that contain useful info to the output dir.

            systemCommands.copyFile("/etc/os-release", Common::FileSystem::join(outputDir, "os-release"));
            systemCommands.copyFile("/proc/cpuinfo", Common::FileSystem::join(outputDir, "cpuinfo"));
            systemCommands.copyFile("/proc/meminfo", Common::FileSystem::join(outputDir, "meminfo"));
            systemCommands.copyFile("/etc/selinux/config", Common::FileSystem::join(outputDir, "selinux-config"));
            systemCommands.copyFile("/etc/fstab", Common::FileSystem::join(outputDir, "fstab"));
            systemCommands.copyFile("/var/log/boot.log", Common::FileSystem::join(outputDir, "boot.log"));
            systemCommands.copyFile("/etc/rsylog.conf", Common::FileSystem::join(outputDir, "rsylog.conf"));
            systemCommands.copyFile("/etc/hosts", Common::FileSystem::join(outputDir, "hosts"));
            systemCommands.copyFile("/etc/resolve.conf", Common::FileSystem::join(outputDir, "resolve.conf"));
            systemCommands.copyFile(
                "/etc/systemd/system.conf", Common::FileSystem::join(outputDir, "systemd-system.conf"));
        }
        catch (std::invalid_argument& e)
        {
            std::cout << e.what() << std::endl;
            return 2;
        }
        return 0;
    }

} // namespace diagnose