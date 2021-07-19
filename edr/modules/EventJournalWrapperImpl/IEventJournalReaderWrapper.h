/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace Common
{
    namespace EventJournalWrapper
    {
        enum class Subject
        {
            Detections,

            NumSubjects
        };

        struct Entry
        {
            Entry() : producerUniqueID(0), timestamp(0)
            {
            }

            uint64_t producerUniqueID;
            uint64_t timestamp;
            std::string jrl;
            std::vector<uint8_t> data;
        };

        struct Detection
        {
            std::string subType;
            std::string data;
        };

        class IEventJournalReaderWrapper
        {
        public:
            virtual ~IEventJournalReaderWrapper() = default;
            virtual std::string getCurrentJRLForId(const std::string& idFilePath) = 0;
            virtual void updateJrl(const std::string& idFilePath, const std::string& jrl) = 0;

            virtual std::vector<Entry> getEntries(std::vector<Subject> subjectFilter, const std::string& jrl) = 0;

            virtual std::vector<Entry> getEntries(
                std::vector<Subject> subjectFilter,
                uint32_t limit,
                uint64_t startTime,
                uint64_t endTime) = 0;

            virtual  std::pair<bool, Detection> decode(const std::vector<uint8_t>& data) = 0;
        };
    }
}