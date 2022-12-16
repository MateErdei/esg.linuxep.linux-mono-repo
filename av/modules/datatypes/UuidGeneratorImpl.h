// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "IUuidGenerator.h"

#include <mutex>

namespace datatypes
{
    class UuidGeneratorImpl : public IUuidGenerator
    {
    public:
        [[nodiscard]] std::string generate() const override;
    };

    class UuidGeneratorReplacer
    {
    public:
        explicit UuidGeneratorReplacer(std::unique_ptr<IUuidGenerator> uuidGenerator);
        ~UuidGeneratorReplacer();

    private:
        std::lock_guard<std::mutex> m_guard;
    };
} // namespace datatypes
