// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "ResponseDispatcher.h"

// Package
#include "Logger.h"

// SPL
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/StringUtils.h"

// Thirdparty
#include <json.hpp>

// System
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
            if (m_valueType.second == OsquerySDK::ColumnType::TEXT_TYPE ||
                m_valueType.second == OsquerySDK::ColumnType::BLOB_TYPE
                )
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

                try
                {
                    if(m_valueType.second == OsquerySDK::ColumnType::UNSIGNED_BIGINT_TYPE)
                    {
                        jsonArray.push_back(extractNonTextValues<uint64_t>(value));
                    }
                    else if (m_valueType.second == OsquerySDK::ColumnType::BIGINT_TYPE)
                    {
                        jsonArray.push_back(extractNonTextValues<long long>(value));
                    }
                    else if (m_valueType.second == OsquerySDK::ColumnType::INTEGER_TYPE)
                    {
                        jsonArray.push_back(extractNonTextValues<long>(value));
                    }
                    else if (m_valueType.second == OsquerySDK::ColumnType::DOUBLE_TYPE)
                    {
                        jsonArray.push_back(extractNonTextValues<double>(value));
                    }
                }
                catch(std::runtime_error &)
                {
                    // if any failure to convert from string to integer based type default vaule to zero.
                    jsonArray.push_back(0);
                }
            }
        }

        livequery::ResponseData::ValueType m_valueType;
    };

    std::string safeDumpJson(nlohmann::json& jsonArray)
    {
        return jsonArray.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    }

    nlohmann::json queryMetaDataObject(const livequery::QueryResponse& queryResponse, size_t sizeInBytes, bool exceededLimit)
    {
        nlohmann::json queryMetaData;

        queryMetaData["sizeBytes"] = sizeInBytes;

        // We define this to be the end of the query as we're done querying and we have also serialised much of the data to JSON.
        if (queryResponse.metaData().getQueryStart() != 0)
        {
            queryMetaData["durationMillis"] = queryResponse.metaData().getMillisSinceStarted();
        }

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

        return queryMetaData;
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
        return safeDumpJson(columnMetaData);
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
        return safeDumpJson(columnDataJson);
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
                                   ResponseData::emptyResponse(),
                                   ResponseMetaData()};
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
    std::string ResponseDispatcher::serializeToJson(const QueryResponse&  response)
    {
        std::string columnDataObjectSerialized;
        bool limitExceeded = false;
        size_t sizeBytes = 0;
        if (!response.data().columnData().empty())
        {
            columnDataObjectSerialized = columnDataObject(response);
            sizeBytes = columnDataObjectSerialized.size();
            if (sizeBytes > 10 * 1024 * 1024)
            {
                LOGWARN("Limit exceeded. Response would have: " << sizeBytes << " bytes");
                limitExceeded = true;
            }
        }

        std::stringstream serializedJson;

        serializedJson << R"({
"type": "sophos.mgt.response.RunLiveQuery")";


        auto queryMetaData = queryMetaDataObject(response, sizeBytes, limitExceeded);
        serializedJson << R"(,
"queryMetaData": )" << safeDumpJson(queryMetaData);

        if (response.data().hasHeaders())
        {
            serializedJson << R"(,
"columnMetaData": )" << columnMetaDataObject(response);
        }
        // if we have somehow ran a successful query that doesn't have headers, we need to give an empty columnMetaData entry
        else if (response.status().errorCode() == livequery::ErrorCode::SUCCESS && !response.data().hasHeaders())
        {
            serializedJson << R"(,
"columnMetaData":[])";
        }
        if ( !limitExceeded && !columnDataObjectSerialized.empty())
        {
            serializedJson << R"(,
"columnData": )" << columnDataObjectSerialized ;
        }
        else if (response.status().errorCode() == livequery::ErrorCode::SUCCESS && !limitExceeded)
        {
            serializedJson << R"(,
"columnData":[])";
        }
        serializedJson << R"(
})";

        nlohmann::json telemetryJson;
        telemetryJson["errorcode"] = livequery::ResponseStatus::errorCodeName(response.status().errorCode());


        setTelemetry(safeDumpJson(telemetryJson));
        return serializedJson.str();
    }

    std::unique_ptr<IResponseDispatcher> ResponseDispatcher::clone()
    {
        return std::unique_ptr<IResponseDispatcher>(new ResponseDispatcher{});
    }

    std::string ResponseDispatcher::getTelemetry()
    {
        return m_telemetry;
    }

    void ResponseDispatcher::setTelemetry(const std::string &json)
    {
        m_telemetry = json;
    }
} // namespace livequery
