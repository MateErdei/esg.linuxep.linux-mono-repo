// Copyright 2021-2023 Sophos Limited. All rights reserved.

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

    std::string getOpenFileFromDirectory(const std::string& location, const std::string& producer, const std::string& subject);
    bool isSubjectFile(const std::string& subject, const std::string& filename);
    bool isOpenSubjectFile(const std::string& subject, const std::string& filename);


    class Writer : public IEventJournalWriter
    {
    public:
        Writer();
        Writer(const std::string& location);
        Writer(const std::string& location, const std::string& producer);

        void insert(Subject subject, const std::vector<uint8_t>& data) override;

        bool readFileInfo(const std::string& path, FileInfo& info) const override;
        void pruneTruncatedEvents(const std::string& path) override;

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

        struct PbufInfo
        {
            uint64_t firstProducerID;
            int64_t firstTimestamp;
            uint64_t lastProducerID;
            int64_t lastTimestamp;
            uint32_t count;
            uint32_t totalLength;
            bool truncated;

            PbufInfo() :
                firstProducerID(0),
                firstTimestamp(0),
                lastProducerID(0),
                lastTimestamp(0),
                count(0),
                totalLength(0),
                truncated(false)
            {
            }
        };

        std::string getNewFilename(const std::string& subject, uint64_t uniqueID, uint64_t timestamp) const;
        uint64_t parseLastUniqueID(const std::string& subject, const std::string& filename) const;

        std::string getClosedFilePath(const std::string& filepath, const FileInfo& header) const;
        uint64_t readHighestUniqueID() const;

        void writeRIFFAndSJRNHeader(std::vector<uint8_t>& data, const std::string& subject) const;
        void appendPbufHeader(
            std::vector<uint8_t>& data,
            uint32_t length,
            uint64_t producerUniqueID,
            uint64_t timestamp) const;

        bool readHeader(const std::string& path, Header& header) const;
        bool readPbufInfo(const std::string& path, uint32_t sjrnLength, PbufInfo& info, uint32_t limit = 0, bool logWarnings = true) const;
        void checkHeader(FileInfo& info) const;
        void copyPbufInfo(FileInfo& info, const PbufInfo& pbufInfo) const;

        void closeFile(const std::string& path, const FileInfo& header) const;
        void removeFile(const std::string& path) const;
        bool shouldCloseFile(const FileInfo& info) const;
        bool shouldCloseFile(const Header& header) const;
        bool shouldRemoveFile(const FileInfo& info) const;

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
