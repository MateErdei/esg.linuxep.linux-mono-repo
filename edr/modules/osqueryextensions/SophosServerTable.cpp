#include <sstream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <osquery/tables.h>
#include <osquery/sql/dynamic_table_row.h>
#include <osquery/flagalias.h>
#include <osquery/flags.h>
#include <extensions/interface.h>
#include <osquery/flags.h>
#include <osquery/sdk/sdk.h>
#include <osquery/system.h>
#pragma GCC diagnostic pop
#include <iostream>
#include <redist/boost/include/boost/property_tree/ini_parser.hpp>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFileSystem.h>

namespace osquery
{
    class SophosServerTablePlugin : public osquery::TablePlugin
    {
    private:
        TableColumns columns() const override {
            return {

                    std::make_tuple("endpoint_id", TEXT_TYPE, ColumnOptions::DEFAULT),
                    std::make_tuple("customer_id", TEXT_TYPE, ColumnOptions::DEFAULT)
            };
        }

        TableRows generate(QueryContext& /*request*/) override {
            TableRows results;
            auto r = osquery::make_table_row();
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
                    LOG(ERROR) << "Failed to read MCSID configuration from config file: " << mcsConfigPath << " with error: " << ex.what();
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
                    LOG(ERROR) << "Failed to read MCSID configuration from config file: " << mcsPolicyConfigPath << " with error: " << ex.what();
                }
            }
            results.push_back(std::move(r));
            return results;
        }
    };
    REGISTER_EXTERNAL(SophosServerTablePlugin, "table", "sophos_server_information");
}

//// Note 3: Use REGISTER_EXTERNAL to define your plugin or table.
//using namespace osquery;
//int main(int argc, char* argv[]) {
//    osquery::Initializer runner(argc, argv, ToolType::EXTENSION);
//
//    auto status = startExtension("sophos_server_information", "1.0.0");
//    if (!status.ok())
//    {
//        LOG(ERROR) << status.getMessage();
//        runner.requestShutdown(status.getCode());
//    }
//
//    // Finally wait for a signal / interrupt to shutdown.
//    runner.waitThenShutdown();
//    return 0;
//}
