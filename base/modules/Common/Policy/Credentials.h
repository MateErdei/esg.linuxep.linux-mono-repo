// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <string>

namespace Common::Policy
{
    class Credentials
    {
    public:
        explicit Credentials(std::string username = "", std::string password = "") :
            username_(std::move(username)), password_(std::move(password))
        {
        }

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