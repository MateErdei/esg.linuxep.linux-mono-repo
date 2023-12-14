// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "common/fuzzer/FuzzerUtils.h"
#include "tests/LibFuzzerTests/vector_strings.pb.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystemImpl/FilePermissionsImpl.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/PluginApiImpl/BaseServiceAPI.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginProtocol/Protocol.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZMQWrapperApi/IContextSharedPtr.h"
#include "Common/ZeroMQWrapper/IIPCException.h"

#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/TempDir.h"

#include "src/libfuzzer/libfuzzer_macro.h"
#include "src/mutator.h"

#include <google/protobuf/text_format.h>

#include <future>
#include <thread>

/** this class is just to allow the tests to be executed without requiring root*/
class PluginNullFilePermission : public Common::FileSystem::FilePermissionsImpl
{
public:
    void chown(const Path& , const std::string& , const std::string& ) const override
    {
    }
    void chmod(const Path& , __mode_t ) const override
    {
    }
    gid_t getGroupId([[maybe_unused]] const std::string& groupString) const override
    {
        return 1;
    }
};

class PluginScopedFilePermission
{
public:
    PluginScopedFilePermission()
    {
        Tests::replaceFilePermissions( std::unique_ptr<Common::FileSystem::IFilePermissions>( new PluginNullFilePermission()) );
    }
    ~PluginScopedFilePermission()
    {
        Tests::restoreFilePermissions();
    }
};

///**
// * This class holds the Dummy Plugin instance running on its own thread and the zmq socket to communicate with
// * the plugin.
// * It is constructed as static object from the fuzzer test to allow the dummy plugin to be running for the whole
// * test phase.
//example of file:
//parts: "\n\004test\020\a\032\004Test\"\bEventXml"
//parts: "\n\004test\020\006\032\004Test\"\bEventXml"
// */
//
class DummyPlugin: public  Common::PluginApi::IPluginCallbackApi
{
   std::promise<void> m_promise;

public:
    void applyNewPolicy(const std::string& ) override
    {
        std::cout << "Apply policy" << std::endl;
    }

    void queueAction(const std::string& ) override
    {
        std::cout << "queue action" << std::endl;
    }

    void onShutdown() override
    {
        std::cout << "shutdown" << std::endl;
        m_promise.set_value();
    }
    void join()
    {
        m_promise.get_future().get();
    }

    Common::PluginApi::StatusInfo getStatus(const std::string& ) override
    {
        std::cout << "get status" << std::endl;
        return Common::PluginApi::StatusInfo{};
    }
    std::string getTelemetry() override
    {
        std::cout << "send telemetry" << std::endl;
        return std::string{};
    }

    std::string getHealth() override
    {
        std::cout << "get health" << std::endl;
        return std::string{};
    }

};

class DummyPluginRunner : public Runner
{

public:
    ~DummyPluginRunner()
    {}

    DummyPluginRunner() : Runner(), m_consoleSetup{Common::Logging::LOGOFFFORTEST()}
    {
        PluginScopedFilePermission scopedFilePermission;

        Tests::TempDir tempdir("/tmp");
        tempdir.makeDirs("var/ipc/plugins/");

        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, tempdir.dirPath());
        Common::ApplicationConfiguration::applicationConfiguration().setData("Test", "inproc://test.ipc");

        m_contextPtr = Common::ZMQWrapperApi::createContext();
        Common::ZMQWrapperApi::IContextSharedPtr copyContext = m_contextPtr;

        setMainLoop([copyContext, this]()
        {
            Common::PluginApiImpl::PluginResourceManagement resourceManagement(copyContext);
            std::string pluginName = "Test";
            auto dummy = std::make_shared<DummyPlugin>();
            // the code inside the try catch is copied from createPluginAPI but with registerWithManagementagent removed
            try
            {
                auto requester = m_contextPtr->getRequester();
                requester->setTimeout(1000);
                requester->setConnectionTimeout(1000);

                std::string mng_address =
                        Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
                requester->connect(mng_address);

                std::unique_ptr<Common::PluginApiImpl::BaseServiceAPI> pluginSetup(
                        new Common::PluginApiImpl::BaseServiceAPI(pluginName, std::move(requester)));

                auto replier = m_contextPtr->getReplier();
                Common::PluginApiImpl::PluginResourceManagement::setupReplier(*replier, pluginName, 1000, 1000);

                pluginSetup->setPluginCallback(pluginName, dummy, std::move(replier));

                auto plugin = std::unique_ptr<Common::PluginApi::IBaseServiceApi>(pluginSetup.release());

                dummy->join();
            }
            catch (Common::PluginApi::ApiException& ex)
            {
                std::cout << "main loop threw exception: " << ex.what() << std::endl;
            }
        });

        std::this_thread::sleep_for(std::chrono::seconds(2));

        m_requester = m_contextPtr->getRequester();
        m_requester->setTimeout(1000);
        m_requester->setConnectionTimeout(1000);
        std::string dummy_address =
                Common::ApplicationConfiguration::applicationPathManager().getPluginSocketAddress("Test");
        m_requester->connect(dummy_address);
    }

    /** Helper method to allow sending requests to Dummy Plugin*/
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

    bool dummyPluginRunning() { return threadRunning(); }

private:
    Common::Logging::ConsoleLoggingSetup m_consoleSetup;
    Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
};


#ifdef USING_LIBFUZZER
DEFINE_PROTO_FUZZER(const VectorStringsProto::Query & query)
{
#else
void mainTest(const VectorStringsProto::Query & query)
{
#endif
//    // It is static to ensure that only one DummyPluginRunner will exist while the full fuzz test runs.
//    // this also means that the 'state' is not dependent only on the input message which may become a problem
//    // but allows for a very robust verification of the DummyPlugin as it will be test throughout the whole
//    // fuzz test.
    static DummyPluginRunner dummyRunner;

    for (auto & part: query.parts())
    {
        std::vector<std::string> data;
        data.push_back(part);
        dummyRunner.getReply(data);
    }

    if (!dummyRunner.dummyPluginRunning())
    {
            std::cout << "Dummy Plugin may have crashed"<< std::endl;
            abort();
    }
}

/**
 * LibFuzzer works only with clang, and developers machine are configured to run gcc.
 * For this reason, the flag USING_LIBFUZZER has been used to enable buiding 2 outputs:
 *   * the real fuzzer tool
 *   * An output that is capable of consuming the same sort of input file that is used by the fuzzer
 *     but can be build and executed inside the developers IDE.
 */
#ifndef USING_LIBFUZZER
int main(int argc, char* argv[])
{

    Common::Logging::ConsoleLoggingSetup loggingSetup;
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
