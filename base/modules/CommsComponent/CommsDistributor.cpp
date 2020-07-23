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

    CommsDistributor::CommsDistributor(const std::string& dirPath, const std::string& positiveFilter, MessageChannel& messageChannel) :
        m_monitorDir(dirPath,positiveFilter),
        m_monitorDirPath(dirPath),
        m_messageChannel(messageChannel),
        m_done(false)

    {}




    void CommsDistributor::handleResponses()
    {
        while (!m_done)
        {
            std::string incoming_message;
            m_messageChannel.pop(incoming_message, std::chrono::milliseconds(10));
            if (!incoming_message.empty())
            {
                LOGINFO("We got a response: " << incoming_message);
            }
        }
    }

    // request_<id>.json and request_<id>_body

    void CommsDistributor::handleRequests()
    {
        while (!m_done)
        {
            std::optional<std::string> request_id_optional_filename = m_monitorDir.next(std::chrono::milliseconds(10));

            if (request_id_optional_filename.has_value())
            {
                std::string requestIdFilename = request_id_optional_filename.value();
                if (Common::UtilityImpl::StringUtils::startswith(requestIdFilename, "request_") &&
                    Common::UtilityImpl::StringUtils::endswith(requestIdFilename, ".json"))
                {
                    LOGINFO("We got a request : " << requestIdFilename);
                    int nameSize = requestIdFilename.size();
                    std::string id = requestIdFilename.substr(8,nameSize-(9+5));
                    std::stringstream requestBodyFileName;
                    requestBodyFileName << "request_" << id << "_body";

                    Path requestIdFilePath = Common::FileSystem::join(m_monitorDirPath, requestIdFilename);
                    Path requestBodyFilePath = Common::FileSystem::join(m_monitorDirPath, requestBodyFileName.str());


                    std::string requestFileContents = m_fileSystem->readFile(requestIdFilePath);
                    std::string bodyFileContents = m_fileSystem->readFile(requestBodyFilePath);

                    Common::HttpSender::RequestConfig requestConfig = CommsComponent::CommsMsg::requestConfigFromJson(requestFileContents);
                    requestConfig.setData(bodyFileContents);
                    CommsMsg msg;
                    msg.content = requestConfig;
                    msg.id = id;

                    std::string serializedRequestMessage = CommsComponent::CommsMsg::serialize(msg);
                    //m_messageChannel.push("We got a request : " << requestIdFilename)
                }
            }

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
        m_done = true;
    }
} // namespace CommsComponent
