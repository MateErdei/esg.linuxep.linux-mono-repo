/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommsDistributor.h"
#include "CommsMsg.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/FileSystem/IFileSystemException.h>

#include <utility>

#include "Logger.h"

namespace CommsComponent
{

    const std::string CommsDistributor::m_leadingRequestFileNameString = "request_";
    const std::string CommsDistributor::m_trailingRequestJsonString = ".json";
    const std::string CommsDistributor::m_trailingRequestBodyString = "_body";

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
    /*
     * handleRequests requires body files to be created in the monitored directory prior to the request json being
     * moved in to successfully pick up both together
     */
    {
        try
        {
            while (true)
            {
                std::optional<std::string> requestIdOptionalFilename = m_monitorDir.next();

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

    std::string CommsDistributor::getSerializedRequest(const std::string& requestFileContents, const std::string& bodyFileContents, std::string id)
    {
        Common::HttpSender::RequestConfig requestConfig = CommsComponent::CommsMsg::requestConfigFromJson(
                requestFileContents);
        requestConfig.setData(bodyFileContents);
        CommsMsg msg;
        msg.content = requestConfig;
        msg.id = std::move(id);

        return CommsComponent::CommsMsg::serialize(msg);
    }

    std::string CommsDistributor::getIdFromRequestBaseName(
            const std::string& baseName,
            const std::string& prepender,
            const std::string& appender)
    {
        int nameSize = baseName.size();
        int prependerSize = prepender.size();
        int appenderSize = appender.size();
        int idLength = nameSize - (prependerSize + appenderSize);

        if (idLength <= 0)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to extract id between: " << prepender << " & " << appender << " from " << baseName;
            throw std::runtime_error(errorMsg.str());
        }
        return baseName.substr(prependerSize, idLength);
    }

    std::string CommsDistributor::getExpectedRequestBodyBaseNameFromId(const std::string &id)
    {
        std::stringstream requestBodyFileName;
        requestBodyFileName << m_leadingRequestFileNameString << id << m_trailingRequestBodyString;
        return requestBodyFileName.str();
    }

    std::string CommsDistributor::getExpectedRequestJsonBaseNameFromId(const std::string &id)
    {
        std::stringstream requestJsonFileName;
        requestJsonFileName << m_leadingRequestFileNameString << id << m_trailingRequestJsonString;
        return requestJsonFileName.str();
    }

    void CommsDistributor::forwardRequest(const std::string& requestBaseName)
    {
        Path requestJsonFilePath = Common::FileSystem::join(m_monitorDirPath, requestBaseName);
        std::string id = getIdFromRequestBaseName(requestBaseName, m_leadingRequestFileNameString, m_trailingRequestJsonString);
        std::string requestBodyBaseName = getExpectedRequestBodyBaseNameFromId(id);
        Path requestBodyFilePath = Common::FileSystem::join(m_monitorDirPath, requestBodyBaseName);

        try {
            if (Common::UtilityImpl::StringUtils::startswith(requestBaseName, m_leadingRequestFileNameString) &&
                Common::UtilityImpl::StringUtils::endswith(requestBaseName, m_trailingRequestJsonString))
            {
                LOGINFO("Received a request: " << requestBaseName);

                std::string requestFileContents = m_fileSystem->readFile(requestJsonFilePath);
                std::string bodyFileContents = m_fileSystem->readFile(requestBodyFilePath);

                std::string serializedRequestMessage = getSerializedRequest(requestFileContents, bodyFileContents, id);
                m_childProxy.pushMessage(serializedRequestMessage);
            }
            else
            {
                LOGDEBUG("Received request: " << requestBaseName << ", that did not match expected format. Discarding.");
            }
            // attempt to clean up request body here because we cannot guarantee we have calculated it at the end of this function
            cleanupFile(requestBodyFilePath);
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to forward request: " << requestBaseName << ", Reason: " << exception.what());
        }
        catch (std::runtime_error& exception)
        {
            LOGERROR("Failed to forward request: " << requestBaseName << ", Reason: " << exception.what());
        }

        // attempt to clean up request json
        cleanupFile(requestJsonFilePath);
    }

    void CommsDistributor::cleanupFile(const Path& filePath)
    {
        try
        {
            if (m_fileSystem->isFile(filePath))
            {
                LOGDEBUG("Deleting: " << filePath);
                m_fileSystem->removeFile(filePath);
            }
            else
            {
                LOGWARN("Could not remove: " << filePath << ", file does not exist");
            }
        }
        catch(Common::FileSystem::IFileSystemException& exception)
        {
            LOGWARN("Failed to clean up file: " << filePath << ", reason: " << exception.what());
        }
    }

    InboundFiles CommsDistributor::getExpectedPath(const std::string& id)
    {
        Path expectedRequestDir = Common::ApplicationConfiguration::applicationPathManager().getCommsRequestDirPath();
        std::string expectedRequestJsonName = getExpectedRequestJsonBaseNameFromId(id);
        std::string expectedRequestBodyName = getExpectedRequestBodyBaseNameFromId(id);
        Path expectedRequestJsonPath = Common::FileSystem::join(expectedRequestDir, expectedRequestJsonName);
        Path expectedRequestBodyPath = Common::FileSystem::join(expectedRequestDir, expectedRequestBodyName);
        return InboundFiles{expectedRequestJsonPath, expectedRequestBodyPath};
    }


} // namespace CommsComponent
