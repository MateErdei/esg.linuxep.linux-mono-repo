// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include <string>

namespace Common::Policy
{
    class Credentials
    {
    public:
        Credentials(std::string username = "", std::string password = "");

        [[nodiscard]] const std::string& getUsername() const { return username_; }

        [[nodiscard]] const std::string& getPassword() const { return password_; }

        bool operator==(const Credentials& rhs) const
        {
            return (username_ == rhs.username_ && password_ == rhs.password_);
        }

        bool operator!=(const Credentials& rhs) const { return !operator==(rhs); }

    private:
        std::string username_;
        std::string password_;
    };
}