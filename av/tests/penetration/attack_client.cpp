/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <unixsocket/threatDetectorSocket/ScanningClientSocket.h>

#include <datatypes/Print.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <string>
#include <cassert>

#include <fcntl.h>

static scan_messages::ScanResponse scan(unixsocket::ScanningClientSocket& socket, datatypes::AutoFd& fd, const std::string& filename)
{
    scan_messages::ClientScanRequest request;
    request.setPath(filename);
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");
    return socket.scan(fd, request);
}

int main(int argc, char* argv[])
{
    static Common::Logging::ConsoleLoggingSetup m_loggingSetup;

    std::string filename = "/etc/fstab";
    if (argc > 1)
    {
        filename = argv[1];
    }

    bool direct = true;
    if (argc > 2)
    {
        std::string arg2 = argv[2];
        direct = (arg2 == "direct");
    }

    unsigned oflags = O_RDONLY;
    if (direct)
    {
        oflags |= O_DIRECT;
    }
    datatypes::AutoFd file_fd(open(filename.c_str(), static_cast<int>(oflags)));
    assert(file_fd >= 0);

    const std::string path = "/opt/sophos-spl/plugins/av/chroot/var/scanning_socket";
    unixsocket::ScanningClientSocket socket(path);
    auto response = scan(socket, file_fd, filename);
    file_fd.close();

    if (response.allClean())
    {
        PRINT(filename << " is all clean");
    }
    else
    {
        PRINT(filename << " is not all clean");
    }

    for (const auto& detection: response.getDetections())
    {
        if (detection.name.empty())
        {
            PRINT(filename << " is clean");
        }
        else
        {
            PRINT(filename << " is infected with " <<detection.name);
        }
    }

    return 0;
}
