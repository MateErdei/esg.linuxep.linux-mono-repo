// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Credentials.h"

#include "Common/ObfuscationImpl/SecureCollection.h"

#include <utility>

namespace Common::Policy
{
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
} // namespace Common::Policy
