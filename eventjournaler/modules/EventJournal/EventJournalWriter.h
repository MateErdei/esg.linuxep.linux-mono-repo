/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "IEventJournalWriter.h"

#include <atomic>
#include <string>
#include <vector>

namespace EventJournal
{
    struct Detection
    {
        std::string subType;
        std::string data;
    };

    std::vector<uint8_t> encode(const Detection& detection);


    class Writer : public IEventJournalWriter
    {
    public:
        Writer();
        Writer(const std::string& location);
        Writer(const std::string& location, const std::string& producer);

        void insert(Subject subject, const std::vector<uint8_t>& data) override;

    private:
        static constexpr uint32_t FCC_TYPE_RIFF = 0x46464952; // "RIFF"
        static constexpr uint32_t FCC_TYPE_SJRN = 0x6e726a73; // "sjrn"
        static constexpr uint32_t FCC_TYPE_HDR  = 0x20726468; // "hdr "
        static constexpr uint32_t FCC_TYPE_PBUF = 0x66756270; // "pbuf"
        static constexpr uint32_t RIFF_HEADER_LENGTH = 8;
        static constexpr uint32_t SJRN_HEADER_LENGTH = 12;
        static constexpr uint16_t SJRN_VERSION       = 1;
        static constexpr uint32_t PBUF_HEADER_LENGTH = 24;
        static constexpr uint32_t MAX_RECORD_LENGTH  = 0x31000; // 196K
        static constexpr uint32_t MAX_FILE_SIZE      = 100000000;

        std::string getNewFilename(const std::string& subject, uint64_t uniqueID, uint64_t timestamp) const;
        bool isSubjectFile(const std::string& subject, const std::string& filename) const;

        std::string getExistingFile(const std::string& subject) const;
        std::string getClosedFilePath(const std::string& filepath) const;
        uint64_t readHighestUniqueID() const;
        uint64_t readHighestUniqueID(const std::string& file) const;

        void writeRIFFAndSJRNHeader(std::vector<uint8_t>& data, const std::string& subject) const;
        void appendPbufHeader(std::vector<uint8_t>& data, uint32_t length, uint64_t producerUniqueID, uint64_t timestamp) const;

        uint64_t getAndIncrementNextUniqueID();

        uint32_t get64bitAlignedLength(uint32_t length) const;
        bool is64bitAligned(uint32_t length) const;

        std::string getSubjectName(Subject subject) const;

        std::string getSerialisationMethod() const;
        std::string getSerialisationVersion() const;

        std::string m_location;
        std::string m_producer;

        std::atomic<uint64_t> m_nextUniqueID;
    };
} // namespace EventJournalWriter
