/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "HttpRequester.h"
#include "CommsMsg.h"
#include "Configurator.h"
#include "CommsDistributor.h"
#include "MonitorDir.h"
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>


namespace CommsComponent
{
    Common::HttpSender::HttpResponse  HttpRequester::triggerRequest(const std::string & requesterName, Common::HttpSender::RequestConfig&& request , std::string && body)
    {
        std::string id = generateId(requesterName); 
        auto expectedPaths = CommsDistributor::getExpectedPath(id); 
        if (!body.empty())
        {            
            Common::FileSystem::fileSystem()->writeFile(expectedPaths.bodyPath, body); 
        }

        Common::FileSystem::fileSystem()->writeFileAtomically(expectedPaths.requestPath,  CommsMsg::toJson(request),
            Common::ApplicationConfiguration::applicationPathManager().getTempPath()); 


        MonitorDir monitorDir{CommsConfigurator::outboundDirectory(), id}; 
        
        // TODO handle timeout
        std::optional<std::string> responseFilePath = monitorDir.next(); 

        std::string fileContent = Common::FileSystem::fileSystem()->readFile(responseFilePath.value()); 

        Common::HttpSender::HttpResponse response = CommsMsg::httpResponseFromJson(fileContent); 

        // TODO: handle error conditions
        return response;
    }

    std::string HttpRequester::generateId(const std::string & requesterName)
    {
        //TODO create the method to generate the id
        // name+time+counter
        return "a" + requesterName; 
    }


}