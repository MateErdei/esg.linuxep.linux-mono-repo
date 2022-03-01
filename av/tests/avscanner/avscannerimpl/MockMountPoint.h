/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "avscanner/mountinfo/IMountPoint.h"
#include "avscanner/mountinfo/IMountInfo.h"
#include "datatypes/sophos_filesystem.h"

#include <gmock/gmock.h>

namespace fs = sophos_filesystem;

namespace
{
    class MockMountPoint : public avscanner::mountinfo::IMountPoint
    {
    public:
        MOCK_CONST_METHOD0(device, std::string());
        MOCK_CONST_METHOD0(filesystemType, std::string());
        MOCK_CONST_METHOD0(isHardDisc, bool());
        MOCK_CONST_METHOD0(isNetwork, bool());
        MOCK_CONST_METHOD0(isOptical, bool());
        MOCK_CONST_METHOD0(isRemovable, bool());
        MOCK_CONST_METHOD0(isSpecial, bool());
        MOCK_CONST_METHOD0(mountPoint, std::string());
    };

    class FakeMountPoint : public avscanner::mountinfo::IMountPoint
    {
    public:
        std::string m_mountPoint;
        enum type { hardDisc, special };
        type m_type;

        explicit FakeMountPoint(const fs::path& fakeMount, const type mountType=type::hardDisc)
            : m_mountPoint(fakeMount),
            m_type(mountType)
        {}

        [[nodiscard]] std::string device() const override
        {
            return {};
        }

        [[nodiscard]] std::string filesystemType() const override
        {
            return {};
        }

        [[nodiscard]] bool isHardDisc() const override
        {
            return m_type == type::hardDisc;
        }

        [[nodiscard]] bool isNetwork() const override
        {
            return false;
        }

        [[nodiscard]] bool isOptical() const override
        {
            return false;
        }

        [[nodiscard]] bool isRemovable() const override
        {
            return false;
        }

        [[nodiscard]] bool isSpecial() const override
        {
            return m_type == type::special;
        }

        [[nodiscard]] std::string mountPoint() const override
        {
            return m_mountPoint;
        }
    };

    class FakeMountInfo : public avscanner::mountinfo::IMountInfo
    {
    public:
        std::vector<std::shared_ptr<avscanner::mountinfo::IMountPoint>> m_mountPoints;
        std::vector<std::shared_ptr<avscanner::mountinfo::IMountPoint>> mountPoints() override
        {
            return m_mountPoints;
        }
    };
}