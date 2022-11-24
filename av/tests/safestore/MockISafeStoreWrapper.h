// Copyright 2022, Sophos Limited. All rights reserved.

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
        (const std::string&, const std::string&, const ThreatId&, const std::string&, ObjectHandleHolder&));
    MOCK_METHOD(std::optional<uint64_t>, getConfigIntValue, (ConfigOption));
    MOCK_METHOD(bool, setConfigIntValue, (ConfigOption, uint64_t));
    MOCK_METHOD(SearchResults, find, (const SafeStoreFilter&));
    MOCK_METHOD(std::string, getObjectName, (const ObjectHandleHolder&));
    MOCK_METHOD(std::string, getObjectLocation, (const ObjectHandleHolder&));
    MOCK_METHOD(ObjectId, getObjectId, (const ObjectHandleHolder&));
    MOCK_METHOD(ObjectType, getObjectType, (const ObjectHandleHolder&));
    MOCK_METHOD(ObjectStatus, getObjectStatus, (const ObjectHandleHolder&));
    MOCK_METHOD(ThreatId, getObjectThreatId, (const ObjectHandleHolder&));
    MOCK_METHOD(std::string, getObjectThreatName, (const ObjectHandleHolder&));
    MOCK_METHOD(int64_t, getObjectStoreTime, (const ObjectHandleHolder&));
    MOCK_METHOD(bool, getObjectHandle, (const ObjectId&, std::shared_ptr<ObjectHandleHolder>));
    MOCK_METHOD(bool, finaliseObject, (ObjectHandleHolder&));
    MOCK_METHOD(bool, setObjectCustomData, (ObjectHandleHolder&, const std::string&, const std::vector<uint8_t>&));
    MOCK_METHOD(std::vector<uint8_t>, getObjectCustomData, (const ObjectHandleHolder&, const std::string&));
    MOCK_METHOD(bool, setObjectCustomDataString, (ObjectHandleHolder&, const std::string&, const std::string&));
    MOCK_METHOD(std::string, getObjectCustomDataString, (ObjectHandleHolder&, const std::string&));
    MOCK_METHOD(bool, restoreObjectById, (const ObjectId& objectId));
    MOCK_METHOD(bool, restoreObjectByIdToLocation, (const ObjectId& objectId, const std::string& path));
    MOCK_METHOD(bool, restoreObjectsByThreatId, (const ThreatId& threatId));
    MOCK_METHOD(bool, deleteObjectById, (const ObjectId& objectId));
    MOCK_METHOD(bool, deleteObjectsByThreatId, (const ThreatId& threatId));
};

class MockISafeStoreReleaseMethods : public ISafeStoreReleaseMethods
{
public:
    MOCK_METHOD(void, releaseObjectHandle, (SafeStoreObjectHandle));
    MOCK_METHOD(void, releaseSearchHandle, (SafeStoreSearchHandle));
};

class MockISafeStoreGetIdMethods : public ISafeStoreGetIdMethods
{
public:
    MOCK_METHOD(ObjectId, getObjectId, (SafeStoreObjectHandle));
};
