// Copyright 2023 Sophos All rights reserved.

#include "Telemetry.h"
// Package
#include "Logger.h"
// Component
#include "common/ApplicationPaths.h"
#include "common/PluginUtils.h"
#include "common/StatusFile.h"
#include "datatypes/sophos_filesystem.h"
// Product
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
// Std C++
#include <fstream>

using namespace Plugin;
namespace fs = sophos_filesystem;

namespace
{
    std::string getLrDataHash()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path filerep(appConfig.getData("PLUGIN_INSTALL"));
        filerep /= "chroot/susi/update_source/reputation/filerep.dat";
        fs::path signerrep(appConfig.getData("PLUGIN_INSTALL"));
        signerrep /= "chroot/susi/update_source/reputation/signerrep.dat";

        std::ifstream filerepFs(filerep, std::ifstream::in);
        std::ifstream signerrepFs(signerrep, std::ifstream::in);

        if (filerepFs.good() && signerrepFs.good())
        {
            std::stringstream lrDataContents;
            lrDataContents << filerepFs.rdbuf() << signerrepFs.rdbuf();
            return Common::SslImpl::calculateDigest(Common::SslImpl::Digest::sha256, lrDataContents.str());
        }
        return "unknown";
    }

    std::string getMlLibHash()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path mlLib(appConfig.getData("PLUGIN_INSTALL"));
        mlLib /= "chroot/susi/update_source/model/model.so";
        std::ifstream mlLibFs (mlLib, std::ifstream::in);

        if(mlLibFs.good())
        {
            std::stringstream mlLibContents;
            mlLibContents << mlLibFs.rdbuf();
            return Common::SslImpl::calculateDigest(Common::SslImpl::Digest::sha256, mlLibContents.str());
        }
        return "unknown";
    }

    fs::path getVdbDir()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path vdbDir(appConfig.getData("PLUGIN_INSTALL"));
        vdbDir /= "chroot/susi/update_source/vdl";
        return vdbDir;
    }

    void add_vdl_telemetry(Common::Telemetry::TelemetryHelper& telemetry)
    {
        auto vdbDir = getVdbDir();
        if (!fs::is_directory(vdbDir))
        {
            telemetry.set("vdl-ide-count", 0L);
            return;
        }

        auto constexpr ide_filter = [](const fs::path& p) {
            return p.extension() == ".ide";
        };

        long ideCount = 0;
        fs::path lastIde;
        uintmax_t combinedSize = 0;
        for (const auto& item : fs::directory_iterator(vdbDir))
        {
            if (ide_filter(item))
            {
                ideCount++;
                auto stem = item.path().stem();
                if (stem > lastIde)
                {
                    lastIde = stem;
                }
            }
            combinedSize += fs::file_size(item);
        }

        telemetry.set("vdl-ide-count", ideCount);
        telemetry.set("vdl-newest-ide", lastIde);
        telemetry.set("vdl-size", combinedSize);
    }

    std::string getMlModelVersion()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path mlModel(appConfig.getData("PLUGIN_INSTALL"));
        mlModel /= "chroot/susi/update_source/model/model.dat";

        std::ifstream in( mlModel, std::ios::binary );
        std::string versionStr = "unknown";

        if (in.good())
        {
            // The version number is 4 bytes long starting from an offset of 28 and is little-endian
            std::uint32_t version;
            in.seekg(28*sizeof(char));
            in.read(reinterpret_cast<char*>(&version), sizeof(version));
            versionStr = std::to_string(version);
        }
        in.close();

        return versionStr;
    }

    int getProcessPidFromFile(Common::FileSystem::IFileSystem* fileSystem, const Path& pidFilePath)
    {
        int pid;
        std::string pidAsString;

        try
        {
            pidAsString = fileSystem->readFile(pidFilePath);

            std::pair<int, std::string> pidStringToIntResult =
                Common::UtilityImpl::StringUtils::stringToInt(pidAsString);
            if (!pidStringToIntResult.second.empty())
            {
                LOGWARN("Failed to read Pid file to int due to: " << pidStringToIntResult.second);
                return 0;
            }
            pid = pidStringToIntResult.first;
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGWARN("Error accessing pid file: " << pidFilePath << " due to: " << e.what());
            return 0;
        }

        try
        {
            if (!fileSystem->isDirectory("/proc/" + pidAsString))
            {
                LOGDEBUG("Process no longer running: " << pidAsString);
                return 0;
            }
        }
        catch (const Common::FileSystem::IFileSystemException& e)
        {
            LOGWARN("Error accessing proc directory of pid: " << pidAsString << " due to: " << e.what());
            return 0;
        }

        return pid;
    }
}

Telemetry::Telemetry(
    const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls,
    Common::FileSystem::IFileSystem* fs)
        : syscalls_(sysCalls),
          filesystem_(fs)
{
}


std::string Telemetry::getTelemetry()
{
    LOGSUPPORT("Received get telemetry request");
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    telemetry.set("lr-data-hash", getLrDataHash());
    telemetry.set("ml-lib-hash", getMlLibHash());
    telemetry.set("ml-pe-model-version", getMlModelVersion());
    telemetry.set("vdl-version", getVirusDataVersion());
    telemetry.set("version", common::getPluginVersion());
    telemetry.set("on-access-status", common::StatusFile::isEnabled(getOnAccessStatusPath()));

    auto [memoryUsage, processAge] = getThreatScannerProcessinfo();
    telemetry.set("threatMemoryUsage", memoryUsage);
    telemetry.set("threatProcessAge", processAge);

    add_vdl_telemetry(telemetry);

    telemetry.increment("scan-now-count", 0ul);
    telemetry.increment("scheduled-scan-count", 0ul);
    telemetry.increment("on-access-threat-count", 0ul);
    telemetry.increment("on-access-threat-eicar-count", 0ul);
    telemetry.increment("on-demand-threat-count", 0ul);
    telemetry.increment("on-demand-threat-eicar-count", 0ul);
    telemetry.increment("detections-dropped-from-safestore-queue", 0ul);

    return telemetry.serialiseAndReset();
}

std::string Telemetry::getVirusDataVersion()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    std::string virusDataVersion = "unknown";
    fs::path datasetAmfst(appConfig.getData("PLUGIN_INSTALL"));
    datasetAmfst /= "chroot/susi/update_source/vdl/manifestdata.dat";
    if (fs::exists(datasetAmfst))
    {
        virusDataVersion = "DataSet-A";
    }
    return virusDataVersion;
}

std::pair<unsigned long, unsigned long> Telemetry::getThreatScannerProcessinfo(
    const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls,
    Common::FileSystem::IFileSystem* fileSystem)
{
    const int expectedStatFileSize = 52;
    const int rssEntryInStat = 24;
    const int startTimeEntryInStat = 22;

    unsigned long memoryUsage = 0;
    unsigned long runTime = 0;
    std::optional<std::string> statFileContents;

    Path threatDetectorPidFile = common::getPluginInstallPath() / "chroot/var/threat_detector.pid";
    int pid = getProcessPidFromFile(fileSystem, threatDetectorPidFile);

    if (pid == 0)
    {
        return std::pair(0,0);
    }

    try
    {
        statFileContents = fileSystem->readProcFile(pid, "stat");
        auto procStat = Common::UtilityImpl::StringUtils::splitString(statFileContents.value(), { ' ' });
        if (procStat.size() != expectedStatFileSize)
        {
            LOGDEBUG("The proc stat for " << pid << " is not of expected size");
            LOGERROR("Failed to get Sophos Threat Detector Process Info");
            return std::pair(0, 0);
        }

        memoryUsage = Common::UtilityImpl::StringUtils::stringToLong(procStat.at(rssEntryInStat - 1)).first;

        auto sysUpTime = sysCalls->getSystemUpTime();
        if (sysUpTime.first == -1)
        {
            LOGWARN("Failed to retrieve system info, cant calculate process duration. Returning memory usage only.");
            return std::pair(memoryUsage, 0);
        }
        long startTime = Common::UtilityImpl::StringUtils::stringToLong(procStat.at(startTimeEntryInStat - 1)).first/sysconf(_SC_CLK_TCK);
        runTime = sysUpTime.second - startTime;

    }
    catch (const std::bad_optional_access& e)
    {
        LOGERROR("Stat file of Pid: " << pid << " is empty. Cannot report on Threat Detector Process to Telemetry: " << e.what());
        return std::pair(0,0);
    }
    catch (const Common::FileSystem::IFileSystemException& e)
    {
        LOGERROR("Error reading threat detector stat proc file due to: " << e.what());
        return std::pair(0,0);
    }

    return { memoryUsage, runTime };
}

std::pair<unsigned long, unsigned long> Telemetry::getThreatScannerProcessinfo(
    const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls)
{
    return getThreatScannerProcessinfo(sysCalls, filesystem_);
}

std::pair<unsigned long, unsigned long> Telemetry::getThreatScannerProcessinfo()
{
    return getThreatScannerProcessinfo(syscalls_, filesystem_);
}
