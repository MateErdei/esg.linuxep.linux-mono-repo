/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServerSocket.h"

#include "avscanner/avscannerimpl/ScanClient.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <capnp/message.h>
#include <common/AbortScanException.h>
#include <datatypes/Print.h>
#include <unixsocket/SocketUtilsImpl.h>

#include <chrono>
#include <cstddef>
#include <fstream>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
#define BASE "/tmp/TestPluginAdapter"

namespace fs = sophos_filesystem;

class FakeCallbacks :public avscanner::avscannerimpl::IScanCallbacks
{
public:
    FakeCallbacks() {}
private:
    void cleanFile(const avscanner::avscannerimpl::path& ) override {}
    void infectedFile(const std::map<fs::path, std::string>&, const sophos_filesystem::path&, bool) override {}
    void scanError(const std::string&) override {}
    void scanStarted() override {}
    void logSummary() override {}
};

namespace Fuzzing
{
    static std::string m_socketPathBase = "/tmp/socket_";
    static std::string m_socketPath;

    static std::unique_ptr<FakeDetectionServer::FakeServerSocket> socketServer;

    static std::string m_scanResult  = R"({"results":[{"detections":[{"threatName":"EICAR-AV-Test","threatType":"virus"}],"path":"/home/vagrant/eicar1","sha256":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"}],"time":"2020-10-27T10:14:03Z"})";

    static void doInitialization(uint8_t* data, size_t size)
    {
        if (!socketServer || !socketServer->m_isRunning)
        {
            std::ostringstream ss;
            boost::uuids::uuid uuid = boost::uuids::random_generator()();
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

    static int runFuzzing()
    {
        auto clientSocket = std::make_shared<unixsocket::ScanningClientSocket>(m_socketPath, timespec{0,1});
        auto scanCallbacks = std::make_shared<FakeCallbacks>();

        avscanner::avscannerimpl::ScanClient scanner(*clientSocket, scanCallbacks, false, E_SCAN_TYPE_ON_DEMAND);

        try
        {
            scanner.scan("/etc/passwd", false);
        }
        catch(AbortScanException& e)
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
    Fuzzing::doInitialization(Data, Size);
    return Fuzzing::runFuzzing();
}

#else
static void initializeLogging()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

static int writeSampleFile(std::string path)
{
    ::capnp::MallocMessageBuilder message;
    scan_messages::ScanResponse scanResponse;

    scanResponse.addDetection("/home/vagrant/eicar1", "EICAR-AV-Test","");

    std::ofstream outfile(path, std::ios::binary);

    std::string responseString = scanResponse.serialise();

    int size = responseString.size();

    auto bytes = unixsocket::splitInto7Bits(size);
    auto lengthBytes = unixsocket::addTopBitAndPutInBuffer(bytes);

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
