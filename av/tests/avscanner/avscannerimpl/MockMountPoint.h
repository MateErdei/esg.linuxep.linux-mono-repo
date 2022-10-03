/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "mount_monitor/mountinfo/IMountPoint.h"
#include "mount_monitor/mountinfo/IMountInfo.h"
#include "datatypes/sophos_filesystem.h"

#include <gmock/gmock.h>

namespace fs = sophos_filesystem;

namespace
{
    class MockMountPoint : public mount_monitor::mountinfo::IMountPoint
    {
    public:
        MOCK_METHOD(std::string, device, (), (const));
        MOCK_METHOD(std::string, filesystemType, (), (const));
        MOCK_METHOD(bool, isHardDisc, (), (const));
        MOCK_METHOD(bool, isNetwork, (), (const));
        MOCK_METHOD(bool, isOptical, (), (const));
        MOCK_METHOD(bool, isRemovable, (), (const));
        MOCK_METHOD(bool, isSpecial, (), (const));
        MOCK_METHOD(bool, isDirectory, (), (const));
        MOCK_METHOD(std::string, mountPoint, (), (const));
    };

    class FakeMountPoint : public mount_monitor::mountinfo::IMountPoint
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

        [[nodiscard]] bool isDirectory() const override
        {
            return true;
        }

        [[nodiscard]] std::string mountPoint() const override
        {
            return m_mountPoint;
        }
    };

    class FakeMountInfo : public mount_monitor::mountinfo::IMountInfo
    {
    public:
        std::vector<std::shared_ptr<mount_monitor::mountinfo::IMountPoint>> m_mountPoints;
        std::vector<std::shared_ptr<mount_monitor::mountinfo::IMountPoint>> mountPoints() override
        {
            return m_mountPoints;
        }
    };
}