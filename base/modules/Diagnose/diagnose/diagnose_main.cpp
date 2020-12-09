/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "diagnose_main.h"

#include "CheckForTar.h"
#include "GatherFiles.h"
#include "Logger.h"
#include "SystemCommands.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Logging/ConsoleFileLoggingSetup.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>
#include <iostream>
#include <sstream>

namespace diagnose
{
    int diagnose_main::main(int argc, char* argv[])
    {
        if (argc > 2)
        {
            std::cerr << "Expecting only one parameter got " << (argc - 1) << std::endl;
            return 1;
        }

        std::string outputDir = ".";
        if (argc == 2)
        {
            std::string arg(argv[1]);
            if (arg == "--help")
            {
                std::cerr << "Expected Usage: ./sophos_diagnose <path_to_output_directory>" << std::endl;
                return 0;
            }
            outputDir = arg;
        }

        if (!diagnose::CheckForTar::isTarAvailable())
        {
            std::cerr << "tar command not available: unable to generate diagnose output" << std::endl;
            return 4;
        }

        try
        {
            // Set the umask for the diagnose tool to remove other users' permissions
            umask(007);

            Common::Logging::ConsoleFileLoggingSetup logging("diagnose");

            const std::string installDir = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

            // Setup the file gatherer.
            auto filesystemPtr =
                std::make_unique<Common::FileSystem::FileSystemImpl>(Common::FileSystem::FileSystemImpl());
            GatherFiles gatherFiles(std::move(filesystemPtr));
            gatherFiles.setInstallDirectory(installDir);

            // Create the top level directory in the output directory structure
            Path destination = gatherFiles.createRootDir(outputDir);

            // Create the dir for all the base log files etc
            Path baseFilesDir = gatherFiles.createBaseFilesDir(destination);

            // Create the dir for all the plugin log files etc
            Path pluginFilesDir = gatherFiles.createPluginFilesDir(destination);

            // Create the dir for all the system files and output from system commands
            Path systemFilesDir = gatherFiles.createSystemFilesDir(destination);

            // Copy all files of interest from base.
            gatherFiles.copyBaseFiles(baseFilesDir);

            // Copy additional component generated files
            gatherFiles.copyFilesInComponentDirectories(destination);

            // Copy all files of interest from all the plugins.
            gatherFiles.copyPluginFiles(pluginFilesDir);

            // Copy all audit log files.
            gatherFiles.copyAllOfInterestFromDir("/var/log/audit/", systemFilesDir);

            // other formats of the timestamp in '-since=<timestamp>' result in parse errors as of journalctl version
            // 237
            std::string logCollectionInterval("--since=-10days");

            // Run any system commands that we cant to capture the output from.
            SystemCommands systemCommands(systemFilesDir);

            systemCommands.runCommand("df", { "-h" }, "df");
            systemCommands.runCommand("top", { "-bHn1" }, "top");
            systemCommands.runCommand("dstat", { "-a", "-m", "1", "5" }, "dstat");
            systemCommands.runCommand("iostat", { "1", "5" }, "iostat");
            systemCommands.runCommand("hostnamectl", std::vector<std::string>(), "hostnamectl");
            systemCommands.runCommand("uname", { "-a" }, "uname");
            systemCommands.runCommand("lscpu", std::vector<std::string>(), "lscpu");
            systemCommands.runCommand("lshw", std::vector<std::string>(), "lshw"); // Doesn't work on Amazon
            systemCommands.runCommand("ls", { "-l", "/lib/systemd/system" }, "systemd");
            systemCommands.runCommand("ls", { "-l", "/usr/lib/systemd/system" }, "usr-systemd");
            systemCommands.runCommand("auditctl", { "-l" }, "auditctl");
            systemCommands.runCommand("systemctl", { "status", "auditd" }, "systemctl-status-auditd");
            systemCommands.runCommand("systemctl", { "list-unit-files" }, "list-unit-files");
            systemCommands.runCommand("systemctl", { "status", "sophos-spl" }, "systemctl-status-sophos-spl");
            systemCommands.runCommand(
                "systemctl", { "status", "sophos-spl-update" }, "systemctl-status-sophos-spl-update");
            systemCommands.runCommand("ls", { "/etc/audisp/plugins.d/" }, "plugins.d");
            systemCommands.runCommand(
                "journalctl", { logCollectionInterval, "-u", "sophos-spl" }, "journalctl-sophos-spl");
            systemCommands.runCommand("journalctl", { logCollectionInterval, "-u", "auditd" }, "journalctl-auditd");
            systemCommands.runCommand(
                "journalctl", { logCollectionInterval, "_TRANSPORT=audit" }, "journalctl_TRANSPORT=audit");
            systemCommands.runCommand("journalctl", { "--since=yesterday" }, "journalctl-yesterday");
            systemCommands.runCommand("ps", { "-ef" }, "ps");
            systemCommands.runCommand("getenforce", {}, "getenforce");
            systemCommands.runCommand("ldd", { "--version" }, "ldd-version");
            systemCommands.runCommand("route", { "-n" }, "route");
            systemCommands.runCommand("ip", { "route" }, "ip-route");
            systemCommands.runCommand("dmesg", {}, "dmesg");
            systemCommands.runCommand("env", {}, "env");
            systemCommands.runCommand("ss", { "-an" }, "ss");
            systemCommands.runCommand("uptime", {}, "uptime");
            systemCommands.runCommand("mount", std::vector<std::string>(), "mount");
            systemCommands.runCommand("pstree", { "-ap" }, "pstree");
            systemCommands.runCommand("lsmod", std::vector<std::string>(), "lsmod");
            systemCommands.runCommand("lspci", std::vector<std::string>(), "lspci");
            systemCommands.runCommand("ls", { "-alR", installDir }, "ListAllFilesInSSPLDir");
            systemCommands.runCommand("du", { "-h", installDir, "--max-depth=2" }, "DiskSpaceOfSSPL");
            systemCommands.runCommand("ifconfig", { "-a" }, "ifconfig");
            systemCommands.runCommand("ip", { "addr" }, "ip-addr");
            systemCommands.runCommand("sysctl", { "-a" }, "sysctl");
            systemCommands.runCommand("rpm", { "-qa" }, "rpm-pkgs");
            systemCommands.runCommand("dpkg", { "--get-selections" }, "dpkg-pkgs");
            systemCommands.runCommand("yum", { "-y", "list", "installed" }, "yum-pkgs");
            systemCommands.runCommand("zypper", { "se", "-i" }, "zypper-pkgs");
            systemCommands.runCommand("apt", { "list", "--installed" }, "apt-pkgs");
            systemCommands.runCommand("ldconfig", { "-p" }, "ldconfig");

            // Copy any files that contain useful info to the output dir.
            gatherFiles.copyFile("/etc/os-release", Common::FileSystem::join(systemFilesDir, "os-release"));
            gatherFiles.copyFile("/proc/cpuinfo", Common::FileSystem::join(systemFilesDir, "cpuinfo"));
            gatherFiles.copyFile("/proc/meminfo", Common::FileSystem::join(systemFilesDir, "meminfo"));
            gatherFiles.copyFile("/etc/selinux/config", Common::FileSystem::join(systemFilesDir, "selinux-config"));
            gatherFiles.copyFile("/etc/fstab", Common::FileSystem::join(systemFilesDir, "fstab"));
            gatherFiles.copyFile("/var/log/boot.log", Common::FileSystem::join(systemFilesDir, "boot.log"));
            gatherFiles.copyFile("/etc/rsylog.conf", Common::FileSystem::join(systemFilesDir, "rsylog.conf"));
            gatherFiles.copyFile("/etc/hosts", Common::FileSystem::join(systemFilesDir, "hosts"));
            gatherFiles.copyFile("/etc/resolve.conf", Common::FileSystem::join(systemFilesDir, "resolve.conf"));
            gatherFiles.copyFile(
                "/etc/systemd/system.conf", Common::FileSystem::join(systemFilesDir, "systemd-system.conf"));

            LOGINFO("Completed gathering files.");

            gatherFiles.copyDiagnoseLogFile(destination);

            systemCommands.tarDiagnoseFolder(destination, outputDir);
        }
        catch (std::invalid_argument& e)
        {
            std::cerr << e.what() << std::endl;
            return 2;
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            std::cerr << "File system error: " << e.what() << std::endl;
            return 3;
        }

        return 0;
    }

} // namespace diagnose