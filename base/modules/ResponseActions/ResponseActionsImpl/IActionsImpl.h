// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>
namespace ResponseActionsImpl
{
    class IActionsImpl
    {
    public:
        virtual ~IActionsImpl() = default;
        IActionsImpl() = default;

        /**
         * runs action from central
         * @param json of actions
         * @param correlationID of actions
         * @return true
         */
        virtual std::string run(const std::string& actionJson) const = 0;

    };
}
