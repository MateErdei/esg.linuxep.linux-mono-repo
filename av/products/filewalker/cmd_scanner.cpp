/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "filewalker/FileWalker.h"
#include <unixsocket/ScanningClientSocket.h>
#include <datatypes/Print.h>

#include <iostream>
#include <string>
#include <cassert>
#include <fcntl.h>

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

    private:
        unixsocket::ScanningClientSocket& m_socket;
    };
}

int main(int argc, char* argv[])
{
    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ScanningClientSocket socket(path);

    CallbackImpl callbacks(socket);
    for(int i=1; i < argc; i++)
    {
        filewalker::walk(argv[i], callbacks);
    }

    return 0;
}
