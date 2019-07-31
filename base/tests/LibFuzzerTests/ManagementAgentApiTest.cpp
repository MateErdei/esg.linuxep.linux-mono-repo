/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.


 ManagementAgentApiTest enables to Fuzz the Management Agent Input Socket as defined by the PluginApiMessage.proto.
 By adding files like:


 apimessage
 {
  pluginName: "mdr"
  applicationId: "MDR"
  command: Registration
 }
 apimessage
 {
  pluginName: "mdr"
  applicationId: "MDR"
  command: SendEvent
  payload: "this event from plugin"
 }

 The ManagementAgentApiTest will pick up those two entries and transform than in request which would be used for
 registration and for sending event from the plugin via ManagementAgent.

******************************************************************************************************/

#include "FuzzerUtils.h"

#include "google/protobuf/text_format.h"

#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/PluginApi/ApiException.h>
#include <modules/Common/PluginProtocol/Protocol.h>
#include <modules/Common/ZMQWrapperApi/IContext.h>
#include <modules/Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <modules/Common/ZeroMQWrapper/IIPCException.h>
#include <modules/Common/ZeroMQWrapper/ISocketRequester.h>
#include <modules/ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>

#include <PluginAPIMessage.pb.h>
#include <message.pb.h>
#include <thread>
#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif
#include <fcntl.h>
#include <future>
#include <stddef.h>
#include <stdint.h>

/**
 * Setup of files and directories as expected by ManagementAgent to be present.
 * As well as simple plugin registry and policy to enable ManagementAgent to pick up.
 */
void setupTestFiles()
{
    std::string mdrjson = R"({
  "policyAppIds": [
    "MDR"
  ],
  "statusAppIds": [
    "MDR"
  ],
  "pluginName": "mdr",
  "executableFullPath": "plugins/mdr/bin/mdr",
  "executableArguments": [
  ],
  "environmentVariables": [
    {
      "name": "SOPHOS_INSTALL",
      "value": "SOPHOS_INSTALL_VALUE"
    }
  ],
  "executableUserAndGroup": "root:root"
}
)";
    std::string mdrpolicy = "example of policy content";

    std::string installRoot = "/tmp/fuzz";
    auto fileS = Common::FileSystem::fileSystem();

    for (std::string dir : { "tmp",
                             "base",
                             "base/etc",
                             "base/mcs",
                             "base/mcs/policy",
                             "base/mcs/action",
                             "base/mcs/status",
                             "base/mcs/status/cache",
                             "base/mcs/event",
                             "base/pluginRegistry",
                             "var",
                             "var/ipc",
                             "logs" })
    {
        std::string fullPath = Common::FileSystem::join(installRoot, dir);
        if (!fileS->isDirectory(fullPath))
        {
            fileS->makedirs(fullPath);
        }
    }

    fileS->writeFile("/tmp/fuzz/base/pluginRegistry/mdr.json", mdrjson);
    std::string loggerPath = Common::FileSystem::join(installRoot, "base/etc/logger.conf");
    if (!fileS->isFile(loggerPath))
    {
        fileS->writeFile("/tmp/fuzz/base/etc/logger.conf", "VERBOSITY=OFF");
    }
    std::string mdrPolicyPath = Common::FileSystem::join(installRoot, "base/mcs/policy/MDR_policy.xml");
    if (!fileS->isFile(mdrPolicyPath))
    {
        fileS->writeFile(mdrPolicyPath, mdrpolicy);
    }
}

/**
 * This class holds the ManagementAgent instance running on its own thread and the zmq socket to communicate with
 * the management agent.
 * It is constructed as static object from the fuzzer test to allow the management agent to be running for the whole
 * test phase.
 *
 */

class ManagementRunner : public Runner
{
public:
    ManagementRunner() : Runner()
    {
        setupTestFiles();

        setMainLoop([]() {
            char* argv[] = {
                (char*)"ManagementAgent",
            };
            Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp/fuzz");
            try
            {
                return ManagementAgent::ManagementAgentImpl::ManagementAgentMain::main(1, argv);
            }
            catch (std::exception& ex)
            {
                std::cout << "Failed in the management again main" << std::endl;
                std::cout << ex.what() << std::endl;
                // The goal is that Management Agent should not crash nor exit when receiving requests over the socket.
                // Calling abort is the way to allow LibFuzzer to pick-up the crash.
                abort();
            }
        });

        std::this_thread::sleep_for(std::chrono::seconds(5));

        m_contextPtr = Common::ZMQWrapperApi::createContext();
        m_requester = m_contextPtr->getRequester();
        std::string mng_address =
            Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        m_requester->setTimeout(1000);
        m_requester->connect(mng_address);
    }

    /** Helper method to allow sending requests to Management Agent*/
    Common::PluginProtocol::DataMessage getReply(const std::string& content)
    {
        try
        {
            m_requester->write({ content });
            auto reply = m_requester->read();
            Common::PluginProtocol::Protocol protocol;
            return protocol.deserialize(reply);
        }
        catch (Common::PluginApi::ApiException& ex)
        {
            return Common::PluginProtocol::DataMessage();
        }
        catch (Common::ZeroMQWrapper::IIPCException& ex)
        {
            return Common::PluginProtocol::DataMessage();
        }
        catch (std::exception& ex)
        {
            // The 'contract' of the protocol and message is that the only acceptable
            // exception is the ApiException. Any other is not valid and that
            // is the reason abort is called.
            std::cout << "Failed in getReply" << std::endl;
            std::cout << ex.what() << std::endl;
            abort();
        }
    }

    bool managemementAgentRunning() { return threadRunning(); }

private:
    Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
};

#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const PluginProtocolProto::Msgs& messages)
{
#else
void mainTest(const PluginProtocolProto::Msgs& messages)
{
#endif
    // It is static to ensure that only one ManagementRunner will exist while the full fuzz test runs.
    // this also means that the 'state' is not dependent only on the input message which may become a problem
    // but allows for a very robust verification of the ManagementAgent as it will be test througout the whole
    // fuzz test.
    static ManagementRunner managementRunner;

    // For each message provide by the fuzzer send the request to Management Agent
    // and verify that it does not crash.
    for (const PluginProtocolProto::PluginAPIMessage& message : messages.apimessage())
    {
        std::string onTheWire = message.SerializeAsString();

        auto reply = managementRunner.getReply(onTheWire);

        if (!managementRunner.managemementAgentRunning())
        {
            std::cout << "Management agent may have crashed";

            abort();
        }
    }
}

/**
 * LibFuzzer works only with clang, and developers machine are configured to run gcc.
 * For this reason, the flag HasLibFuzzer has been used to enable buiding 2 outputs:
 *   * the real fuzzer tool
 *   * An output that is capable of consuming the same sort of input file that is used by the fuzzer
 *     but can be build and executed inside the developers IDE.
 */
#ifndef HasLibFuzzer
int main(int argc, char* argv[])
{
    PluginProtocolProto::Msgs messages;
    if (argc < 2)
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }
    std::string content = Common::FileSystem::fileSystem()->readFile(argv[1]);
    google::protobuf::TextFormat::Parser parser;
    parser.AllowPartialMessage(true);
    if (!parser.ParseFromString(content, &messages))
    {
        std::cerr << "Failed to parse file. Content: " << content << std::endl;
        return 3;
    }

    mainTest(messages);
    return 0;
}

#endif
