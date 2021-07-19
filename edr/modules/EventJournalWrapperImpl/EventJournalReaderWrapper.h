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
            std::string getCurrentJRLForId(const std::string& idFilePath) override;
            void updateJrl(const std::string& idFilePath, const std::string& jrl) override;

            std::vector<Entry> getEntries(
                std::vector<Subject> subjectFilter,
                const std::string& jrl) override;

            std::vector<Entry> getEntries(
                std::vector<Subject> subjectFilter,
                uint32_t limit = 0,
                uint64_t startTime = 0,
                uint64_t endTime = 0) override;

            std::pair<bool, Detection> decode(const std::vector<uint8_t>& data) override;

        private:
            std::vector<Entry> getEntries(
                std::vector<Subject> subjectFilter,
                const std::string& jrl,
                uint32_t limit,
                uint64_t startTime,
                uint64_t endTime);
            std::string m_location;
            std::shared_ptr<Sophos::Journal::HelperInterface> m_helper;
        };

    } // namespace EventJournalWrapper
} // namespace Common
