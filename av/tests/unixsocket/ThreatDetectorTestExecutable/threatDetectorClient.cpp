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

    static void DoSomethingWithData(uint8_t* Data, size_t Size)
    {
        Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();

        auto socketPath = "/tmp/unix_socket";

        FakeDetectionServer::FakeServerSocket socketServer("/tmp/unix_socket",0666, Data, Size);
        socketServer.start();

        auto clientSocket = std::make_shared<unixsocket::ScanningClientSocket>(socketPath);
        auto scanCallbacks = std::make_shared<FakeCallbacks>();

        avscanner::avscannerimpl::ScanClient scanner(*clientSocket, scanCallbacks, false, E_SCAN_TYPE_ON_DEMAND);
        scanner.scan("/tmp/build.log", false);
    }


};

#ifdef USING_LIBFUZZER

extern "C" int LLVMFuzzerTestOneInput(uint8_t* Data, size_t Size)
{
    DoSomethingWithData(Data, Size);
    return 0;
}

#else

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

    int main(int /*argc*/, char** /*argv*/)
    {
        createObject();
        processFile();
    }
#endif
