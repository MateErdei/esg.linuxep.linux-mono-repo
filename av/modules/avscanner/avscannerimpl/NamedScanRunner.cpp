/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "NamedScanRunner.h"

#include "ScanClient.h"

#include <filewalker/IFileWalkCallbacks.h>
#include <unixsocket/ScanningClientSocket.h>

#include <fstream>

using namespace avscanner::avscannerimpl;

static std::string readFile(const std::string& path)
{
    std::ifstream s(path);
    std::string contents;
    s >> contents;
    return contents;
}

NamedScanRunner::NamedScanRunner(const std::string& configPath)
{
    m_contents = readFile(configPath);
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

