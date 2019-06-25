/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <PluginAPIMessage.pb.h>
#include <message.pb.h>
#include "google/protobuf/text_format.h"
#include <modules/ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <modules/Common/ZeroMQWrapper/ISocketRequester.h>
#include <modules/Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <modules/Common/ZMQWrapperApi/IContext.h>
#include <modules/Common/ZeroMQWrapper/ISocketRequester.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Common/PluginProtocol/Protocol.h>
#include <modules/Common/PluginApi/ApiException.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#ifdef  HasLibFuzzer
#include <libprotobuf-mutator/src/mutator.h>
#include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#endif
#include <future>
#include <fcntl.h>

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
    std::string installRoot = "/tmp/fuzz";
    auto fileS = Common::FileSystem::fileSystem();
    /*if ( fileS->isDirectory( installRoot))
    {
        fileS->removeFileOrDirectory(installRoot);
    }*/

    for( std::string dir : {"tmp",
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
                            "logs"})
    {
        std::string fullPath = Common::FileSystem::join(installRoot, dir);
        if( !fileS->isDirectory(fullPath))
        {
            fileS->makedirs(fullPath);
        }
    }


    fileS->writeFile("/tmp/fuzz/base/pluginRegistry/mdr.json", mdrjson);
    std::string loggerPath = Common::FileSystem::join(installRoot, "base/etc/logger.conf");
    if ( !fileS->isFile(loggerPath))
    {
        fileS->writeFile("/tmp/fuzz/base/etc/logger.conf", "VERBOSITY=OFF");
    }

}


class ManagementRunner
{
public:
    ManagementRunner()
    {
        setupTestFiles();
        m_fun = std::async(std::launch::async, []()
        {

            char * argv[] = {(char*)"ManagementAgent",};
            Common::ApplicationConfiguration::applicationConfiguration().setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp/fuzz");
            try
            {
                return ManagementAgent::ManagementAgentImpl::ManagementAgentMain::main(1,argv);
            }
            catch (std::exception & ex)
            {
                std::cout << "Failed in the management again main" << std::endl;
                std::cout << ex.what() << std::endl;
                abort();
            }
            return 5;
        }
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        m_contextPtr = Common::ZMQWrapperApi::createContext();
        m_requester = m_contextPtr->getRequester();
        std::string mng_address =
                Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        m_requester->setTimeout(1000);
        m_requester->connect(mng_address);
    }

    Common::PluginProtocol::DataMessage getReply(const std::string& content)
    {
        try
        {
            m_requester->write({content});
            auto reply = m_requester->read();
            Common::PluginProtocol::Protocol protocol;
            return protocol.deserialize(reply);
        }
        catch( Common::PluginApi::ApiException & ex)
        {
            return Common::PluginProtocol::DataMessage();

        }
        catch ( std::exception & ex)
        {
            std::cout << "Failed in getReply" << std::endl;
            std::cout << ex.what() << std::endl;
            abort();
        }


    }

    bool managemementAgentRunning()
    {
        return m_fun.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout;
    }

    ~ManagementRunner()
    {
        std::cout << "Destructor management runner" << std::endl;
        m_fun.get();
    }
private:
    std::future<int> m_fun;
    Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
};


#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const PluginProtocolProto::Msgs& messages) {
#else
void mainTest(const PluginProtocolProto::Msgs& messages){
#endif
    static ManagementRunner managementRunner;

    for( const PluginProtocolProto::PluginAPIMessage & message: messages.apimessage()  )
    {

        std::string onTheWire = message.SerializeAsString();
        if( onTheWire.empty())
        {
            continue;
        }
        auto reply = managementRunner.getReply(onTheWire);

        if( !managementRunner.managemementAgentRunning())
        {
            std::cout << "Management agent may have crashed";
            abort();
        }
    }
}

#ifndef HasLibFuzzer
int main(int argc, char *argv[])
{
    PluginProtocolProto::Msgs messages;
    if ( argc < 2)
    {
        std::cerr << " Invalid argument. Usage: "<< argv[0] << " <input_file>" << std::endl;
        return 1;
    }
    std::string content = Common::FileSystem::fileSystem()->readFile(argv[1]);
    google::protobuf::TextFormat::Parser parser;
    parser.AllowPartialMessage(true);
    if( !parser.ParseFromString(content, &messages))
    {
        std::cerr << "Failed to parse file. Content: " << content << std::endl;
        return 3;
    }

     mainTest(messages);
     return 0;
}

#endif

