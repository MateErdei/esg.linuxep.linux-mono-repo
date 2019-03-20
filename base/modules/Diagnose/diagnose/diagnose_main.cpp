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
            Path destination = gatherFiles.createDiagnoseFolder(outputDir);

            // Perform product file copying.
            gatherFiles.copyBaseFiles(destination);
            gatherFiles.copyPluginFiles(destination);

            //grab systme log files
            gatherFiles.copyAllOfInterestFromDir("/var/log/audit/", destination);

            // Run any system commands that we cant to capture the output from.
            SystemCommands systemCommands(destination);
            systemCommands.runCommandOutputToFile("df -h", "df");
            systemCommands.runCommandOutputToFile("top -bHn1", "top");
            systemCommands.runCommandOutputToFile("dstat -a -m 1 5", "dstat");
            systemCommands.runCommandOutputToFile("iostat 1 5", "iostat");
            systemCommands.runCommandOutputToFile("hostnamectl", "hostnamectl");
            systemCommands.runCommandOutputToFile("uname -a", "uname");
            systemCommands.runCommandOutputToFile("lscpu", "lscpu");
            //Doesn't work on Amazon
            systemCommands.runCommandOutputToFile("lshw", "lshw");
            systemCommands.runCommandOutputToFile("ls -l /lib/systemd/system", "systemd");
            systemCommands.runCommandOutputToFile("ls -l /usr/lib/systemd/system", "systemd");
            systemCommands.runCommandOutputToFile("systemctl list-unit-files", "list-unit-files");
            systemCommands.runCommandOutputToFile("auditctl -l", "auditctl");
            systemCommands.runCommandOutputToFile("systemctl status auditd", "systemctl-status-auditd");
            systemCommands.runCommandOutputToFile("ls /etc/audisp/plugins.d/", "plugins.d");
            systemCommands.runCommandOutputToFile("journalctl -u sophos-spl", "journalctl-sophos-spl");
            systemCommands.runCommandOutputToFile("journalctl -u auditd", "journalctl-auditd");

            //todo We need to check this one... is it correct?
            systemCommands.runCommandOutputToFile("journalctl_TRANSPORT=audit", "journalctl_TRANSPORT=audit");
            systemCommands.runCommandOutputToFile("journalctl --since yesterday | grep -v audit", "journalctl-auditd-yesterday");
            systemCommands.runCommandOutputToFile("ps -ef", "ps");
            systemCommands.runCommandOutputToFile("getenforce", "getenforce");
            systemCommands.runCommandOutputToFile("ldd --version", "ldd-version");
            systemCommands.runCommandOutputToFile("route -n", "route");
            systemCommands.runCommandOutputToFile("ip route", "ip-route");
            systemCommands.runCommandOutputToFile("dmesg", "dmesg");
            systemCommands.runCommandOutputToFile("env", "env");
            systemCommands.runCommandOutputToFile("ss -an", "ss");
            systemCommands.runCommandOutputToFile("uptime", "uptime");
            systemCommands.runCommandOutputToFile("mount", "mount");
            systemCommands.runCommandOutputToFile("pstree -ap", "pstree");
            systemCommands.runCommandOutputToFile("lsmod", "lsmod");
            systemCommands.runCommandOutputToFile("lspci", "lspci");
            systemCommands.runCommandOutputToFile("ls -alR " + installDir, "ListAllFilesInSSPLDir");
            systemCommands.runCommandOutputToFile("du -h " + installDir + " --max-depth=2", "DiskSpaceOfSSPL");
            systemCommands.runCommandOutputToFile("ifconfig -a", "ifconfig");
            systemCommands.runCommandOutputToFile("ip addr", "ip-addr");
            systemCommands.runCommandOutputToFile("sysctl -a", "sysctl");
            systemCommands.runCommandOutputToFile("rpm -qa", "rpm-pkgs");
            systemCommands.runCommandOutputToFile("dpkg --get-selections", "dpkg-pkgs");
            systemCommands.runCommandOutputToFile("yum -y list installed", "yum-pkgs");
            systemCommands.runCommandOutputToFile("zypper se  -i", "zypper-pkgs");
            systemCommands.runCommandOutputToFile("apt list --installed", "apt-pkgs");
            systemCommands.runCommandOutputToFile("ldconfig -p", "ldconfig");

            // Copy any files that contain useful info to the output dir.
            Common::FileSystem::FileSystemImpl fileSystem;
            fileSystem.copyFile("/etc/os-release", Common::FileSystem::join(outputDir, "os-release"));
            fileSystem.copyFile("/proc/cpuinfo", Common::FileSystem::join(outputDir, "cpuinfo"));
            fileSystem.copyFile("/proc/meminfo", Common::FileSystem::join(outputDir, "meminfo"));
            fileSystem.copyFile("/etc/selinux/config", Common::FileSystem::join(outputDir, "selinux-config"));
            fileSystem.copyFile("/etc/fstab", Common::FileSystem::join(outputDir, "fstab"));
            fileSystem.copyFile("/var/log/boot.log", Common::FileSystem::join(outputDir, "boot.log"));
            fileSystem.copyFile("/etc/rsylog.conf", Common::FileSystem::join(outputDir, "rsylog.conf"));
            fileSystem.copyFile("/etc/hosts", Common::FileSystem::join(outputDir, "hosts"));
            fileSystem.copyFile("/etc/resolve.conf", Common::FileSystem::join(outputDir, "resolve.conf"));
            fileSystem.copyFile("/etc/systemd/system.conf", Common::FileSystem::join(outputDir, "systemd-system.conf"));

        }
        catch (std::invalid_argument& e)
        {
            std::cout << e.what() << std::endl;
            return 2;
        }
        return 0;
    }

} // namespace diagnose