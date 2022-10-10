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
    std::optional<SafeStore_Id_t> threatIdFromString(const std::string& threatId)
    {
        if (threatId.length() < sizeof(SafeStore_Id_t))
        {
            return std::nullopt;
        }
        if (threatId[0] != 'T')
        {
            return std::nullopt;
        }
        constexpr unsigned int startOfBytes = 1; // start of the bytes we care about, ignore first T char
        SafeStore_Id_t id {};
        id.Data1 = threatId[startOfBytes] | threatId[startOfBytes + 1] << 8 | threatId[startOfBytes + 2] << 16 |
                   threatId[startOfBytes + 3] << 24;
        id.Data2 = threatId[startOfBytes + 4] | threatId[startOfBytes + 5] << 8;
        id.Data3 = threatId[startOfBytes + 6] | threatId[startOfBytes + 7] << 8;
        id.Data4[0] = threatId[startOfBytes + 8];
        id.Data4[1] = threatId[startOfBytes + 9];
        id.Data4[2] = threatId[startOfBytes + 10];
        id.Data4[3] = threatId[startOfBytes + 11];
        id.Data4[4] = threatId[startOfBytes + 12];
        id.Data4[5] = threatId[startOfBytes + 13];
        id.Data4[6] = threatId[startOfBytes + 14];
        id.Data4[7] = threatId[startOfBytes + 15];

        LOGDEBUG("Converted threat ID: " << threatId << " into safestore ID struct");
        return id;
    }

    std::string stringFromThreatId(const SafeStore_Id_t& id)
    {
        std::string threatId;
        threatId += char(id.Data1 & 0x000000ff);
        threatId += char((id.Data1 & 0x0000ff00) >> 8);
        threatId += char((id.Data1 & 0x00ff0000) >> 16);
        threatId += char((id.Data1 & 0xff000000) >> 24);

        threatId += char(id.Data2 & 0x00ff);
        threatId += char((id.Data2 & 0xff00) >> 8);

        threatId += char(id.Data3 & 0x00ff);
        threatId += char((id.Data3 & 0xff00) >> 8);

        threatId += char(id.Data4[0]);
        threatId += char(id.Data4[1]);
        threatId += char(id.Data4[2]);
        threatId += char(id.Data4[3]);
        threatId += char(id.Data4[4]);
        threatId += char(id.Data4[5]);
        threatId += char(id.Data4[6]);
        threatId += char(id.Data4[7]);

        return threatId;
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
        LOGINFO("before init call");

        auto result = SafeStore_Init(
            &m_safeStoreCtx,
            dbDirName.c_str(),
            dbName.c_str(),
            reinterpret_cast<const uint8_t*>(password.c_str()),
            password.size(),
            0);

        LOGINFO("after init call");

        switch (result)
        {
            case SR_OK:
                LOGINFO("Successfully initialised SafeStore database");
                return InitReturnCode::OK;
            case SR_INVALID_ARG:
                LOGWARN("Failed to initialise SafeStore database: Invalid argument");
                return InitReturnCode::INVALID_ARG;
            case SR_UNSUPPORTED_OS:
                LOGWARN("Failed to initialise SafeStore database: Operating system is not supported by SafeStore");
                return InitReturnCode::UNSUPPORTED_OS;
            case SR_UNSUPPORTED_VERSION:
                LOGWARN("Failed to initialise SafeStore database: Opened SafeStore database file's version is not "
                        "supported");
                return InitReturnCode::UNSUPPORTED_VERSION;
            case SR_OUT_OF_MEMORY:
                LOGWARN("Failed to initialise SafeStore database: There is not enough memory available to complete the "
                        "operation");
                return InitReturnCode::OUT_OF_MEMORY;
            case SR_DB_OPEN_FAILED:
                LOGWARN("Failed to initialise SafeStore database: Could not open the database");
                return InitReturnCode::DB_OPEN_FAILED;
            case SR_DB_ERROR:
                LOGWARN("Failed to initialise SafeStore database: Database operation failed");
                return InitReturnCode::DB_ERROR;
            default:
                LOGWARN("Failed to initialise SafeStore database");
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
        auto threatIdSafeStore = threatIdFromString(threatId);
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
            objectHandle.getRawHandle());

        switch (result)
        {
                // TODO 5675 messages....
            case SR_OK:
                LOGDEBUG("TODO SR_OK");
                return SaveFileReturnCode::OK;
            case SR_INVALID_ARG:
                LOGWARN("TODO INVALID_ARG");
                return SaveFileReturnCode::INVALID_ARG;
            case SR_INTERNAL_ERROR:
                LOGWARN("TODO INTERNAL_ERROR");
                return SaveFileReturnCode::INTERNAL_ERROR;
            case SR_OUT_OF_MEMORY:
                LOGWARN("TODO");
                return SaveFileReturnCode::OUT_OF_MEMORY;
            case SR_FILE_OPEN_FAILED:
                LOGWARN("TODO");
                return SaveFileReturnCode::FILE_OPEN_FAILED;
            case SR_FILE_READ_FAILED:
                LOGWARN("TODO");
                return SaveFileReturnCode::FILE_READ_FAILED;
            case SR_FILE_WRITE_FAILED:
                LOGWARN("TODO");
                return SaveFileReturnCode::FILE_WRITE_FAILED;
            case SR_MAX_OBJECT_SIZE_EXCEEDED:
                LOGWARN("TODO");
                return SaveFileReturnCode::MAX_OBJECT_SIZE_EXCEEDED;
            case SR_MAX_STORE_SIZE_EXCEEDED:
                LOGWARN("TODO");
                return SaveFileReturnCode::MAX_STORE_SIZE_EXCEEDED;
            case SR_DB_ERROR:
                LOGWARN("TODO");
                return SaveFileReturnCode::DB_ERROR;
            default:
                LOGWARN("TODO");
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
        return SafeStore_SetConfigIntValue(m_safeStoreCtx, convertToSafeStoreConfigId(option), value) == SR_OK;
    }

    bool SafeStoreWrapperImpl::findFirst(
        const SafeStoreFilter& filter,
        SearchHandleHolder& searchHandle,
        ObjectHandleHolder& objectHandle)
    {
        // Convert Filter type to SafeStore_Filter_t
        SafeStore_Filter_t ssFilter;
        ssFilter.activeFields = convertFilterFieldsToSafeStoreInt(filter.activeFields);
        if (auto ssThreatId = threatIdFromString(filter.threatId))
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
                m_safeStoreCtx, &ssFilter, searchHandle.getRawHandle(), objectHandle.getRawHandle()) == SR_OK);
    }

    bool SafeStoreWrapperImpl::findNext(
        SearchHandleHolder& searchHandle,
        ObjectHandleHolder& objectHandle)
    {
        return (
            SafeStore_FindNext(m_safeStoreCtx, *(searchHandle.getRawHandle()), objectHandle.getRawHandle()) == SR_OK);
    }

    std::unique_ptr<SearchHandleHolder> SafeStoreWrapperImpl::createSearchHandleHolder()
    {

//        std::weak_ptr<ISafeStoreWrapper> safeStoreWrapper = this;
        // TODO make this nicer - the ref can go out of scope.
        return std::make_unique<SearchHandleHolder>(*this);
    }

    std::unique_ptr<ObjectHandleHolder> SafeStoreWrapperImpl::createObjectHandleHolder()
    {
        // TODO make this nicer - the ref can go out of scope.
        return std::make_unique<safestore::ObjectHandleHolder>(*this);
    }

    SearchResults SafeStoreWrapperImpl::find(const SafeStoreFilter& filter)
    {
        return SearchResults(*this, filter);
    }

    std::string SafeStoreWrapperImpl::getObjectName(ObjectHandleHolder& objectHandle)
    {
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

        auto returnCode = SafeStore_GetObjectName(*(objectHandle.getRawHandle()), buf, &size);

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

    std::string SafeStoreWrapperImpl::getObjectId(ObjectHandleHolder& objectHandle)
    {
        SafeStore_Id_t objectId;
        SafeStore_GetObjectId(objectHandle.getRawHandle(), &objectId);

        return stringFromThreatId(objectId);
    }

    bool SafeStoreWrapperImpl::getObjectHandle(
        const std::string& threatId,
        std::shared_ptr<ObjectHandleHolder> objectHandle)
    {
        //        TODO 5675: Handle errors
        //        Success:
        //            SR_OK
        //        Failure:
        //            SR_INVALID_ARG - an invalid argument was passed to the function
        //            SR_OUT_OF_MEMORY - not enough memory is available to complete the operation
        //            SR_OBJECT_NOT_FOUND - no object was found with the given ID
        //            SR_INTERNAL_ERROR - an internal error has occurred

        if (auto threadIdStr = threatIdFromString(threatId))
        {
            return (
                SafeStore_GetObjectHandle(m_safeStoreCtx, &threadIdStr.value(), objectHandle->getRawHandle()) == SR_OK);
        }

        return false;
    }

    bool SafeStoreWrapperImpl::finaliseObject(ObjectHandleHolder& objectHandle)
    {
        auto result = SafeStore_FinalizeObject(m_safeStoreCtx, *(objectHandle.getRawHandle()));
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

} // namespace safestore