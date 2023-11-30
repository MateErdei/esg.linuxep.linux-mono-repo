// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "IsolationExclusion.h"

#include "Common/Exceptions/IException.h"

namespace Plugin
{
    class NTPPolicyException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };

    class NTPPolicy
    {
    public:
        NTPPolicy() = default;
        explicit NTPPolicy(const std::string& xml);
        [[nodiscard]] const std::string& revId() const;
        [[nodiscard]] const std::vector<IsolationExclusion>& exclusions() const;
    private:
        std::string revId_;
        std::vector<IsolationExclusion> exclusions_;
    };
}
