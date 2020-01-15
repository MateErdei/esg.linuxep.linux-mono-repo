/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <unixsocket/ScanningClientSocket.h>

#include <string>
#include <cassert>
#include <fcntl.h>

int main(int argc, char* argv[])
{
    std::string filename = "/etc/fstab";
    if (argc > 1)
    {
        filename = argv[1];
    }

    int file_fd = open(filename.c_str(), O_RDONLY);
    assert(file_fd >= 0);

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ScanningClientSocket socket(path);
    auto response = socket.scan(file_fd, filename); // takes ownership of file_fd
    static_cast<void>(response);

    return 0;
}
