// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWrapperImpl.h"

#include "Logger.h"

#include "common/ApplicationPaths.h"

extern "C"
{
#include "safestore.h"
}

namespace safestore
{
    std::optional<SafeStore_Id_t> safeStoreIdFromString(const std::string& safeStoreId)
    {
        if (safeStoreId.length() != sizeof(SafeStore_Id_t))
        {
            return std::nullopt;
        }
        constexpr unsigned int startOfBytes = 0;
        SafeStore_Id_t id {};
        id.Data1 = safeStoreId[startOfBytes] | safeStoreId[startOfBytes + 1] << 8 |
                   safeStoreId[startOfBytes + 2] << 16 | safeStoreId[startOfBytes + 3] << 24;
        id.Data2 = safeStoreId[startOfBytes + 4] | safeStoreId[startOfBytes + 5] << 8;
        id.Data3 = safeStoreId[startOfBytes + 6] | safeStoreId[startOfBytes + 7] << 8;
        id.Data4[0] = safeStoreId[startOfBytes + 8];
        id.Data4[1] = safeStoreId[startOfBytes + 9];
        id.Data4[2] = safeStoreId[startOfBytes + 10];
        id.Data4[3] = safeStoreId[startOfBytes + 11];
        id.Data4[4] = safeStoreId[startOfBytes + 12];
        id.Data4[5] = safeStoreId[startOfBytes + 13];
        id.Data4[6] = safeStoreId[startOfBytes + 14];
        id.Data4[7] = safeStoreId[startOfBytes + 15];

        LOGDEBUG("Converted threat ID: " << safeStoreId << " into safestore ID struct");
        return id;
    }

    std::string stringFromSafeStoreId(const SafeStore_Id_t& id)
    {
        std::string idString;
        idString += char(id.Data1 & 0x000000ff);
        idString += char((id.Data1 & 0x0000ff00) >> 8);
        idString += char((id.Data1 & 0x00ff0000) >> 16);
        idString += char((id.Data1 & 0xff000000) >> 24);

        idString += char(id.Data2 & 0x00ff);
        idString += char((id.Data2 & 0xff00) >> 8);

        idString += char(id.Data3 & 0x00ff);
        idString += char((id.Data3 & 0xff00) >> 8);

        idString += char(id.Data4[0]);
        idString += char(id.Data4[1]);
        idString += char(id.Data4[2]);
        idString += char(id.Data4[3]);
        idString += char(id.Data4[4]);
        idString += char(id.Data4[5]);
        idString += char(id.Data4[6]);
        idString += char(id.Data4[7]);

        return idString;
    }

    std::vector<uint8_t> bytesFromSafeStoreId(const SafeStore_Id_t& id)
    {
        std::vector<uint8_t> objectIdBytes;
        int numberOfBytes = sizeof(id);
        objectIdBytes.reserve(numberOfBytes);
        objectIdBytes.push_back(id.Data1 & 0x000000ff);
        objectIdBytes.push_back((id.Data1 & 0x0000ff00) >> 8);
        objectIdBytes.push_back((id.Data1 & 0x00ff0000) >> 16);
        objectIdBytes.push_back((id.Data1 & 0xff000000) >> 24);

        objectIdBytes.push_back(id.Data2 & 0x00ff);
        objectIdBytes.push_back((id.Data2 & 0xff00) >> 8);

        objectIdBytes.push_back(id.Data3 & 0x00ff);
        objectIdBytes.push_back((id.Data3 & 0xff00) >> 8);

        objectIdBytes.push_back(id.Data4[0]);
        objectIdBytes.push_back(id.Data4[1]);
        objectIdBytes.push_back(id.Data4[2]);
        objectIdBytes.push_back(id.Data4[3]);
        objectIdBytes.push_back(id.Data4[4]);
        objectIdBytes.push_back(id.Data4[5]);
        objectIdBytes.push_back(id.Data4[6]);
        objectIdBytes.push_back(id.Data4[7]);

        return objectIdBytes;
    }

    SafeStore_Config_t convertToSafeStoreConfigId(const ConfigOption& option)
    {
        switch (option)
        {
            case ConfigOption::MAX_SAFESTORE_SIZE:
                return SC_MAX_SAFESTORE_SIZE;
            case ConfigOption::MAX_OBJECT_SIZE:
                return SC_MAX_OBJECT_SIZE;
            case ConfigOption::MAX_REG_OBJECT_COUNT:
                return SC_MAX_REG_OBJECT_COUNT;
            case ConfigOption::AUTO_PURGE:
                return SC_AUTO_PURGE;
            case ConfigOption::MAX_STORED_OBJECT_COUNT:
                return SC_MAX_STORED_OBJECT_COUNT;
        }
        LOGWARN("Unknown config option");
        return {};
    }

    SafeStore_ObjectType_t convertObjTypeToSafeStoreObjType(const ObjectType& objectType)
    {
        switch (objectType)
        {
            case ObjectType::ANY:
                return SOT_ANY;
            case ObjectType::FILE:
                return SOT_FILE;
            case ObjectType::REGKEY:
                return SOT_REGKEY;
            case ObjectType::REGVALUE:
                return SOT_REGVALUE;
        }
        return SOT_UNKNOWN;
    }

    SafeStore_ObjectStatus_t convertObjStatusToSafeStoreObjStatus(const ObjectStatus& objectStatus)
    {
        switch (objectStatus)
        {
            case ObjectStatus::ANY:
                return SOS_ANY;
            case ObjectStatus::STORED:
                return SOS_STORED;
            case ObjectStatus::QUARANTINED:
                return SOS_QUARANTINED;
            case ObjectStatus::RESTORE_FAILED:
                return SOS_RESTORE_FAILED;
            case ObjectStatus::RESTORED_AS:
                return SOS_RESTORED_AS;
            case ObjectStatus::RESTORED:
                return SOS_RESTORED;
        }
        return SOS_UNDEFINED;
    }

    // see SafeStore_FilterField_t in safestore.h
    int convertFilterFieldsToSafeStoreInt(const std::vector<FilterField>& fields)
    {
        int activeFields = 0;
        for (const auto& field : fields)
        {
            switch (field)
            {
                case FilterField::THREAT_ID:
                    activeFields += 0x0001;
                    break;
                case FilterField::THREAT_NAME:
                    activeFields += 0x0002;
                    break;
                case FilterField::START_TIME:
                    activeFields += 0x0004;
                    break;
                case FilterField::END_TIME:
                    activeFields += 0x0008;
                    break;
                case FilterField::OBJECT_TYPE:
                    activeFields += 0x0010;
                    break;
                case FilterField::OBJECT_STATUS:
                    activeFields += 0x0020;
                    break;
                case FilterField::OBJECT_LOCATION:
                    activeFields += 0x0040;
                    break;
                case FilterField::OBJECT_NAME:
                    activeFields += 0x0080;
                    break;
            }
        }
        return activeFields;
    }

    SafeStoreWrapperImpl::~SafeStoreWrapperImpl()
    {
        if (m_safeStoreCtx != nullptr)
        {
            SafeStore_DeInit(m_safeStoreCtx);
            m_safeStoreCtx = nullptr;
            LOGDEBUG("Cleaned up SafeStore context");
        }
    }

    InitReturnCode SafeStoreWrapperImpl::initialise(
        const std::string& dbDirName,
        const std::string& dbName,
        const std::string& password)
    {
        auto result = SafeStore_Init(
            &m_safeStoreCtx,
            dbDirName.c_str(),
            dbName.c_str(),
            reinterpret_cast<const uint8_t*>(password.c_str()),
            password.size(),
            0);

        switch (result)
        {
            case SR_OK:
                LOGDEBUG("Successfully initialised SafeStore database");
                return InitReturnCode::OK;
            case SR_INVALID_ARG:
                LOGDEBUG("Failed to initialise SafeStore database: Invalid argument");
                return InitReturnCode::INVALID_ARG;
            case SR_UNSUPPORTED_OS:
                LOGDEBUG("Failed to initialise SafeStore database: Operating system is not supported by SafeStore");
                return InitReturnCode::UNSUPPORTED_OS;
            case SR_UNSUPPORTED_VERSION:
                LOGDEBUG("Failed to initialise SafeStore database: Opened SafeStore database file's version is not "
                        "supported");
                return InitReturnCode::UNSUPPORTED_VERSION;
            case SR_OUT_OF_MEMORY:
                LOGDEBUG("Failed to initialise SafeStore database: There is not enough memory available to complete the "
                        "operation");
                return InitReturnCode::OUT_OF_MEMORY;
            case SR_DB_OPEN_FAILED:
                LOGDEBUG("Failed to initialise SafeStore database: Could not open the database");
                return InitReturnCode::DB_OPEN_FAILED;
            case SR_DB_ERROR:
                LOGDEBUG("Failed to initialise SafeStore database: Database operation failed");
                return InitReturnCode::DB_ERROR;
            default:
                LOGDEBUG("Failed to initialise SafeStore database");
                return InitReturnCode::FAILED;
        }
    }

    SaveFileReturnCode SafeStoreWrapperImpl::saveFile(
        const std::string& directory,
        const std::string& filename,
        const std::string& threatId,
        const std::string& threatName,
        ObjectHandleHolder& objectHandle)
    {
        auto threatIdSafeStore = safeStoreIdFromString(threatId);
        LOGDEBUG("SafeStoreWrapperImpl::saveFile - directory: " << directory);
        LOGDEBUG("SafeStoreWrapperImpl::saveFile - filename: " << filename);
        LOGDEBUG("SafeStoreWrapperImpl::saveFile - threatId: " << threatId);
        LOGDEBUG("SafeStoreWrapperImpl::saveFile - threatName: " << threatName);
        auto result = SafeStore_SaveFile(
            m_safeStoreCtx,
            directory.c_str(),
            filename.c_str(),
            &(threatIdSafeStore.value()),
            threatName.c_str(),
            objectHandle.getRawHandlePtr());

        switch (result)
        {
            case SR_OK:
                LOGDEBUG("Got OK when attempting to save file to SafeStore database");
                return SaveFileReturnCode::OK;
            case SR_INVALID_ARG:
                LOGDEBUG("Got INVALID_ARG when attempting to save file to SafeStore database");
                return SaveFileReturnCode::INVALID_ARG;
            case SR_INTERNAL_ERROR:
                LOGDEBUG("Got INTERNAL_ERROR when attempting to save file to SafeStore database");
                return SaveFileReturnCode::INTERNAL_ERROR;
            case SR_OUT_OF_MEMORY:
                LOGDEBUG("Got OUT_OF_MEMORY when attempting to save file to SafeStore database");
                return SaveFileReturnCode::OUT_OF_MEMORY;
            case SR_FILE_OPEN_FAILED:
                LOGDEBUG("Got FILE_OPEN_FAILED when attempting to save file to SafeStore database");
                return SaveFileReturnCode::FILE_OPEN_FAILED;
            case SR_FILE_READ_FAILED:
                LOGDEBUG("Got FILE_READ_FAILED when attempting to save file to SafeStore database");
                return SaveFileReturnCode::FILE_READ_FAILED;
            case SR_FILE_WRITE_FAILED:
                LOGDEBUG("Got FILE_WRITE_FAILED when attempting to save file to SafeStore database");
                return SaveFileReturnCode::FILE_WRITE_FAILED;
            case SR_MAX_OBJECT_SIZE_EXCEEDED:
                LOGDEBUG("Got MAX_OBJECT_SIZE_EXCEEDED when attempting to save file to SafeStore database");
                return SaveFileReturnCode::MAX_OBJECT_SIZE_EXCEEDED;
            case SR_MAX_STORE_SIZE_EXCEEDED:
                LOGDEBUG("Got MAX_STORE_SIZE_EXCEEDED when attempting to save file to SafeStore database");
                return SaveFileReturnCode::MAX_STORE_SIZE_EXCEEDED;
            case SR_DB_ERROR:
                LOGDEBUG("Got DB_ERROR when attempting to save file to SafeStore database");
                return SaveFileReturnCode::DB_ERROR;
            default:
                LOGDEBUG("Got FAILED when attempting to save file to SafeStore database");
                return SaveFileReturnCode::FAILED;
        }
    }
    std::optional<uint64_t> SafeStoreWrapperImpl::getConfigIntValue(ConfigOption option)
    {
        uint64_t valueRead = 0;
        if (SafeStore_GetConfigIntValue(m_safeStoreCtx, convertToSafeStoreConfigId(option), &valueRead) == SR_OK)
        {
            return valueRead;
        }
        return std::nullopt;
    }

    bool SafeStoreWrapperImpl::setConfigIntValue(ConfigOption option, uint64_t value)
    {
        auto returnCode = SafeStore_SetConfigIntValue(m_safeStoreCtx, convertToSafeStoreConfigId(option), value);
        switch (returnCode)
        {
            case SR_OK:
                return true;
            case SR_INVALID_ARG:
                LOGWARN("Failed to set config value due to invalid arg");
                return false;
            case SR_INTERNAL_ERROR:
                LOGWARN("Failed to set config value due to internal SafeStore error");
                return false;
            default:
                LOGWARN("Failed to set config value fue to unknown reason");
                return false;
        }
    }

    bool SafeStoreWrapperImpl::findFirst(
        const SafeStoreFilter& filter,
        SearchHandleHolder& searchHandle,
        ObjectHandleHolder& objectHandle)
    {
        // Convert Filter type to SafeStore_Filter_t
        SafeStore_Filter_t ssFilter;
        ssFilter.activeFields = convertFilterFieldsToSafeStoreInt(filter.activeFields);
        if (auto ssThreatId = safeStoreIdFromString(filter.threatId))
        {
            ssFilter.threatId = &(ssThreatId.value());
        }
        ssFilter.threatName = filter.threatName.c_str();
        ssFilter.startTime = filter.startTime;
        ssFilter.endTime = filter.endTime;
        ssFilter.objectType = convertObjTypeToSafeStoreObjType(filter.objectType);
        ssFilter.objectStatus = convertObjStatusToSafeStoreObjStatus(filter.objectStatus);
        ssFilter.objectLocation = filter.objectLocation.c_str();
        ssFilter.objectName = filter.objectName.c_str();

        return (
            SafeStore_FindFirst(
                m_safeStoreCtx, &ssFilter, searchHandle.getRawHandle(), objectHandle.getRawHandlePtr()) == SR_OK);
    }

    bool SafeStoreWrapperImpl::findNext(SearchHandleHolder& searchHandle, ObjectHandleHolder& objectHandle)
    {
        return (
            SafeStore_FindNext(m_safeStoreCtx, *(searchHandle.getRawHandle()), objectHandle.getRawHandlePtr()) ==
            SR_OK);
    }

    std::unique_ptr<SearchHandleHolder> SafeStoreWrapperImpl::createSearchHandleHolder()
    {
        return std::make_unique<SearchHandleHolder>(*this);
    }

    std::unique_ptr<ObjectHandleHolder> SafeStoreWrapperImpl::createObjectHandleHolder()
    {
        return std::make_unique<safestore::ObjectHandleHolder>(*this);
    }

    SearchResults SafeStoreWrapperImpl::find(const SafeStoreFilter& filter)
    {
        return SearchResults(*this, filter);
    }

    std::string SafeStoreWrapperImpl::getObjectName(const ObjectHandleHolder& objectHandle)
    {
        if (objectHandle.getRawHandle() == nullptr)
        {
            return {};
        }

        constexpr int nameSize = 200;
        size_t size = nameSize;
        char buf[nameSize];

        // TODO 5675 deal with error codes... e.g. string size too small
        // Success:
        //  *     SR_OK
        //  *   Failure:
        //  *     SR_INVALID_ARG - an invalid argument was passed to the function
        //  *     SR_BUFFER_SIZE_TOO_SMALL - the buffer is too small to hold the output
        //  *     SR_INTERNAL_ERROR - an internal error has occurred

        auto returnCode = SafeStore_GetObjectName(objectHandle.getRawHandle(), buf, &size);

        switch (returnCode)
        {
            case SR_OK:
                break;
            case SR_INVALID_ARG:
                break;
            case SR_INTERNAL_ERROR:
                break;
            case SR_BUFFER_SIZE_TOO_SMALL:
                break;
            default:
                break;
        }
        return std::string(buf);
    }

    std::vector<uint8_t> SafeStoreWrapperImpl::getObjectId(const ObjectHandleHolder& objectHandle)
    {
        if (objectHandle.getRawHandle() == nullptr)
        {
            return {};
        }

        SafeStore_Id_t objectId;
        auto returnCode = SafeStore_GetObjectId(objectHandle.getRawHandle(), &objectId);

        if (returnCode == SR_OK)
        {
            return bytesFromSafeStoreId(objectId);
        }
        else
        {
            LOGERROR("Failed to get object ID");
        }

        return {};
    }

    bool SafeStoreWrapperImpl::getObjectHandle(
        const std::string& objectId,
        std::shared_ptr<ObjectHandleHolder> objectHandle)
    {
        if (auto safeStoreThreadId = safeStoreIdFromString(objectId))
        {

                auto returnCode = SafeStore_GetObjectHandle(m_safeStoreCtx, &safeStoreThreadId.value(), objectHandle->getRawHandlePtr());
                switch (returnCode)
                {
                    case SR_OK:
                        LOGDEBUG("Got OK when getting object handle from SafeStore");
                        return true;
                    case SR_INVALID_ARG:
                        LOGDEBUG("Got INVALID_ARG when getting object handle from SafeStore");
                        return false;
                    case SR_OUT_OF_MEMORY:
                        LOGDEBUG("Got OUT_OF_MEMORY when getting object handle from SafeStore");
                        return false;
                    case SR_OBJECT_NOT_FOUND:
                        LOGDEBUG("Got OBJECT_NOT_FOUND when getting object handle from SafeStore");
                        return false;
                    case SR_INTERNAL_ERROR:
                        LOGDEBUG("Got INTERNAL_ERROR when getting object handle from SafeStore");
                        return false;
                    default:
                        LOGDEBUG("Unknown return code when getting object handle from SafeStore");
                        return false;
                }
        }
        return false;
    }

    bool SafeStoreWrapperImpl::finaliseObject(ObjectHandleHolder& objectHandle)
    {
        if (objectHandle.getRawHandle() == nullptr)
        {
            return false;
        }
        auto result = SafeStore_FinalizeObject(m_safeStoreCtx, objectHandle.getRawHandle());
        return result == SR_OK;
    }

    void SafeStoreWrapperImpl::releaseObjectHandle(SafeStoreObjectHandle objectHandleHolder)
    {
        // TODO handle result
        SafeStore_ReleaseObjectHandle(objectHandleHolder);
    }

    void SafeStoreWrapperImpl::releaseSearchHandle(SafeStoreSearchHandle searchHandleHolder)
    {
        // TODO handle result
        SafeStore_FindClose(m_safeStoreCtx, searchHandleHolder);
    }

    ObjectType SafeStoreWrapperImpl::getObjectType(const ObjectHandleHolder& objectHandle)
    {
        SafeStore_ObjectType_t safeStoreObjectType;
        auto returnCode = SafeStore_GetObjectType(objectHandle.getRawHandle(), &safeStoreObjectType);
        if (returnCode == SR_OK)
        {
            switch (safeStoreObjectType)
            {
                case SOT_ANY:
                    return ObjectType::ANY;
                case SOT_FILE:
                    return ObjectType::FILE;
                case SOT_REGKEY:
                    return ObjectType::REGKEY;
                case SOT_REGVALUE:
                    return ObjectType::REGVALUE;
            }
        }
        LOGWARN("Failed to query object type, returning unknown");
        return ObjectType::UNKNOWN;
    }

    ObjectStatus SafeStoreWrapperImpl::getObjectStatus(const ObjectHandleHolder& objectHandle)
    {
        SafeStore_ObjectStatus_t safeStoreObjectStatus;
        auto returnCode = SafeStore_GetObjectStatus(objectHandle.getRawHandle(), &safeStoreObjectStatus);
        if (returnCode == SR_OK)
        {
            switch (safeStoreObjectStatus)
            {
                case SOS_ANY:
                    return ObjectStatus::ANY;
                case SOS_STORED:
                    return ObjectStatus::STORED;
                case SOS_QUARANTINED:
                    return ObjectStatus::QUARANTINED;
                case SOS_RESTORE_FAILED:
                    return ObjectStatus::RESTORE_FAILED;
                case SOS_RESTORED_AS:
                    return ObjectStatus::RESTORED_AS;
                case SOS_RESTORED:
                    return ObjectStatus::RESTORED;
            }
        }
        LOGWARN("Failed to query object status, returning undefined");
        return ObjectStatus::UNDEFINED;
    }

    std::string SafeStoreWrapperImpl::getObjectThreatId(const ObjectHandleHolder& objectHandle)
    {
        SafeStore_Id_t threatId;
        auto returnCode = SafeStore_GetObjectThreatId(objectHandle.getRawHandle(), &threatId);
        if (returnCode == SR_OK)
        {
            return stringFromSafeStoreId(threatId);
        }
        LOGWARN("Failed to query object threat ID, returning blank ID");
        return "";
    }

    std::string SafeStoreWrapperImpl::getObjectThreatName(const ObjectHandleHolder& objectHandle)
    {
        constexpr int nameSize = 200;
        size_t size = nameSize;
        char buf[nameSize];

        // TODO 5675 deal with error codes... e.g. string size too small

        auto returnCode = SafeStore_GetObjectThreatName(objectHandle.getRawHandle(), buf, &size);
        switch (returnCode)
        {
            case SR_OK:
                break;
            case SR_INVALID_ARG:
                break;
            case SR_INTERNAL_ERROR:
                break;
            case SR_BUFFER_SIZE_TOO_SMALL:
                break;
            default:
                break;
        }
        return std::string(buf);
    }

    int64_t SafeStoreWrapperImpl::getObjectStoreTime(const ObjectHandleHolder& objectHandle)
    {
        SafeStore_Time_t safeStoreTime;
        SafeStore_GetObjectStoreTime(objectHandle.getRawHandle(), &safeStoreTime);
        return safeStoreTime;
    }

    bool SafeStoreWrapperImpl::setObjectCustomData(
        ObjectHandleHolder& objectHandle,
        const std::string& dataName,
        const std::vector<uint8_t>& value)
    {
        // TODO 5675 define max size for custom data

        auto returnCode = SafeStore_SetObjectCustomData(
            m_safeStoreCtx, objectHandle.getRawHandle(), dataName.c_str(), value.data(), value.size());

        switch (returnCode)
        {
            case SR_OK:
                LOGDEBUG("Stored custom data: " << dataName);
                return true;
            case SR_DB_ERROR:
                LOGWARN("Failed to set custom data due to database error, name: " << dataName);
                return false;
            case SR_INVALID_ARG:
                LOGWARN("Invalid arg when setting custom data: " << dataName);
                return false;
            case SR_MAX_CUSTOM_DATA_SIZE_EXCEEDED:
                LOGWARN("Custom-data max size exceeded when setting custom data, name: " << dataName);
                return false;
            case SR_DATA_TAG_NOT_SET:
                LOGWARN("Could not set any custom data with specified name: " << dataName);
                return false;
            default:
                LOGWARN("Failed to set custom data for unknown reason, name: " << dataName);
                return false;
        }
    }

    std::vector<uint8_t> SafeStoreWrapperImpl::getObjectCustomData(
        const ObjectHandleHolder& objectHandle,
        const std::string& dataName)
    {
        // TODO 5675 define max size for custom data
        // TODO 5675 deal with size coming back not equal to what we're expecting
        constexpr int dataSize = 1024;
        size_t size = dataSize;
        uint8_t buf[dataSize];

        size_t bytesRead = 0;
        auto returnCode = SafeStore_GetObjectCustomData(
            m_safeStoreCtx, objectHandle.getRawHandle(), dataName.c_str(), buf, &size, &bytesRead);
        std::vector<uint8_t> dataToReturn;
        switch (returnCode)
        {
            case SR_OK:
                if (bytesRead < dataSize)
                {
                    dataToReturn.assign(buf, buf + bytesRead);
                }
                else
                {
                    LOGWARN("Size of data returned from getting object custom data is larger than buffer, returning "
                            "empty data");
                }
                break;
            case SR_INVALID_ARG:
                LOGWARN("Invalid arg when getting custom data: " << dataName);
                break;
            case SR_MAX_CUSTOM_DATA_SIZE_EXCEEDED:
                LOGWARN("Custom-data max size exceeded when setting custom data, name: " << dataName);
                break;
            case SR_DATA_TAG_NOT_SET:
                LOGWARN("Could not find any custom data with specified name: " << dataName);
                break;
            default:
                LOGWARN("Failed to get custom data for unknown reason, name: " << dataName);
                break;
        }
        return dataToReturn;
    }
    bool SafeStoreWrapperImpl::setObjectCustomDataString(
        ObjectHandleHolder& objectHandle,
        const std::string& dataName,
        const std::string& value)
    {
        std::vector<uint8_t> valueAsBytes(value.begin(), value.end());
        return setObjectCustomData(objectHandle, dataName, valueAsBytes);
    }
    std::string SafeStoreWrapperImpl::getObjectCustomDataString(
        ObjectHandleHolder& objectHandle,
        const std::string& dataName)
    {
        auto bytes = getObjectCustomData(objectHandle, dataName);
        return std::string(bytes.begin(), bytes.end());
    }

} // namespace safestore