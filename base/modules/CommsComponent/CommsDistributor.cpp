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

namespace
{
    class StopOnEndScope{
        CommsComponent::CommsDistributor & m_ref; 
        public: 
        StopOnEndScope(CommsComponent::CommsDistributor& ref): m_ref{ref}{}
        ~StopOnEndScope()
        {
            m_ref.stop(); 
        }
    };
}

namespace CommsComponent
{

    const std::string CommsDistributor::m_requestPrepender = "request_";
    const std::string CommsDistributor::m_responsePrepender = "response_";
    const std::string CommsDistributor::m_jsonAppender = ".json";
    const std::string CommsDistributor::m_bodyAppender = "_body";

    CommsDistributor::CommsDistributor(const std::string& dirPath, const std::string& positiveFilter, const std::string& responseDirPath, MessageChannel& messageChannel, IOtherSideApi& childProxy, bool withSupportForProxy) :
        m_monitorDir(dirPath,positiveFilter),
        m_monitorDirPath(dirPath),
        m_messageChannel(messageChannel),
        m_childProxy(childProxy),
        m_stopRequested{ATOMIC_FLAG_INIT},
        m_withSupportForProxy(withSupportForProxy),
        m_responseDirPath(responseDirPath)
        {}

    void CommsDistributor::handleResponses()
    {
        StopOnEndScope stopOnEnd{*this}; 
        try
        {
            while (true)
            {
                std::string incomingMessage;
                m_messageChannel.pop(incomingMessage);
                LOGDEBUG("Received response of length: " << incomingMessage.size());
                if (!incomingMessage.empty())
                {
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
        StopOnEndScope stopOnEnd{*this}; 
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
        auto future1 = std::async(std::launch::async, [this](){this->handleResponses();}); 
        auto future2 = std::async(std::launch::async, [this](){this->handleRequests();}); 

        try{
            future1.get(); 
        }catch(std::exception& ex)
        {
            LOGERROR("Failure in handling responses: " << ex.what()); 
        }
        try{
            future2.get(); 
        }catch(std::exception& ex)
        {
            LOGERROR("Failure in handling requests " << ex.what()); 
        }        
    }

    void CommsDistributor::stop()
    {
        if (!m_stopRequested.test_and_set())
        {
            m_childProxy.notifyOtherSideAndClose();
            m_monitorDir.stop();
            m_messageChannel.pushStop();
        }
    }

    void CommsDistributor::createResponseJsonFile(const std::string& jsonContent, const std::string& destination, const std::string& midPoint)
    {
        LOGDEBUG("Creating temporary file in: " << midPoint << ", then moving it to: " << destination);
        Common::FileSystem::fileSystem()->writeFileAtomically(destination, jsonContent, midPoint);
    }

    void CommsDistributor::forwardResponse(const std::string& incomingMessage)
    {
        std::string responseId;
        try
        {
            CommsMsg msg = CommsMsg::fromString(incomingMessage);
            responseId = msg.id;

            if (std::holds_alternative<Common::HttpSender::HttpResponse>(msg.content))
            {
                std::string responseJson = CommsMsg::toJson(std::get<Common::HttpSender::HttpResponse>(msg.content));
                std::string responseBasename = getExpectedResponseJsonBaseNameFromId(responseId);
                Path responsePath = Common::FileSystem::join(m_responseDirPath, responseBasename);
                LOGDEBUG("Writing response: " << responseId << " to " << responsePath);
                createResponseJsonFile(responseJson, responsePath, Common::ApplicationConfiguration::applicationPathManager().getTempPath());
            }
        }
        catch (InvalidCommsMsgException& ex)
        {
            LOGERROR("Failed to convert incoming comms response of length: " << incomingMessage.size() << " into CommsMsg, reason: " << ex.what());
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to create response file for response with id: " << responseId << ", reason: " << ex.what());
        }
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
        requestBodyFileName << m_requestPrepender << id << m_bodyAppender;
        return requestBodyFileName.str();
    }



    std::string CommsDistributor::getExpectedRequestJsonBaseNameFromId(const std::string &id)
    {
        std::stringstream requestJsonFileName;
        requestJsonFileName << m_requestPrepender << id << m_jsonAppender;
        return requestJsonFileName.str();
    }

    std::string CommsDistributor::getExpectedResponseJsonBaseNameFromId(const std::string &id)
    {
        std::stringstream responseJsonFileName;
        responseJsonFileName << m_responsePrepender << id << m_jsonAppender;
        return responseJsonFileName.str();
    }

    void CommsDistributor::forwardRequest(const std::string& requestBaseName)
    {
        Path requestJsonFilePath = Common::FileSystem::join(m_monitorDirPath, requestBaseName);
        std::string id = getIdFromRequestBaseName(requestBaseName, m_requestPrepender, m_jsonAppender);
        std::string requestBodyBaseName = getExpectedRequestBodyBaseNameFromId(id);
        Path requestBodyFilePath = Common::FileSystem::join(m_monitorDirPath, requestBodyBaseName);

        try {
            if (Common::UtilityImpl::StringUtils::startswith(requestBaseName, m_requestPrepender) &&
                Common::UtilityImpl::StringUtils::endswith(requestBaseName, m_jsonAppender))
            {
                LOGINFO("Received a request: " << requestBaseName);

                std::string requestFileContents = m_fileSystem->readFile(requestJsonFilePath);
                std::string bodyFileContents = m_fileSystem->readFile(requestBodyFilePath);

                std::string serializedRequestMessage = getSerializedRequest(requestFileContents, bodyFileContents, id);
                if ( m_withSupportForProxy)
                {
                    setupProxy(); 
                }
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
//        catch (InvalidCommsMsgException& exception)
//        {
//            LOGERROR("Couldn't convert request: " << requestBaseName << " into CommsMsg, Reason: " << exception.what());
//        }

        // attempt to clean up request json
        cleanupFile(requestJsonFilePath);
    }

    void CommsDistributor::setupProxy()
    {
        CommsComponent::CommsConfig config;
        config.addProxyInfoToConfig(); 
        CommsMsg comms;
        comms.id = "ProxyConfig";
        comms.content = config;
        m_childProxy.pushMessage(CommsMsg::serialize(comms));
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
