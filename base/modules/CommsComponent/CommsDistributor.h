/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "MessageChannel.h"
#include "OtherSideApi.h"
#include "MonitorDir.h"

#include <Common/FileSystem/IFileSystem.h>

namespace CommsComponent
{
    class CommsDistributor
    {
    public:
        CommsDistributor(const std::string& path, const std::string& filter, const std::string& responseDirPath, MessageChannel& channel, IOtherSideApi& childProxy);
        void handleRequestsAndResponses();
        void stop();
    private:


        // pick up files moved into response directory and forward the requests to the network component through m_messageChannel
        void handleRequests();

        // pick up responses sent from the network component through m_messageChannel and create files for them in response directory
        void handleResponses();

        virtual void forwardRequest(const std::string& requestFilename);

        virtual void forwardResponse(const std::string& incomingMessage);

        MonitorDir m_monitorDir;
        std::string m_monitorDirPath;
        std::string m_responseDirPath;
        MessageChannel& m_messageChannel;
        IOtherSideApi& m_childProxy;
        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
        const std::string m_leadingRequestFileNameString = "request_";
        const std::string m_trailingRequestJsonString = ".json";
        const std::string m_trailingRequestBodyString = "_body";


    };
} // namespace CommsComponent

