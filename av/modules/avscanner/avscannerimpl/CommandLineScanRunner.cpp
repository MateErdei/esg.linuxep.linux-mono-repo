/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommandLineScanRunner.h"
#include "ScanClient.h"

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
        void cleanFile(const path& p) override
        {
            PRINT(p << " is fake clean");
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

int CommandLineScanRunner::run(const std::vector<std::string>& paths)
{
    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    const std::string unix_socket_path = "/opt/sophos-spl/plugins/sspl-plugin-anti-virus/chroot/unix_socket";
    unixsocket::ScanningClientSocket socket(unix_socket_path);
    CallbackImpl callbacks(socket, scanCallbacks);

    for (const auto& path : paths)
    {
        filewalker::walk(path, callbacks);
    }

    return 0;
}
