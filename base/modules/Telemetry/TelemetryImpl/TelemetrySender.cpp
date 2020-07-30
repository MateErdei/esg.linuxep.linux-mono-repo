/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetrySender.h"
#include <Telemetry/LoggerImpl/Logger.h>
#include <modules/CommsComponent/HttpRequester.h>
#include <chrono>
namespace Telemetry
{
    int TelemetrySender::doHttpsRequest(const Common::HttpSender::RequestConfig& requestConfig)
    {
        Common::HttpSender::RequestConfig copy{requestConfig}; 
        if (!copy.getCertPath().empty()){
            copy.setCertPath(Common::FileSystem::join("/base/mcs/certs/", Common::FileSystem::basename(requestConfig.getCertPath()))); 
        }        

        try{
            auto response = CommsComponent::HttpRequester::triggerRequest("telemetry", copy, std::chrono::minutes(5)); 
            if ( response.httpCode != 200 )
            {
                LOGINFO("Response HttpCode: " << response.httpCode); 
                LOGINFO(response.description);
            }
            return response.exitCode; 
        }
        catch(std::exception & ex)
        {
            LOGERROR( "Perform request failed: " <<ex.what()); 
            return 1; 
        }
    }
}