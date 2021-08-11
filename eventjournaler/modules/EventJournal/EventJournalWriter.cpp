/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "EventJournalWriter.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/RegexUtilities.h>
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <sys/stat.h>

#include <Event.capnp.h>
#include <cstddef>
#include <cstring>
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

        uint64_t lastUniqueID = readHighestUniqueID();
        if (lastUniqueID != 0)
        {
            LOGDEBUG("last producer unique ID " << lastUniqueID);
            m_nextUniqueID.exchange(lastUniqueID + 1);
        }
    }

    void Writer::insert(Subject subject, const std::vector<uint8_t>& data)
    {
        std::string subjectName = getSubjectName(subject);

        if (!is64bitAligned(data.size()))
        {
            throw std::runtime_error("input data not 64-bit aligned");
        }

        if ((data.size() + PBUF_HEADER_LENGTH) > MAX_RECORD_LENGTH)
        {
            throw std::runtime_error("input data exceeds maximum record length");
        }

        auto fs = Common::FileSystem::fileSystem();
        auto directory = Common::FileSystem::join(m_location, m_producer, subjectName);
        if (!fs->exists(directory))
        {
            Common::FileSystem::fileSystem()->makedirs(directory);

            mode_t directoryPermissions = S_IXUSR | S_IXGRP | S_IRUSR | S_IWUSR | S_IRGRP;
            Common::FileSystem::filePermissions()->chmod(m_location, directoryPermissions);
            Common::FileSystem::filePermissions()->chmod(Common::FileSystem::join(m_location, m_producer), directoryPermissions);
            Common::FileSystem::filePermissions()->chmod(directory, directoryPermissions);
        }

        time_t now = time(NULL);
        uint64_t producerUniqueID = getAndIncrementNextUniqueID();
        int64_t timestamp = Common::UtilityImpl::TimeUtils::EpochToWindowsFileTime(now);
        bool isNewFile = false;

        auto path = getExistingFileFromDirectory(m_location, m_producer, subjectName);
        if (path.empty())
        {
            path = Common::FileSystem::join(directory, getNewFilename(subjectName, producerUniqueID, timestamp));
            isNewFile = true;
            LOGDEBUG("Create " << path);
        }
        else
        {
            auto fileSize = fs->fileSize(path);
            if ((fileSize + PBUF_HEADER_LENGTH + data.size()) > MAX_FILE_SIZE)
            {
                std::string closedFile = getClosedFilePath(path);
                try
                {
                    fs->moveFile(path, closedFile);
                }
                catch (Common::FileSystem::IFileSystemException& exception)
                {
                    LOGERROR("Failed to close file: "<< path<< " due to error: " << exception.what());
                }
                path = Common::FileSystem::join(directory, getNewFilename(subjectName, producerUniqueID, timestamp));
                isNewFile = true;
            }

            LOGDEBUG("Update " << path);
        }

        std::ios::openmode mode = std::ios::binary | std::ios::in | std::ios::out;
        if (fs->exists(path))
        {
            mode |= std::ios::ate;
        }
        else
        {
            mode |= std::ios::trunc;
        }
        std::fstream f(path, mode);

        std::vector<uint8_t> contents;
        if (isNewFile)
        {
            Common::FileSystem::filePermissions()->chmod(path, S_IRUSR | S_IWUSR | S_IRGRP);
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

    bool isSubjectFile(const std::string& subject, const std::string& filename)
    {
        return Common::UtilityImpl::StringUtils::startswith(filename, subject) &&
               Common::UtilityImpl::StringUtils::endswith(filename, ".bin");
    }

    bool isOpenSubjectFile(const std::string& subject, const std::string& filename)
    {
        auto strings = Common::UtilityImpl::StringUtils::splitString(filename, "-");

        return !Common::UtilityImpl::returnFirstMatch(subject + R"(-([0-9a-f]{16})-([0-9]{18})\.bin)", filename).empty();
    }

    std::string getExistingFileFromDirectory(const std::string& location, const std::string& producer, const std::string& subject)
    {
        auto subjectDirectory = Common::FileSystem::join(location, producer, subject);
        if (Common::FileSystem::fileSystem()->isDirectory(subjectDirectory))
        {
            auto files = Common::FileSystem::fileSystem()->listFiles(subjectDirectory);
            for (const auto& file : files)
            {
                if (isOpenSubjectFile(subject, Common::FileSystem::basename(file)))
                {
                    return file;
                }
            }
        }

        return "";
    }

    std::string Writer::getClosedFilePath(const std::string& filepath) const
    {
        std::vector<std::string> list = Common::UtilityImpl::StringUtils::splitString(Common::FileSystem::basename(filepath),"-");
        std::string subject  = list[0];
        std::string firstOss = list[1];;
        std::string firstTimestamp = list[2].substr(0,list[2].find_first_of("."));

        std::ostringstream oss;
        uint64_t lastUniqueId = 0;
        int64_t lasttimestamp= 0;
        readLastUniqueIDAndTimestamp(filepath, lastUniqueId, lasttimestamp);
        oss << std::hex << std::setfill('0') << std::setw(16) << lastUniqueId;
        std::string lastOss = oss.str();

        std::string filename = subject + "-" + firstOss + "-" + lastOss+ "-" + firstTimestamp+ "-" +std::to_string(lasttimestamp) + ".bin";
        return Common::FileSystem::join(Common::FileSystem::dirName(filepath),filename);
    }

    uint64_t Writer::readHighestUniqueID() const
    {
        uint64_t uniqueID = 0;

        auto directory = Common::FileSystem::join(m_location, m_producer);
        if (Common::FileSystem::fileSystem()->exists(directory))
        {
            auto subjects = Common::FileSystem::fileSystem()->listDirectories(directory);
            for (const auto& subject : subjects)
            {
                auto files = Common::FileSystem::fileSystem()->listFiles(subject);
                for (const auto& file : files)
                {
                    if (isSubjectFile(Common::FileSystem::basename(subject), Common::FileSystem::basename(file)))
                    {
                        int64_t timestamp =0;
                        uint64_t id = 0;
                        if (readLastUniqueIDAndTimestamp(file, id, timestamp))
                        {
                            if (id > uniqueID)
                            {
                                uniqueID = id;
                            }
                        }
                    }
                }
            }
        }

        return uniqueID;
    }

    bool Writer::readLastUniqueIDAndTimestamp(const std::string& file, uint64_t& lastUniqueId, int64_t& lastTimestamp) const
    {
        std::vector<uint8_t> buffer(32);

        std::ifstream f(file, std::ios::binary | std::ios::in);
        f.seekg(0, std::ios::end);
        size_t fileSize = f.tellg();
        f.seekg(0, std::ios::beg);

        size_t bytesRemaining = fileSize;

        if (bytesRemaining < (RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH))
        {
            LOGWARN("File " << Common::FileSystem::basename(file) << " not valid");
            return false;
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
            return false;
        }

        if (length != (fileSize - RIFF_HEADER_LENGTH))
        {
            LOGWARN(
                "File " << Common::FileSystem::basename(file) << " invalid RIFF length " << length << " (file size "
                        << fileSize << ")");
            return false;
        }

        uint32_t sjrn_length = 0;
        memcpy(&sjrn_length, &buffer[16], sizeof(sjrn_length));
        sjrn_length = get64bitAlignedLength(sjrn_length);

        if (bytesRemaining < sjrn_length)
        {
            LOGWARN("File " << Common::FileSystem::basename(file) << " invalid SJRN length " << sjrn_length);
            return false;
        }

        sjrn_length -= sizeof(sjrn_length);
        f.seekg(sjrn_length, std::ios::cur);
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
            f.seekg(length, std::ios::cur);
            bytesRemaining -= length;

            if (id <= uniqueID)
            {
                LOGWARN(
                    "File " << Common::FileSystem::basename(file) << " unique ID " << id
                            << " out-of-sequence - previous ID " << uniqueID);
                break;
            }

            lastUniqueId = id;
            lastTimestamp = timestamp;

            if (f.eof())
            {
                break;
            }
        }

        return true;
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
