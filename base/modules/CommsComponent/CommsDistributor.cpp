/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommsDistributor.h"
#include "CommsMsg.h"
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/FileSystem/IFileSystemException.h>

#include "Logger.h"

namespace CommsComponent
{

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
                std::string incomingMessage;
                m_messageChannel.pop(incomingMessage);
                LOGDEBUG("Received response of length: " << incomingMessage.size());
                if (!incomingMessage.empty()) {
                    forwardResponse(incomingMessage);
                }
                else
                {
                    LOGDEBUG("Received an empty message");
                }
            }
        }
        catch (ChannelClosedException& exception)
        {
            LOGDEBUG("Exiting Response Handler Loop");
        }
    }

    void CommsDistributor::handleRequests()
    {
        try
        {
            while (true)
            {
                std::optional<std::string> requestIdOptionalFilename = m_monitorDir.next(std::chrono::milliseconds(-1));

                if (requestIdOptionalFilename.has_value())
                {


                    std::string requestIdBasename = Common::FileSystem::basename(requestIdOptionalFilename.value());
                    LOGINFO("Received a request file with value: " << requestIdOptionalFilename.value());
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
        // TODO - To be completed in LINUXDAR-1954.
        //  Also remove line below as it exists only to avoid potential warning about unused variables
        (void)responseId;
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
        Path requestIdFilePath;
        Path requestBodyFilePath;

        size_t nameSize = requestBaseName.size();
        size_t idLength = nameSize - (m_leadingRequestFileNameString.size() + m_trailingRequestJsonString.size());
        std::string id = requestBaseName.substr(m_leadingRequestFileNameString.size(), idLength);
        std::stringstream requestBodyFileName;
        requestBodyFileName << m_leadingRequestFileNameString << id << m_trailingRequestBodyString;

        requestIdFilePath = Common::FileSystem::join(m_monitorDirPath, requestBaseName);
        requestBodyFilePath = Common::FileSystem::join(m_monitorDirPath, requestBodyFileName.str());

        try {
            if (Common::UtilityImpl::StringUtils::startswith(requestBaseName, m_leadingRequestFileNameString) &&
                Common::UtilityImpl::StringUtils::endswith(requestBaseName, m_trailingRequestJsonString))
            {
                LOGINFO("Received a request: " << requestBaseName);

                std::string requestFileContents = m_fileSystem->readFile(requestIdFilePath);
                std::string bodyFileContents = m_fileSystem->readFile(requestBodyFilePath);

                Common::HttpSender::RequestConfig requestConfig = CommsComponent::CommsMsg::requestConfigFromJson(
                        requestFileContents);
                requestConfig.setData(bodyFileContents);
                CommsMsg msg;
                msg.content = requestConfig;
                msg.id = id;

                std::string serializedRequestMessage = CommsComponent::CommsMsg::serialize(msg);
                m_childProxy.pushMessage(serializedRequestMessage);
            }
            else
            {
                LOGDEBUG("Received request: " << requestBaseName << ", that did not match expected format. Discarding.");
            }
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to forward request: " << requestBaseName << ", Reason: " << exception.what());
        }

        std::array<Path,2> filesToCleanUp = {requestIdFilePath, requestBodyFilePath};
        for(int i = 0; i < 2; i++)
        {
            try
            {
                if (m_fileSystem->isFile(filesToCleanUp[i]))
                {
                    LOGDEBUG("Deleting: " << filesToCleanUp[i]);
                    m_fileSystem->removeFile(filesToCleanUp[i]);
                }
            }
            catch (Common::FileSystem::IFileSystemException& exception)
            {
                LOGWARN("Failed to delete: " << filesToCleanUp[i] << ", Reason: " << exception.what());
            }
        }
    }
} // namespace CommsComponent
