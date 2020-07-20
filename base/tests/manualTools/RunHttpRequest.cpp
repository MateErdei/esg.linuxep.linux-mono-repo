/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/CommsComponent/SplitProcesses.h>
#include <modules/CommsComponent/NetworkSide.h>
#include <modules/CommsComponent/OtherSideApi.h>
#include <modules/CommsComponent/CommsMsg.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/FileSystem/IFilePermissions.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <unistd.h>
#include <getopt.h>
void printUsageAndExit(std::string name, int code)
{
    std::string usage{R"(RunHttpRequest can either run a normal request or run an http request inside the jail root. 
    
    Usage: ./RunHttpRequest -i jsonfile1 [ -i jsonfile2 ... ]
           ./RunHttpRequest -i jsonfile1 --child-user <name> --child-group <name> --parent-user <name> --parent-group <name> --jail-root
           Either all the options for the jail needs to be given, or none of them should be provided. 
    The json files should be eiter relative or absolute path and must be visible to the parent process (in case of a jail run). 
    
    A request should be a json file like:
    {
"requestType": "GET",
 "server": "www.domain.com",
 "resourcePath": "/index.html",
 "bodyContent": "",
 "port": "445",
 "headers": [
  "header1",
  "header2"]
  }
    )"}; 

        std::cerr <<  "Usage: " << name << " usernameOfParent  usernameOfChild  pathOfExecutable" << std::endl; 
        exit(code); 

}


struct Config{
    std::string childUser; 
    std::string childGroup; 
    std::string parentUser; 
    std::string parentGroup;
    std::string jailRoot; 
    std::vector<std::string> requestFiles; 
};



Config parseArguments(int argc, char* argv[])
{
    Config config; 
    while (1) {
        int option_index = 0;
        enum Option{ChildUser=0, ChildGroup=1, ParentUser=2, ParentGroup=3,RequestFile=4, JailRoot=5}; 
        static struct option long_options[] = {
            {"child-user",      required_argument, 0,  0 },
            {"child-group",     required_argument, 0,  0 },
            {"parent-user",     required_argument, 0,  0 },
            {"parent-group",    required_argument, 0,  0 },
            {"request-file",    required_argument, 0,  0 },
            {"jail-root",       required_argument, 0,  0 },
            {0,         0,                 0,  0 }
        };

        int c = getopt_long(argc, argv, "i:",
                long_options, &option_index);
        if (c == -1)
        {
            break;
        }
        if (c == 0)
        {
            c = option_index; 
        }
        switch (c) {
        case Option::ChildUser:
            config.childUser = optarg;             
            break;

        case Option::ChildGroup:
            config.childGroup = optarg;             
            break;
        case Option::ParentUser:
            config.parentUser = optarg;             
            break;
        case Option::ParentGroup:
            config.parentGroup = optarg;             
            break;
        case Option::JailRoot:
            config.jailRoot = optarg; 
            break; 
        case Option::RequestFile:
        case 'i':
            config.requestFiles.push_back(optarg); 
            break;
        case '?':
        default:
            printUsageAndExit(argv[0],1); 
            break; 
        }
    }

    // confirm is it a valid config: 
    if ( config.requestFiles.empty())
    {
        std::cerr << "No file provided for http request" << std::endl;  
        printUsageAndExit(argv[0], 2); 
    }

    if ( !config.childGroup.empty() 
        || !config.childUser.empty()
        || !config.parentGroup.empty()
        || !config.parentUser.empty()
        || !config.jailRoot.empty() 
    )
    {
        // if anyone was provided, all need to be given
        if( config.childGroup.empty() 
        || config.childUser.empty()
        || config.parentGroup.empty()
        || config.parentUser.empty()
        || config.jailRoot.empty() )
        {
            std::cerr << "all arguments for the jail configuration must be passed or none of them" << std::endl; 
            printUsageAndExit(argv[0], 3); 
        }
    }
    return config; 

}

void serialize(const Common::HttpSender::HttpResponse &response , const std::string& filename)
{
    auto content = CommsComponent::CommsMsg::toJson(response); 
    std::string path = filename + "resp.json"; 
    Common::FileSystem::fileSystem()->writeFile(path, content); 
}


void runSimpleHttpRequests(const std::vector<std::string> & files)
{
    Common::Logging::ConsoleLoggingSetup consoleSetup; 
    LOGINFO("Running runSimpleHttpRequests"); 
    auto networkSide = CommsComponent::NetworkSide::createOrShareNetworkSide(); 
    for(auto & filePath : files)
    {
        LOGINFO("Process file: " << filePath); 
        auto content = Common::FileSystem::fileSystem()->readFile(filePath); 
        auto response = networkSide->performRequest( CommsComponent::CommsMsg::requestConfigFromJson(content)); 
        serialize(response, Common::FileSystem::basename(filePath));
    }    
}

void runHttpRequestsInTheJail(Config& config)
{
    using namespace CommsComponent; 
    CommsComponent::UserConf parentConf{config.parentUser, config.parentGroup, "logparent" }; 
    CommsComponent::UserConf childConf{config.childUser, config.childGroup, "logchild"}; 
    CommsComponent::CommsConfigurator configurator(config.jailRoot, childConf, parentConf,
                                                std::vector<ReadOnlyMount>() );


    auto parentProc = [&config](std::shared_ptr<MessageChannel> channel, OtherSideApi & childProxy)
    {
        for(auto & filePath : config.requestFiles)
        {
            auto content = Common::FileSystem::fileSystem()->readFile(filePath); 
            CommsMsg comms; 
            comms.id = Common::FileSystem::basename(filePath); 
            comms.content = CommsComponent::CommsMsg::requestConfigFromJson(content);
            childProxy.pushMessage(CommsMsg::serialize(comms)); 
        }
        size_t responses = 0; 
        while(responses < config.requestFiles.size())
        {
            responses++; 
            std::string message;
            channel->pop(message); 
            CommsMsg comms = CommsComponent::CommsMsg::fromString(message);  
            serialize(std::get<Common::HttpSender::HttpResponse>(comms.content), comms.id ); 
        }
    };

    auto childProc = [](std::shared_ptr<MessageChannel> channel, OtherSideApi & parentProxy){
        auto networkSide = CommsComponent::NetworkSide::createOrShareNetworkSide(); 
        while(true)
        {
            std::string message; 
            channel->pop(message); 
            CommsMsg comms = CommsComponent::CommsMsg::fromString(message);
            auto response = networkSide->performRequest(std::get<Common::HttpSender::RequestConfig>(comms.content));
            comms.content = response; 
            parentProxy.pushMessage(CommsMsg::serialize(comms)); 
        }
    };

    std::cout << "Running the splitProcesses Reactors" << std::endl; 
    int code =  CommsComponent::splitProcessesReactors(parentProc, std::move(childProc), configurator); 
    std::cout << "Return code: " << code << std::endl; 
}
 
int main(int argc, char * argv[])
{
    auto config = parseArguments(argc, argv); 
    if ( config.jailRoot.empty())
    {
        runSimpleHttpRequests(config.requestFiles);             
    }
    else
    {
        runHttpRequestsInTheJail(config); 
    }
    
}
