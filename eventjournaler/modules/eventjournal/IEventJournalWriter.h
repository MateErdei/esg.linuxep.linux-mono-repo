/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <vector>

namespace EventJournal
{
    enum class Subject
    {
        Detections,

        NumSubjects
    };

    class IWriter
    {
    public:
        virtual ~IWriter() = default;

        virtual void insert(Subject subject, const std::vector<uint8_t>& data) = 0;
    };
} // namespace EventJournal
