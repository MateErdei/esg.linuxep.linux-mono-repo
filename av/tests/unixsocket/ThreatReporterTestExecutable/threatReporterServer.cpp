// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "datatypes/Print.h"
#include "unixsocket/SocketUtilsImpl.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <string>

#include "tests/scan_messages/SampleThreatDetected.h"

#define handle_error(msg)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

using namespace common::CentralEnums;

namespace
{
    class MessageCallbacks : public IMessageCallback
    {
    public:
        void processMessage(scan_messages::ThreatDetected) override
        {
            // noop
        }
    };
} // namespace

static void initializeLogging()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

static int DoSomethingWithData(const uint8_t* Data, size_t Size)
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup{ Common::Logging::LOGOFFFORTEST() };
    // create a socket pair
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    if (ret != 0 || socket_fds[0] < 0 || socket_fds[1] < 0)
    {
        handle_error("Failed to create unix socket pair");
    }
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);

    // start a scanning server
    std::shared_ptr<MessageCallbacks> messageCallbacks = std::make_shared<MessageCallbacks>();
    auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
    unixsocket::ThreatReporterServerConnectionThread connectionThread(serverFd, messageCallbacks, sysCalls);
    connectionThread.start();

    // send our request
    ::send(clientFd.get(), Data, Size, 0);
    ::close(clientFd.get());
    while (true)
    {
        if (!connectionThread.isRunning())
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
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size)
{
    return DoSomethingWithData(Data, Size);
}
#else

static int writeSampleFile(const std::string& filename)
{
    scan_messages::ThreatDetected threatDetected = createThreatDetectedWithRealFd({});
    std::string request_str = threatDetected.serialise();

    auto size = request_str.size();

    auto bytes = unixsocket::splitInto7Bits(size);
    auto lengthBytes = unixsocket::addTopBitAndPutInBuffer(bytes);

    std::ofstream outfile(filename, std::ios::binary);

    outfile.write(reinterpret_cast<const char*>(lengthBytes.get()), bytes.size());

    outfile.write(request_str.data(), request_str.size());
    outfile.close();
    return EXIT_SUCCESS;
}

static int processFile(const std::string& filename)
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
    if (argc < 2)
    {
        PRINT("missing arg");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--write-valid-request") == 0)
    {
        if (argc < 3)
        {
            PRINT("missing filename");
            return EXIT_FAILURE;
        }

        return writeSampleFile(argv[2]);
    }

    if (sophos_filesystem::is_directory(argv[1]))
    {
        for (auto& p : sophos_filesystem::directory_iterator(argv[1]))
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