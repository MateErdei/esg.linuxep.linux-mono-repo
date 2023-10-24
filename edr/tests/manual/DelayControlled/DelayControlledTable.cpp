// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "DelayControlledTable.h"

#include <thread>
#include <iostream>
#include <sstream>


// Note 2: Define at least one plugin or table.
namespace OsquerySDK
{
    std::vector<TableColumn>& DelayControlledTable::GetColumns(){ return m_columns; }

    std::string& DelayControlledTable::GetName() { return m_name; }


    TableRows DelayControlledTable::Generate(QueryContextInterface& request)
    {
        OsquerySDK::TableRows results;

        std::set<std::string> paths = request.GetConstraints("delay",EQUALS);
        if (paths.empty())
        {
            return results;
        }

        std::stringstream delays{*paths.begin()};
        int delayv{0};
        delays >> delayv;

        size_t start = time(NULL);
        std::this_thread::sleep_for(std::chrono::seconds(delayv));
        size_t stop = time(NULL);
        TableRow r;
        r["start"] = std::to_string(start);
        r["stop"] = std::to_string(stop);
        r["delay"] = std::to_string(delayv);
        results.push_back(std::move(r));
        return results;

    }
}

int main(int argc, char* argv[])
{
    try
    {
        auto flags = OsquerySDK::ParseFlags(&argc, &argv);

        auto extension = OsquerySDK::CreateExtension(flags, "SophosExtensionDelay", "1.0.0");
        extension->AddTablePlugin(std::make_unique<OsquerySDK::DelayControlledTable>());

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