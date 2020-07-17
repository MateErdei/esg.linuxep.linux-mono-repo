/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CommsMsg.h"
#include <CommsMsg.pb.h>

namespace
{

    using namespace CommsComponent;
    using namespace Common::HttpSender;

    struct CommsMsgVisitorSerializer
    {
        CommsMsgProto::CommsMsg & m_msg; 
        CommsMsgVisitorSerializer(CommsMsgProto::CommsMsg& msg): m_msg(msg){}
        void operator()(const HttpResponse& httpResponse)
        {
            auto proto = m_msg.mutable_httpresponse(); 
            proto->set_httpcode(httpResponse.httpCode); 
            proto->set_description( httpResponse.description); 
            proto->set_bodycontent( httpResponse.bodyContent);
        }
        void operator()(const RequestConfig& requestConfig)
        {
            auto proto = m_msg.mutable_httprequest();
            proto->set_bodycontent(requestConfig.getData());
            proto->set_server(requestConfig.getServer());
            proto->set_resourcepath(requestConfig.getResourcePath());
            proto->set_requesttype(requestConfig.getRequestTypeAsString());
            proto->set_port(requestConfig.getPort());

            for(const auto& header : requestConfig.getAdditionalHeaders())
            {
                proto->add_headers(header);
            }
        }
    };

    Common::HttpSender::RequestConfig from(const CommsMsgProto::HttpRequest& requestProto)
    {

        Common::HttpSender::RequestConfig requestConfig;
        requestConfig.setServer(requestProto.server());
        requestConfig.setResourcePath(requestProto.resourcepath());
        requestConfig.setData(requestProto.bodycontent());
        requestConfig.setPort(requestProto.port());
        std::vector<std::string> headers; 
        for(auto& header: requestProto.headers())
        {
            headers.push_back(header);             
        }
        requestConfig.setAdditionalHeaders(std::move(headers));
        return requestConfig;
    }
    Common::HttpSender::HttpResponse from(const CommsMsgProto::HttpResponse& responseProto)
    {
        Common::HttpSender::HttpResponse httpResponse;
        httpResponse.httpCode = responseProto.httpcode();
        httpResponse.bodyContent = responseProto.bodycontent();
        httpResponse.description = responseProto.description();
        return httpResponse;
    }

    //TODO LINUXDAR-1956 the below is skeleton code for this ticket
/*    TelemetryRequest from(const CommsMsgProto::TelemetryRequest& proto)
    {
        CommsComponent::TelemetryRequest telemetryRequest; 
        telemetryRequest.telemetryContent = proto.telemetrycontent(); 
        return telemetryRequest; 
    }
    DiagnoseRequest from(const CommsMsgProto::DiagnoseRequest& proto)
    {
        CommsComponent::DiagnoseRequest diagnoseRequest; 
        diagnoseRequest.diagnoseContent = proto.diagnosecontent(); 
        return diagnoseRequest;         
    }

*/

}

namespace CommsComponent{

        CommsMsg CommsMsg::fromString(const std::string & serializedString)
        {
            CommsMsgProto::CommsMsg protoMsg; 
            if (!protoMsg.ParseFromString(serializedString))
            {
                throw InvalidCommsMsgException("failed");
            }
            if (!protoMsg.has_id())
            {
                throw InvalidCommsMsgException("failed");
            }

            CommsMsg commsMsg; 
            commsMsg.id = protoMsg.id(); 
            if (protoMsg.has_httprequest())
            {
                commsMsg.content = from(protoMsg.httprequest());

            }
            if(protoMsg.has_httpresponse())
            {
                commsMsg.content = from(protoMsg.httpresponse());

            }
            //TODO LINUXDAR-1956 the below is skeleton code for this ticket
            // if (protoMsg.has_diagnoserequest())
            // {
            //     commsMsg.content = from(protoMsg.diagnoserequest()); 
            // }
            // else if( protoMsg.has_telemetryrequest())
            // {
            //     commsMsg.content = from(protoMsg.telemetryrequest()); 
            // } 
            // else
            // {
            //     throw std::logic_error("Developer forgot to implement one of the fields");
            // }
            return commsMsg; 
        }

        std::string CommsMsg::serialize(const CommsMsg& commsMsg)
        {
            CommsMsgProto::CommsMsg protoMsg; 
            protoMsg.set_id(commsMsg.id); 
            
            CommsMsgVisitorSerializer visitor{protoMsg}; 

            std::visit(visitor, commsMsg.content); 

            return protoMsg.SerializeAsString();  
        }

}