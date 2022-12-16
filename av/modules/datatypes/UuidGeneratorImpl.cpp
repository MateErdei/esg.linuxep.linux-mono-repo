// Copyright 2022 Sophos Limited. All rights reserved.

#include "UuidGeneratorImpl.h"

#include <random>

namespace
{
    std::mutex uuidGeneratorReplaceMutex;
    std::unique_ptr<datatypes::IUuidGenerator> uuidGeneratorGlobal = std::make_unique<datatypes::UuidGeneratorImpl>();
} // namespace

namespace datatypes
{
    std::string UuidGeneratorImpl::generate() const
    {
        static std::random_device device;
        static std::mt19937 rng(device());

        static std::uniform_int_distribution<int> dist(0, 15);

        static constexpr char charOptions[] = { 'a', 'b', 'c', 'd', 'e', 'f', '0', '1',
                                                '2', '3', '4', '5', '6', '7', '8', '9' };

        std::string uuid;
        for (int i = 0; i < 32; i++)
        {
            if (i == 8 || i == 12 || i == 16 || i == 20)
            {
                uuid += "-";
            }
            uuid += charOptions[dist(rng)];
        }
        return uuid;
    }

    const IUuidGenerator& uuidGenerator()
    {
        return *uuidGeneratorGlobal;
    }

    UuidGeneratorReplacer::UuidGeneratorReplacer(std::unique_ptr<IUuidGenerator> uuidGenerator) :
        m_guard{ uuidGeneratorReplaceMutex }
    {
        uuidGeneratorGlobal = std::move(uuidGenerator);
    }

    UuidGeneratorReplacer::~UuidGeneratorReplacer()
    {
        uuidGeneratorGlobal = std::make_unique<datatypes::UuidGeneratorImpl>();
    }
} // namespace datatypes
