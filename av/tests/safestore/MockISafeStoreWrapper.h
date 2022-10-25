// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace safestore::SafeStoreWrapper;

class MockISafeStoreWrapper : public ISafeStoreWrapper
{
public:
    MOCK_METHOD0(createSearchHandleHolder, std::unique_ptr<SearchHandleHolder>());
    MOCK_METHOD0(createObjectHandleHolder, std::unique_ptr<ObjectHandleHolder>());
    MOCK_METHOD3(initialise, InitReturnCode(const std::string&, const std::string&, const std::string&));
    MOCK_METHOD5(
        saveFile,
        SaveFileReturnCode(
            const std::string&,
            const std::string&,
            const std::string&,
            const std::string&,
            ObjectHandleHolder&));
    MOCK_METHOD1(getConfigIntValue, std::optional<uint64_t>(ConfigOption));
    MOCK_METHOD2(setConfigIntValue, bool(ConfigOption, uint64_t));
    MOCK_METHOD1(find, SearchResults(const SafeStoreFilter&));
    MOCK_METHOD1(getObjectName, std::string(const ObjectHandleHolder&));
    MOCK_METHOD1(getObjectId, ObjectIdType(const ObjectHandleHolder&));
    MOCK_METHOD1(getObjectType, ObjectType(const ObjectHandleHolder&));
    MOCK_METHOD1(getObjectStatus, ObjectStatus(const ObjectHandleHolder&));
    MOCK_METHOD1(getObjectThreatId, std::string(const ObjectHandleHolder&));
    MOCK_METHOD1(getObjectThreatName, std::string(const ObjectHandleHolder&));
    MOCK_METHOD1(getObjectStoreTime, int64_t(const ObjectHandleHolder&));
    MOCK_METHOD2(getObjectHandle, bool(const ObjectIdType&, std::shared_ptr<ObjectHandleHolder>));
    MOCK_METHOD1(finaliseObject, bool(ObjectHandleHolder&));
    MOCK_METHOD3(setObjectCustomData, bool(ObjectHandleHolder&, const std::string&, const std::vector<uint8_t>&));
    MOCK_METHOD2(getObjectCustomData, std::vector<uint8_t>(const ObjectHandleHolder&, const std::string&));
    MOCK_METHOD3(setObjectCustomDataString, bool(ObjectHandleHolder&, const std::string&, const std::string&));
    MOCK_METHOD2(getObjectCustomDataString, std::string(ObjectHandleHolder&, const std::string&));
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
