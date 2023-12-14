// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "FakeServerSocket.h"

#include "avscanner/avscannerimpl/ScanClient.h"
#include "common/AbortScanException.h"
#include "datatypes/Print.h"
#include "datatypes/IUuidGenerator.h"
#include "unixsocket/SocketUtilsImpl.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <capnp/message.h>
#include <log4cplus/logger.h>

#include <fstream>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
// #define BASE "/tmp/TestPluginAdapter"

namespace fs = sophos_filesystem;

class FakeCallbacks :public avscanner::avscannerimpl::IScanCallbacks
{
public:
    FakeCallbacks() = default;
private:
    void cleanFile(const avscanner::avscannerimpl::path& ) override {}
    void infectedFile(const std::map<fs::path, std::string>&, const sophos_filesystem::path&, const std::string&, bool) override {}
    void scanError(const std::string&, std::error_code) override {}
    void scanStarted() override {}
    void logSummary() override {}
};

namespace {
    class QuietLogSetup {
    public:
        QuietLogSetup() {
            Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
        }

        ~QuietLogSetup() {
            log4cplus::Logger::shutdown();
        }
    };
}

namespace Fuzzing
{
    static std::string m_socketPathBase = "/tmp/socket_";
    static std::string m_socketPath;

    static std::unique_ptr<FakeDetectionServer::FakeServerSocket> socketServer;

    // static std::string m_scanResult  = R"({"results":[{"detections":[{"threatName":"EICAR-AV-Test","threatType":"virus"}],"path":"/home/vagrant/eicar1","sha256":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"}],"time":"2020-10-27T10:14:03Z"})";

    static void doInitialization(uint8_t* data, size_t size)
    {


        if (!socketServer || !socketServer->m_isRunning)
        {
            std::ostringstream ss;
            std::string uuid = datatypes::uuidGenerator().generate();
            ss << m_socketPathBase << uuid;
            m_socketPath = ss.str();

            socketServer = std::make_unique<FakeDetectionServer::FakeServerSocket>(m_socketPath, 0666);
            socketServer->start();
        }

        std::shared_ptr<std::vector<uint8_t>> data_vector;
        data_vector = std::make_shared<std::vector<uint8_t>>();
        data_vector->resize(size);
        memcpy(data_vector->data(), data, size);

        socketServer->initializeData(data_vector);
    }

    static int runFuzzing(uint8_t* data, size_t size)
    {
        Fuzzing::doInitialization(data, size);
        auto clientSocket = std::make_shared<unixsocket::ScanningClientSocket>(m_socketPath);
        auto scanCallbacks = std::make_shared<FakeCallbacks>();

        avscanner::avscannerimpl::ScanClient scanner(*clientSocket, scanCallbacks, false, false, true, E_SCAN_TYPE_ON_DEMAND);

        try
        {
            scanner.scan("/etc/passwd", false);
        }
        catch(common::AbortScanException& e)
        {
            // ScanClient throws these exceptions when it receives a malformed capnp message.
            // We don't want to stop the fuzzing when that happens.
        }

        return 0;
    }
}

#ifdef USING_LIBFUZZER

extern "C" int LLVMFuzzerTestOneInput(uint8_t* Data, size_t Size)
{
    QuietLogSetup quietLogSetup;
    return Fuzzing::runFuzzing(Data, Size);
}

#else
static void initializeLogging()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

static int writeSampleFile(const std::string& path)
{
    ::capnp::MallocMessageBuilder message;
    scan_messages::ScanResponse scanResponse;

    scanResponse.addDetection(
        "/home/vagrant/eicar1",
        "virus",
        "EICAR-AV-Test",
        "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def");

    std::string responseString = scanResponse.serialise();

    auto size = responseString.size();

    auto bytes = unixsocket::splitInto7Bits(size);
    auto lengthBytes = unixsocket::addTopBitAndPutInBuffer(bytes);

    std::ofstream outfile(path, std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(lengthBytes.get()), bytes.size());
    outfile.write(responseString.data(), responseString.size());
    outfile.close();

    return EXIT_SUCCESS;
}

static int processFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
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
    file.close();
    auto x = reinterpret_cast<uint8_t*>(buffer.data());
    Fuzzing::doInitialization(x, size);
    Fuzzing::runFuzzing();
    return 0;
}

int main(int argc, char** argv)
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
