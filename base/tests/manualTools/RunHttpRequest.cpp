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
    
    Usage: Option1: ./RunHttpRequest -i jsonfile1 [ -i jsonfile2 ... ]
           Option2: ./RunHttpRequest -i jsonfile1 --child-user <name> --child-group <name> --parent-user <name> --parent-group <name> --jail-root <path> --parent-root <path>
           Option3: ./RunHttpRequest --jail-root <path>

           For Option1 and 2 additional proxy can be provided: 
           --proxy <hostname:port>   --proxy_auth <user:password>
           Either all the options for the jail needs to be given, or none of them should be provided. 
    Option1: will run the requests as configured by the json file.
    Option2: does the same thing of option1, but before applies the chroot and drop-privileges.
    Option3: Only execute the Configurator::cleanDefaultMountedPaths to clean up mounted paths.

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

    std::cerr << "Usage: " << name << " " << usage << std::endl;
    exit(code);

}


struct Config
{
    enum RunOption
    {
        Option1, Option2, Option3
    };
    RunOption runOption;
    std::string childUser;
    std::string childGroup;
    std::string parentUser;
    std::string parentGroup;
    std::string jailRoot;
    std::string parentRoot;
    std::string proxy; 
    std::string proxyCredential; 
    std::vector<std::string> requestFiles;
};



Config parseArguments(int argc, char* argv[])
{
    Config config; 
    while (1) {
        int option_index = 0;
        enum Option
        {
            ChildUser = 0,
            ChildGroup = 1,
            ParentUser = 2,
            ParentGroup = 3,
            RequestFile = 4,
            JailRoot = 5,
            ParentRoot = 6,
            Proxy = 7, 
            ProxyCreds = 8
        };
        static struct option long_options[] = {
                {"child-user",   required_argument, 0, 0},
                {"child-group",  required_argument, 0, 0},
                {"parent-user",  required_argument, 0, 0},
                {"parent-group", required_argument, 0, 0},
                {"request-file", required_argument, 0, 0},
                {"jail-root",    required_argument, 0, 0},
                {"parent-root",  required_argument, 0, 0},
                {"proxy",        required_argument, 0, 0},
                {"proxy-auth",   required_argument, 0, 0},            
                {0, 0,                              0, 0}
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
            case Option::ParentRoot:
                config.parentRoot = optarg;
                break;
            case Option::Proxy:
                config.proxy = optarg; 
                break; 
            case Option::ProxyCreds:
                config.proxyCredential = optarg; 
                break; 
            case Option::RequestFile:
            case 'i':
                config.requestFiles.push_back(optarg);
                break;
            case '?':
            default:
                printUsageAndExit(argv[0], 1);
                break;
        }
    }

    std::vector<bool> usersConfig{
            config.childGroup.empty(),
            config.childUser.empty(),
            config.parentUser.empty(),
            config.parentGroup.empty()
    };

    bool allUserConfigNotEmpty = std::all_of(usersConfig.begin(), usersConfig.end(), [](bool b) { return !b; });
    bool allUserConfigEmpty = std::all_of(usersConfig.begin(), usersConfig.end(), [](bool b) { return b; });


    if (config.jailRoot.empty())
    {
        if (allUserConfigEmpty&&!config.requestFiles.empty())
        {
            config.runOption = Config::RunOption::Option1;
            return config;
        }
    }
    else
    {
        if ( allUserConfigNotEmpty && !config.requestFiles.empty())
        {
            config.runOption = Config::RunOption::Option2;
            return config;
        }

        if (allUserConfigEmpty&&config.requestFiles.empty())
        {
            config.runOption = Config::RunOption::Option3;
            return config;
        }
    }
    std::cerr << "Invalid combination of arguments. " << std::endl;
    printUsageAndExit(argv[0], 3);

    return config;
}

void serialize(const Common::HttpSender::HttpResponse &response , const std::string& filename)
{
    auto content = CommsComponent::CommsMsg::toJson(response); 
    std::string path = filename + "resp.json"; 
    Common::FileSystem::fileSystem()->writeFile(path, content); 
}


int runSimpleHttpRequests(const std::vector<std::string> & files, const Common::HttpSenderImpl::ProxySettings& proxy)
{
    Common::Logging::ConsoleLoggingSetup consoleSetup; 
    LOGINFO("Running runSimpleHttpRequests"); 
    auto networkSide = CommsComponent::NetworkSide::createOrShareNetworkSide(); 
    networkSide->setProxy(proxy); 
    for(auto & filePath : files)
    {
        LOGINFO("Process file: " << filePath); 
        auto content = Common::FileSystem::fileSystem()->readFile(filePath); 
        auto response = networkSide->performRequest( CommsComponent::CommsMsg::requestConfigFromJson(content)); 
        serialize(response, Common::FileSystem::basename(filePath));
    }
    return 0;
}

int runUnmountPaths(const std::string & chroot)
{
    Common::Logging::ConsoleLoggingSetup consoleSetup;
    CommsComponent::CommsConfigurator::cleanDefaultMountedPaths(chroot);
    return 0;
}

int runHttpRequestsInTheJail(Config& config, const Common::HttpSenderImpl::ProxySettings & proxy)
{
    using namespace CommsComponent; 
    CommsComponent::UserConf parentConf{config.parentUser, config.parentGroup, "logparent" }; 
    CommsComponent::UserConf childConf{config.childUser, config.childGroup, "logchild"};
    CommsComponent::CommsConfigurator configurator(config.jailRoot, childConf, parentConf, CommsComponent::CommsConfigurator::getListOfDependenciesToMount());


    auto parentProc = [&config,proxy](std::shared_ptr<MessageChannel> channel, OtherSideApi & childProxy) {
        if (!config.parentRoot.empty())
        {
            chdir(config.parentRoot.c_str());
        }

        if ( !proxy.proxy.empty() )
        {
            CommsMsg comms;
            comms.id = "ProxyConfig";
            CommsComponent::CommsConfig config; 
            config.setProxy(proxy.proxy); 
            config.setDeobfuscatedCredential(proxy.credentials); 
            comms.content = config;
            childProxy.pushMessage(CommsMsg::serialize(comms));
            LOGINFO("Send the config message for proxy"); 
            
        }
        for (auto& filePath : config.requestFiles)
        {
            LOGINFO("Sending the request related to the filePath: " << filePath ); 
            auto content = Common::FileSystem::fileSystem()->readFile(filePath);
            CommsMsg comms;
            comms.id = Common::FileSystem::basename(filePath);
            comms.content = CommsComponent::CommsMsg::requestConfigFromJson(content);
            childProxy.pushMessage(CommsMsg::serialize(comms));
            LOGINFO("Request sent filePath: " << filePath ); 
        }

        size_t responses = 0; 
        while(responses < config.requestFiles.size())
        {
            responses++; 
            std::string message;
            channel->pop(message);
            LOGINFO("Received response from child: " << message.size());  
            CommsMsg comms = CommsComponent::CommsMsg::fromString(message);  
            LOGINFO("Received response from with id: " << comms.id);  
            serialize(std::get<Common::HttpSender::HttpResponse>(comms.content), comms.id ); 
            LOGINFO("File produced " << comms.id); 
        }
    };

    CommsComponent::CommsNetwork childProc;

    std::cout << "Running the splitProcesses Reactors" << std::endl; 
    int code =  CommsComponent::splitProcessesReactors(parentProc, std::move(childProc), configurator); 
    std::cout << "Return code: " << code << std::endl;
    return code;
}



int main(int argc, char * argv[])
{
    auto config = parseArguments(argc, argv); 
    Common::HttpSenderImpl::ProxySettings proxy; 
    proxy.proxy = config.proxy; 
    proxy.credentials = config.proxyCredential; 
    switch (config.runOption)
    {
        case Config::RunOption::Option1:            
            return runSimpleHttpRequests(config.requestFiles, proxy);
            break;
        case Config::RunOption::Option2:
            return runHttpRequestsInTheJail(config, proxy);
            break;
        case Config::RunOption::Option3:
            return runUnmountPaths(config.jailRoot);
            break;
        default:
            break;
    }
    return 1;
}
