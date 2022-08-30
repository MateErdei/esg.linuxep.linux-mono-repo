//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include <unixsocket/threatDetectorSocket/ScanningClientSocket.h>

#include <datatypes/Print.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <string>
#include <cassert>

#include <fcntl.h>

static scan_messages::ScanResponse scan(unixsocket::ScanningClientSocket& socket,int fd, const std::string& filename)
{
    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    request->setPath(filename);
    request->setScanInsideArchives(false);
    request->setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request->setUserID("root");
    request->setFd(fd);
    socket.connect();
    socket.sendRequest(request);

    scan_messages::ScanResponse response;
    auto ret = socket.receiveResponse(response);
    if (!ret)
    {
        response.setErrorMsg("Failed to get scan response");
    }
    return response;
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
    auto file_fd = open(filename.c_str(), static_cast<int>(oflags));
    assert(file_fd >= 0);

    const std::string path = "/opt/sophos-spl/plugins/av/chroot/var/scanning_socket";
    unixsocket::ScanningClientSocket socket(path);
    auto response = scan(socket, file_fd, filename); // takes ownership of file_fd

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
