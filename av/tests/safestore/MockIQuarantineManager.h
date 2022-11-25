// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "safestore/QuarantineManager/IQuarantineManager.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace safestore::QuarantineManager;

class MockIQuarantineManager : public IQuarantineManager
{
public:
    MOCK_METHOD(void, initialise, ());
    MOCK_METHOD(safestore::QuarantineManager::QuarantineManagerState, getState, ());
    MOCK_METHOD(void, setState, (const safestore::QuarantineManager::QuarantineManagerState&));
    MOCK_METHOD(bool, deleteDatabase, ());
    MOCK_METHOD(
        common::CentralEnums::QuarantineResult,
        quarantineFile,
        (const std::string&, const std::string&, const std::string&, const std::string&, datatypes::AutoFd));
    MOCK_METHOD(
        std::vector<FdsObjectIdsPair>,
        extractQuarantinedFiles,
        (datatypes::ISystemCallWrapper & sysCallWrapper));
    MOCK_METHOD(void, rescanDatabase, ());
    MOCK_METHOD(
        std::vector<safestore::SafeStoreWrapper::ObjectId>,
        scanExtractedFilesForRestoreList,
        (std::vector<FdsObjectIdsPair> files));
    MOCK_METHOD(std::optional<scan_messages::RestoreReport>, restoreFile, (const std::string& objectId));
};
