// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWrapperImpl.h"

#include "safestore/Logger.h"

#include "common/ApplicationPaths.h"

extern "C"
{
#include "safestore.h"
}

namespace safestore::SafeStoreWrapper
{
    std::optional<SafeStore_Id_t> safeStoreIdFromString(const std::string& safeStoreId)
    {
        if (safeStoreId.length() != sizeof(SafeStore_Id_t))
        {
            return std::nullopt;
        }

        ObjectIdType IdAsBytes(safeStoreId.begin(), safeStoreId.end());
        return safeStoreIdFromBytes(IdAsBytes);
    }

    std::optional<SafeStore_Id_t> safeStoreIdFromBytes(const ObjectIdType& safeStoreId)
    {
        if (safeStoreId.size() != sizeof(SafeStore_Id_t))
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
        m_safeStoreHolder->deinit();
    }

    InitReturnCode SafeStoreWrapperImpl::initialise(
        const std::string& dbDirName,
        const std::string& dbName,
        const std::string& password)
    {
        return m_safeStoreHolder->init(dbDirName, dbName, password);
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
            m_safeStoreHolder->getHandle(),
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
        if (SafeStore_GetConfigIntValue(
                m_safeStoreHolder->getHandle(), convertToSafeStoreConfigId(option), &valueRead) == SR_OK)
        {
            return valueRead;
        }
        return std::nullopt;
    }

    bool SafeStoreWrapperImpl::setConfigIntValue(ConfigOption option, uint64_t value)
    {
        auto returnCode =
            SafeStore_SetConfigIntValue(m_safeStoreHolder->getHandle(), convertToSafeStoreConfigId(option), value);
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

    std::unique_ptr<ObjectHandleHolder> SafeStoreWrapperImpl::createObjectHandleHolder()
    {
        return std::make_unique<safestore::SafeStoreWrapper::ObjectHandleHolder>(m_getIdMethods, m_releaseMethods);
    }

    std::unique_ptr<SearchHandleHolder> SafeStoreWrapperImpl::createSearchHandleHolder()
    {
        return std::make_unique<SearchHandleHolder>(m_releaseMethods);
    }

    SearchResults SafeStoreWrapperImpl::find(const SafeStoreFilter& filter)
    {
        return SearchResults(m_releaseMethods, m_searchMethods, m_getIdMethods, filter);
    }

    std::string SafeStoreWrapperImpl::getObjectName(const ObjectHandleHolder& objectHandle)
    {
        if (objectHandle.getRawHandle() == nullptr)
        {
            return {};
        }

        size_t size = MAX_OBJECT_NAME_LENGTH;
        char buf[MAX_OBJECT_NAME_LENGTH];

        auto returnCode = SafeStore_GetObjectName(objectHandle.getRawHandle(), buf, &size);

        switch (returnCode)
        {
            case SR_OK:
                LOGDEBUG("Got OK when getting object name from SafeStore");
                break;
            case SR_INVALID_ARG:
                LOGERROR("Got INVALID_ARG when getting object name from SafeStore");
                break;
            case SR_INTERNAL_ERROR:
                LOGERROR("Got INTERNAL_ERROR when getting object name from SafeStore");
                break;
            case SR_BUFFER_SIZE_TOO_SMALL:
                LOGERROR("Got BUFFER_SIZE_TOO_SMALL when getting object name from SafeStore, size: " << size);
                break;
            default:
                LOGERROR("Failed for unknown reason when getting object name from SafeStore, rc: " << returnCode);
                break;
        }
        return {buf};
    }

    bool SafeStoreWrapperImpl::getObjectHandle(
        const ObjectIdType& objectId,
        std::shared_ptr<ObjectHandleHolder> objectHandle)
    {
        if (auto safeStoreThreadId = safeStoreIdFromBytes(objectId))
        {
            auto returnCode =
                SafeStore_GetObjectHandle(m_safeStoreHolder->getHandle(), &safeStoreThreadId.value(), objectHandle->getRawHandlePtr());
            switch (returnCode)
            {
                case SR_OK:
                    LOGDEBUG("Got OK when getting object handle from SafeStore");
                    return true;
                case SR_INVALID_ARG:
                    LOGERROR("Got INVALID_ARG when getting object handle from SafeStore");
                    return false;
                case SR_OUT_OF_MEMORY:
                    LOGERROR("Got OUT_OF_MEMORY when getting object handle from SafeStore");
                    return false;
                case SR_OBJECT_NOT_FOUND:
                    LOGERROR("Got OBJECT_NOT_FOUND when getting object handle from SafeStore");
                    return false;
                case SR_INTERNAL_ERROR:
                    LOGERROR("Got INTERNAL_ERROR when getting object handle from SafeStore");
                    return false;
                default:
                    LOGERROR("Unknown return code when getting object handle from SafeStore");
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
        auto result = SafeStore_FinalizeObject(m_safeStoreHolder->getHandle(), objectHandle.getRawHandle());
        return result == SR_OK;
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
        size_t size = MAX_OBJECT_THREAT_NAME_LENGTH;
        char buf[MAX_OBJECT_THREAT_NAME_LENGTH];
        auto returnCode = SafeStore_GetObjectThreatName(objectHandle.getRawHandle(), buf, &size);
        switch (returnCode)
        {
            case SR_OK:
                LOGDEBUG("Got OK when getting object threat name from SafeStore");
                break;
            case SR_INVALID_ARG:
                LOGERROR("Got INVALID_ARG when getting object threat name from SafeStore");
                break;
            case SR_INTERNAL_ERROR:
                LOGERROR("Got INTERNAL_ERROR when getting object threat name from SafeStore");
                break;
            case SR_BUFFER_SIZE_TOO_SMALL:
                LOGERROR("Got BUFFER_SIZE_TOO_SMALL when getting object threat name from SafeStore, size: " << size);
                break;
            default:
                LOGERROR(
                    "Failed for unknown reason when getting object threat name from SafeStore, rc: " << returnCode);
                break;
        }
        return {buf};
    }

    int64_t SafeStoreWrapperImpl::getObjectStoreTime(const ObjectHandleHolder& objectHandle)
    {
        SafeStore_Time_t safeStoreTime = 0;
        auto returnCode = SafeStore_GetObjectStoreTime(objectHandle.getRawHandle(), &safeStoreTime);
        switch (returnCode)
        {
            case SR_OK:
                LOGDEBUG("Successfully got object store time from SafeStore");
                return safeStoreTime;
            case SR_INVALID_ARG:
                LOGERROR("Failed to get object store time from SafeStore, returning 0");
                break;
            default:
                break;
        }
        return 0;
    }

    bool SafeStoreWrapperImpl::setObjectCustomData(
        ObjectHandleHolder& objectHandle,
        const std::string& dataName,
        const std::vector<uint8_t>& value)
    {
        if (value.size() > MAX_CUSTOM_DATA_SIZE)
        {
            LOGWARN(
                "Failed to set custom data against SafeStore object, max size: "
                << MAX_CUSTOM_DATA_SIZE << "bytes, attempted to store: " << value.size() << "bytes");
            return false;
        }

        auto returnCode = SafeStore_SetObjectCustomData(
            m_safeStoreHolder->getHandle(), objectHandle.getRawHandle(), dataName.c_str(), value.data(), value.size());

        switch (returnCode)
        {
            case SR_OK:
                LOGDEBUG("Stored custom data: " << dataName);
                return true;
            case SR_DB_ERROR:
                LOGERROR("Failed to set custom data due to database error, name: " << dataName);
                return false;
            case SR_INVALID_ARG:
                LOGERROR("Invalid arg when setting custom data: " << dataName);
                return false;
            case SR_MAX_CUSTOM_DATA_SIZE_EXCEEDED:
                LOGERROR("Custom-data max size exceeded when setting custom data, name: " << dataName);
                return false;
            case SR_DATA_TAG_NOT_SET:
                LOGERROR("Could not set any custom data with specified name: " << dataName);
                return false;
            default:
                LOGERROR("Failed to set custom data for unknown reason, name: " << dataName);
                return false;
        }
    }

    std::vector<uint8_t> SafeStoreWrapperImpl::getObjectCustomData(
        const ObjectHandleHolder& objectHandle,
        const std::string& dataName)
    {
        size_t size = MAX_CUSTOM_DATA_SIZE;
        uint8_t buf[MAX_CUSTOM_DATA_SIZE];

        size_t bytesRead = 0;
        auto returnCode = SafeStore_GetObjectCustomData(
            m_safeStoreHolder->getHandle(), objectHandle.getRawHandle(), dataName.c_str(), buf, &size, &bytesRead);
        std::vector<uint8_t> dataToReturn;
        switch (returnCode)
        {
            case SR_OK:
                if (bytesRead <= MAX_CUSTOM_DATA_SIZE)
                {
                    dataToReturn.assign(buf, buf + bytesRead);
                }
                else
                {
                    LOGERROR("Size of data returned from getting object custom data is larger than buffer, returning "
                            "empty data");
                }
                break;
            case SR_INVALID_ARG:
                LOGERROR("Invalid arg when getting custom data: " << dataName);
                break;
            case SR_MAX_CUSTOM_DATA_SIZE_EXCEEDED:
                LOGERROR("Custom-data max size exceeded when setting custom data, name: " << dataName);
                break;
            case SR_DATA_TAG_NOT_SET:
                LOGERROR("Could not find any custom data with specified name: " << dataName);
                break;
            default:
                LOGERROR("Failed to get custom data for unknown reason, name: " << dataName);
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

    SafeStoreWrapperImpl::SafeStoreWrapperImpl() :
        m_safeStoreHolder(std::make_shared<SafeStoreHolder>()),
        m_releaseMethods(std::make_shared<SafeStoreReleaseMethodsImpl>(m_safeStoreHolder)),
        m_searchMethods(std::make_shared<SafeStoreSearchMethodsImpl>(m_safeStoreHolder)),
        m_getIdMethods(std::make_shared<SafeStoreGetIdMethodsImpl>(m_safeStoreHolder))
    {
    }

    ObjectIdType SafeStoreWrapperImpl::getObjectId(const ObjectHandleHolder& objectHandle)
    {
        return m_getIdMethods->getObjectId(objectHandle.getRawHandle());
    }

    InitReturnCode SafeStoreHolder::init(
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
                LOGDEBUG(
                    "Failed to initialise SafeStore database: There is not enough memory available to complete the "
                    "operation");
                return InitReturnCode::OUT_OF_MEMORY;
            case SR_DB_OPEN_FAILED:
                LOGDEBUG("Failed to initialise SafeStore database: Could not open the database");
                return InitReturnCode::DB_OPEN_FAILED;
            case SR_DB_ERROR:
                LOGDEBUG("Failed to initialise SafeStore database: Database operation failed");
                return InitReturnCode::DB_ERROR;
            default:
                LOGDEBUG("Failed to initialise SafeStore database. SafeStore returned unexpected error code: " << static_cast<int>(result));
                return InitReturnCode::FAILED;
        }
    }

    void SafeStoreHolder::deinit()
    {
        if (m_safeStoreCtx != nullptr)
        {
            SafeStore_DeInit(m_safeStoreCtx);
            m_safeStoreCtx = nullptr;
            LOGDEBUG("Cleaned up SafeStore context");
        }
    }

    SafeStoreContext SafeStoreHolder::getHandle()
    {
        return m_safeStoreCtx;
    }

    SafeStoreReleaseMethodsImpl::SafeStoreReleaseMethodsImpl(std::shared_ptr<ISafeStoreHolder> safeStoreHolder) :
        m_safeStoreHolder(safeStoreHolder)
    {
    }

    void SafeStoreReleaseMethodsImpl::releaseObjectHandle(SafeStoreObjectHandle objectHandle)
    {
        if (objectHandle != nullptr)
        {
            auto returnCode = SafeStore_ReleaseObjectHandle(objectHandle);
            objectHandle = nullptr;
            switch (returnCode)
            {
                case SR_OK:
                    LOGDEBUG("Got OK when cleaning up safestore object handle");
                    break;
                case SR_INVALID_ARG:
                    LOGERROR("Got INVALID_ARG when cleaning up safestore object handle");
                    break;
                default:
                    LOGERROR("Failed to clean up safestore object handle for unknown reason");
            }
        }
    }

    void SafeStoreReleaseMethodsImpl::releaseSearchHandle(void* searchHandle)
    {
        if (searchHandle != nullptr)
        {
            auto returnCode = SafeStore_FindClose(m_safeStoreHolder->getHandle(), searchHandle);
            searchHandle = nullptr;
            switch (returnCode)
            {
                case SR_OK:
                    LOGDEBUG("Got OK when cleaning up safestore search handle");
                    break;
                case SR_INVALID_ARG:
                    LOGERROR("Got INVALID_ARG when cleaning up safestore search handle");
                    break;
                default:
                    LOGERROR("Failed to clean up safestore search handle for unknown reason");
            }
        }
    }

    SafeStoreSearchMethodsImpl::SafeStoreSearchMethodsImpl(std::shared_ptr<ISafeStoreHolder> safeStoreHolder) :
        m_safeStoreHolder(safeStoreHolder)
    {
    }

    bool SafeStoreSearchMethodsImpl::findFirst(
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

        auto returnCode = SafeStore_FindFirst(
                m_safeStoreHolder->getHandle(),
                &ssFilter,
                searchHandle.getRawHandlePtr(),
                objectHandle.getRawHandlePtr());

        switch (returnCode)
        {
            case SR_OK:
                return true;
            case SR_INVALID_ARG:
                LOGERROR("Got INVALID_ARG when performing FindFirst on SafeStore database");
                return false;
            case SR_INTERNAL_ERROR:
                LOGERROR("Got INTERNAL_ERROR when performing FindFirst on SafeStore database");
                return false;
            case SR_OUT_OF_MEMORY:
                LOGERROR("Got OUT_OF_MEMORY when performing FindFirst on SafeStore database");
                return false;
            case SR_DB_ERROR:
                LOGERROR("Got DB_ERROR when performing FindFirst on SafeStore database");
                return false;
            case SR_OBJECT_NOT_FOUND:
                LOGDEBUG("No objects found in SafeStore database that satisfy the filter criteria");
                return false;
            default:
                LOGERROR("Failed for unexpected reason when performing FindFirst on SafeStore database");
                return false;
        }
    }

    bool SafeStoreSearchMethodsImpl::findNext(SearchHandleHolder& searchHandle, ObjectHandleHolder& objectHandle)
    {
      auto returnCode = SafeStore_FindNext(
                m_safeStoreHolder->getHandle(), searchHandle.getRawHandle(), objectHandle.getRawHandlePtr());
      switch (returnCode)
      {
          case SR_OK:
              return true;
          case SR_INVALID_ARG:
              LOGERROR("Got INVALID_ARG when performing FindNext on SafeStore database");
              return false;
          case SR_OUT_OF_MEMORY:
              LOGERROR("Got INVALID_ARG when performing FindNext on SafeStore database");
              return false;
          case SR_OBJECT_NOT_FOUND:
              LOGDEBUG("No further objects found in SafeStore database");
              return false;
          default:
              LOGERROR("Failed for unexpected reason when performing FindNext on SafeStore database");
              return false;
      }
    }

    SafeStoreGetIdMethodsImpl::SafeStoreGetIdMethodsImpl(std::shared_ptr<ISafeStoreHolder> safeStoreHolder) :
        m_safeStoreHolder(safeStoreHolder)
    {
    }

    ObjectIdType SafeStoreGetIdMethodsImpl::getObjectId(SafeStoreObjectHandle objectHandle)
    {
        if (objectHandle == nullptr)
        {
            return {};
        }

        SafeStore_Id_t objectId;
        auto returnCode = SafeStore_GetObjectId(objectHandle, &objectId);

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
} // namespace safestore::SafeStoreWrapper