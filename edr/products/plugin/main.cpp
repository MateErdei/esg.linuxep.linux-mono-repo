/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "config.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IPidLockFileUtils.h>
#include <Common/Logging/PluginLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/ErrorCodes.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/IPluginResourceManagement.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/Logger.h>
#include <modules/pluginimpl/OsqueryProcessImpl.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <modules/livequery/ResponseDispatcher.h>


std::string g_pluginName = PLUGIN_NAME;

class FakeQueryProcessor: public livequery::IQueryProcessor
{
public:
     livequery::QueryResponse query(const std::string & /*query*/) override
     {
         livequery::ResponseStatus status{ livequery::ErrorCode::SUCCESS};
         livequery::ResponseMetaData metaData;
         livequery::ResponseData::ColumnHeaders headers;
         headers.emplace_back("first", livequery::ResponseData::AcceptedTypes::STRING);
         headers.emplace_back("second", livequery::ResponseData::AcceptedTypes::BIGINT);
         livequery::ResponseData::ColumnData  columnData;
         livequery::ResponseData::RowData  rowData;
         rowData["first"] = "first1";
         rowData["second"] = "1";
         columnData.push_back(rowData);
         rowData["first"] = "first2";
         rowData["second"] = "2";
         columnData.push_back(rowData);

         livequery::QueryResponse response{status, livequery::ResponseData{headers,columnData}};
         response.setMetaData(metaData);
         return response;
     }
};

int main()
{
    using namespace Plugin;
    int ret = 0;
    Common::Logging::PluginLoggingSetup loggerSetup(g_pluginName);

    std::unique_ptr<Common::FileSystem::ILockFileHolder> lockFile;
    try
    {
        lockFile = Common::FileSystem::acquireLockFile(lockFilePath());
    }
    catch( std::system_error & ex)
    {
        LOGERROR( ex.what());
        LOGERROR("Only one instance of EDR can run.");
        return ex.code().value();
    }

    std::unique_ptr<Common::PluginApi::IPluginResourceManagement> resourceManagement =
        Common::PluginApi::createPluginResourceManagement();

    auto queueTask = std::make_shared<QueueTask>();
    auto sharedPluginCallBack = std::make_shared<PluginCallback>(queueTask);

    std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService;

    try
    {
        baseService = resourceManagement->createPluginAPI(g_pluginName, sharedPluginCallBack);
    }
    catch (const Common::PluginApi::ApiException & apiException)
    {
        LOGERROR("Plugin Api could not be instantiated: " << apiException.what());
        return Common::PluginApi::ErrorCodes::PLUGIN_API_CREATION_FAILED;
    }
    std::unique_ptr<livequery::IQueryProcessor> queryProcessor(new FakeQueryProcessor{});
    std::unique_ptr<livequery::IResponseDispatcher> queryResponder(new livequery::ResponseDispatcher{});

    PluginAdapter pluginAdapter(queueTask, std::move(baseService), sharedPluginCallBack,
                                std::move(queryProcessor), std::move(queryResponder));

    try
    {
        pluginAdapter.mainLoop();
    }
    catch (const std::exception& ex)
    {
        LOGERROR("Plugin threw an exception at top level: " << ex.what());
        ret = 40;
    }
    LOGINFO("Plugin Finished.");
    return ret;
}
