/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanRunner.h"

#include "ScanClient.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <filewalker/IFileWalkCallbacks.h>
#include <unixsocket/ScanningClientSocket.h>

#include <fcntl.h>
#include <fstream>
#include <unistd.h>

using namespace avscanner::avscannerimpl;

NamedScanRunner::NamedScanRunner(const std::string& configPath)
{
    int fd = open(configPath.c_str(), O_RDONLY);
    ::capnp::StreamFdMessageReader message(fd);

    NamedScanRunner(message.getRoot<Sophos::ssplav::NamedScan>());

    close(fd);
}

NamedScanRunner::NamedScanRunner(const Sophos::ssplav::NamedScan::Reader& namedScanConfig)
{
    m_scanName = namedScanConfig.getName();

    auto excludePaths = namedScanConfig.getExcludePaths();
    m_excludePaths.reserve(excludePaths.size());
    for (const auto& item : excludePaths)
    {
        m_excludePaths.emplace_back(item);
    }
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
        explicit CallbackImpl(unixsocket::ScanningClientSocket& socket, std::shared_ptr<IScanCallbacks> callbacks)
                : m_scanner(socket, std::move(callbacks))
        {}

        void processFile(const sophos_filesystem::path& p) override
        {
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

int NamedScanRunner::run()
{
    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    const std::string unix_socket_path = "/opt/sophos-spl/plugins/av/chroot/unix_socket";
    unixsocket::ScanningClientSocket socket(unix_socket_path);
    CallbackImpl callbacks(socket, scanCallbacks);

    return 0;
}

std::string NamedScanRunner::getScanName() const
{
    return m_scanName;
}

std::vector<std::string> NamedScanRunner::getExcludePaths() const
{
    return m_excludePaths;
}