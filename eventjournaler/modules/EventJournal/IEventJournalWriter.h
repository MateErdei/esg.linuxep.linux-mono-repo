// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace EventJournal
{
    enum class Subject
    {
        Detections,

        NumSubjects
    };

    struct Header
    {
        std::string producer;
        std::string subject;
        std::string serialisationMethod;
        std::string serialisationVersion;
        uint32_t riffLength;
        uint32_t sjrnLength;
        uint16_t sjrnVersion;

        Header() : riffLength(0), sjrnLength(0), sjrnVersion(0) {}
    };

    struct FileInfo
    {
        Header header;
        uint64_t firstProducerID;
        int64_t firstTimestamp;
        uint64_t lastProducerID;
        int64_t lastTimestamp;
        uint32_t numEvents;
        uint32_t size;
        bool riffLengthMismatch;
        bool truncated;
        bool anyLengthErrors;

        FileInfo() :
            firstProducerID(0),
            firstTimestamp(0),
            lastProducerID(0),
            lastTimestamp(0),
            numEvents(0),
            size(0),
            riffLengthMismatch(false),
            truncated(false),
            anyLengthErrors(false)
        {
        }
    };

    class IEventJournalWriter
    {
    public:
        virtual ~IEventJournalWriter() = default;

        virtual void insert(Subject subject, const std::vector<uint8_t>& data) = 0;

        virtual bool readFileInfo(const std::string& path, FileInfo& info) const = 0;
        virtual void pruneTruncatedEvents(const std::string& path) = 0;
    };
} // namespace EventJournal
