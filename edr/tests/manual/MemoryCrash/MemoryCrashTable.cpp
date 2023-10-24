/******************************************************************************************************

Copyright 2020-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MemoryCrashTable.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

// Note 2: Define at least one plugin or table.
namespace OsquerySDK
{
    std::vector<TableColumn>& MemoryCrashTable::GetColumns(){ return m_columns; }

    std::string& MemoryCrashTable::GetName() { return m_name; }


    TableRows MemoryCrashTable::Generate(QueryContextInterface& /*request*/)
    {
        OsquerySDK::TableRows results;

        std::vector<std::string> list;

        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        uint64_t totalSize = pages * page_size;
        int limit = (0.8 * totalSize)/100;


        int i = 0;
        while (i< limit)
        {
            ++i;
            list.emplace_back("stringstringstringstringstringstringstringstringstringstringstringstringstringstringstringstringstri");
        }
        TableRow r;
        std::this_thread::sleep_for(std::chrono::seconds(60));
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
        flags.timeout = 3;
        flags.interval = 3;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        auto extension = OsquerySDK::CreateExtension(flags, "SophosExtensionCrash", "1.0.0");
        extension->AddTablePlugin(std::make_unique<OsquerySDK::MemoryCrashTable>());

        extension->Start();
        std::cout << "Extension memorycrash started "  << std::endl;
        extension->Wait();
        return extension->GetReturnCode();
    }
    catch (const std::exception& err)
    {
        std::cout << "An error occurred while the extension logger was unavailable: " << err.what() << std::endl;

        return 1;
    }
}