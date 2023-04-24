/***********************************************************************************************

Copyright 2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "IEventJournalReaderWrapper.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace Sophos
{
    namespace Journal
    {
        class HelperInterface;
        class ViewInterface;
    }
} // namespace Sophos

namespace Common
{
    namespace EventJournalWrapper
    {
        class Reader : public IEventJournalReaderWrapper
        {
        public:
            Reader();
            Reader(const std::string& location);
            Reader(std::shared_ptr<Sophos::Journal::HelperInterface> myHelper);
            std::string getCurrentJRLForId(const std::string& idFilePath) override;
            void updateJrl(const std::string& idFilePath, const std::string& jrl) override;
            u_int32_t getCurrentJRLAttemptsForId(const std::string& trackerFilePath) override;
            void updateJRLAttempts(const std::string& trackerFilePath, const u_int32_t attempts) override;
            void clearJRLFile(const std::string& filePath) override;
            std::vector<Entry> getEntries(std::vector<Subject> subjectFilter, uint64_t startTime, uint64_t endTime, uint32_t maxMemoryThreshold, bool& moreAvailable) override;
            std::vector<Entry> getEntries(std::vector<Subject> subjectFilter, const std::string& jrl, uint32_t maxMemoryThreshold, bool& moreAvailable) override;

            std::pair<bool, Detection> decode(const std::vector<uint8_t>& data) override;

        private:
            std::vector<Entry> getEntries(std::vector<Subject> subjectFilter, const std::string& jrl,  uint64_t startTime, uint64_t endTime, uint32_t maxMemoryThreshold, bool& moreAvailable);
            std::shared_ptr<Sophos::Journal::ViewInterface> getJournalView(std::vector<Subject> subjectFilter, const std::string& jrl, uint64_t startTime, uint64_t endTime);
            size_t getEntrySize(std::shared_ptr<Sophos::Journal::ViewInterface> journalView);
            std::string m_location;
            std::shared_ptr<Sophos::Journal::HelperInterface> m_helper;
        };

    } // namespace EventJournalWrapper
} // namespace Common
