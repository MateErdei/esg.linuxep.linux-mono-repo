// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ConfigReader.h"

#include "safestore/Logger.h"
#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"

#include "common/ApplicationPaths.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

#include <thirdparty/nlohmann-json/json.hpp>

namespace safestore
{
    void parseConfig(safestore::SafeStoreWrapper::ISafeStoreWrapper& safeStore)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        if (fileSystem->isFile(Plugin::getSafeStoreConfigPath()))
        {
            LOGINFO("Config file found, parsing optional arguments.");
            try
            {
                auto configContents = fileSystem->readFile(Plugin::getSafeStoreConfigPath());
                nlohmann::json j = nlohmann::json::parse(configContents);
                for (const auto& [option, optionAsString] : safestore::SafeStoreWrapper::GL_OPTIONS_MAP)
                {
                    if (j.contains(optionAsString) && j[optionAsString].is_number_unsigned())
                    {
                        if (safeStore.setConfigIntValue(option, j[optionAsString]))
                        {
                            LOGINFO("Setting config option: " << optionAsString << " to: " << j[optionAsString]);
                        }
                        else
                        {
                            LOGWARN("Failed to set config option: " << optionAsString << " to: " << j[optionAsString]);
                        }
                    }
                }
            }
            catch (nlohmann::json::parse_error& e)
            {
                LOGERROR("Failed to parse SafeStore config json: " << e.what());
            }
            catch (Common::FileSystem::IFileSystemException& e)
            {
                LOGERROR("Failed to read SafeStore config json: " << e.what());
            }
        }
    }
} // namespace safestore
