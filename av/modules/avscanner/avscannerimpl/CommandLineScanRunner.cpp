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
#include <exception>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

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
                : m_scanner(socket, std::move(callbacks), false)
        {
        }

        void processFile(const fs::path& p) override
        {
            PRINT("Scanning " << p);
            try
            {
                m_scanner.scan(p);
            } catch (const std::exception& e)
            {
                PRINT("Scanner failed to scan: " << p);
                m_returnCode = 1;
            }
        }

        bool includeDirectory(const fs::path&) override
        {
            return true;
        }

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }

    private:
        ScanClient m_scanner;
        int m_returnCode = 0;
    };
}

CommandLineScanRunner::CommandLineScanRunner(std::vector<std::string> paths)
    : m_paths(std::move(paths))
{
}

int CommandLineScanRunner::run()
{
    auto scanCallbacks = std::make_shared<ScanCallbackImpl>();

    const std::string unix_socket_path = "/opt/sophos-spl/plugins/av/chroot/unix_socket";
    unixsocket::ScanningClientSocket socket(unix_socket_path);
    CallbackImpl callbacks(socket, scanCallbacks);

    for (const auto& path : m_paths)
    {
        try
        {
            filewalker::walk(path, callbacks);
        }catch (fs::filesystem_error& e)
        {
            return e.code().value();
        }
    }

    return callbacks.returnCode();
}
