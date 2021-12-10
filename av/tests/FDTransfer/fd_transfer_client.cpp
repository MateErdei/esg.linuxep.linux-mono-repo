/******************************************************************************************************

Copyright 2019-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <unixsocket/threatDetectorSocket/ScanningClientSocket.h>

#include <string>
#include <cassert>
#include <fcntl.h>
#include <datatypes/Print.h>

static scan_messages::ScanResponse scan(unixsocket::ScanningClientSocket& socket, int file_fd, const std::string& filename)
{
    datatypes::AutoFd fd(file_fd);
    scan_messages::ClientScanRequest request;
    request.setPath(filename);
    request.setScanInsideArchives(false);
    request.setScanInsideImages(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");
    return socket.scan(fd, request);
}

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
    auto response = scan(socket, file_fd, filename); // takes ownership of file_fd

    for (const auto& detection: response.getDetections())
    {
        if (detection.name.empty())
        {
            PRINT(filename << " is fake clean");
        }
        else
        {
            PRINT(filename << " is fake infected");
        }
    }

    return 0;
}
