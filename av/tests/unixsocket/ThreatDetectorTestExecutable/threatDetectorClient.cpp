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
#include <datatypes/Print.h>

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

    static void createObject()
    {
        ::capnp::MallocMessageBuilder message;
        scan_messages::ScanResponse scanResponse;

        scanResponse.addDetection("IT-IS-A-FILE", "A-THREAT");
        //scanResponse.setFullScanResult(m_scanResult);

        std::ofstream outfile("/tmp/filename", std::ios::binary);

        std::string request_str = scanResponse.serialise();

        char size = request_str.size();
        outfile.write(&size, sizeof(size));

        outfile.write(request_str.data(), request_str.size());
        outfile.close();
    }

    static void DoSomethingWithData(uint8_t* Data, size_t Size, std::string dataString)
    {
        Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();

        auto socketPath = "/tmp/unix_socket";

        FakeDetectionServer::FakeServerSocket socketServer("/tmp/unix_socket",0666, Data, Size, dataString);
        socketServer.start();

        auto clientSocket = std::make_shared<unixsocket::ScanningClientSocket>(socketPath);
        auto scanCallbacks = std::make_shared<FakeCallbacks>();

        avscanner::avscannerimpl::ScanClient scanner(*clientSocket, scanCallbacks, false, E_SCAN_TYPE_ON_DEMAND);
        scanner.scan("/tmp/build.log", false);
    }

    static void processFile()
    {
        std::ifstream file("/tmp/filename", std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();

        if (size < 0)
        {
            PRINT("cannot open file");
            return;
        }

        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size))
        {
            PRINT("cannot read file");
            return;
        }
        auto x = reinterpret_cast<uint8_t*>(buffer.data());
        DoSomethingWithData(x, size, std::string(buffer.begin(), buffer.end()));
    }
};

#ifdef USING_LIBFUZZER
static int runFuzzing(const uint8_t* Data, size_t Size)
{
    // sends a scan request
    // writes to a socket
    // waits for reply
    // does stuff
    fakeServerSocket("/tmp/unix_socket",0666);

    //init ScanClient
    //init fakeDetectorServer: that returns (fuzzed) Data and Size when a request is written on the socket it listens
    //ScanClient sends request
    //fakeDetectorServer reads and discards client request
    //fakeDetectorServer writes on socket for ScanningClientSocket->ScanClient to read using Data and Size
    //?? profit ??

    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size)
{
    return runFuzzing(Data, Size);
}
#else
int main(int /*argc*/, char** /*argv*/)
{
    createObject();
    processFile();
}
#endif