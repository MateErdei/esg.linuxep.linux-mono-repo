/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CommsMsg.h"
#include <CommsMsg.pb.h>
#include <Common/ProtobufUtil/MessageUtility.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <google/protobuf/util/json_util.h>

#pragma GCC diagnostic pop

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
        requestConfig.setRequestTypeFromString(requestProto.requesttype());
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


        std::string CommsMsg::toJson(const Common::HttpSender::RequestConfig& requestConfig )
        {
            CommsMsgProto::CommsMsg protoMsg;                       
            CommsMsgVisitorSerializer visitor{protoMsg};
            visitor(requestConfig); 
            return Common::ProtobufUtil::MessageUtility::protoBuf2Json(protoMsg.httprequest()); 
        }

        std::string CommsMsg::toJson( const Common::HttpSender::HttpResponse& httpResponse   )
        {
            CommsMsgProto::CommsMsg protoMsg;                       
            CommsMsgVisitorSerializer visitor{protoMsg};
            visitor(httpResponse); 
            return Common::ProtobufUtil::MessageUtility::protoBuf2Json(protoMsg.httpresponse());
        }

        Common::HttpSender::RequestConfig CommsMsg::requestConfigFromJson( const std::string & jsonContent)
        {
            CommsMsgProto::HttpRequest protoHttpRequest;
            google::protobuf::util::JsonParseOptions jsonParseOptions;
            jsonParseOptions.ignore_unknown_fields = true; 

            auto status = google::protobuf::util::JsonStringToMessage(jsonContent, &protoHttpRequest, jsonParseOptions);

            if (!status.ok())
            {
                throw InvalidCommsMsgException(std::string("Failed to load json string: ") + status.ToString());
            }
            return from(protoHttpRequest); 
        } 
        Common::HttpSender::HttpResponse CommsMsg::httpResponseFromJson( const std::string & jsonContent)
        {
            CommsMsgProto::HttpResponse protoHttpResponse;
            google::protobuf::util::JsonParseOptions jsonParseOptions;
            jsonParseOptions.ignore_unknown_fields = true; 

            auto status = google::protobuf::util::JsonStringToMessage(jsonContent, &protoHttpResponse, jsonParseOptions);

            if (!status.ok())
            {
                throw InvalidCommsMsgException(std::string("Failed to load json string: ") + status.ToString());
            }
            return from(protoHttpResponse); 
        }
}