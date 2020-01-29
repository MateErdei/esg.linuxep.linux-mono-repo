/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/Options.h"
#include "avscanner/avscannerimpl/ScanClient.h"

#include "filewalker/FileWalker.h"

#include <unixsocket/ScanningClientSocket.h>

#include <string>
#include <fstream>
#include <avscanner/avscannerimpl/NamedScanRunner.h>

using namespace avscanner::avscannerimpl;

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

static int runCommandLineScan(const std::vector<std::string>& paths)
{
    const std::string unix_socket_path = "/opt/sophos-spl/plugins/sspl-plugin-anti-virus/chroot/unix_socket";
    unixsocket::ScanningClientSocket socket(unix_socket_path);
    CallbackImpl callbacks(socket);

    for (const auto& path : paths)
    {
        filewalker::walk(path, callbacks);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    Options options(argc, argv);
    auto paths = options.paths();
    auto config = options.config();

    if (!config.empty())
    {
        avscanner::avscannerimpl::NamedScanRunner runner(config);
        runner.run();
    }
    else
    {
        return runCommandLineScan(paths);
    }

}
