/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosServerTable.h"
#include <iostream>
#include <redist/boost/include/boost/property_tree/ini_parser.hpp>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFileSystem.h>

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
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(mcsConfigPath, ptree);
                r["endpoint_id"]  = ptree.get<std::string>("MCSID") ;

            }
            catch (boost::property_tree::ptree_error& ex)
            {
                std::cerr << "Failed to read MCSID configuration from config file: " << mcsConfigPath << " with error: " << ex.what();
            }
        }

        if (fileSystem->isFile(mcsPolicyConfigPath))
        {
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(mcsPolicyConfigPath, ptree);
                r["customer_id"]  = ptree.get<std::string>("customerId") ;

            }
            catch (boost::property_tree::ptree_error& ex)
            {
                std::cerr << "Failed to read MCSID configuration from config file: " << mcsPolicyConfigPath << " with error: " << ex.what();
            }
        }
        results.push_back(std::move(r));
        return results;
    }

}

