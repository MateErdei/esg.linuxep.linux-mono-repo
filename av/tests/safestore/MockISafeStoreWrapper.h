// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace safestore::SafeStoreWrapper;

class MockISafeStoreWrapper : public ISafeStoreWrapper
{
public:
    MOCK_METHOD(std::unique_ptr<SearchHandleHolder>, createSearchHandleHolder, ());
    MOCK_METHOD(std::unique_ptr<ObjectHandleHolder>, createObjectHandleHolder, ());
    MOCK_METHOD(InitReturnCode, initialise, (const std::string&, const std::string&, const std::string&));
    MOCK_METHOD(
        SaveFileReturnCode,
        saveFile,
        (const std::string&, const std::string&, const std::string&, const std::string&, ObjectHandleHolder&));
    MOCK_METHOD(std::optional<uint64_t>, getConfigIntValue, (ConfigOption));
    MOCK_METHOD(bool, setConfigIntValue, (ConfigOption, uint64_t));
    MOCK_METHOD(SearchResults, find, (const SafeStoreFilter&));
    MOCK_METHOD(std::string, getObjectName, (const ObjectHandleHolder&));
    MOCK_METHOD(ObjectIdType, getObjectId, (const ObjectHandleHolder&));
    MOCK_METHOD(ObjectType, getObjectType, (const ObjectHandleHolder&));
    MOCK_METHOD(ObjectStatus, getObjectStatus, (const ObjectHandleHolder&));
    MOCK_METHOD(std::string, getObjectThreatId, (const ObjectHandleHolder&));
    MOCK_METHOD(std::string, getObjectThreatName, (const ObjectHandleHolder&));
    MOCK_METHOD(int64_t, getObjectStoreTime, (const ObjectHandleHolder&));
    MOCK_METHOD(bool, getObjectHandle, (const ObjectIdType&, std::shared_ptr<ObjectHandleHolder>));
    MOCK_METHOD(bool, finaliseObject, (ObjectHandleHolder&));
    MOCK_METHOD(bool, setObjectCustomData, (ObjectHandleHolder&, const std::string&, const std::vector<uint8_t>&));
    MOCK_METHOD(std::vector<uint8_t>, getObjectCustomData, (const ObjectHandleHolder&, const std::string&));
    MOCK_METHOD(bool, setObjectCustomDataString, (ObjectHandleHolder&, const std::string&, const std::string&));
    MOCK_METHOD(std::string, getObjectCustomDataString, (ObjectHandleHolder&, const std::string&));
    MOCK_METHOD(bool, restoreObjectById, (const ObjectIdType& objectId));
    MOCK_METHOD(bool, restoreObjectByIdToLocation, (const ObjectIdType& objectId, const std::string& path));
    MOCK_METHOD(bool, restoreObjectsByThreatId, (const std::string& threatId));
    MOCK_METHOD(bool, deleteObjectById, (const ObjectIdType& objectId));
    MOCK_METHOD(bool, deleteObjectsByThreatId, (const std::string& threatId));
};

class MockISafeStoreReleaseMethods : public ISafeStoreReleaseMethods
{
public:
    MOCK_METHOD1(releaseObjectHandle, void(SafeStoreObjectHandle));
    MOCK_METHOD1(releaseSearchHandle, void(SafeStoreSearchHandle));
};

class MockISafeStoreGetIdMethods : public ISafeStoreGetIdMethods
{
public:
    MOCK_METHOD1(getObjectId, ObjectIdType(SafeStoreObjectHandle));
};
