/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommsDistributor.h"
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
        m_messageChannel(messageChannel),
        m_done(false)
    {}


    // request_<id>.json and request_<id>_body

    void CommsDistributor::handleResponses()
    {
        while (!m_done)
        {
            std::string incoming_message;
            m_messageChannel.pop(incoming_message);
            LOGINFO("We got a response: " << incoming_message);
        }
    }

    void CommsDistributor::handleRequests()
    {
        while (!m_done)
        {
            std::optional<std::string> request_id_filename = m_monitorDir.next(std::chrono::milliseconds(10));
            if (request_id_filename.has_value() && Common::UtilityImpl::StringUtils::startswith(request_id_filename.value(), "request_"))
            {
//                m_messageChannel.push("We got a request : " << request_id_filename)
                LOGINFO("We got a request : " << request_id_filename.value());
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
