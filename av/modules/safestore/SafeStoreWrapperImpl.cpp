// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWrapperImpl.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
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

    //////////
    SafeStoreObjectHandleHolderImpl::~SafeStoreObjectHandleHolderImpl()
    {
        if (m_handle != nullptr)
        {
            SafeStore_ReleaseObjectHandle(m_handle);
            m_handle = nullptr;
            LOGDEBUG("Cleaned up SafeStore object handle");
        }
    }
    SafeStoreObjectHandle* SafeStoreObjectHandleHolderImpl::get()
    {
        return &m_handle;
    }

    ////////////////

    SafeStoreSearchHandleHolderImpl::SafeStoreSearchHandleHolderImpl(SafeStoreContext safeStoreCtx) :
        m_safeStoreCtx(safeStoreCtx)
    {
    }

    SafeStoreSearchHandleHolderImpl::~SafeStoreSearchHandleHolderImpl()
    {
        if (m_handle != nullptr)
        {
            SafeStore_FindClose(m_safeStoreCtx, m_handle);
            m_handle = nullptr;
            LOGDEBUG("Cleaned up SafeStore search handle");
        }
    }

    SafeStoreSearchHandle* SafeStoreSearchHandleHolderImpl::get()
    {
        return &m_handle;
    }

    ///////////

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
        SafeStoreObjectHandleHolder& objectHandle)
    {
        auto threatIdSafeStore = threatIdFromString(threatId);

        auto result = SafeStore_SaveFile(
            m_safeStoreCtx,
            directory.c_str(),
            filename.c_str(),
            &(threatIdSafeStore.value()),
            threatName.c_str(),
            objectHandle.get());

        switch (result)
        {
                // TODO 5675 messages....
            case SR_OK:
                LOGDEBUG("TODO YAY");
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
        SafeStoreFilter filter,
        SafeStoreSearchHandleHolder& searchHandle,
        SafeStoreObjectHandleHolder& objectHandle)
    {
        // Convert Filter type to SafeStore_Filter_t
        SafeStore_Filter_t ssFilter;
        ssFilter.activeFields = filter.activeFields;
        if (auto ssThreatId = threatIdFromString(filter.threatId))
        {
            ssFilter.threatId = &(ssThreatId.value());
        }
        ssFilter.threatName = filter.threatName.c_str();
        ssFilter.threatName = filter.threatName.c_str();
        ssFilter.startTime = filter.startTime;
        ssFilter.endTime = filter.endTime;
        ssFilter.objectType = convertObjTypeToSafeStoreObjType(filter.objectType);
        ssFilter.objectStatus = convertObjStatusToSafeStoreObjStatus(filter.objectStatus);
        ssFilter.objectLocation = filter.objectLocation.c_str();
        ssFilter.objectName = filter.objectName.c_str();

        return (SafeStore_FindFirst(m_safeStoreCtx, &ssFilter, searchHandle.get(), objectHandle.get()) == SR_OK);
    }

    bool SafeStoreWrapperImpl::findNext(
        SafeStoreSearchHandleHolder& searchHandle,
        SafeStoreObjectHandleHolder& objectHandle)
    {
        return (SafeStore_FindNext(m_safeStoreCtx,  searchHandle.get(), objectHandle.get()) == SR_OK);
    }

} // namespace safestore