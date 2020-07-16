/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CommsMsg.h"
#include <CommsMsg.pb.h>

namespace{
    using namespace CommsComponent;
    struct CommsMsgVisitorSerializer{
        CommsMsgProto::CommsMsg & m_msg; 
        CommsMsgVisitorSerializer(CommsMsgProto::CommsMsg& msg): m_msg(msg){}
        void operator()(const HttpResponse& httpResponse)
        {
            auto proto = m_msg.mutable_httpresponse(); 
            proto->set_httpcode(httpResponse.httpCode); 
            proto->set_description( httpResponse.description); 
            if (std::holds_alternative<CommsComponent::Body>(httpResponse.bodyOption))
            {
                proto->set_bodycontent( std::get<CommsComponent::Body>(httpResponse.bodyOption).body ); 
            }
            else
            {
                proto->set_bodyfilepath( std::get<CommsComponent::BodyFile>(httpResponse.bodyOption).path2File ); 
            }
        }
        void operator()(const HttpRequest& httpRequest)
        {
            auto proto = m_msg.mutable_httprequest();

            if (std::holds_alternative<CommsComponent::Body>(httpRequest.bodyOption))
            {
                proto->set_bodycontent( std::get<CommsComponent::Body>(httpRequest.bodyOption).body ); 
            }
            else
            {
                proto->set_bodyfilepath( std::get<CommsComponent::BodyFile>(httpRequest.bodyOption).path2File ); 
            }
            proto->set_url(httpRequest.url);
            proto->set_port(httpRequest.port); 
            if (httpRequest.method == HttpRequest::Method::Post)
            {
                proto->set_method(CommsMsgProto::HttpRequest_Method_POST);
            }
            else
            {
                proto->set_method(CommsMsgProto::HttpRequest_Method_GET);
            }

            proto->set_proxyallowed(httpRequest.proxyAllowed);

            proto->set_timetout(httpRequest.timeout); 
        }
    };

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

    NetworkResponse from(const CommsMsgProto::NetWorkResponse& proto)
    {
        CommsComponent::NetworkResponse networkResponse; 
        
        (void) proto;
        // FIXME
        return networkResponse; 
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

            // if (protoMsg.has_diagnoserequest())
            // {
            //     commsMsg.content = from(protoMsg.diagnoserequest()); 
            // }
            // else if( protoMsg.has_telemetryrequest())
            // {
            //     commsMsg.content = from(protoMsg.telemetryrequest()); 
            // } 
            // else if( protoMsg.has_networkresponse())
            // {
            //     commsMsg.content = from(protoMsg.networkresponse());
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