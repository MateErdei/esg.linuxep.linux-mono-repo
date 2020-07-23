/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "MessageChannel.h"
#include "OtherSideApi.h"
#include "MonitorDir.h"

namespace CommsComponent
{
    class CommsDistributor
    {
    public:
        CommsDistributor(const std::string& path, const std::string& filter, MessageChannel& channel);
        void handleRequestsAndResponses();
        void stop();
        //void operator()(const std::shared_ptr<MessageChannel>& channel, OtherSideApi &parentProxy);
    private:


        // pick up files moved into response directory and forward the requests to the network component through m_messageChannel
        void handleRequests();

        // pick up responses sent from the network component through m_messageChannel and create files for them in response directory
        void handleResponses();

        MonitorDir m_monitorDir;
        MessageChannel& m_messageChannel;
        bool m_done;

    };
} // namespace CommsComponent

