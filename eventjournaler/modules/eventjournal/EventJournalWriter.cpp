/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "EventJournalWriter.h"

#include "EventJournalTimeUtils.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <capnp/message.h>
#include <capnp/serialize.h>

#include <Event.capnp.h>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace EventJournal
{
    std::vector<uint8_t> encode(const Detection& detection)
    {
        capnp::MallocMessageBuilder builder;

        auto event = builder.initRoot<Sophos::Journal::Event>();
        auto json = event.initJson();
        json.setSubType(detection.subType);
        json.setData(detection.data);

        auto serialised = messageToFlatArray(builder);
        auto bytes = serialised.asBytes();

        return std::vector<uint8_t>(bytes.begin(), bytes.end());
    }

    Writer::Writer() : Writer("", "") {}

    Writer::Writer(const std::string& location) : Writer(location, "") {}

    Writer::Writer(const std::string& location, const std::string& producer) :
        m_location(location), m_producer(producer), m_nextUniqueID(1)
    {
        if (m_location.empty())
        {
            m_location = Common::ApplicationConfiguration::applicationPathManager().getEventJournalsPath();
        }

        if (m_producer.empty())
        {
            m_producer = "SophosSPL";
        }
    }

    void Writer::insert(Subject subject, const std::vector<uint8_t>& data)
    {
        std::string subjectName = getSubjectName(subject);

        if (!is64bitAligned(data.size()))
        {
            throw std::runtime_error("input data not 64-bit aligned");
        }

        auto directory = Common::FileSystem::join(m_location, m_producer, subjectName);
        if (!Common::FileSystem::fileSystem()->exists(directory))
        {
            Common::FileSystem::fileSystem()->makedirs(directory);
        }

        auto path = getExistingFile(subjectName);
        if (!path.empty())
        {
            uint64_t uniqueID = readHighestUniqueID(path);
            if (uniqueID == 0)
            {
                LOGWARN("File " << Common::FileSystem::basename(path) << " does not contain any events");
                path.clear();
                return;
            }
            else
            {
                m_nextUniqueID.exchange(uniqueID + 1);
            }
        }

        time_t now = time(NULL);
        uint64_t producerUniqueID = getAndIncrementNextUniqueID();
        int64_t timestamp = UNIXToWindowsFileTime(now);
        bool isNewFile = false;

        if (path.empty())
        {
            path = Common::FileSystem::join(directory, getNewFilename(subjectName, producerUniqueID, timestamp));
            isNewFile = true;
            LOGDEBUG("Create " << path);
        }
        else
        {
            LOGDEBUG("Update " << path);
        }

        std::ios::openmode mode = std::ios::binary | std::ios::in | std::ios::out;
        if (Common::FileSystem::fileSystem()->exists(path))
            mode |= std::ios::ate;
        else
            mode |= std::ios::trunc;
        std::fstream f(path, mode);

        std::vector<uint8_t> contents;
        if (isNewFile)
        {
            writeRIFFAndSJRNHeader(contents, subjectName);
        }
        appendPbufHeader(contents, data.size(), producerUniqueID, timestamp);
        contents.insert(contents.end(), data.begin(), data.end());
        f.write(reinterpret_cast<const char*>(contents.data()), contents.size());

        uint32_t length = f.tellg();
        length -= RIFF_HEADER_LENGTH;
        f.seekg(4, std::ios::beg);
        f.write(reinterpret_cast<const char*>(&length), sizeof(length));
    }

    std::string Writer::getNewFilename(const std::string& subject, uint64_t uniqueID, uint64_t timestamp) const
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << uniqueID;

        return subject + "-" + oss.str() + "-" + std::to_string(timestamp) + ".bin";
    }

    std::string Writer::getExistingFile(const std::string& subject) const
    {
        auto subjectDirectory = Common::FileSystem::join(m_location, m_producer, subject);
        if (Common::FileSystem::fileSystem()->isDirectory(subjectDirectory))
        {
            auto files = Common::FileSystem::fileSystem()->listFiles(subjectDirectory);
            for (const auto& file : files)
            {
                if (Common::UtilityImpl::StringUtils::endswith(file, ".bin"))
                {
                    return file;
                }
            }
        }

        return "";
    }

    uint64_t Writer::readHighestUniqueID(const std::string& file) const
    {
        std::vector<uint8_t> buffer(2 * MAX_RECORD_LENGTH);

        std::ifstream f(file, std::ios::binary | std::ios::in);
        f.seekg(0, std::ios::end);
        size_t fileSize = f.tellg();
        f.seekg(0, std::ios::beg);

        size_t bytesRemaining = fileSize;

        if (bytesRemaining < (RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH))
        {
            LOGWARN("File " << Common::FileSystem::basename(file) << " not valid");
            return 0;
        }

        f.read(reinterpret_cast<char*>(&buffer[0]), RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH);
        bytesRemaining -= RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH;

        uint32_t fcc = 0;
        uint32_t length = 0;
        memcpy(&fcc, &buffer[0], sizeof(fcc));
        memcpy(&length, &buffer[4], sizeof(length));
        if (fcc != FCC_TYPE_RIFF)
        {
            LOGWARN("File " << Common::FileSystem::basename(file) << " unexpected RIFF type 0x" << std::hex << fcc);
            return 0;
        }

        if (length != (fileSize - RIFF_HEADER_LENGTH))
        {
            LOGWARN(
                "File " << Common::FileSystem::basename(file) << " invalid RIFF length " << length << " (file size "
                        << fileSize << ")");
            return 0;
        }

        uint32_t sjrn_length = 0;
        memcpy(&sjrn_length, &buffer[16], sizeof(sjrn_length));
        sjrn_length = get64bitAlignedLength(sjrn_length);

        if (bytesRemaining < sjrn_length)
        {
            LOGWARN("File " << Common::FileSystem::basename(file) << " invalid SJRN length " << sjrn_length);
            return 0;
        }

        sjrn_length -= sizeof(sjrn_length);
        f.read(reinterpret_cast<char*>(&buffer[0]), sjrn_length);
        bytesRemaining -= sjrn_length;

        uint64_t uniqueID = 0;

        while (bytesRemaining > 0)
        {
            if (bytesRemaining < PBUF_HEADER_LENGTH)
            {
                LOGWARN(
                    "File " << Common::FileSystem::basename(file) << " invalid PBUF chunk - " << bytesRemaining
                            << " bytes remaining");
                break;
            }

            f.read(reinterpret_cast<char*>(&buffer[0]), PBUF_HEADER_LENGTH);
            bytesRemaining -= PBUF_HEADER_LENGTH;

            uint64_t id = 0;
            int64_t timestamp = 0;
            memcpy(&fcc, &buffer[0], sizeof(fcc));
            memcpy(&length, &buffer[4], sizeof(length));
            memcpy(&id, &buffer[8], sizeof(id));
            memcpy(&timestamp, &buffer[16], sizeof(timestamp));

            if (fcc != FCC_TYPE_PBUF)
            {
                LOGWARN("File " << Common::FileSystem::basename(file) << " unexpected PBUF type 0x" << std::hex << fcc);
                break;
            }

            if (bytesRemaining < (length - sizeof(id) - sizeof(timestamp)))
            {
                LOGWARN(
                    "File " << Common::FileSystem::basename(file) << " invalid PBUF length " << length << " - "
                            << bytesRemaining << " bytes remaining");
                break;
            }

            length -= sizeof(id) + sizeof(timestamp);
            f.read(reinterpret_cast<char*>(&buffer[0]), length);
            bytesRemaining -= length;

            if (id <= uniqueID)
            {
                LOGWARN(
                    "File " << Common::FileSystem::basename(file) << " unique ID " << id
                            << " out-of-sequence - previous ID " << uniqueID);
                break;
            }

            uniqueID = id;

            if (f.eof())
            {
                break;
            }
        }

        return uniqueID;
    }

    void Writer::writeRIFFAndSJRNHeader(std::vector<uint8_t>& data, const std::string& subject) const
    {
        const std::string serialisationMethod = getSerialisationMethod();
        const std::string serialisationVersion = getSerialisationVersion();

        uint32_t headerLength = RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sizeof(uint16_t) + m_producer.length() + 1 +
                                subject.length() + 1 + serialisationMethod.length() + 1 +
                                serialisationVersion.length() + 1;
        headerLength = get64bitAlignedLength(headerLength);
        data.reserve(headerLength);

        uint32_t riff = FCC_TYPE_RIFF;
        uint32_t riff_length = 0;
        uint32_t sjrn = FCC_TYPE_SJRN;
        uint32_t hdr = FCC_TYPE_HDR;
        uint32_t sjrn_length = headerLength - RIFF_HEADER_LENGTH - SJRN_HEADER_LENGTH;
        uint16_t version = SJRN_VERSION;

        data.resize(RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sizeof(uint16_t));
        memcpy(&data[0], &riff, sizeof(riff));
        memcpy(&data[4], &riff_length, sizeof(riff_length));
        memcpy(&data[8], &sjrn, sizeof(sjrn));
        memcpy(&data[12], &hdr, sizeof(hdr));
        memcpy(&data[16], &sjrn_length, sizeof(sjrn_length));
        memcpy(&data[20], &version, sizeof(version));

        data.insert(data.end(), m_producer.begin(), m_producer.end());
        data.push_back(0);
        data.insert(data.end(), subject.begin(), subject.end());
        data.push_back(0);
        data.insert(data.end(), serialisationMethod.begin(), serialisationMethod.end());
        data.push_back(0);
        data.insert(data.end(), serialisationVersion.begin(), serialisationVersion.end());
        data.push_back(0);

        data.resize(headerLength);
    }

    void Writer::appendPbufHeader(
        std::vector<uint8_t>& data,
        uint32_t length,
        uint64_t producerUniqueID,
        uint64_t timestamp) const
    {
        uint32_t fcc = FCC_TYPE_PBUF;

        length += sizeof(producerUniqueID) + sizeof(timestamp);

        std::vector<uint8_t> header(PBUF_HEADER_LENGTH);
        memcpy(&header[0], &fcc, sizeof(fcc));
        memcpy(&header[4], &length, sizeof(length));
        memcpy(&header[8], &producerUniqueID, sizeof(producerUniqueID));
        memcpy(&header[16], &timestamp, sizeof(timestamp));

        data.insert(data.end(), header.begin(), header.end());
    }

    uint64_t Writer::getAndIncrementNextUniqueID() { return m_nextUniqueID.fetch_add(1); }

    uint32_t Writer::get64bitAlignedLength(uint32_t length) const
    {
        if (!is64bitAligned(length))
        {
            length = (length & 0xfffffff8) + 8;
        }

        return length;
    }

    bool Writer::is64bitAligned(uint32_t length) const
    {
        if (length & 0x00000007)
        {
            return false;
        }

        return true;
    }

    std::string Writer::getSubjectName(Subject subject) const
    {
        switch (subject)
        {
            case Subject::Detections:
                return "Detections";
            default:
                throw std::runtime_error("unsupported subject");
        }
    }

    std::string Writer::getSerialisationMethod() const { return "CapnProto"; }

    std::string Writer::getSerialisationVersion() const
    {
        static const std::string version = std::to_string(CAPNP_VERSION_MAJOR) + "." +
                                           std::to_string(CAPNP_VERSION_MINOR) + "." +
                                           std::to_string(CAPNP_VERSION_MICRO);
        return version;
    }
} // namespace EventJournal
