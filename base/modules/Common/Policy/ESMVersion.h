// Copyright 2023 Sophos All rights reserved.

#pragma once

#include <string>

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

    private:
        std::string name_;
        std::string token_;
};