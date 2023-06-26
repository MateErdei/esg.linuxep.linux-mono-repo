// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "Common/ObfuscationImpl/SecureCollection.h"

#include <string>
#include <utility>

namespace Common::Policy
{
    class Credentials
    {
    public:
        explicit Credentials(std::string username="", std::string password="")
            : username_(std::move(username)),
              password_(std::move(password))
        {}

        [[nodiscard]] const std::string& getUsername() const
        {
            return username_;
        }

        [[nodiscard]] const std::string& getPassword() const
        {
            return password_;
        }

        bool operator==(const Credentials& rhs) const
        {
            return (username_ == rhs.username_ && password_ == rhs.password_);
        }

        bool operator!=(const Credentials& rhs) const { return !operator==(rhs); }

    private:
        std::string username_;
        std::string password_;
    };

    class ProxyCredentials : public Credentials
    {
    public:
        explicit ProxyCredentials(
            const std::string& username = "",
            const std::string& password = "",
            std::string m_proxyType = "");

        [[nodiscard]] Common::ObfuscationImpl::SecureString getDeobfuscatedPassword() const;
        [[nodiscard]] const std::string& getProxyType() const;

        bool operator==(const ProxyCredentials& rhs) const
        {
            return (Credentials::operator==(rhs) && m_proxyType == rhs.m_proxyType);
        }

        bool operator!=(const ProxyCredentials& rhs) const { return !operator==(rhs); }

    private:
        std::string m_proxyType;
    };
}
