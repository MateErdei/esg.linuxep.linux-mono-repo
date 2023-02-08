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
        virtual void run(const std::string& actionJson,const std::string& correlationID) = 0;

    };
}
