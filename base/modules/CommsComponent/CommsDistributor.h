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

    struct InboundFiles
    {
        std::string requestPath; 
        std::string bodyPath; 
    }; 
    class CommsDistributor
    {
    public:
        CommsDistributor(const std::string& path, const std::string& filter, const std::string& responseDirPath, MessageChannel& channel, IOtherSideApi& childProxy, bool withSupportForProxy=false);
        void handleRequestsAndResponses();
        void stop();
        static std::string getExpectedRequestBodyBaseNameFromId(const std::string &id);
        static std::string getExpectedRequestJsonBaseNameFromId(const std::string &id);
        static std::string getExpectedResponseJsonBaseNameFromId(const std::string &id);
        static std::string getIdFromRequestBaseName(const std::string& baseName, const std::string& prepender, const std::string& appender);
        static std::string getSerializedRequest(const std::string& requestFileContents, const std::string& bodyFileContents, std::string id);
        static InboundFiles getExpectedPath(const std::string & id);
        static void writeAndMoveWithGroupReadCapability(const std::string& path, const std::string & fileContent); 

        static const std::string RequestPrepender;
        static const std::string ResponsePrepender;
        static const std::string JsonAppender;
        static const std::string BodyAppender;

        static void createErrorResponseFile(std::string message, Path responseDir, std::string id);

        static void clearFilesOlderThan1Hour();
        
    private:

        // pick up files moved into response directory and forward the requests to the network component through m_messageChannel
        void handleRequests();

        // pick up responses sent from the network component through m_messageChannel and create files for them in response directory
        void handleResponses();

        void cleanupFile(const Path& filePath);
        virtual void forwardRequest(const std::string& requestFilename);

        virtual void forwardResponse(const std::string& incomingMessage);
        void setupProxy();

        MonitorDir m_monitorDir;
        std::string m_monitorDirPath;
        MessageChannel& m_messageChannel;
        IOtherSideApi& m_childProxy;
        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
        std::atomic_flag m_stopRequested;
        bool m_withSupportForProxy; 
        std::string m_responseDirPath;
    };
} // namespace CommsComponent

