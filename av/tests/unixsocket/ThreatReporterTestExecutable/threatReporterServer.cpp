/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/Print.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <boost/assert.hpp>
#include <unixsocket/SocketUtils.h>
#include <unixsocket/SocketUtilsImpl.h>

#include <cstring>
#include <fstream>
#include <string>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

namespace
{
    class MessageCallbacks : public IMessageCallback
    {
    public:
        void processMessage(const scan_messages::ServerThreatDetected&) override
        {
            // noop
        }
    };
}

static void initializeLogging()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

static int DoSomethingWithData(const uint8_t *Data, size_t Size)
{
    initializeLogging();
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
    std::shared_ptr<MessageCallbacks> messageCallbacks = std::make_shared<MessageCallbacks>();
    unixsocket::ThreatReporterServerConnectionThread connectionThread(serverFd, messageCallbacks);
    connectionThread.start();

    // send our request
    ::send(clientFd.get(), Data, Size, 0);
    ::close(clientFd);
    while (true)
    {
        if(!connectionThread.isRunning())
        {
            break;
        }
        usleep(100);
    }

    connectionThread.requestStop();
    connectionThread.join();

    return 0;
}

#ifdef USING_LIBFUZZER
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    return DoSomethingWithData(Data, Size);
}
#else

static int writeSampleFile(std::string filename)
{
    std::string threatName = "test-eicar";
    std::string threatPath = "/path/to/test-eicar";
    std::time_t detectionTimeStamp = std::time(nullptr);
    std::string userID = std::getenv("USER");

    scan_messages::ThreatDetected threatDetected;

    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(detectionTimeStamp);
    threatDetected.setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(scan_messages::E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(scan_messages::E_SMT_THREAT_ACTION_SHRED);

    std::string request_str = threatDetected.serialise();


    int size = request_str.size();

    auto bytes = unixsocket::splitInto7Bits(size);
    auto lengthBytes = unixsocket::addTopBitAndPutInBuffer(bytes);

    std::ofstream outfile(filename, std::ios::binary);

    outfile.write(reinterpret_cast<const char*>(lengthBytes.get()), bytes.size());

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
    initializeLogging();
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