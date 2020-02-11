/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanRunner.h"

#include "ScanClient.h"

#include <filewalker/IFileWalkCallbacks.h>
#include <unixsocket/ScanningClientSocket.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <fstream>

#include <fcntl.h>
#include <unistd.h>

using namespace avscanner::avscannerimpl;


NamedScanRunner::NamedScanRunner(const std::string& configPath)
    : m_config(configFromFile(configPath))
{
}

NamedScanRunner::NamedScanRunner(const Sophos::ssplav::NamedScan::Reader& namedScanConfig)
    : m_config(namedScanConfig)
{
}

namespace
{
    class ScanCallbackImpl : public IScanCallbacks
    {
    public:
        void cleanFile(const path&) override
        {}
        void infectedFile(const path&, const std::string&) override
        {}
    };

    class CallbackImpl : public filewalker::IFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(ScanClient scanner, NamedScanConfig& config)
                : m_scanner(std::move(scanner)), m_config(config)
        {}

        void processFile(const sophos_filesystem::path& p) override
        {
            m_scanner.scan(p);
        }

        bool includeDirectory(const sophos_filesystem::path&) override
        {
            // Exclude based on config
            // Exclude all mount points
            return true;
        }

    private:
        ScanClient m_scanner;
        NamedScanConfig& m_config;
    };
}

int NamedScanRunner::run()
{
    // evaluate mount information

    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    const std::string unix_socket_path = "/opt/sophos-spl/plugins/av/chroot/unix_socket";
    unixsocket::ScanningClientSocket socket(unix_socket_path);
    ScanClient scanner(socket, scanCallbacks);
    CallbackImpl callbacks(scanner, m_config);

    // work out which filesystems are included based of config and mount information

    // for each select included mount point call filewalker for that mount point

    return 0;
}

NamedScanConfig& NamedScanRunner::getConfig()
{
    return m_config;
}
