/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include "datatypes/Print.h"
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fstream>
#include <fcntl.h>
#include <cassert>
#include <unixsocket/SocketUtils.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

namespace
{
    class FakeScanner : public threat_scanner::IThreatScanner
    {
        scan_messages::ScanResponse scan(
                datatypes::AutoFd& /* fd */,
                const std::string& /*file_path*/,
                int64_t /*scanType*/,
                const std::string& /*userID*/) override
        {
            // PRINT(file_path);
            scan_messages::ScanResponse response;
            response.addDetection("/bin/bash", "");
            return response;
        }
    };
    class FakeScannerFactory : public threat_scanner::IThreatScannerFactory
    {
        threat_scanner::IThreatScannerPtr createScanner(bool) override
        {
            return std::make_unique<FakeScanner>();
        }
    };
}

static inline bool fd_isset(int fd, fd_set* fds)
{
    assert(fd >= 0);
    return FD_ISSET(static_cast<unsigned>(fd), fds); // NOLINT
}

static inline void internal_fd_set(int fd, fd_set* fds)
{
    assert(fd >= 0);
    FD_SET(static_cast<unsigned>(fd), fds); // NOLINT
}

static int addFD(fd_set* fds, int fd, int currentMax)
{
    internal_fd_set(fd, fds);
    return std::max(fd, currentMax);
}

static int DoSomethingWithData(const uint8_t *Data, size_t Size)
{
    // create a socket pair
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    if (ret != 0 || socket_fds[0] < 0 || socket_fds[1] < 0 )
    {
        handle_error("Failed to create unix socket pair");
    }
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);

    // start a scanning server
    auto scannerFactory = std::make_shared<FakeScannerFactory>();
    unixsocket::ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, 1);
    connectionThread.start();

    // send our request
    ::send(clientFd.get(), Data, Size, 0);

    // send a file descriptor
    datatypes::AutoFd devNull(::open("/dev/null", O_RDONLY));
    unixsocket::send_fd(clientFd.get(), devNull.get()); // send our test fd

    // TODO - could check for activity on our fake scanner here

    // check for a pending response
    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;
    max = addFD(&readFDs, clientFd, max);
    fd_set tempRead = readFDs;
    const struct timespec timeout = { .tv_sec = 0, .tv_nsec = 10'000'000 }; // 0.010s
    int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, &timeout, nullptr);

    if (activity < 0)
    {
        // perror("pselect failed");

    }
    else if(activity != 0)
    {
        if (fd_isset(clientFd, &tempRead))
        {
            // receive the response
            int length = unixsocket::readLength(clientFd);
            if (length > 0)
            {
                std::vector<char> buffer(length);
                ::recv(clientFd.get(), buffer.data(), length, 0);
            }
        }
    }

    ::close(clientFd);

    // connectionThread.requestStop();

    connectionThread.join();

    return 0;
}

#ifdef USING_LIBFUZZER
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    return DoSomethingWithData(Data, Size);
}
#else
static bool DoInitialization()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();

    return true;
}

int main(int argc, char* argv[])
{
    if( argc < 2 )
    {
        PRINT("missing arg");
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[1], "--write-valid-request") == 0)
    {
        if( argc < 3 )
        {
            PRINT("missing filename");
            exit(EXIT_FAILURE);
        }
        scan_messages::ClientScanRequest request;
        request.setPath("/dir/file");
        request.setScanInsideArchives(false);
        request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
        request.setUserID("root");

        std::ofstream outfile(argv[2], std::ios::binary);

        std::string request_str = request.serialise();

        char size = request_str.size();
        outfile.write(&size, sizeof(size));

        outfile.write(request_str.data(), request_str.size());
        outfile.close();
        exit(EXIT_SUCCESS);

    }


    static bool Initialized = DoInitialization();
    static_cast<void>(Initialized);

    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();

    if ( size < 0 )
    {
        PRINT("cannot open file");
        exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
    {
        PRINT("cannot read file");
        exit(EXIT_FAILURE);
    }

    return DoSomethingWithData(reinterpret_cast<const uint8_t *>(buffer.data()), size);
}
#endif