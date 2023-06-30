// Copyright 2023 Sophos All rights reserved.

#pragma once

#include <string>

struct ESMVersion {
    ESMVersion(std::string name, std::string token) :
        name_(std::move(name)),
        token_(std::move(token))
    {}

    ESMVersion() = default;

    std::string name_;
    std::string token_;

    bool isValid()
    {
        bool emptyName = name_.empty();
        bool emptyToken = token_.empty();

        return emptyName == emptyToken;
    }
};