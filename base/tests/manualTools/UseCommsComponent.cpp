/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/CommsComponent/HttpRequester.h>
#include <modules/CommsComponent/CommsMsg.h>
#include <modules/Common/Logging/ConsoleLoggingSetup.h>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
void printUsageAndExit(std::string name, int code)
{
    std::string usage{R"(UseCommsComponent can either run a normal request or run an http request inside the jail root. 
    
    Usage: ./UseCommsComponent -i jsonfile1 -b bodyfile

    In order to suceed, the user should belong to the group sophos-spl-group.
       
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
  The json can also have a certPath for not root stored certificates. The certificates must be placed in /opt/sophos-spl/base/mcs/certs and the certPath should be given as /base/mcs/certs/xxx.crt.
    )"}
    ;

    std::cerr << "Usage: " << name << " " << usage << std::endl;
    exit(code);

}


struct Config
{
    std::string requestFile;
    std::string bodyFile; 
};



Config parseArguments(int argc, char* argv[])
{
    Config config; 
    while (1) {
        int option_index = 0;
        enum Option
        {
            RequestFile = 4,
            RequestBody = 5
        };
        static struct option long_options[] = {
                {"request-file", required_argument, 0, 0},
                {"request-body", required_argument, 0, 0},
                {0, 0,                              0, 0}
        };

        int c = getopt_long(argc, argv, "i:b:",
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
            case Option::RequestFile:
            case 'i':
                config.requestFile = std::string{optarg};
                break;
            case Option::RequestBody:
            case 'b':
                config.bodyFile = std::string{optarg};
                break;                
            case '?':
            default:
                printUsageAndExit(argv[0], 1);
                break;
        }
    }
    return config;
}



int main(int argc, char * argv[])
{
    Common::Logging::ConsoleLoggingSetup consoleSetup;     
    auto config = parseArguments(argc, argv);
    auto content = Common::FileSystem::fileSystem()->readFile(config.requestFile); 
    std::string bodyContent; 
    if ( !config.bodyFile.empty())
    {
        bodyContent = Common::FileSystem::fileSystem()->readFile(config.bodyFile); 
    }
    
    Common::HttpSender::RequestConfig request = CommsComponent::CommsMsg::requestConfigFromJson(content); 
    if (!bodyContent.empty())
    {
        request.setData(bodyContent); 
    }

    auto response = CommsComponent::HttpRequester::triggerRequest("UseCommsComponent", request, std::chrono::minutes(1)); 
    std::cout << "Response: \nCurlCode: " << response.exitCode << "\n"
        << "httpCode: " << response.exitCode << "\n"
        << "description: " << response.description << std::endl; 
    
    // just to allow the tester to do run redirecting the standard error for another file if the body content were to be too big.
    if (response.bodyContent.size() > 1000)
    {
        std::cerr << response.bodyContent << std::endl; 
    }
    else
    {
        std::cout << response.bodyContent << std::endl; 
    }

    return 0;
}
