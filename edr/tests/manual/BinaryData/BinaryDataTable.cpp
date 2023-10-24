/******************************************************************************************************

Copyright 2020-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BinaryDataTable.h"

#include <iostream>
#include <fstream>
#include <sstream>

namespace OsquerySDK
{
    std::vector<TableColumn>& BinaryDataTable::GetColumns(){ return m_columns; }

    std::string& BinaryDataTable::GetName() { return m_name; }

        TableRows BinaryDataTable::Generate(QueryContextInterface& request)
        {
            OsquerySDK::TableRows results;
            std::set<std::string> paths = request.GetConstraints("size",EQUALS);
            if (paths.empty())
            {
                return results;
            }

            std::stringstream count{*paths.begin()};
            int countv{0};
            count >> countv;
            TableRow r;
            std::ifstream input("/usr/bin/touch", std::ios::binary);

            // copies all data into buffer
            std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
            std::string data;
            int iterator = 0;
            for (auto character : buffer)
            {
                if (iterator > countv)
                {
                    break;
                }

                data += character;
                iterator += 1;
            }
            r["data"] = data;
            r["size"] = std::to_string(countv);
            results.push_back(std::move(r));
            return results;
        }
}

int main(int argc, char* argv[])
{
    try
    {
        auto flags = OsquerySDK::ParseFlags(&argc, &argv);

        auto extension = OsquerySDK::CreateExtension(flags, "SophosExtensionBinary", "1.0.0");
        extension->AddTablePlugin(std::make_unique<OsquerySDK::BinaryDataTable>());

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
