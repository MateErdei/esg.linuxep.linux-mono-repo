/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommandLineScanRunner.h"
#include "ScanClient.h"
#include "Mounts.h"

#include "datatypes/Print.h"
#include "filewalker/FileWalker.h"
#include <unixsocket/ScanningClientSocket.h>

#include <memory>
#include <utility>

using namespace avscanner::avscannerimpl;

namespace
{
    class ScanCallbackImpl : public IScanCallbacks
    {
    public:
        void cleanFile(const path&) override
        {
        }
        void infectedFile(const path& p, const std::string& threatName) override
        {
            PRINT(p << " is infected with " << threatName);
        }
    };

    class CallbackImpl : public filewalker::IFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(unixsocket::ScanningClientSocket& socket, std::shared_ptr<IScanCallbacks> callbacks)
                : m_scanner(socket, std::move(callbacks))
        {}

        void processFile(const sophos_filesystem::path& p) override
        {
            PRINT("Scanning " << p);
            m_scanner.scan(p);
        }

        bool includeDirectory(const sophos_filesystem::path&) override
        {
            return true;
        }

    private:
        ScanClient m_scanner;
    };
}

CommandLineScanRunner::CommandLineScanRunner(std::vector<std::string> paths)
    : m_paths(std::move(paths))
{
}

int CommandLineScanRunner::run()
{
    // evaluate mount information
    // Setup exclusions
    std::vector<std::shared_ptr<IMountPoint>> includedMountpoints;

    // For each mount point
    std::shared_ptr<IMountInfo> mountInfo = std::make_shared<Mounts>();
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    for (auto & mp : allMountpoints)
    {
        if ( mp->isSpecial() )
        {
            PRINT("Mount point " << mp->mountPoint().c_str() << " is system and will be excluded from the scan");
        }
        else
        {
            includedMountpoints.push_back(mp);
        }
    }

    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    const std::string unix_socket_path = "/opt/sophos-spl/plugins/av/chroot/unix_socket";
    unixsocket::ScanningClientSocket socket(unix_socket_path);
    CallbackImpl callbacks(socket, scanCallbacks);

    // for each select included mount point call filewalker for that mount point
    for (const auto& path : m_paths)
    {
        for (auto & mp : includedMountpoints)
        {
            std::string mountpointToScan = mp->mountPoint();
            if (path.rfind(mountpointToScan, 0) == 0)
            {
                PRINT("Scanning: " << path << " because it is in included mountpoint: " << mountpointToScan);
                filewalker::walk(path, callbacks);
                continue;
            }
        }
    }

    return 0;
}
