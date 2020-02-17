/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace avscanner::avscannerimpl
{
    class IMountPoint
    {

    public:
        inline IMountPoint() = default;

        inline virtual ~IMountPoint() = default;

        virtual std::string device() const = 0;

        virtual std::string filesystemType() const = 0;

        virtual bool isHardDisc() const = 0;

        virtual bool isNetwork() const = 0;

        virtual bool isOptical() const = 0;

        virtual bool isRemovable() const = 0;

        /**
         * @return true if this is a special filesystem mount that we should avoid
         * scanning.
         */
        virtual bool isSpecial() const = 0;

        virtual std::string mountPoint() const = 0;

    };
}
