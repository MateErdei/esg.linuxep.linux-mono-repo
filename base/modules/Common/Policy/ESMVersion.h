// Copyright 2023 Sophos All rights reserved.

#pragma once

#include <string>

namespace Common::Policy
{
    class ESMVersion {
        public:
            ESMVersion(std::string name, std::string token) :
                name_(std::move(name)),
                token_(std::move(token))
            {}

            ESMVersion() = default;

            [[nodiscard]] std::string name() const { return name_; }
            [[nodiscard]] std::string token() const { return token_; }

            bool isValid() const
            {
                bool emptyName = name_.empty();
                bool emptyToken = token_.empty();

                return emptyName == emptyToken;
            }

            bool isEnabled() const
            {
                return !name_.empty() && !token_.empty();
            }

        private:
            std::string name_;
            std::string token_;
    };
}