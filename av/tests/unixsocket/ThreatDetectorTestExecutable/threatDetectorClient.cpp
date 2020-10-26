/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FakeServerSocket.h"

#include "avscanner/avscannerimpl/ScanClient.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <Common/Threads/AbstractThread.h>
#include <capnp/message.h>
#include <common/AbortScanException.h>
#include <datatypes/Print.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <cstddef>
#include <fstream>
#include <chrono>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
#define BASE "/tmp/TestPluginAdapter"

namespace fs = sophos_filesystem;

class FakeCallbacks :public avscanner::avscannerimpl::IScanCallbacks
{
public:
    FakeCallbacks() {}
private:
    void cleanFile(const avscanner::avscannerimpl::path& ) override {}
    void infectedFile(const avscanner::avscannerimpl::path& , const std::string& , bool )
    override
    {
    }
    void scanError(const std::string&) override {}
    void scanStarted() override {}
    void logSummary() override {}
};

namespace
{
    static std::string m_socketPathBase = "/tmp/socket_";
    static std::string m_socketPath;

    static std::unique_ptr<FakeDetectionServer::FakeServerSocket> socketServer;

    static std::string m_scanResult  = "{\n"
                                       "    \"results\":\n"
                                       "    [\n"
                                       "        {\n"
                                       "            \"detections\": [\n"
                                       "               {\n"
                                       "                   \"threatName\": \"MAL/malware-A\",\n"
                                       "                   \"threatType\": \"malware/trojan/PUA/...\",\n"
                                       "                   \"threatPath\": \"...\",\n"
                                       "                   \"longName\": \"...\",\n"
                                       "                   \"identityType\": 0,\n"
                                       "                   \"identitySubtype\": 0\n"
                                       "               }\n"
                                       "            ],\n"
                                       "            \"Local\": {\n"
                                       "                \"LookupType\": 1,\n"
                                       "                \"Score\": 20,\n"
                                       "                \"SignerStrength\": 30\n"
                                       "            },\n"
                                       "            \"Global\": {\n"
                                       "                \"LookupType\": 1,\n"
                                       "                \"Score\": 40\n"
                                       "            },\n"
                                       "            \"ML\": {\n"
                                       "                \"ml-pe\": 50\n"
                                       "            },\n"
                                       "            \"properties\": {\n"
                                       "                \"GENES/SUPPRESSML\": 1\n"
                                       "            },\n"
                                       "            \"telemetry\": [\n"
                                       "                {\n"
                                       "                    \"identityName\": \"xxx\",\n"
                                       "                    \"dataHex\": \"6865646765686f67\"\n"
                                       "                }\n"
                                       "            ],\n"
                                       "            \"mlscores\": [\n"
                                       "                {\n"
                                       "                    \"score\": 12,\n"
                                       "                    \"secScore\": 34,\n"
                                       "                    \"featuresHex\": \"6865646765686f67\",\n"
                                       "                    \"mlDataVersion\": 56\n"
                                       "                }\n"
                                       "            ]\n"
                                       "        }\n"
                                       "    ]\n"
                                       "}";


    static void DoInitialization(uint8_t* Data, size_t Size)
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

        socketServer->initializeData(Data, Size);
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
            PRINT("Scan aborted, error expected & handled: " << e.what());
        }

        return 0;
    }
}

#ifdef USING_LIBFUZZER

extern "C" int LLVMFuzzerTestOneInput(uint8_t* Data, size_t Size)
{
        DoInitialization(Data, Size);
        return runFuzzing();
}

#else
static bool DoInitialization()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();

    return true;
}

static int writeSampleFile(std::string path)
{
    ::capnp::MallocMessageBuilder message;
    scan_messages::ScanResponse scanResponse;

    scanResponse.addDetection("IT-IS-A-FILE", "A-THREAT");
    scanResponse.setFullScanResult(m_scanResult);

    std::ofstream outfile(path, std::ios::binary);

    std::string request_str = scanResponse.serialise();

    char size = request_str.size();
    outfile.write(&size, sizeof(size));

    outfile.write(request_str.data(), request_str.size());
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
    DoInitialization(x, size);
    runFuzzing();
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

    static bool Initialized = DoInitialization();
    static_cast<void>(Initialized);

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
