// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreWrapperImpl.h"

#include "common/ApplicationPaths.h"
#include "safestore/Logger.h"

#include "Common/UtilityImpl/Uuid.h"

#include <charconv>
#include <iomanip>

extern "C"
{
#include "safestore.h"
}

namespace
{
    class SearchHandleHolder
    {
    public:
        explicit SearchHandleHolder(safestore::SafeStoreWrapper::ISafeStoreHolder& safeStoreHolder) :
            safeStoreHolder_{ safeStoreHolder }
        {
        }

        ~SearchHandleHolder()
        {
            if (searchHandle_ == nullptr)
            {
                return;
            }

            auto returnCode = SafeStore_FindClose(safeStoreHolder_.getHandle(), searchHandle_);
            if (returnCode != SR_OK)
            {
                LOGWARN(
                    "Failed to close a SafeStore search handle: "
                    << safestore::SafeStoreWrapper::safeStoreReturnCodeToString(returnCode));
            }
        }

        SearchHandleHolder(const SearchHandleHolder&) = delete;
        SearchHandleHolder& operator=(const SearchHandleHolder&) = delete;
        SearchHandleHolder(SearchHandleHolder&&) = delete;
        SearchHandleHolder& operator=(SearchHandleHolder&&) = delete;

        [[nodiscard]] SafeStore_Search_t& get()
        {
            return searchHandle_;
        }

    private:
        SafeStore_Search_t searchHandle_ = nullptr;
        safestore::SafeStoreWrapper::ISafeStoreHolder& safeStoreHolder_;
    };
} // namespace

namespace safestore::SafeStoreWrapper
{
    std::optional<SafeStore_Id_t> safeStoreIdFromUuidString(const std::string& uuidString)
    {
        if (!Common::UtilityImpl::Uuid::IsValid(uuidString))
        {
            return {};
        }

        SafeStore_Id_t id{};

        // UUID format for reference:
        // xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        // 012345678901234567890123456789012345

        const char* const p = uuidString.c_str();
        // We skip over the hyphens
        std::from_chars(p + 0, p + 8, id.Data1, 16);
        std::from_chars(p + 9, p + 13, id.Data2, 16);
        std::from_chars(p + 14, p + 18, id.Data3, 16);
        for (int byteIndex = 0, charIndex = 19; byteIndex < 8; ++byteIndex, charIndex += 2)
        {
            if (charIndex == 23)
            {
                ++charIndex; // Skip hyphen
            }
            std::from_chars(p + charIndex, p + charIndex + 2, id.Data4[byteIndex], 16);
        }

        return id;
    }

    std::string uuidStringFromSafeStoreId(const SafeStore_Id_t& id)
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        oss << std::setw(8) << id.Data1;
        oss << '-';
        oss << std::setw(4) << id.Data2;
        oss << '-';
        oss << std::setw(4) << id.Data3;
        oss << '-';
        for (int i = 0; i < 8; i += 2)
        {
            if (i == 2)
            {
                oss << '-';
            }
            // If we were to output a single byte, ostream will output it as the character it represents, rather than 2
            // hexadecimal digits. Instead, we output two bytes simultaneously to prevent this.
            oss << std::setw(4) << (id.Data4[i] << 8 | id.Data4[i + 1]);
        }

        return oss.str();
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

    /**
     * Converts the abstract filter type to the concrete SafeStore filter
     * @param filter
     * @param ssThreatId A reference to a SafeStore object. Will be written into by this function if threatId is set in
     * the filter. The object pointed by this reference must stay valid at least as long as the result (but only if
     * threatId was specified in filter).
     * @return
     */
    SafeStore_Filter_t convertFilterToSafeStoreFilter(const SafeStoreFilter& filter, SafeStore_Id_t& ssThreatId)
    {
        SafeStore_Filter_t ssFilter{};

        if (filter.threatId)
        {
            auto ssThreatIdOption = safeStoreIdFromUuidString(filter.threatId.value());
            if (ssThreatIdOption)
            {
                ssThreatId = std::move(ssThreatIdOption.value());
                ssFilter.threatId = &ssThreatId;
                ssFilter.activeFields |= SFF_THREAT_ID;
            }
            else
            {
                LOGWARN("Invalid threatId value passed to filter SafeStore search");
            }
        }
        if (filter.threatName)
        {
            ssFilter.threatName = filter.threatName.value().c_str();
            ssFilter.activeFields |= SFF_THREAT_NAME;
        }
        if (filter.startTime)
        {
            ssFilter.startTime = filter.startTime.value();
            ssFilter.activeFields |= SFF_START_TIME;
        }
        if (filter.endTime)
        {
            ssFilter.endTime = filter.endTime.value();
            ssFilter.activeFields |= SFF_END_TIME;
        }
        if (filter.objectType)
        {
            ssFilter.objectType = convertObjTypeToSafeStoreObjType(filter.objectType.value());
            ssFilter.activeFields |= SFF_OBJECT_TYPE;
        }
        if (filter.objectStatus)
        {
            ssFilter.objectStatus = convertObjStatusToSafeStoreObjStatus(filter.objectStatus.value());
            ssFilter.activeFields |= SFF_OBJECT_STATUS;
        }
        if (filter.objectLocation)
        {
            ssFilter.objectLocation = filter.objectLocation.value().c_str();
            ssFilter.activeFields |= SFF_OBJECT_LOCATION;
        }
        if (filter.objectName)
        {
            ssFilter.objectName = filter.objectName.value().c_str();
            ssFilter.activeFields |= SFF_OBJECT_NAME;
        }

        return ssFilter;
    }

    std::string safeStoreReturnCodeToString(SafeStore_Result_t result)
    {
        switch (result)
        {
            case SR_OK:
                return "OK";
            case SR_INVALID_ARG:
                return "INVALID_ARG";
            case SR_NOT_IMPLEMENTED:
                return "NOT_IMPLEMENTED";
            case SR_INTERNAL_ERROR:
                return "INTERNAL_ERROR";
            case SR_UNSUPPORTED_OS:
                return "UNSUPPORTED_OS";
            case SR_UNSUPPORTED_VERSION:
                return "UNSUPPORTED_VERSION";
            case SR_OUT_OF_MEMORY:
                return "OUT_OF_MEMORY";
            case SR_DB_ERROR:
                return "DB_ERROR";
            case SR_DB_OPEN_FAILED:
                return "DB_OPEN_FAILED";
            case SR_DB_INCONSISTENT:
                return "DB_INCONSISTENT";
            case SR_FILE_OPEN_FAILED:
                return "FILE_OPEN_FAILED";
            case SR_FILE_READ_FAILED:
                return "FILE_READ_FAILED";
            case SR_FILE_WRITE_FAILED:
                return "FILE_WRITE_FAILED";
            case SR_REGISTRY_OPEN_FAILED:
                return "REGISTRY_OPEN_FAILED";
            case SR_MAX_STORE_SIZE_EXCEEDED:
                return "MAX_STORE_SIZE_EXCEEDED";
            case SR_MAX_OBJECT_SIZE_EXCEEDED:
                return "MAX_OBJECT_SIZE_EXCEEDED";
            case SR_MAX_REG_OBJECT_COUNT_EXCEEDED:
                return "MAX_REG_OBJECT_COUNT_EXCEEDED";
            case SR_PARTIAL_RESTORE:
                return "PARTIAL_RESTORE";
            case SR_RESTORE_FAILED:
                return "RESTORE_FAILED";
            case SR_OBJECT_NOT_FOUND:
                return "OBJECT_NOT_FOUND";
            case SR_OBJECT_EXISTS:
                return "OBJECT_EXISTS";
            case SR_MAX_STORED_OBJECT_COUNT_EXCEEDED:
                return "MAX_STORED_OBJECT_COUNT_EXCEEDED";
            case SR_MAX_CUSTOM_DATA_SIZE_EXCEEDED:
                return "MAX_CUSTOM_DATA_SIZE_EXCEEDED";
            case SR_DATA_TAG_NOT_SET:
                return "DATA_TAG_NOT_SET";
            case SR_BUFFER_SIZE_TOO_SMALL:
                return "BUFFER_SIZE_TOO_SMALL";
            case SR_EXPORT_FILE_EXISTS:
                return "EXPORT_FILE_EXISTS";
            case SR_ILLEGAL_RESTORE_LOCATION:
                return "ILLEGAL_RESTORE_LOCATION";
            case SR_PERMISSION_DENIED:
                return "PERMISSION_DENIED";
            default:
                return "UNKNOWN, code: " + std::to_string(static_cast<int>(result));
        }
    }

    SafeStoreWrapperImpl::SafeStoreWrapperImpl() :
        m_safeStoreHolder(std::make_shared<SafeStoreHolder>()),
        m_releaseMethods(std::make_shared<SafeStoreReleaseMethodsImpl>(m_safeStoreHolder)),
        m_getIdMethods(std::make_shared<SafeStoreGetIdMethodsImpl>(m_safeStoreHolder))
    {
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
        const ThreatId& threatId,
        const std::string& threatName,
        ObjectHandleHolder& objectHandle)
    {
        auto threatIdSafeStore = safeStoreIdFromUuidString(threatId);
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

        LOGDEBUG(
            "Got " << safeStoreReturnCodeToString(result) << " when attempting to save file to SafeStore database");
        switch (result)
        {
            case SR_OK:
                return SaveFileReturnCode::OK;
            case SR_INVALID_ARG:
                return SaveFileReturnCode::INVALID_ARG;
            case SR_INTERNAL_ERROR:
                return SaveFileReturnCode::INTERNAL_ERROR;
            case SR_OUT_OF_MEMORY:
                return SaveFileReturnCode::OUT_OF_MEMORY;
            case SR_FILE_OPEN_FAILED:
                return SaveFileReturnCode::FILE_OPEN_FAILED;
            case SR_FILE_READ_FAILED:
                return SaveFileReturnCode::FILE_READ_FAILED;
            case SR_FILE_WRITE_FAILED:
                return SaveFileReturnCode::FILE_WRITE_FAILED;
            case SR_MAX_OBJECT_SIZE_EXCEEDED:
                return SaveFileReturnCode::MAX_OBJECT_SIZE_EXCEEDED;
            case SR_MAX_STORE_SIZE_EXCEEDED:
                return SaveFileReturnCode::MAX_STORE_SIZE_EXCEEDED;
            case SR_DB_ERROR:
                return SaveFileReturnCode::DB_ERROR;
            default:
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
        if (returnCode == SR_OK)
        {
            LOGINFO("Setting config option: " << GL_OPTIONS_MAP.at(option) << " to: " << value);
            return true;
        }
        LOGWARN(
            "Got " << safeStoreReturnCodeToString(returnCode)
                   << " when setting config option: " << GL_OPTIONS_MAP.at(option) << " to: " << value);
        return false;
    }

    std::unique_ptr<ObjectHandleHolder> SafeStoreWrapperImpl::createObjectHandleHolder()
    {
        return std::make_unique<safestore::SafeStoreWrapper::ObjectHandleHolder>(m_getIdMethods, m_releaseMethods);
    }

    std::vector<ObjectHandleHolder> SafeStoreWrapperImpl::find(const SafeStoreFilter& filter)
    {
        // This needs to stay valid at least as long as ssFilter can be used
        SafeStore_Id_t ssThreatId;

        SafeStore_Filter_t ssFilter = convertFilterToSafeStoreFilter(filter, ssThreatId);

        std::vector<ObjectHandleHolder> results;

        SearchHandleHolder searchHandle{ *m_safeStoreHolder };

        LOGDEBUG("Starting SafeStore search");

        int i = 0;
        while (true)
        {
            ObjectHandleHolder objectHandle{ m_getIdMethods, m_releaseMethods };

            SafeStore_Result_t returnCode;
            if (i == 0)
            {
                returnCode = SafeStore_FindFirst(
                    m_safeStoreHolder->getHandle(), &ssFilter, &searchHandle.get(), objectHandle.getRawHandlePtr());
            }
            else
            {
                returnCode = SafeStore_FindNext(
                    m_safeStoreHolder->getHandle(), searchHandle.get(), objectHandle.getRawHandlePtr());
            }

            if (returnCode == SR_OBJECT_NOT_FOUND)
            {
                if (i == 0)
                {
                    LOGDEBUG("No objects found");
                }
                else
                {
                    LOGDEBUG("No more objects found");
                }
                break;
            }
            else if (returnCode != SR_OK)
            {
                LOGWARN("Got " << safeStoreReturnCodeToString(returnCode) << " when searching the SafeStore database");
                break;
            }
            else if (searchHandle.get() == nullptr || objectHandle.getRawHandle() == nullptr)
            {
                LOGWARN("SafeStore returned invalid results after successful search");
                break;
            }

            LOGDEBUG("Found object " << i);

            results.push_back(std::move(objectHandle));

            ++i;
        }

        return results;
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
        LOGDEBUG("Got " << safeStoreReturnCodeToString(returnCode) << " when getting object name from SafeStore");
        return { buf };
    }

    std::string SafeStoreWrapperImpl::getObjectLocation(const ObjectHandleHolder& objectHandle)
    {
        if (objectHandle.getRawHandle() == nullptr)
        {
            return {};
        }

        size_t size = MAX_OBJECT_PATH_LENGTH;
        char buf[MAX_OBJECT_PATH_LENGTH];
        auto returnCode = SafeStore_GetObjectLocation(objectHandle.getRawHandle(), buf, &size);
        LOGDEBUG("Got " << safeStoreReturnCodeToString(returnCode) << " when getting object location from SafeStore");
        return { buf };
    }

    bool SafeStoreWrapperImpl::getObjectHandle(
        const ObjectId& objectId,
        std::shared_ptr<ObjectHandleHolder> objectHandle)
    {
        if (auto safeStoreObjectId = safeStoreIdFromUuidString(objectId))
        {
            auto returnCode = SafeStore_GetObjectHandle(
                m_safeStoreHolder->getHandle(), &safeStoreObjectId.value(), objectHandle->getRawHandlePtr());
            if (returnCode == SR_OK)
            {
                LOGDEBUG("Got OK when getting object handle from SafeStore");
                return true;
            }
            LOGERROR("Got " << safeStoreReturnCodeToString(returnCode) << " when getting object handle from SafeStore");
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

    ThreatId SafeStoreWrapperImpl::getObjectThreatId(const ObjectHandleHolder& objectHandle)
    {
        SafeStore_Id_t threatId;
        auto returnCode = SafeStore_GetObjectThreatId(objectHandle.getRawHandle(), &threatId);
        if (returnCode == SR_OK)
        {
            return uuidStringFromSafeStoreId(threatId);
        }
        LOGWARN(
            "Got " << safeStoreReturnCodeToString(returnCode) << " while getting object threat ID, returning blank ID");
        return "";
    }

    std::string SafeStoreWrapperImpl::getObjectThreatName(const ObjectHandleHolder& objectHandle)
    {
        size_t size = MAX_OBJECT_THREAT_NAME_LENGTH;
        char buf[MAX_OBJECT_THREAT_NAME_LENGTH];
        auto returnCode = SafeStore_GetObjectThreatName(objectHandle.getRawHandle(), buf, &size);
        if (returnCode == SR_OK)
        {
            LOGDEBUG("Got OK when getting object threat name from SafeStore");
        }
        else
        {
            LOGERROR(
                "Got " << safeStoreReturnCodeToString(returnCode) << " when getting object threat name from SafeStore");
        }
        return { buf };
    }

    int64_t SafeStoreWrapperImpl::getObjectStoreTime(const ObjectHandleHolder& objectHandle)
    {
        SafeStore_Time_t safeStoreTime = 0;
        auto returnCode = SafeStore_GetObjectStoreTime(objectHandle.getRawHandle(), &safeStoreTime);
        if (returnCode == SR_OK)
        {
            LOGDEBUG("Successfully got object store time from SafeStore");
            return safeStoreTime;
        }
        LOGERROR(
            "Got " << safeStoreReturnCodeToString(returnCode)
                   << " while getting object store time from SafeStore, returning 0");
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
        if (returnCode == SR_OK)
        {
            LOGDEBUG("Stored custom data: " << dataName);
            return true;
        }
        LOGERROR("Got " << safeStoreReturnCodeToString(returnCode) << " while setting custom data, name: " << dataName);
        return false;
    }

    std::vector<uint8_t> SafeStoreWrapperImpl::getObjectCustomData(
        const ObjectHandleHolder& objectHandle,
        const std::string& dataName)
    {
        size_t size = MAX_CUSTOM_DATA_SIZE;
        uint8_t buf[MAX_CUSTOM_DATA_SIZE];

        size_t bytesRead = 0;

        // We need to get the size of the stored data, which is then passed as the buffer size rather than the true
        // buffer size. This is needed because if the size of the data is larger than 64 bytes, it will report bytesRead
        // to be 'size', rather than the actual size of the stored data.
        // This looks to be a SafeStore defect, but it hasn't been yet confirmed by Core.
        size_t actualSize = 0;
        auto returnCode = SafeStore_GetObjectCustomData(
            m_safeStoreHolder->getHandle(), objectHandle.getRawHandle(), dataName.c_str(), nullptr, &actualSize, nullptr);
        if (returnCode == SR_OK)
        {
            if (actualSize <= size)
            {
                size = actualSize;
            }
            else
            {
                LOGERROR("Size of data returned from getting object custom data is larger than buffer, returning empty data");
                return {};
            }
        }
        else
        {
            LOGERROR(
                "Got " << safeStoreReturnCodeToString(returnCode) << " when getting custom data, name: " << dataName);
            return {};
        }

        returnCode = SafeStore_GetObjectCustomData(
            m_safeStoreHolder->getHandle(), objectHandle.getRawHandle(), dataName.c_str(), buf, &size, &bytesRead);
        std::vector<uint8_t> dataToReturn;

        if (returnCode == SR_OK)
        {
            if (bytesRead <= MAX_CUSTOM_DATA_SIZE)
            {
                dataToReturn.assign(buf, buf + bytesRead);
            }
            else
            {
                LOGERROR("Size of data returned from getting object custom data is larger than buffer, returning empty "
                         "data");
            }
        }
        else
        {
            LOGERROR(
                "Got " << safeStoreReturnCodeToString(returnCode) << " when getting custom data, name: " << dataName);
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

    ObjectId SafeStoreWrapperImpl::getObjectId(const ObjectHandleHolder& objectHandle)
    {
        return m_getIdMethods->getObjectId(objectHandle.getRawHandle());
    }

    bool SafeStoreWrapperImpl::restoreObjectById(const ObjectId& objectId)
    {
        return restoreObjectByIdToLocation(objectId, "");
    }

    bool SafeStoreWrapperImpl::restoreObjectByIdToLocation(const ObjectId& objectId, const std::string& path)
    {
        if (auto safeStoreObjectId = safeStoreIdFromUuidString(objectId))
        {
            auto returnCode = SafeStore_RestoreObjectById(
                m_safeStoreHolder->getHandle(), &safeStoreObjectId.value(), path.empty() ? nullptr : path.c_str());
            if (returnCode == SR_OK)
            {
                if (path.empty())
                {
                    LOGDEBUG("Successfully restored object to original path");
                }
                else
                {
                    LOGDEBUG("Successfully restored object to custom path: " << path);
                }
                return true;
            }
            LOGERROR("Got " << safeStoreReturnCodeToString(returnCode) << " when trying to restore an object");
        }
        return false;
    }

    bool SafeStoreWrapperImpl::restoreObjectsByThreatId(const ThreatId& threatId)
    {
        if (auto ssThreatId = safeStoreIdFromUuidString(threatId))
        {
            auto returnCode = SafeStore_RestoreObjectsByThreatId(m_safeStoreHolder->getHandle(), &ssThreatId.value());
            if (returnCode == SR_OK)
            {
                LOGDEBUG("Successfully restored object with threat ID: " << threatId);
                return true;
            }
            LOGERROR(
                "Got " << safeStoreReturnCodeToString(returnCode)
                       << " while trying to restore an object by threat ID: " << threatId);
        }
        return false;
    }

    bool SafeStoreWrapperImpl::deleteObjectById(const ThreatId& objectId)
    {
        if (auto safeStoreObjectId = safeStoreIdFromUuidString(objectId))
        {
            auto returnCode = SafeStore_DeleteObjectById(m_safeStoreHolder->getHandle(), &safeStoreObjectId.value());
            if (returnCode == SR_OK)
            {
                LOGDEBUG("Successfully deleted object from SafeStore");
                return true;
            }
            LOGERROR(
                "Got " << safeStoreReturnCodeToString(returnCode)
                       << " while trying to delete an object from SafeStore");
        }
        return false;
    }

    bool SafeStoreWrapperImpl::deleteObjectsByThreatId(const ThreatId& threatId)
    {
        if (auto ssThreatId = safeStoreIdFromUuidString(threatId))
        {
            auto returnCode = SafeStore_DeleteObjectsByThreatId(m_safeStoreHolder->getHandle(), &ssThreatId.value());
            if (returnCode == SR_OK)
            {
                LOGDEBUG("Successfully deleted object from SafeStore with threat ID: " << threatId);
                return true;
            }
            LOGERROR(
                "Got " << safeStoreReturnCodeToString(returnCode)
                       << " while trying to delete an object from SafeStore with threat ID: " << threatId);
        }
        return false;
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

        if (result == SR_OK)
        {
            LOGDEBUG("Successfully initialised SafeStore database");
        }
        else
        {
            LOGDEBUG("Failed SafeStore_Init: " << safeStoreReturnCodeToString(result));
        }

        switch (result)
        {
            case SR_OK:
                return InitReturnCode::OK;
            case SR_INVALID_ARG:
                return InitReturnCode::INVALID_ARG;
            case SR_UNSUPPORTED_OS:
                return InitReturnCode::UNSUPPORTED_OS;
            case SR_UNSUPPORTED_VERSION:
                return InitReturnCode::UNSUPPORTED_VERSION;
            case SR_OUT_OF_MEMORY:
                return InitReturnCode::OUT_OF_MEMORY;
            case SR_DB_OPEN_FAILED:
                return InitReturnCode::DB_OPEN_FAILED;
            case SR_DB_ERROR:
                return InitReturnCode::DB_ERROR;
            default:
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
        m_safeStoreHolder(std::move(safeStoreHolder))
    {
    }

    void SafeStoreReleaseMethodsImpl::releaseObjectHandle(SafeStoreObjectHandle objectHandle)
    {
        if (objectHandle != nullptr)
        {
            auto returnCode = SafeStore_ReleaseObjectHandle(objectHandle);
            objectHandle = nullptr;
            if (returnCode == SR_OK)
            {
                LOGDEBUG("Got OK when cleaning up SafeStore object handle");
            }
            else
            {
                LOGERROR(
                    "Got " << safeStoreReturnCodeToString(returnCode) << " when cleaning up SafeStore object handle");
            }
        }
    }

    SafeStoreGetIdMethodsImpl::SafeStoreGetIdMethodsImpl(std::shared_ptr<ISafeStoreHolder> safeStoreHolder) :
        m_safeStoreHolder(safeStoreHolder)
    {
    }

    ObjectId SafeStoreGetIdMethodsImpl::getObjectId(SafeStoreObjectHandle objectHandle)
    {
        if (objectHandle == nullptr)
        {
            return {};
        }

        SafeStore_Id_t objectId;
        auto returnCode = SafeStore_GetObjectId(objectHandle, &objectId);

        if (returnCode == SR_OK)
        {
            return uuidStringFromSafeStoreId(objectId);
        }
        else
        {
            LOGERROR("Failed to get object ID");
        }

        return {};
    }
} // namespace safestore::SafeStoreWrapper