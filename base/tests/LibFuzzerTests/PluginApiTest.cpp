/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.



******************************************************************************************************/

#include "FuzzerUtils.h"

#include "google/protobuf/text_format.h"

#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/PluginApiImpl/PluginResourceManagement.h>
#include <modules/Common/PluginApi/ApiException.h>
#include <modules/Common/PluginProtocol/Protocol.h>
#include <modules/Common/ZMQWrapperApi/IContext.h>
#include <modules/Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <modules/Common/ZeroMQWrapper/IIPCException.h>
#include <modules/Common/ZeroMQWrapper/ISocketRequester.h>
//#include <modules/ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <vector_strings.pb.h>
#include <thread>
#ifdef HasLibFuzzer
#    include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#    include <libprotobuf-mutator/src/mutator.h>
#endif
#include <fcntl.h>
#include <future>
#include <stddef.h>
#include <stdint.h>


///**
// * This class holds the ManagementAgent instance running on its own thread and the zmq socket to communicate with
// * the management agent.
// * It is constructed as static object from the fuzzer test to allow the management agent to be running for the whole
// * test phase.
//example of file:
//parts: "hello"
//parts: "world"
// */
//
class DummyPlugin: public  Common::PluginApi::IPluginCallbackApi
{
public:
    void applyNewPolicy(const std::string& ) override
    {
        std::cout << "Apply policy";
    }

    void queueAction(const std::string& ) override
    {
        std::cout << "queue action";
    }

    void onShutdown() override
    {
        std::cout << "shutdown";
    }
    Common::PluginApi::StatusInfo getStatus(const std::string& ) override
    {
        std::cout << "get status";
        return Common::PluginApi::StatusInfo{};
    }
    std::string getTelemetry() override
    {
        std::cout << "send telemetry";
        return std::string{};
    }

};

void setup()
{
    auto fileS = Common::FileSystem::fileSystem();

    std::string fullPath = Common::FileSystem::join(Common::ApplicationConfiguration::SOPHOS_INSTALL, "");
    fileS->makedirs("/tmp/fuzz/var/ipc/plugins/");

}

class ManagementRunner : public Runner
{
public:
    ~ManagementRunner()
    {
        try
        {
            m_promise.set_value();
        }catch (std::exception&)
        {}
    }
    ManagementRunner() : Runner()
    {
        setup();
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp/fuzz");

        Common::ApplicationConfiguration::applicationConfiguration().setData("Test", "inproc://test.ipc");
        m_contextPtr = Common::ZMQWrapperApi::createContext();
        Common::ZMQWrapperApi::IContextSharedPtr copyContext = m_contextPtr;
        setMainLoop([copyContext, this]() {
            Common::PluginApiImpl::PluginResourceManagement resourceManagement(copyContext);
            auto plugin = resourceManagement.createPluginAPI("Test", std::make_shared<DummyPlugin>());
            auto future = m_promise.get_future();
            future.wait();
        });

        std::this_thread::sleep_for(std::chrono::seconds(2));

        m_requester = m_contextPtr->getRequester();
        std::string mng_address =
                Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress("Test");
        m_requester->setTimeout(1000);
        m_requester->connect(mng_address);
    }

    /** Helper method to allow sending requests to Management Agent*/
    Common::PluginProtocol::DataMessage getReply(const std::vector<std::string>& content)
    {
        try
        {
            m_requester->write(content);
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
    std::promise<void> m_promise;
    Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
};


#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const VectorStringsProto::Query & query)
{
#else
void mainTest(const VectorStringsProto::Query & query)
{
#endif
//    // It is static to ensure that only one ManagementRunner will exist while the full fuzz test runs.
//    // this also means that the 'state' is not dependent only on the input message which may become a problem
//    // but allows for a very robust verification of the ManagementAgent as it will be test througout the whole
//    // fuzz test.
    static ManagementRunner managementRunner;
    std::vector<std::string> data;
    for( auto & part: query.parts())
    {
        data.push_back(part);
    }
    if( !managementRunner.managemementAgentRunning())
    {
            std::cout << "Management agent may have crashed"<< std::endl;

            abort();
    }
    managementRunner.getReply(data);
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
    VectorStringsProto::Query query;
    if (argc < 2)
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }



    std::string content = Common::FileSystem::fileSystem()->readFile(argv[1]);
    google::protobuf::TextFormat::Parser parser;
    parser.AllowPartialMessage(true);
    if (!parser.ParseFromString(content, &query))
    {
        std::cerr << "Failed to parse file. Content: " << content << std::endl;
        return 3;
    }

    mainTest(query);
    return 0;
}

#endif
