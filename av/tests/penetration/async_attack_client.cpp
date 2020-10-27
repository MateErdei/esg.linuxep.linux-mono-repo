/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/SocketUtils.h"
#include <scan_messages/ClientScanRequest.h>

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"
#include <datatypes/Print.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <cassert>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <wait.h>

using ScanRequest = scan_messages::ClientScanRequest;

static ScanRequest generateScanRequest(const std::string& filename)
{
    ScanRequest request;
    request.setPath(filename);
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");
    return request;
}

void send_fd(const int socket_fd, const std::string& filename)
{
    int file_fd = ::open(filename.c_str(), O_RDONLY);
    assert(file_fd > 0);

    ::fcntl(file_fd, F_SETLEASE, F_RDLCK, sizeof(int));
    ::fcntl(file_fd, F_SETSIG, SIGIO, sizeof(int));

    unixsocket::send_fd(socket_fd, file_fd);

    ::close(file_fd);
}

int main(int argc, char* argv[])
{
    static Common::Logging::ConsoleLoggingSetup m_loggingSetup;

    std::string filename = "/etc/fstab";
    bool fork = false;

    for(int i=1; i<argc; ++i)
    {
        std::string arg(argv[i]);

        if (arg == "--fork")
        {
            fork = true;
        }
        else if (arg.rfind("--", 0) != std::string::npos)
        {
            PRINT("Unrecognised option: " << arg);
            return EXIT_FAILURE;
        }
        else
        {
            filename = arg;
        }
    }

    const std::string socketPath = "/opt/sophos-spl/plugins/av/chroot/scanning_socket";

    datatypes::AutoFd socket_fd(socket(AF_UNIX, SOCK_STREAM, 0));
    assert(socket_fd >= 0);

    struct sockaddr_un addr = { };
    addr.sun_family = AF_UNIX;
    ::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path));

    int ret = ::connect(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), SUN_LEN(&addr));
    if (ret != 0)
    {
        perror("Failed to connect to scanning_socket");
        return EXIT_FAILURE;
    }

    ScanRequest request = generateScanRequest(filename);
    unixsocket::writeLengthAndBuffer(socket_fd, request.serialise());

    if (!fork)
    {
        // send the file fd from this process.
        send_fd(socket_fd, filename);
    }
    else
    {
        // send the fd from a short-lived subprocess, so that the OS doesn't send us the signal
        int fork_ret = ::fork();
        switch (fork_ret)
        {
            case -1:
                perror("fork failed");
                return EXIT_FAILURE;
            case 0:
            {
                send_fd(socket_fd, filename);
                _exit(EXIT_SUCCESS);
            }
            default:
            {
                int status;
                ::wait(&status);
            }
        }
    }

    // re-open the file for write, to trigger the lease
    int new_file_fd = ::open(filename.c_str(), O_RDWR);
    assert(new_file_fd > 0);
    ::close(new_file_fd);

    // get the raw response, but don't parse/process it
    int length = unixsocket::readLength(socket_fd);
    assert(length > 0);

    std::vector<char> response;
    response.resize(length);
    int bytes_received = ::recv(socket_fd, response.data(), length, MSG_NOSIGNAL);
    assert(bytes_received == length);

    socket_fd.close();

    return EXIT_SUCCESS;
}
