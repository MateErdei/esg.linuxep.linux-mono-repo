// Copyright 2019-2022, Sophos Limited.  All rights reserved.

#include "safestore/ISafeStoreWrapper.h"
#include <gmock/gmock.h>

class MockISafeStoreWrapper : public safestore::ISafeStoreWrapper
{
public:
    MOCK_METHOD0(createSearchHandleHolder, std::unique_ptr<safestore::SearchHandleHolder>());
    MOCK_METHOD1(releaseSearchHandle, void(safestore::SafeStoreSearchHandle));
    MOCK_METHOD0(createObjectHandleHolder, std::unique_ptr<safestore::ObjectHandleHolder>());
    MOCK_METHOD1(releaseObjectHandle, void(safestore::SafeStoreObjectHandle));
    MOCK_METHOD3(initialise, safestore::InitReturnCode(const std::string&, const std::string&, const std::string&));
    MOCK_METHOD5(
        saveFile,
        safestore::SaveFileReturnCode(
            const std::string&,
            const std::string&,
            const std::string&,
            const std::string&,
            safestore::ObjectHandleHolder&));
    MOCK_METHOD1(getConfigIntValue, std::optional<uint64_t>(safestore::ConfigOption));
    MOCK_METHOD2(setConfigIntValue, bool(safestore::ConfigOption, uint64_t));
    MOCK_METHOD1(find, safestore::SearchResults(const safestore::SafeStoreFilter&));
    MOCK_METHOD3(
        findFirst,
        bool(const safestore::SafeStoreFilter&, safestore::SearchHandleHolder&, safestore::ObjectHandleHolder&));
    MOCK_METHOD2(findNext, bool(safestore::SearchHandleHolder&, safestore::ObjectHandleHolder&));
    MOCK_METHOD1(getObjectName, std::string(safestore::ObjectHandleHolder&));
    MOCK_METHOD1(getObjectId, std::string(safestore::ObjectHandleHolder&));
    MOCK_METHOD2(getObjectHandle, bool(const std::string&, std::shared_ptr<safestore::ObjectHandleHolder>));
    MOCK_METHOD1(finaliseObject, bool(safestore::ObjectHandleHolder&));
};