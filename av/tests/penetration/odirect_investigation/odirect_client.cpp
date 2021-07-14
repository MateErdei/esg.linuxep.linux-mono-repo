/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <datatypes/Print.h>
#include <unixsocket/threatDetectorSocket/ScanningClientSocket.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

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

    datatypes::AutoFd file_fd(open(filename.c_str(), O_RDONLY));
    assert(file_fd.get() >= 0);

    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ScanningClientSocket socket(path);

    std::thread flipper(&flip_odirect, filename, file_fd.get());

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
