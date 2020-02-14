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

enum E_ERROR_CODES: int
{
    E_CLEAN = 0,
    E_GENERIC_FAILURE = 1,
    E_VIRUS_FOUND = 69
};

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
            m_returnCode = E_VIRUS_FOUND;
        }

        [[nodiscard]] int returnCode() const
        {
            return m_returnCode;
        }

    private:
        int m_returnCode = E_CLEAN;
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
                m_returnCode = E_GENERIC_FAILURE;
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
        int m_returnCode = E_CLEAN;
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
            m_returnCode = e.code().value();
        }

        // we want virus found to override any other return code
        if (scanCallbacks->returnCode() == E_VIRUS_FOUND)
        {
            m_returnCode = E_VIRUS_FOUND;
        }
    }

    return m_returnCode;
}
