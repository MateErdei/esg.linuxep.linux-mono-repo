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
    class CallbackImpl : public filewalker::IFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(unixsocket::ScanningClientSocket& socket)
                : m_scanner(socket)
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

void NamedScanRunner::run()
{

}

