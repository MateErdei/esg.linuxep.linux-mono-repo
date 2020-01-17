/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "filewalker/FileWalker.h"
#include <unixsocket/ScanningClientSocket.h>
#include <unixsocket/Print.h>

#include <iostream>
#include <string>
#include <cassert>
#include <fcntl.h>

static void printout(unixsocket::ScanningClientSocket& socket, const fs::path& p)
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

int main(int argc, char* argv[])
{
    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ScanningClientSocket socket(path);

    for(int i=1; i < argc; i++)
    {
        filewalker::FileWalker fw(
                argv[i],
                [&socket](auto p)
                { printout(socket, p); }
        );
        fw.run();
    }

    return 0;
}
