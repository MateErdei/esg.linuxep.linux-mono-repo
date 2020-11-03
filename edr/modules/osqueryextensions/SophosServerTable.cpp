/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosServerTable.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/FileUtils.h>

#include <iostream>

namespace OsquerySDK
{
    std::vector<TableColumn> SophosServerTable::GetColumns()
    {
        return
                {
                        OsquerySDK::TableColumn{"endpoint_id", TEXT_TYPE, ColumnOptions::DEFAULT},
                        OsquerySDK::TableColumn{"customer_id", TEXT_TYPE, ColumnOptions::DEFAULT}
                };
    }

    std::string SophosServerTable::GetName() { return "sophos_endpoint_info"; }

    TableRows SophosServerTable::Generate(QueryContextInterface& /*request*/)
    {
        TableRows results;
        TableRow r;
        r["endpoint_id"] = "";
        r["customer_id"] = "";
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        auto fileSystem = Common::FileSystem::fileSystem();

        std::string mcsConfigPath = Common::FileSystem::join(installPath,"base/etc/sophosspl/mcs.config");
        std::string mcsPolicyConfigPath = Common::FileSystem::join(installPath,"base/etc/sophosspl/mcs_policy.config");

        if (fileSystem->isFile(mcsConfigPath))
        {
            r["endpoint_id"] = Common::UtilityImpl::FileUtils::extractValueFromFile(mcsConfigPath,"MCSID");
            if (r["endpoint_id"].empty())
            {
                LOGWARN("Failed to read MCSID configuration from config file: " << mcsConfigPath << " with error: file doesn't contain key MCSID");
            }
        }
        else
        {
            LOGWARN("Failed to read MCSID configuration from config file: " << mcsConfigPath << " with error: file doesn't exist");
        }

        if (fileSystem->isFile(mcsPolicyConfigPath))
        {
            r["customer_id"] = Common::UtilityImpl::FileUtils::extractValueFromFile(mcsPolicyConfigPath,"customerId");
            if (r["customer_id"].empty())
            {
                LOGWARN("Failed to read customerID configuration from config file: " << mcsPolicyConfigPath << " with error: file doesn't contain key customerId");
            }
        }
        else
        {
            LOGWARN("Failed to read customerID configuration from config file: " << mcsPolicyConfigPath << " with error: file doesn't exist");
        }
        results.push_back(std::move(r));
        return results;
    }

}

