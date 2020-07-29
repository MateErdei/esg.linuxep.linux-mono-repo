/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "HttpRequester.h"
#include "CommsMsg.h"
#include "Configurator.h"
#include "CommsDistributor.h"
#include "MonitorDir.h"
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/TimeUtils.h>

#include "Logger.h"

namespace
{
    static int COUNTER=0;
} // namespace

namespace CommsComponent
{
    Common::HttpSender::HttpResponse
    HttpRequester::triggerRequest(const std::string &requesterName, Common::HttpSender::RequestConfig &&request,
                                  std::string &&body, std::chrono::milliseconds timeout)
    {
        LOGDEBUG("Attempting to trigger request on behalf of " << requesterName);
        Common::HttpSender::HttpResponse response;
        std::string id = generateId(requesterName);
        try
        {

            auto expectedPaths = CommsDistributor::getExpectedPath(id);
            if (!body.empty())
            {
                Common::FileSystem::fileSystem()->writeFile(expectedPaths.bodyPath, body);
            }

            LOGDEBUG("Creating monitor dir watching: " << Common::ApplicationConfiguration::applicationPathManager().getCommsResponseDirPath() << " with filter: " << id);
            MonitorDir monitorDir{Common::ApplicationConfiguration::applicationPathManager().getCommsResponseDirPath(), id};

            Common::FileSystem::fileSystem()->writeFileAtomically(expectedPaths.requestPath,  CommsMsg::toJson(request),
                                                                  Common::ApplicationConfiguration::applicationPathManager().getTempPath());


            std::optional<std::string> responseFilePath;
            try
            {
                LOGSUPPORT("Beginning to monitor response directory: " << Common::ApplicationConfiguration::applicationPathManager().getCommsResponseDirPath());
                responseFilePath = monitorDir.next(timeout);
                if (!responseFilePath.has_value())
                {
                    std::stringstream errorMsg;
                    errorMsg << "Timed out while waiting for file with filter: " << id;
                    throw HttpRequesterException(errorMsg.str());
                }
                LOGDEBUG("Detected response file: " << responseFilePath.value());
            }
            catch (MonitorDirClosedException& exception)
            {
                LOGINFO("Directory monitor closed while waiting for response with id: " << id);
            }

            if (responseFilePath.has_value())
            {
                std::string fileContent = Common::FileSystem::fileSystem()->readFile(responseFilePath.value());
                response = CommsMsg::httpResponseFromJson(fileContent);
            }

            return response;
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to perform request: " << id << ", reason: " << ex.what());
            throw;
        }
    }

    std::string HttpRequester::generateId(const std::string & requesterName)
    {
        std::stringstream id;

        Common::UtilityImpl::FormattedTime time;
        std::string now = time.currentEpochTimeInSeconds();

        id << requesterName << "_" << now << "_"  << COUNTER;
        COUNTER += 1;

        return id.str();
    }


}