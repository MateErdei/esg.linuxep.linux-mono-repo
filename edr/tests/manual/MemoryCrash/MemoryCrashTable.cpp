/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MemoryCrashTable.h"

#include <OsquerySDK/OsquerySDK.h>

#include <iostream>
#include <sstream>
#include <thread>

// Note 2: Define at least one plugin or table.
namespace OsquerySDK
{
    std::vector<TableColumn> MemoryCrashTable::GetColumns()
    {
        return
        {
            OsquerySDK::TableColumn{"string", INTEGER_TYPE, ColumnOptions::DEFAULT}
        };
    }

    std::string MemoryCrashTable::GetName() { return "memorycrashtable"; }


    TableRows MemoryCrashTable::Generate(QueryContextInterface& /*request*/)
    {
        OsquerySDK::TableRows results;

        std::vector<std::string> list;
        bool run = true;
        while (run)
        {
            list.emplace_back("randimstring");
        }
        TableRow r;

        r["string"] = "randimstring";
        results.push_back(std::move(r));
        return results;

    }
}

int main(int argc, char* argv[])
{
    try
    {
        auto flags = OsquerySDK::ParseFlags(&argc, &argv);

        auto extension = OsquerySDK::CreateExtension(flags, "SophosExtensionCrash", "1.0.0");
        extension->AddTablePlugin(std::make_unique<OsquerySDK::MemoryCrashTable>());

        extension->Start();
        extension->Wait();
        return extension->GetReturnCode();
    }
    catch (const std::exception& err)
    {
        std::cout << "An error occurred while the extension logger was unavailable: " << err.what() << std::endl;

        return 1;
    }
}