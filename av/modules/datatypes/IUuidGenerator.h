// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <memory>

namespace datatypes
{
    class IUuidGenerator
    {
    public:
        virtual ~IUuidGenerator() = default;

        [[nodiscard]] virtual std::string generate() const = 0;
    };

    [[nodiscard]] const IUuidGenerator& uuidGenerator();
}