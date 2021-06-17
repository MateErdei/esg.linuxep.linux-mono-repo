/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PluginUtils.h"
#include "Logger.h"

#include <Common/HttpSender/RequestConfig.h>
#include <CommsComponent/HttpRequester.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/FileSystem/IFileSystemException.h>

namespace RemoteDiagnoseImpl
{
    std::string PluginUtils::processAction(const std::string& actionXml) {
        LOGDEBUG("Processing action: " << actionXml);
        Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(actionXml);

        auto action = attributesMap.lookup("a:action");
        std::string actionType(action.value("type"));
        if (actionType != "SDURun") {
            std::stringstream errorMessage;
            errorMessage << "Malformed action received , type is : " << actionType << " not SDURun";
            throw std::runtime_error(errorMessage.str());
        }
        std::string url = action.value("uploadUrl");
        LOGDEBUG("Upload url: " << url);
        return url;}

    void PluginUtils::processZip(const std::string& url)
    {
        std::string output = Common::ApplicationConfiguration::applicationPathManager().getDiagnoseOutputPath();
        auto fs = Common::FileSystem::fileSystem();
        std::string filepath = Common::FileSystem::join(output,"sspl.zip");

        UrlData data = processUrl(url);
        std::string processedfilepath = Common::FileSystem::join(output, data.filename);

        try
        {
            fs->moveFile(filepath, processedfilepath);
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("failed to process zip file due to error: " << exception.what());
        }
        std::string chrootPath = Common::FileSystem::join("/base/remote-diagnose/output", data.filename);

        Common::HttpSender::RequestConfig requestConfig{Common::HttpSender::RequestType::PUT,
                                                        std::vector<std::string>{},data.domain,
                                                        443,"",data.resourcePath,
                                                        chrootPath};

        try
        {
            auto response = CommsComponent::HttpRequester::triggerRequest("UploadDiagnose", requestConfig, std::chrono::minutes(1));

            if (response.httpCode != 200)
            {
                LOGINFO("Response HttpCode: " << response.httpCode);
                LOGINFO(response.description);

            }

        }
        catch (std::exception& ex)
        {
            LOGERROR("Perform request failed: " << ex.what());

        }
        LOGINFO("Diagnose finished");
    }

    PluginUtils::UrlData PluginUtils::processUrl(const std::string& url)
    {
        UrlData data;
        data.filename = Common::FileSystem::basename(url);
        if (!Common::UtilityImpl::StringUtils::startswith(url,"https://"))
        {
            throw std::runtime_error("Malformed url missing protocol");
        }
        std::string noProtocol = url.substr(8);
        data.resourcePath = noProtocol.substr(noProtocol.find_first_of("/")+1);// plus one so we dont include the first slash
        data.domain = noProtocol.substr(0,noProtocol.find_first_of("/"));

        return data;
    }

    std::string PluginUtils::getFinishedStatus()
    {
        std::string statusTemplate {R"sophos(<?xml version="1.0" encoding="utf-8" ?>
    <status version="@VERSION@" is_running="@IS_RUNNING@" />)sophos" };
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::string versionFile = Common::ApplicationConfiguration::applicationPathManager().getVersionIniFileForComponent(
                "ServerProtectionLinux-Base-component");
        std::string version = Common::UtilityImpl::StringUtils::extractValueFromIniFile(versionFile, "PRODUCT_VERSION");

        std::string newStatus = Common::UtilityImpl::StringUtils::replaceAll(statusTemplate, "@VERSION@", version);
        newStatus = Common::UtilityImpl::StringUtils::replaceAll(newStatus,"@IS_RUNNING@","0");
        return newStatus;

    }

}