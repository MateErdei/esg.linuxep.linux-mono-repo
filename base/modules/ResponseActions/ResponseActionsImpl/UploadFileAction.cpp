// Copyright 2023 Sophos Limited. All rights reserved.

#include "UploadFileAction.h"
#include "ActionStructs.h"
#include "ActionsUtils.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <json.hpp>
namespace ResponseActionsImpl
{
    std::string UploadFileAction::run(const std::string& actionJson) const
    {
        nlohmann::json response;
        response["type"] = "sophos.mgt.response.UploadFile";
        ActionsUtils actionsUtils;
        UploadInfo info;

        try
        {
            info = actionsUtils.readUploadAction(actionJson, UploadType::FILE);
        }
        catch (const std::runtime_error& exception)
        {
            LOGWARN(exception.what());
            actionsUtils.setErrorInfo(response,1,"invalid_path","Error parsing command from Central");
            return response.dump();
        }

        if (actionsUtils.isExpired(info.expiration))
        {
            std::string error = "Action has expired";
            LOGWARN(error);
            actionsUtils.setErrorInfo(response,4,error);
            return response.dump();
        }

        auto fs = Common::FileSystem::fileSystem();
        if (!fs->isFile(info.targetPath))
        {
            std::string error = info.targetPath + " is not a file";
            LOGWARN(error);
            actionsUtils.setErrorInfo(response,1,error,"invalid_path");
            return response.dump();
        }

        response["sizeBytes"] = fs->fileSize(info.targetPath);
        if (response["sizeBytes"] > info.maxSize)
        {
            std::stringstream error;
            error << info.targetPath + " is above the size limit" << info.maxSize;
            LOGWARN(error.str());
            actionsUtils.setErrorInfo(response,1,error.str(),"exceed_size_limit");
            return response.dump();
        }

        return response.dump();
    }
}