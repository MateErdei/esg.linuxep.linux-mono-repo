// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "mount_monitor/mountinfo/IDeviceUtil.h"

#include <gmock/gmock.h>

namespace
{
    class MockDeviceUtil : public mount_monitor::mountinfo::IDeviceUtil
    {
    public:
        MockDeviceUtil()
        {
            using namespace ::testing;
            ON_CALL(*this, isFloppy).WillByDefault(Return(false));
            ON_CALL(*this, isLocalFixed).WillByDefault(Return(true));
            ON_CALL(*this, isNetwork).WillByDefault(Return(false));
            ON_CALL(*this, isOptical).WillByDefault(Return(false));
            ON_CALL(*this, isRemovable).WillByDefault(Return(false));
            ON_CALL(*this, isSystem).WillByDefault(Return(false));
            ON_CALL(*this, isNotSupported).WillByDefault(Return(false));
            ON_CALL(*this, isCachable).WillByDefault(Return(true));
        }

        MOCK_METHOD(
            bool,
            isFloppy,
            (const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType));
        MOCK_METHOD(
            bool,
            isLocalFixed,
            (const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType));
        MOCK_METHOD(
            bool,
            isNetwork,
            (const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType));
        MOCK_METHOD(
            bool,
            isOptical,
            (const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType));
        MOCK_METHOD(
            bool,
            isRemovable,
            (const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType));
        MOCK_METHOD(
            bool,
            isSystem,
            (const std::string& devicePath, const std::string& mountPoint, const std::string& filesystemType));
        MOCK_METHOD(bool, isNotSupported, (const std::string& filesystemType));
        MOCK_METHOD(bool, isCachable, (int fd));
    };
}