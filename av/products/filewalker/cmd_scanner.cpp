/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Options.h"

#include "filewalker/FileWalker.h"

#include <unixsocket/ScanningClientSocket.h>
#include <datatypes/Print.h>

#include <iostream>
#include <string>
#include <cassert>
#include <fcntl.h>

bool startswith(const char* value, const char* substr);

static void scan(unixsocket::ScanningClientSocket& socket, const sophos_filesystem::path& p)
{
    PRINT("Scanning " << p);
    int file_fd = open(p.c_str(), O_RDONLY);
    assert(file_fd >= 0);

    auto response = socket.scan(file_fd, p); // takes ownership of file_fd
    static_cast<void>(response);

    if (response.clean())
    {
        PRINT(p << " is fake clean");
    }
    else
    {
        PRINT(p << " is fake infected");
    }
}

namespace
{
    class CallbackImpl : public filewalker::IFileWalkCallbacks
    {
    public:
        explicit CallbackImpl(unixsocket::ScanningClientSocket& socket)
                : m_socket(socket)
        {}

        void processFile(const sophos_filesystem::path& p) override
        {
            scan(m_socket, p);
        }

        bool includeDirectory(const sophos_filesystem::path&) override
        {
            return true;
        }

    private:
        unixsocket::ScanningClientSocket& m_socket;
    };
}

int main(int argc, char* argv[])
{
    avscanner::Options options;
    options.handleArgs(argc, argv);
    auto paths = options.paths();

    const std::string unix_socket_path = "/opt/sophos-spl/plugins/sspl-plugin-anti-virus/chroot/unix_socket";
    unixsocket::ScanningClientSocket socket(unix_socket_path);
    CallbackImpl callbacks(socket);

    for (const auto& path : paths)
    {
        filewalker::walk(path, callbacks);
    }

    return 0;
}
