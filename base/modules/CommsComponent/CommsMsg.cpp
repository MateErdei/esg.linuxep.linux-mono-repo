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
            proto->set_certpath(requestConfig.getCertPath());
            proto->set_requesttype(requestConfig.getRequestTypeAsString());
            proto->set_port(requestConfig.getPort());

            for(const auto& header : requestConfig.getAdditionalHeaders())
            {
                proto->add_headers(header);
            }
        }
        void operator()(const CommsComponent::CommsConfig& config)
        {
            auto proto = m_msg.mutable_config();
            std::map<std::string,std::string> key_value = config.getKeyValue();
            for(auto [key,value] : key_value){
                CommsMsgProto::KeyValue keyvalue;
                keyvalue.add_value(value);
                keyvalue.set_key(key);

                proto->add_keyvalue(keyvalue);
            }

        }
    };

    Common::HttpSender::RequestConfig from(const CommsMsgProto::HttpRequest& requestProto)
    {
        Common::HttpSender::RequestConfig requestConfig;
        requestConfig.setRequestTypeFromString(requestProto.requesttype());
        requestConfig.setServer(requestProto.server());
        requestConfig.setResourcePath(requestProto.resourcepath());
        requestConfig.setCertPath(requestProto.certpath());
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