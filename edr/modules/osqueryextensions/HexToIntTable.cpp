// Copyright 2023 Sophos Limited. All rights reserved.

#include "HexToIntTable.h"
#include "Logger.h"

#include "ConstraintHelpers.h"


namespace OsquerySDK
{
    OsquerySDK::TableRows HexToIntTable::Generate(OsquerySDK::QueryContextInterface& context)
    {
        ConstraintHelpers constraintHelpers;
        LOGDEBUG(constraintHelpers.BuildQueryLogMessage(context, GetName()));
        OsquerySDK::TableRows results;
        OsquerySDK::TableRow row;

        std::set<std::string> hexStringConstraints =
            context.GetConstraints("hex_string", OsquerySDK::ConstraintOperator::EQUALS);

        if (hexStringConstraints.size() != 1)
        {
            std::string errorMessage =
                "Invalid hex_to_int query constraints. One hex_string equals constraint should be provided.";
            throw std::runtime_error(errorMessage);
        }

        std::string hexString = *hexStringConstraints.begin();
        row["hex_string"] = hexString;
        std::stringstream hexStream;
        hexStream << std::hex << hexString;
        try
        {
            row["int"] = std::to_string(std::stoi(hexStream.str(), 0, 16));
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error { "Failed to convert hex_string to int: " + std::string { e.what() } };
        }

        results.push_back(std::move(row));
        return results;
    }
}