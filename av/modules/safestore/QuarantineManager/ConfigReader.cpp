// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ConfigReader.h"
#include "safestore/Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "common/ApplicationPaths.h"
#include <thirdparty/nlohmann-json/json.hpp>

void parseConfig(const std::unique_ptr<safestore::SafeStoreWrapper::ISafeStoreWrapper>& safeStore)
{
    auto fileSystem = Common::FileSystem::fileSystem();
    if (fileSystem->isFile(Plugin::getSafeStoreConfigPath()))
    {
        LOGINFO("Config file found, parsing optional arguments.");
        try
        {
            auto configContents = fileSystem->readFile(Plugin::getSafeStoreConfigPath());
            nlohmann::json j = nlohmann::json::parse(configContents);
            for (const auto& pair : optionsMap)
            {
                if (j.contains(pair.first) && j[pair.first].is_number_unsigned())
                {
                    LOGINFO("Setting config option: " << pair.first << " to: " << j[pair.first]);
                    safeStore->setConfigIntValue(pair.second, j[pair.first]);
                }
            }
        }
        catch (nlohmann::json::parse_error& e)
        {
            LOGERROR("Failed to parse SafeStore config json: " << e.what());
        }
    }
}
