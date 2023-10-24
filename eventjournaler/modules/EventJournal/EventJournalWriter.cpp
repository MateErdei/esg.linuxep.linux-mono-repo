// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "EventJournalWriter.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/UtilityImpl/RegexUtilities.h"
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef SPL_BAZEL
#include "cpp_lib/Event.capnp.h"
#else
#include "Event.capnp.h"
#endif
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

        auto path = getOpenFileFromDirectory(m_location, m_producer, subjectName);
        if (path.empty())
        {
            path = Common::FileSystem::join(directory, getNewFilename(subjectName, producerUniqueID, timestamp));
            isNewFile = true;
        }
        else
        {
            auto fileSize = fs->fileSize(path);

            FileInfo info;
            bool shouldCreateFile = false;

            if (readHeader(path, info.header))
            {
                info.size = fileSize;
                checkHeader(info);

                PbufInfo pbufInfo;
                if (readPbufInfo(path, info.header.sjrnLength, pbufInfo, 1))
                {
                    copyPbufInfo(info, pbufInfo);

                    if (((fileSize + PBUF_HEADER_LENGTH + data.size()) > MAX_FILE_SIZE) || shouldCloseFile(info))
                    {
                        LOGDEBUG("Close " << path);
                        FileInfo fullInfo;
                        if (readFileInfo(path, fullInfo))
                        {
                            closeFile(path, fullInfo);
                        }
                        else
                        {
                            LOGWARN("Failed to read full file info from " << path);
                            closeFile(path, info);
                        }
                        shouldCreateFile = true;
                    }
                    else if (shouldRemoveFile(info))
                    {
                        LOGDEBUG("Remove " << path);
                        removeFile(path);
                        shouldCreateFile = true;
                    }
                }
                else
                {
                    LOGDEBUG("Remove invalid file " << path);
                    removeFile(path);
                    shouldCreateFile = true;
                }
            }
            else
            {
                LOGDEBUG("Remove invalid file " << path);
                removeFile(path);
                shouldCreateFile = true;
            }

            if (shouldCreateFile)
            {
                path = Common::FileSystem::join(directory, getNewFilename(subjectName, producerUniqueID, timestamp));
                isNewFile = true;
            }
        }

        if (isNewFile)
        {
            LOGDEBUG("Create " << path);
        }
        else
        {
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

    bool Writer::readFileInfo(const std::string& path, FileInfo& info) const
    {
        if (!readHeader(path, info.header))
        {
            return false;
        }

        info.size = Common::FileSystem::fileSystem()->fileSize(path);

        checkHeader(info);

        PbufInfo pbufInfo;
        if (!readPbufInfo(path, info.header.sjrnLength, pbufInfo))
        {
            return false;
        }

        copyPbufInfo(info, pbufInfo);

        return true;
    }

    void Writer::pruneTruncatedEvents(const std::string& path)
    {
        Header header;
        if (!readHeader(path, header))
        {
            return;
        }

        PbufInfo info;
        if (readPbufInfo(path, header.sjrnLength, info, 0, false))
        {
            if (info.totalLength == 0)
            {
                return;
            }

            uint32_t length = RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + header.sjrnLength + info.totalLength;

            if (truncate(path.c_str(), length) != 0)
            {
                throw std::runtime_error("truncate failed on file: " + path);
            }

            std::fstream f(path, std::ios::binary | std::ios::in | std::ios::out);
            length -= RIFF_HEADER_LENGTH;
            f.seekg(4, std::ios::beg);
            f.write(reinterpret_cast<const char*>(&length), sizeof(length));
        }
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

    uint64_t Writer::parseLastUniqueID(const std::string& subject, const std::string& filename) const
    {
        if (Common::UtilityImpl::StringUtils::startswith(filename, subject) &&
            (Common::UtilityImpl::StringUtils::endswith(filename, ".bin") ||
             Common::UtilityImpl::StringUtils::endswith(filename, ".xz")))
        {
            auto strings = Common::UtilityImpl::StringUtils::splitString(filename, "-");

            if (strings.size() == 5)
            {
                if (strings[0].compare(subject) == 0)
                {
                    try
                    {
                        return std::stoul(strings[2], nullptr, 16);
                    }
                    catch (...)
                    {
                    }
                }
            }
        }

        return 0;
    }

    bool isOpenSubjectFile(const std::string& subject, const std::string& filename)
    {
        auto strings = Common::UtilityImpl::StringUtils::splitString(filename, "-");

        return !Common::UtilityImpl::returnFirstMatch(subject + R"(-([0-9a-f]{16})-([0-9]{18})\.bin)", filename).empty();
    }

    std::string getOpenFileFromDirectory(const std::string& location, const std::string& producer, const std::string& subject)
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

    std::string Writer::getClosedFilePath(const std::string& filepath, const FileInfo& info) const
    {
        std::vector<std::string> list = Common::UtilityImpl::StringUtils::splitString(Common::FileSystem::basename(filepath),"-");
        std::string subject  = list[0];
        std::string firstOss = list[1];
        std::string firstTimestamp = list[2].substr(0,list[2].find_first_of("."));

        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << info.lastProducerID;
        std::string lastOss = oss.str();

        std::string filename = subject + "-" + firstOss + "-" + lastOss + "-" + firstTimestamp + "-" + std::to_string(info.lastTimestamp) + ".bin";
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
                        FileInfo info;
                        if (readFileInfo(file, info))
                        {
                            if (info.lastProducerID > uniqueID)
                            {
                                uniqueID = info.lastProducerID;
                            }

                            if (shouldCloseFile(info))
                            {
                                LOGDEBUG("Close " << file);
                                closeFile(file, info);
                            }
                            else if (shouldRemoveFile(info))
                            {
                                LOGDEBUG("Remove " << file);
                                removeFile(file);
                            }
                        }
                    }
                    else
                    {
                        uint64_t id = parseLastUniqueID(Common::FileSystem::basename(subject), Common::FileSystem::basename(file));
                        if (id > uniqueID)
                        {
                            uniqueID = id;
                        }
                    }
                }
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

    bool Writer::readHeader(const std::string& path, Header& header) const
    {
        if (!Common::FileSystem::fileSystem()->exists(path))
        {
            LOGWARN("File " << Common::FileSystem::basename(path) << " does not exist");
            return false;
        }

        std::ifstream f(path, std::ios::binary | std::ios::in);
        f.seekg(0, std::ios::end);
        size_t bytesRemaining = f.tellg();
        f.seekg(0, std::ios::beg);

        if (bytesRemaining < (RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sizeof(uint16_t)))
        {
            LOGWARN("File " << Common::FileSystem::basename(path) << " not valid");
            return false;
        }

        std::vector<uint8_t> buffer(32);
        f.read(reinterpret_cast<char*>(&buffer[0]), RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sizeof(uint16_t));
        bytesRemaining -= RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sizeof(uint16_t);

        uint32_t riff_fcc = 0;
        uint32_t riff_length = 0;
        uint32_t sjrn_fcc = 0;
        uint32_t hdr_fcc = 0;
        uint32_t sjrn_length = 0;
        uint16_t sjrn_version = 0;
        memcpy(&riff_fcc, &buffer[0], sizeof(riff_fcc));
        memcpy(&riff_length, &buffer[4], sizeof(riff_length));
        memcpy(&sjrn_fcc, &buffer[8], sizeof(sjrn_fcc));
        memcpy(&hdr_fcc, &buffer[12], sizeof(hdr_fcc));
        memcpy(&sjrn_length, &buffer[16], sizeof(sjrn_length));
        memcpy(&sjrn_version, &buffer[20], sizeof(sjrn_version));

        if (riff_fcc != FCC_TYPE_RIFF)
        {
            LOGWARN(
                "File " << Common::FileSystem::basename(path) << " unexpected RIFF type 0x" << std::hex << riff_fcc);
            return false;
        }
        if (sjrn_fcc != FCC_TYPE_SJRN)
        {
            LOGWARN(
                "File " << Common::FileSystem::basename(path) << " unexpected sjrn type 0x" << std::hex << sjrn_fcc);
            return false;
        }
        if (hdr_fcc != FCC_TYPE_HDR)
        {
            LOGWARN("File " << Common::FileSystem::basename(path) << " unexpected hdr type 0x" << std::hex << hdr_fcc);
            return false;
        }

        if (sjrn_length > bytesRemaining)
        {
            LOGWARN("File " << Common::FileSystem::basename(path) << " invalid sjrn length " << sjrn_length);
            return false;
        }

        if (sjrn_length > 0x400) // sanity check
        {
            LOGWARN("File " << Common::FileSystem::basename(path) << " unexpected sjrn length " << sjrn_length);
            return false;
        }


        std::vector<std::string> sjrn_header;
        buffer.resize(sjrn_length);
        f.read(reinterpret_cast<char*>(&buffer[0]), sjrn_length);

        for (auto next = buffer.begin(); next != buffer.end(); )
        {
            if (sjrn_header.size() == 4)
            {
                break;
            }

            auto it = std::find(next, buffer.end(), 0);
            if (it == buffer.end())
            {
                break;
            }

            std::string string(next, it);

            if (string.empty())
            {
                break;
            }

            sjrn_header.push_back(string);

            next = ++it;
        }

        if (sjrn_header.size() < 4)
        {
            LOGWARN("File " << Common::FileSystem::basename(path) << " incomplete sjrn header");
            return false;
        }

        header.producer = sjrn_header[0];
        header.subject = sjrn_header[1];
        header.serialisationMethod = sjrn_header[2];
        header.serialisationVersion = sjrn_header[3];
        header.riffLength = riff_length;
        header.sjrnLength = sjrn_length;
        header.sjrnVersion = sjrn_version;

        return true;
    }

    bool Writer::readPbufInfo(const std::string& path, uint32_t sjrnLength, PbufInfo& info, uint32_t limit, bool logWarnings) const
    {
        std::ifstream f(path, std::ios::binary | std::ios::in);
        f.seekg(0, std::ios::end);
        size_t bytesRemaining = f.tellg();

        if (bytesRemaining < (RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sjrnLength))
        {
            return false;
        }

        f.seekg(RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sjrnLength, std::ios::beg);
        bytesRemaining -= RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sjrnLength;

        std::vector<uint8_t> buffer(32);

        while (bytesRemaining > 0)
        {
            if (bytesRemaining < PBUF_HEADER_LENGTH)
            {
                if (logWarnings)
                {
                    LOGWARN(
                        "File " << Common::FileSystem::basename(path) << " invalid PBUF chunk - " << bytesRemaining
                                << " bytes remaining");
                }
                info.truncated = true;
                break;
            }

            f.read(reinterpret_cast<char*>(&buffer[0]), PBUF_HEADER_LENGTH);
            bytesRemaining -= PBUF_HEADER_LENGTH;

            if (f.eof())
            {
                info.truncated = true;
                break;
            }

            uint32_t fcc = 0;
            uint32_t length = 0;
            uint64_t id = 0;
            int64_t timestamp = 0;
            memcpy(&fcc, &buffer[0], sizeof(fcc));
            memcpy(&length, &buffer[4], sizeof(length));
            memcpy(&id, &buffer[8], sizeof(id));
            memcpy(&timestamp, &buffer[16], sizeof(timestamp));

            if (fcc != FCC_TYPE_PBUF)
            {
                if (logWarnings)
                {
                    LOGWARN(
                        "File " << Common::FileSystem::basename(path) << " unexpected PBUF type 0x" << std::hex << fcc);
                }
                info.truncated = true;
                break;
            }

            if (length < sizeof(id) + sizeof(timestamp))
            {
                if (logWarnings)
                {
                    LOGWARN(
                        "File " << Common::FileSystem::basename(path) << " invalid PBUF length " << length << " - "
                                << bytesRemaining << " bytes remaining");
                }
                info.truncated = true;
                break;
            }

            length -= sizeof(id) + sizeof(timestamp);

            if (length > bytesRemaining)
            {
                if (logWarnings)
                {
                    LOGWARN(
                        "File " << Common::FileSystem::basename(path) << " truncated PBUF at offset "
                                << RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH + sjrnLength + info.totalLength);
                }
                info.truncated = true;
                break;
            }

            if (info.firstProducerID == 0)
            {
                info.firstProducerID = id;
            }
            if (info.firstTimestamp == 0)
            {
                info.firstTimestamp = timestamp;
            }
            info.lastProducerID = id;
            info.lastTimestamp = timestamp;
            info.count++;
            info.totalLength += PBUF_HEADER_LENGTH + length;

            f.seekg(length, std::ios::cur);
            bytesRemaining -= length;

            if (limit && (info.count >= limit))
            {
                break;
            }
        }

        return true;
    }

    void Writer::checkHeader(FileInfo& info) const
    {
        if ((info.header.riffLength + RIFF_HEADER_LENGTH) != info.size)
        {
            info.riffLengthMismatch = true;
            info.anyLengthErrors = true;
        }
    }

    void Writer::copyPbufInfo(FileInfo& info, const PbufInfo& pbufInfo) const
    {
        info.firstProducerID = pbufInfo.firstProducerID;
        info.firstTimestamp = pbufInfo.firstTimestamp;
        info.lastProducerID = pbufInfo.lastProducerID;
        info.lastTimestamp = pbufInfo.lastTimestamp;
        info.numEvents = pbufInfo.count;

        if (pbufInfo.truncated)
        {
            info.truncated = true;
            info.anyLengthErrors = true;
        }
    }

    void Writer::closeFile(const std::string& path, const FileInfo& info) const
    {
        std::string closedFile = getClosedFilePath(path, info);

        try
        {
            Common::FileSystem::fileSystem()->moveFile(path, closedFile);
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to close file: " << path << " due to error: " << exception.what());
        }
    }

    void Writer::removeFile(const std::string& path) const
    {
        try
        {
            Common::FileSystem::fileSystem()->removeFile(path, true);
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to remove file: " << path << " due to error: " << exception.what());
        }
    }

    bool Writer::shouldCloseFile(const FileInfo& info) const
    {
        if (shouldRemoveFile(info))
        {
            return false;
        }

        return shouldCloseFile(info.header) || info.anyLengthErrors;
    }

    bool Writer::shouldCloseFile(const Header& header) const
    {
        return (
            (header.serialisationMethod.compare(getSerialisationMethod()) != 0) ||
            (header.serialisationVersion.compare(getSerialisationVersion()) != 0) ||
            (header.sjrnVersion != SJRN_VERSION));
    }

    bool Writer::shouldRemoveFile(const FileInfo& info) const { return info.numEvents == 0; }

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
