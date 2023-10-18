// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "datatypes/Print.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <cassert>
#include <string>
#include <thread>

#include <fcntl.h>

static void flip_odirect(const std::string& file_path, int file_fd)
{
    auto flags = static_cast<unsigned>(::fcntl(file_fd, F_GETFL, 0));
    auto without_odirect = flags & ~static_cast<unsigned>(O_DIRECT);
    auto with_odirect = flags | O_DIRECT;
    for (int i=0; i<300; i++)
    {
        ::fcntl(file_fd, F_SETFL, with_odirect);
        PRINT("setting "<< file_path << " with O_DIRECT");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ::fcntl(file_fd, F_SETFL, without_odirect);
        PRINT("setting "<< file_path << " without O_DIRECT");
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

static scan_messages::ScanResponse scan(unixsocket::ScanningClientSocket& socket, int fd, const std::string& filename)
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

    auto file_fd = open(filename.c_str(), O_RDONLY);
    assert(file_fd >= 0);

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ScanningClientSocket socket(path);

    std::thread flipper(&flip_odirect, filename, file_fd);

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

    flipper.join();

    return 0;
}
