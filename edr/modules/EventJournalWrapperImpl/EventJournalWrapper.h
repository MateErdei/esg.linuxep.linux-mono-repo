/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

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
        void initialise();

        enum class Subject
        {
            Detections,

            NumSubjects
        };

        struct Entry
        {
            Entry() : producerUniqueID(0), timestamp(0) {}

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

        class Reader
        {
        public:
            Reader();
            Reader(const std::string& location);

            std::string getCurrentJRLForId(const std::string& idFilePath);
            void updateJrl(const std::string& idFilePath, const std::string& jrl);

            std::vector<Entry> getEntries(
                std::vector<Subject> subjectFilter,
                const std::string& jrl);

            std::vector<Entry> getEntries(
                std::vector<Subject> subjectFilter,
                uint32_t limit = 0,
                uint64_t startTime = 0,
                uint64_t endTime = 0);
            bool decode(const std::vector<uint8_t>& data, Detection& detection);

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

        class Writer
        {
        public:
            Writer();
            Writer(const std::string& location);
            Writer(const std::string& location, const std::string& producer);

            bool insert(Subject subject, const std::vector<uint8_t>& data);
            std::vector<uint8_t> encode(const Detection& detection);

            bool insert(const std::string& subject);
            bool encode(std::vector<uint8_t>& data);

        private:
            std::string getNewFilename(const std::string& subject, uint64_t uniqueID, uint64_t timestamp) const;

            std::string getExistingFile(const std::string& subject) const;
            uint64_t readExistingUniqueID(const std::string& file) const;

            void writeRIFFHeader(std::vector<uint8_t>& data, const std::string& subject) const;
            void writePbufHeader(std::vector<uint8_t>& data, uint32_t dataLength, uint64_t producerUniqueID, uint64_t timestamp) const;

            uint64_t getAndIncrementNextUniqueID();

            std::string getSerialisationMethod() const;
            std::string getSerialisationVersion() const;

            static constexpr uint32_t FCC_TYPE_RIFF = 0x46464952; // "RIFF"
            static constexpr uint32_t FCC_TYPE_SJRN = 0x6e726a73; // "sjrn"
            static constexpr uint32_t FCC_TYPE_HDR  = 0x20726468; // "hdr "
            static constexpr uint32_t FCC_TYPE_PBUF = 0x66756270; // "pbuf"
            static constexpr uint32_t RIFF_HEADER_LENGTH = 8;
            static constexpr uint32_t SJRN_HEADER_LENGTH = 14;

            std::string m_location;
            std::string m_producer;

            std::atomic<uint64_t> m_nextUniqueID;

            std::vector<uint8_t> temp;
        };
    } // namespace EventJournalWrapper
} // namespace Common
