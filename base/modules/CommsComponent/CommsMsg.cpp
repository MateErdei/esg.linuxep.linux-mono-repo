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

            // if (std::holds_alternative<CommsComponent::Body>(httpRequest.bodyOption))
            // {
            //     proto->set_bodycontent( std::get<CommsComponent::Body>(httpRequest.bodyOption).body ); 
            // }
            // else
            // {
            //     proto->set_bodyfilepath( std::get<CommsComponent::BodyFile>(httpRequest.bodyOption).path2File ); 
            // }
            //proto->set_url(httpRequest.url);
            proto->set_port(requestConfig.getPort()); 
            // if (requestConfig.method == HttpRequest::Method::Post)
            // {
            //     proto->set_method(CommsMsgProto::HttpRequest_Method_POST);
            // }
            // else
            // {
            //     proto->set_method(CommsMsgProto::HttpRequest_Method_GET);
            // }

            // proto->set_proxyallowed(httpRequest.proxyAllowed);

            // proto->set_timetout(httpRequest.timeout);
            // for (auto & pairEntry : httpRequest.headers)
            // {
            //     CommsMsgProto::KeyValue keyValue;
            //     keyValue.set_key(pairEntry.first);
            //     keyValue.set_value(pairEntry.second);
            //     proto->mutable_headers()->Add(dynamic_cast<CommsMsgProto::KeyValue &&>(keyValue));
            // }
        }
    };

    Common::HttpSender::RequestConfig from(const CommsMsgProto::HttpRequest& requestProto)
    {

        Common::HttpSender::RequestConfig requestConfig;
        (void)requestProto;
        //httpRequest.url = requestProto.url();
        return requestConfig;
    }
    Common::HttpSender::HttpResponse from(const CommsMsgProto::HttpResponse& responseProto)
    {
        Common::HttpSender::HttpResponse httpResponse;
        (void)responseProto;
        //httpResponse.httpCode = responseProto.httpcode();
        return httpResponse;
    }

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
            if ( !protoMsg.ParseFromString(serializedString))
            {
                throw std::runtime_error("failed");
            }
            if (!protoMsg.has_id())
            {
                throw std::runtime_error("failed");                
            }

            CommsMsg commsMsg; 
            commsMsg.id = protoMsg.id(); 
            if (protoMsg.has_httprequest())
            {
                //commsMsg.content =from(protoMsg.httprequest());
                std::cout << "stuff";
            }
            else
                {
                std::cout << "stuff1";
                }
            if (!protoMsg.ParseFromString(serializedString))
            {
                commsMsg.content =from(protoMsg.httprequest());
            }
            if(protoMsg.has_httpresponse())
            {
                commsMsg.content = from(protoMsg.httpresponse());
            }
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