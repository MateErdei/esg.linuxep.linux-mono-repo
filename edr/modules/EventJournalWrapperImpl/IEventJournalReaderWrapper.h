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
            virtual u_int32_t getCurrentJRLAttemptsForId(const std::string& trackerFilePath) = 0;
            virtual void updateJRLAttempts(const std::string& trackerFilePath, const u_int32_t attempts) = 0;
            virtual void clearJRLFile(const std::string& filePath) = 0;
            virtual std::vector<Entry> getEntries(std::vector<Subject> subjectFilter, const std::string& jrl, uint32_t limit, bool& moreAvailable) = 0;

            virtual std::vector<Entry> getEntries(std::vector<Subject> subjectFilter, uint64_t startTime, uint64_t endTime, uint32_t limit, bool& moreAvailable) = 0;

            virtual  std::pair<bool, Detection> decode(const std::vector<uint8_t>& data) = 0;
        };
    }
}