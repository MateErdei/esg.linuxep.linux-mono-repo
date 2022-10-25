/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>

namespace mount_monitor::mountinfo
{
    class IMountPoint
    {

    public:
        inline IMountPoint() = default;

        inline virtual ~IMountPoint() = default;

        [[nodiscard]] virtual std::string device() const = 0;

        [[nodiscard]] virtual std::string filesystemType() const = 0;

        [[nodiscard]] virtual bool isHardDisc() const = 0;

        [[nodiscard]] virtual bool isNetwork() const = 0;

        [[nodiscard]] virtual bool isOptical() const = 0;

        [[nodiscard]] virtual bool isRemovable() const = 0;

        /**
         * @return true if this is a special filesystem mount that we should avoid
         * scanning.
         */
        [[nodiscard]] virtual bool isSpecial() const = 0;

        [[nodiscard]] virtual std::string mountPoint() const = 0;

        [[nodiscard]] virtual bool isDirectory() const = 0;

        [[nodiscard]] virtual bool isReadOnly() const = 0;

    };

    using IMountPointSharedPtr = std::shared_ptr<IMountPoint>;
}
