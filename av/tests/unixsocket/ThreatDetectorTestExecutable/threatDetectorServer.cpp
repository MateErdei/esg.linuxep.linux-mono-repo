/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/Print.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/SystemCallWrapper.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <unixsocket/SocketUtils.h>

#include <fstream>
#include <string>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

namespace
{
    class FakeScanner : public threat_scanner::IThreatScanner
    {
        scan_messages::ScanResponse scan(
            datatypes::AutoFd& fd,
            const scan_messages::ScanRequest& info) override
        {
            scan_messages::ScanResponse response;
            std::stringstream fullResult;
            fullResult << "fd=" << fd << ", scanType=" << info.getScanType()
                       << ", pid=" << info.getPid() << ", exePath="
                       << info.getExecutablePath() << "usserid=" << info.getUserId();
            response.addDetection(info.getPath(), "fuzz-test","");
            return response;
        }

    };
    class FakeScannerFactory : public threat_scanner::IThreatScannerFactory
    {
        threat_scanner::IThreatScannerPtr createScanner(bool, bool, bool) override
        {
            return std::make_unique<FakeScanner>();
        }

        bool update() override
        {
            return true;
        }

        bool reload() override
        {
            return true;
        }

        void shutdown() override
        {
        }

        bool susiIsInitialized() override
        {
            return true;
        }

        bool updateSusiConfig() override
        {
            return false;
        }

        bool detectPUAsEnabled() override
        {
            return true;
        }
    };
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
    auto sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
    unixsocket::ScanningServerConnectionThread connectionThread(serverFd, scannerFactory, sysCalls, 1);
    connectionThread.start();

    // send our request
    ::send(clientFd.get(), Data, Size, 0);

    // send a file descriptor
    datatypes::AutoFd devNull(::open("/dev/null", O_RDONLY));
    unixsocket::send_fd(clientFd.get(), devNull.get()); // send our test fd

    // check for a pending response
    struct pollfd fds[] {
        { .fd = clientFd.get(), .events = POLLIN, .revents = 0 },
    };

    const struct timespec timeout = { .tv_sec = 0, .tv_nsec = 10'000'000 }; // 0.010s
    auto activity = ::ppoll(fds, std::size(fds), &timeout, nullptr);

    if (activity < 0)
    {
        // perror("ppoll failed");

    }
    else if(activity != 0)
    {
        if ((fds[0].revents & POLLIN) != 0)
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
static void initializeLogging()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

static int writeSampleFile(std::string filename)
{

    scan_messages::ClientScanRequest request;
    request.setPath("/dir/file");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");
    request.setExecutablePath("/tmp/random/Path");
    request.setPid(1000);

    std::ofstream outfile(filename, std::ios::binary);

    std::string request_str = request.serialise();

    char size = request_str.size();
    outfile.write(&size, sizeof(size));

    outfile.write(request_str.data(), request_str.size());
    outfile.close();
    return EXIT_SUCCESS;
}

static int processFile(std::string filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();

    if (size < 0)
    {
        PRINT("cannot open file");
        return EXIT_FAILURE;
    }

    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
    {
        PRINT("cannot read file");
        return EXIT_FAILURE;
    }

    return DoSomethingWithData(reinterpret_cast<const uint8_t*>(buffer.data()), size);
}

int main(int argc, char* argv[])
{
    if( argc < 2 )
    {
        PRINT("missing arg");
        return EXIT_FAILURE;
    }

    if(strcmp(argv[1], "--write-valid-request") == 0)
    {
        if( argc < 3 )
        {
            PRINT("missing filename");
            return EXIT_FAILURE;
        }

        return writeSampleFile(argv[2]);
    }

    initializeLogging();

    if (sophos_filesystem::is_directory(argv[1]))
    {
        for (auto& p: sophos_filesystem::directory_iterator(argv[1]))
        {
            if (!sophos_filesystem::is_regular_file(p))
            {
                std::cout << "Skipping " << p.path() << '\n';
                continue;
            }

            std::cout << "Processing " << p.path() << '\n';
            int ret = processFile(p.path());
            if (ret != EXIT_SUCCESS)
            {
                std::cerr << "Error while processing file " << p.path() << '\n';
                return ret;
            }
        }
    }
    else if (sophos_filesystem::is_regular_file(argv[1]))
    {
        int ret = processFile(argv[1]);
        return ret;
    }
    else
    {
        std::cerr << "Error: not a file or directory" << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif