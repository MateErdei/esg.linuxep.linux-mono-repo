/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommsDistributor.h"
#include "CommsMsg.h"
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/StringUtils.h>

#include "Logger.h"

namespace CommsComponent
{
//    void CommsDistributor::operator()(const std::shared_ptr<MessageChannel>& channel, OtherSideApi &parentProxy)
//    {
////        run_thread_that_moves_data_from_the_MessageChannel_to_response(channel);
////        auto mesagequeue = createMessagequeue();
////        run_thread_that_move_request_from_the_queue_to_other_side(mesagequeue, parentProxy);
//        // if shutdown received, close the two threads
//
//
//
//    }

    CommsDistributor::CommsDistributor(const std::string& dirPath, const std::string& positiveFilter, const std::string& responseDirPath, MessageChannel& messageChannel, IOtherSideApi& childProxy) :
        m_monitorDir(dirPath,positiveFilter),
        m_monitorDirPath(dirPath),
        m_responseDirPath(responseDirPath),
        m_messageChannel(messageChannel),
        m_childProxy(childProxy) {}




    void CommsDistributor::handleResponses()
    {
        try
        {
            while (true) {
                std::string incoming_message;
                m_messageChannel.pop(incoming_message);
                LOGDEBUG("Got response" << incoming_message);
                if (!incoming_message.empty()) {
                    forwardResponse(incoming_message);
                }
            }
        }
        catch (ChannelClosedException& exception)
        {
            LOGDEBUG("Exiting Response Handler Loop");
        }
    }

    // request_<id>.json and request_<id>_body

    void CommsDistributor::handleRequests()
    {
        try
        {
            while (true)
            {
                std::optional<std::string> request_id_optional_filename = m_monitorDir.next(std::chrono::milliseconds(-1));

                if (request_id_optional_filename.has_value())
                {


                    std::string requestIdBasename = Common::FileSystem::basename(request_id_optional_filename.value());
                    LOGINFO("Got a file with value: " << request_id_optional_filename.value());
                    forwardRequest(requestIdBasename);
                }

            }
        }
        catch (MonitorDirClosedException& exception)
        {
            LOGDEBUG("Exiting Request Handler Loop");
        }
    }

    void CommsDistributor::handleRequestsAndResponses()
    {
        std::thread responseThread(&CommsDistributor::handleResponses, this);
        std::thread requestThread(&CommsDistributor::handleRequests, this);

        requestThread.join();
        responseThread.join();

    }

    void CommsDistributor::stop()
    {
        m_childProxy.notifyOtherSideAndClose();
        m_monitorDir.stop();
        m_messageChannel.pushStop();
    }

    void CommsDistributor::forwardResponse(const std::string& incomingMessage)
    {
        CommsMsg msg = CommsMsg::fromString(incomingMessage);
        std::string responseId = msg.id;
        // TODO - To be completed in LINUXDAR-1954
//        if (std::holds_alternative<Common::HttpSender::HttpResponse>(msg.content))
//        {
//            if (!std::get<Common::HttpSender::HttpResponse>(msg.content).bodyContent.empty())
//            {
//                std::stringstream responseBodyBasename;
//                responseBodyBasename << "response_" << responseId << "_body";
//                Path responseBodyPath = Common::FileSystem::join(m_responseDirPath, responseBodyBasename.str());
//                m_fileSystem->writeFile(responseBodyPath, std::get<Common::HttpSender::HttpResponse>(msg.content).bodyContent);
//            }
//            std::string json = CommsMsg::toJson(std::get<Common::HttpSender::HttpResponse>(msg.content));
//            std::stringstream responseBasename;
//            responseBasename << "response_" << responseId << ".json";
//            Path responsePath = Common::FileSystem::join(m_responseDirPath, responseBasename.str());
//            m_fileSystem->writeFile(responsePath, json);


//            LOGINFO(json);
//        }
    }

    void CommsDistributor::forwardRequest(const std::string& requestBaseName)
    {
        if (Common::UtilityImpl::StringUtils::startswith(requestBaseName, m_leadingRequestIdString) &&
            Common::UtilityImpl::StringUtils::endswith(requestBaseName, m_trailingRequestIdString))
        {
            LOGINFO("We got a request : " << requestBaseName);
            int nameSize = requestBaseName.size();
            std::string id = requestBaseName.substr(m_leadingRequestIdString.size(), nameSize - (m_leadingRequestIdString.size() + m_trailingRequestIdString.size()));
            std::stringstream requestBodyFileName;
            requestBodyFileName << m_leadingRequestBodyString << id << m_trailingRequestBodyString;

            Path requestIdFilePath = Common::FileSystem::join(m_monitorDirPath, requestBaseName);
            Path requestBodyFilePath = Common::FileSystem::join(m_monitorDirPath, requestBodyFileName.str());


            std::string requestFileContents = m_fileSystem->readFile(requestIdFilePath);
            std::string bodyFileContents = m_fileSystem->readFile(requestBodyFilePath);

            Common::HttpSender::RequestConfig requestConfig = CommsComponent::CommsMsg::requestConfigFromJson(requestFileContents);
            requestConfig.setData(bodyFileContents);
            CommsMsg msg;
            msg.content = requestConfig;
            msg.id = id;

            std::string serializedRequestMessage = CommsComponent::CommsMsg::serialize(msg);
            m_childProxy.pushMessage(serializedRequestMessage);
            LOGDEBUG("Deleting: " << requestIdFilePath << " & " << requestIdFilePath);
            m_fileSystem->removeFile(requestIdFilePath);
            m_fileSystem->removeFile(requestBodyFilePath);

        }
    }
} // namespace CommsComponent
