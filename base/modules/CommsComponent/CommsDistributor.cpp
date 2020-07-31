/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CommsDistributor.h"
#include "CommsMsg.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFilePermissions.h>

#include <utility>
#include <Common/UtilityImpl/TimeUtils.h>

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

    const std::string CommsDistributor::RequestPrepender = "request_";
    const std::string CommsDistributor::ResponsePrepender = "response_";
    const std::string CommsDistributor::JsonAppender = ".json";
    const std::string CommsDistributor::BodyAppender = "_body";

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
                std::chrono::milliseconds oneHourInMilliseconds{3600000};
                std::optional<std::string> requestIdOptionalFilename = m_monitorDir.next(oneHourInMilliseconds);

                if (requestIdOptionalFilename.has_value())
                {
                    std::string requestIdBasename = Common::FileSystem::basename(requestIdOptionalFilename.value());
                    LOGINFO("Received a request file with value: " << requestIdOptionalFilename.value());
                    forwardRequest(requestIdBasename);
                }
                else
                {
                    clearFilesOlderThan1Hour();
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
                writeAndMoveWithGroupReadCapability(responsePath, responseJson);
            }
        }
        catch (InvalidCommsMsgException& ex)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to convert incoming comms response of length: " << incomingMessage.size() << " into CommsMsg, reason: " << ex.what();
            LOGERROR(errorMsg.str());
            createErrorResponseFile(errorMsg.str(), m_responseDirPath, responseId);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to create response file for response with id: " << responseId << ", reason: " << ex.what();
            LOGERROR(errorMsg.str());
            createErrorResponseFile(errorMsg.str(), m_responseDirPath, responseId);
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
        requestBodyFileName << RequestPrepender << id << BodyAppender;
        return requestBodyFileName.str();
    }



    std::string CommsDistributor::getExpectedRequestJsonBaseNameFromId(const std::string &id)
    {
        std::stringstream requestJsonFileName;
        requestJsonFileName << RequestPrepender << id << JsonAppender;
        return requestJsonFileName.str();
    }

    std::string CommsDistributor::getExpectedResponseJsonBaseNameFromId(const std::string &id)
    {
        std::stringstream responseJsonFileName;
        responseJsonFileName << ResponsePrepender << id << JsonAppender;
        return responseJsonFileName.str();
    }

    void CommsDistributor::forwardRequest(const std::string& requestBaseName)
    {
        Path requestJsonFilePath = Common::FileSystem::join(m_monitorDirPath, requestBaseName);
        std::string id = getIdFromRequestBaseName(requestBaseName, RequestPrepender, JsonAppender);
        std::string requestBodyBaseName = getExpectedRequestBodyBaseNameFromId(id);
        Path requestBodyFilePath = Common::FileSystem::join(m_monitorDirPath, requestBodyBaseName);
        try {
            if (Common::UtilityImpl::StringUtils::startswith(requestBaseName, RequestPrepender) &&
                Common::UtilityImpl::StringUtils::endswith(requestBaseName, JsonAppender))
            {
                LOGINFO("Received a request: " << requestBaseName);

                std::string requestFileContents = m_fileSystem->readFile(requestJsonFilePath);
                std::string bodyFileContents;
                if (m_fileSystem->exists(requestBodyFilePath))
                {
                    bodyFileContents = m_fileSystem->readFile(requestBodyFilePath);
                }


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
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to forward request: " << requestBaseName << ", Reason: " << exception.what();
            LOGERROR(errorMsg.str());
            createErrorResponseFile(errorMsg.str(), m_responseDirPath, id);
        }
        catch (std::runtime_error& exception)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to forward request: " << requestBaseName << ", Reason: " << exception.what();
            LOGERROR(errorMsg.str());
            createErrorResponseFile(errorMsg.str(), m_responseDirPath, id);
        }

        // attempt to clean up request json and body
        cleanupFile(requestJsonFilePath);
        cleanupFile(requestBodyFilePath);
    }

    void CommsDistributor::setupProxy()
    {
        try{
            CommsComponent::CommsConfig config;
            config.addProxyInfoToConfig();
            CommsMsg comms;
            comms.id = "ProxyConfig";
            comms.content = config;
            m_childProxy.pushMessage(CommsMsg::serialize(comms));
        }catch(std::exception & ex)
        {
            LOGERROR("Failed to configure proxy. Reason: " << ex.what());
        }
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

    void CommsDistributor::writeAndMoveWithGroupReadCapability(const std::string& path, const std::string & fileContent)
    {
        Common::FileSystem::fileSystem()->writeFileAtomically(path, fileContent,
                Common::ApplicationConfiguration::applicationPathManager().getTempPath(), 0640);
    }

    void CommsDistributor::createErrorResponseFile(std::string message, Path responseDir, std::string id)
    {
        std::stringstream fileBaseName;
        if (id.empty())
        {
            id = "unknownId";
        }
        fileBaseName << ResponsePrepender << id << "_error";
        Path responsePath = Common::FileSystem::join(responseDir, fileBaseName.str());
        LOGINFO("Creating error response file: " << responsePath);
        try
        {
            writeAndMoveWithGroupReadCapability(responsePath, message);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to create error response file: " << responsePath << ", reason: " << ex.what());
        }
    }

    void CommsDistributor::clearFilesOlderThan1Hour()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::vector<Path> outboundFiles = fileSystem->listFiles(Common::ApplicationConfiguration::applicationPathManager().getCommsRequestDirPath());
        std::vector<Path> inboundFiles = fileSystem->listFiles(Common::ApplicationConfiguration::applicationPathManager().getCommsResponseDirPath());
        std::vector<Path> combinedFiles = std::move(outboundFiles);

        combinedFiles.insert(combinedFiles.end(), inboundFiles.begin(), inboundFiles.end());
        std::time_t now = Common::UtilityImpl::TimeUtils::getCurrTime();
        for (size_t i = 0; i < combinedFiles.size(); i++)
        {
            std::time_t lastModifiedTime = fileSystem->lastModifiedTime(combinedFiles[0]);
            const std::time_t hour = 60*60;
            if ((lastModifiedTime + hour) < now)
            {
                fileSystem->removeFile(combinedFiles[i]);
            }
        }
    }


} // namespace CommsComponent
