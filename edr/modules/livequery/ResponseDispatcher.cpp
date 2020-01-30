/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ResponseDispatcher.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <thirdparty/nlohmann-json/json.hpp>
#include "Logger.h"
#include <sstream>

namespace
{

    class AppendToJsonArray
    {
        template <typename T> [[nodiscard]] T extractNonTextValues(const std::string& value) const
        {
            std::stringstream s(value);
            T valueOfTypeT;
            s >> valueOfTypeT;
            if (s.fail())
            {
                LOGERROR("Failed to convert value to " << typeid(valueOfTypeT).name() << ": " << value);
                // very unlikely to happen as the value was of type T for osquery
                // to claim it was of type T.
                throw std::runtime_error("Conversion failure");
            }
            return valueOfTypeT;
        }

    public:
        explicit AppendToJsonArray(livequery::ResponseData::ValueType valueType) : m_valueType {std::move(valueType) }
        {
        }
        void extractValueAndAddToJson(const livequery::ResponseData::RowData& rowData, nlohmann::json& jsonArray) const
        {
            auto valueIterator = rowData.find(m_valueType.first);
            // setting missing records to null.
            // this can happen if some of the rows have more or less entries than the others.
            // to cope with this situation and still send user all the data, the missing values are set to null.
            if ( valueIterator == rowData.end())
            {
                jsonArray.push_back(nullptr);
                return;
            }

            const std::string& value = valueIterator->second;
            if (m_valueType.second == livequery::ResponseData::AcceptedTypes::STRING ||
                m_valueType.second == livequery::ResponseData::AcceptedTypes::DATE ||
                m_valueType.second == livequery::ResponseData::AcceptedTypes::DATETIME ||
                m_valueType.second == livequery::ResponseData::AcceptedTypes::UNSIGNED_BIGINT)
            {
                jsonArray.push_back(value);
            }
            else
            {
                if (value.empty())
                {
                    jsonArray.push_back(nullptr);
                    return;
                }

                if (m_valueType.second == livequery::ResponseData::AcceptedTypes::BIGINT)
                {
                    jsonArray.push_back(extractNonTextValues<long long>(value));
                }
                else if (m_valueType.second == livequery::ResponseData::AcceptedTypes::INTEGER)
                {
                    jsonArray.push_back(extractNonTextValues<long>(value));
                }
            }
        }

        livequery::ResponseData::ValueType m_valueType;
    };

    std::string queryMetaDataObject(const livequery::QueryResponse& queryResponse, bool exceededLimit)
    {
        nlohmann::json queryMetaData;
        // queryMetaData["durationMillis"] = 32;
        // queryMetaData["sizeBytes"] = 490;
        if (queryResponse.status().errorCode() != livequery::ErrorCode::OSQUERYERROR &&
            !queryResponse.data().hasDataExceededLimit() && queryResponse.data().hasHeaders())
        {
            queryMetaData["rows"] = queryResponse.data().columnData().size();
        }
        if ( exceededLimit)
        {
            livequery::ResponseStatus exceedError{livequery::ErrorCode::RESPONSEEXCEEDLIMIT};
            queryMetaData["errorCode"] = exceedError.errorValue();
            queryMetaData["errorMessage"] = exceedError.errorDescription();
        }
        else{
            queryMetaData["errorCode"] = queryResponse.status().errorValue();
            queryMetaData["errorMessage"] = queryResponse.status().errorDescription();
        }

        return queryMetaData.dump();
    }

    std::string columnMetaDataObject(const livequery::QueryResponse& queryResponse)
    {
        nlohmann::json columnMetaData = nlohmann::json::array();
        const auto& headers = queryResponse.data().columnHeaders();
        for (auto& entry : headers)
        {
            std::map<std::string, std::string> cell;
            cell["name"] = entry.first;
            cell["type"] = livequery::ResponseData::AcceptedTypesToString(entry.second);
            columnMetaData.push_back(cell);
        }
        return columnMetaData.dump();
    }

    std::string columnDataObject(const livequery::QueryResponse& queryResponse)
    {
        const auto& headers = queryResponse.data().columnHeaders();
        const auto& columnData = queryResponse.data().columnData();
        std::vector<AppendToJsonArray> dataExtractors {};
        for (auto& entry : headers)
        {
            dataExtractors.emplace_back(entry);
        }

        nlohmann::json columnDataJson = nlohmann::json::array();

        for (auto& rowData : columnData)
        {
            nlohmann::json rowDataJson = nlohmann::json::array();
            for (auto& dataExtractor : dataExtractors)
            {
                dataExtractor.extractValueAndAddToJson(rowData, rowDataJson);
            }
            columnDataJson.push_back(rowDataJson);
        }
        return columnDataJson.dump();
    }
} // namespace

namespace livequery
{
    void ResponseDispatcher::sendResponse(const std::string& correlationId, const livequery::QueryResponse& response)
    {
        std::string fileContent;
        try
        {
            fileContent = serializeToJson(response);
        }
        catch (std::exception & error) // NOLINT
        {
            LOGERROR("Serialize to Json failed: " << error.what());
            // consistent with Windows behaviour: https://stash.sophos.net/projects/WINEP/repos/livequery/browse/src/ExtensionLib/QueryProcessorCallback.cpp#210
            QueryResponse error102{ResponseStatus{ErrorCode::UNEXPECTEDERROR},
                                   ResponseData::emptyResponse()};
            fileContent = serializeToJson(error102);
        }
        LOGDEBUG("Query result: " << fileContent);
        std::string tmpPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string rootInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string targetDir = Common::FileSystem::join(rootInstall, "base/mcs/response");
        std::string fileName = "LiveQuery_" + correlationId + "_response.json";
        std::string fullTargetName = Common::FileSystem::join(targetDir, fileName);

        Common::FileSystem::createAtomicFileToSophosUser(fileContent, fullTargetName, tmpPath);
    }

    /**
     *  Create json respecting the requirement that the order of the first keys must be fixed.
     *
     *  type
     *  queryMetaData
     *  columnMetaData
     *  columnData
     *  https://wiki.sophos.net/pages/viewpage.action?spaceKey=SophosCloud&title=EMP%3A+action-run-live-query
     *
     *  Since this is not enforced by the json library, it has been composed in directly.
     * @param response the QueryResponse which contain all the information for the json
     * @return  The content of the serialized json.
     * @exceptions Will throw std::exception if it fails serialize the response
     */
    std::string ResponseDispatcher::serializeToJson(const QueryResponse& response)
    {
        std::string columnDataObjectSerialized;
        bool limitExceeded = false;
        if (!response.data().columnData().empty())
        {
            columnDataObjectSerialized = columnDataObject(response);
            auto sizeBytes = columnDataObjectSerialized.size();
            if (sizeBytes > 10 * 1024 * 1024)
            {
                LOGWARN("Limit exceeded. Response would have: " << sizeBytes << " bytes");
                limitExceeded = true;
            }
        }

        std::stringstream serializedJson;
        serializedJson << R"({
"type": "sophos.mgt.response.RunLiveQuery")";

        serializedJson << R"(,
"queryMetaData": )" << queryMetaDataObject(response, limitExceeded);

        if (response.data().hasHeaders())
        {
            serializedJson << R"(,
"columnMetaData": )" << columnMetaDataObject(response);
        }

        if ( !limitExceeded && !columnDataObjectSerialized.empty())
        {
            serializedJson << R"(,
"columnData": )" << columnDataObjectSerialized ;
        }
        else if (response.status().errorCode() == livequery::ErrorCode::SUCCESS \
        || response.status().errorCode() == livequery::ErrorCode::RESPONSEEXCEEDLIMIT)
        {
            serializedJson << R"(,
"columnData":[])";
        }
        serializedJson << R"(
})";
        return serializedJson.str();
    }
} // namespace livequery
