/***********************************************************************************************

Copyright 2021 Sophos Limited. All rights reserved.

***********************************************************************************************/
#include "EventJournalWrapper.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <cstddef>
#include <cstring>

#include <fstream>
#include <iomanip>

#include "EventJournalTimeUtils.h"
#include "Logger.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <Event.capnp.h>
#include <Journal.h>

namespace Common
{
    namespace EventJournalWrapper
    {
        void initialise()
        {
            LOGINFO("Journal initialise");
            auto helper = Sophos::Journal::GetHelper("/home/pair/dataset-01");
            if (!helper)
            {
                LOGINFO("no helper");
                return;
            }

            Sophos::Journal::Subjects subjects;
            subjects.push_back("System");

            std::string jrl;

            {
                auto view = helper->GetJournalView(subjects, Sophos::Journal::MinTime);
                if (!view)
                {
                    LOGINFO("no view");
                    return;
                }

                int i = 0;
                for (auto it = view->cbegin(); it != view->cend(); ++it)
                {
                    auto entry = *it;
                    if (entry)
                        LOGINFO(entry->GetProducerUniqueId() << " " << it.GetJournalResourceLocator());
                    if (i == 2)
                        jrl = it.GetJournalResourceLocator();
                    i++;
                }
            }

            if (!jrl.empty())
            {
                LOGINFO("JRL " << jrl);
                auto view = helper->GetJournalView(subjects, jrl);
                if (!view)
                {
                    LOGINFO("no view");
                    return;
                }

                for (auto it = view->cbegin(); it != view->cend(); ++it)
                {
                    auto entry = *it;
                    if (entry)
                        LOGINFO(entry->GetProducerUniqueId() << " " << it.GetJournalResourceLocator());
                }
            }
        }

        static std::string getSubjectName(Subject subject)
        {
            switch (subject)
            {
                case Subject::Detections: return "Detections";
                default: return "";
            }

            return "";
        }

        Reader::Reader()
        {
            m_helper = Sophos::Journal::GetHelper();
        }

        Reader::Reader(const std::string& location) : m_location(location)
        {
            m_helper = Sophos::Journal::GetHelper(m_location);
        }

        std::string Reader::getCurrentJRLForId(const std::string& idFilePath)
        {
            std::string jrl("");
            auto filesystem = Common::FileSystem::fileSystem();

            if(filesystem->isFile(idFilePath))
            {
                jrl = filesystem->readFile(idFilePath);
            }
            return jrl;
        }
        void Reader::updateJrl(const std::string& idFilePath, const std::string& jrl)
        {
            if(jrl.empty() || idFilePath.empty())
            {
                return;
            }
            auto filesystem = Common::FileSystem::fileSystem();

            if(filesystem->isFile(idFilePath))
            {
                filesystem->removeFile(idFilePath);
            }

            filesystem->writeFile(idFilePath, jrl);
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, const std::string& jrl)
        {
            return getEntries(subjectFilter, jrl, 0,0,0);
        }

        std::vector<Entry> Reader::getEntries(
            std::vector<Subject> subjectFilter,
            uint32_t limit,
            uint64_t startTime,
            uint64_t endTime)
        {
            return getEntries(subjectFilter, "", limit,startTime, endTime);
        }

        std::vector<Entry> Reader::getEntries(
            std::vector<Subject> subjectFilter,
            const std::string& jrl,
            uint32_t limit,
            uint64_t startTime,
            uint64_t endTime
            )
        {
            std::vector<Entry> entries;
            (void)limit;
            (void)startTime;
            (void)endTime;

            std::vector<std::string> subjects;
            for (auto s : subjectFilter)
            {
                subjects.push_back(getSubjectName(s));
            }
            std::shared_ptr <Sophos::Journal::ViewInterface> view;
            if(!jrl.empty())
            {
                view = m_helper->GetJournalView(subjects, jrl);
            }
            else
            {
                view = m_helper->GetJournalView(subjects, Sophos::Journal::MinTime);
            }

            if (!view)
            {
                throw std::runtime_error("");
            }

            for (auto it = view->cbegin(); it != view->cend(); ++it)
            {
                auto entry = *it;
                if (!entry)
                {
                    continue;
                }

                Entry e;
                e.producerUniqueID = entry->GetProducerUniqueId();
                e.timestamp = getWindowsFileTime(entry->GetTimestamp());
                e.jrl = it.GetJournalResourceLocator();
                e.data.resize(entry->GetDataSize());
                memcpy(e.data.data(), entry->GetData(), entry->GetDataSize());
                entries.push_back(e);
            }

            return entries;
        }

        bool Reader::decode(const std::vector<uint8_t>& data, Detection& detection)
        {
            auto array = kj::ArrayPtr<const capnp::word>(reinterpret_cast<const capnp::word*>(data.data()), data.size() / sizeof(capnp::word));
            capnp::FlatArrayMessageReader reader(array);
            auto event = reader.getRoot<Sophos::Journal::Event>();

            if (!event.isJson())
            {
                LOGWARN("unexpected detection event " << event.which());
                return false;
            }

            auto json = event.getJson();
            detection.subType = json.getSubType();
            detection.data = json.getData();

            return true;
        }

        Writer::Writer() : Writer("", "")
        {
        }

        Writer::Writer(const std::string& location) : Writer(location, "")
        {
        }

        Writer::Writer(const std::string& location, const std::string& producer) : m_location(location), m_producer(producer), m_nextUniqueID(1)
        {
            if (m_location.empty())
            {
                std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
                m_location = Common::FileSystem::join(installPath, "plugins/edr/etc");
                //m_location = ApplicationConfiguration::applicationPathManager().getEventJournalsPath();
            }
            if (m_producer.empty())
            {
                m_producer = "SophosSPL";
            }
        }

        bool Writer::insert(const std::string& subject)
        {
            const std::string producer = "SophosED";
            const std::string serialisation_method = "CapnProto";
            const std::string serialisation_version = "0.6.1";
            std::cout << producer.length() << " " << subject.length() << " " << serialisation_method.length() << " "
                      << serialisation_version.length() << std::endl;
            uint32_t length = producer.length() + subject.length() + serialisation_method.length() +
                              serialisation_version.length() + 4;
            std::cout << length << std::endl;
            length += 16 + 6;
            if (length & 0x00000007)
                length = (length & 0xfffffff8) + 8;
            std::cout << length << std::endl;

            uint8_t header[22];

            std::vector<uint8_t> data;
            data.reserve(length);
            std::cout << data.size() << std::endl;

            std::string path = m_location;
            path = Common::FileSystem::join(path, subject);
            std::cout << path << std::endl;
            auto fs = Common::FileSystem::fileSystem();
            std::ios::openmode mode = std::ios::binary | std::ios::in | std::ios::out;
            if (fs->exists(path))
                mode |= std::ios::ate;
            else
                mode |= std::ios::trunc;
            std::fstream f(path, mode);

            uint32_t v = FCC_TYPE_RIFF;
            memcpy(&header[0], &v, sizeof(v));
            v = 0; // length
            memcpy(&header[4], &v, sizeof(v));
            v = FCC_TYPE_SJRN;
            memcpy(&header[8], &v, sizeof(v));
            v = FCC_TYPE_HDR;
            memcpy(&header[12], &v, sizeof(v));
            v = length - 16 - 4;
            memcpy(&header[16], &v, sizeof(v));
            uint16_t version = 1;
            memcpy(&header[20], &version, sizeof(version));
            data.insert(data.end(), header, header + sizeof(header));
            data.insert(data.end(), producer.begin(), producer.end());
            data.push_back(0);
            data.insert(data.end(), subject.begin(), subject.end());
            data.push_back(0);
            data.insert(data.end(), serialisation_method.begin(), serialisation_method.end());
            data.push_back(0);
            data.insert(data.end(), serialisation_version.begin(), serialisation_version.end());
            data.push_back(0);
            data.resize(length);

            uint8_t pbuf[24];
            v = FCC_TYPE_PBUF;
            memcpy(&pbuf[0], &v, sizeof(v));
            v = temp.size() + 16;
            memcpy(&pbuf[4], &v, sizeof(v));
            uint64_t producer_unique_id = 5965440;
            memcpy(&pbuf[8], &producer_unique_id, sizeof(producer_unique_id));
            uint64_t timestamp = 131805542161666681;
            memcpy(&pbuf[16], &timestamp, sizeof(timestamp));

            data.insert(data.end(), pbuf, pbuf + sizeof(pbuf));
            data.insert(data.end(), temp.begin(), temp.end());
            v = data.size() - 8;
            memcpy(&data[4], &v, sizeof(v));
            f.write(reinterpret_cast<const char*>(data.data()), data.size());
            // fseek(f, 4, SEEK_SET);
            // fwrite(&v, 1, sizeof(v), f);
            // fclose(f);
            return true;
        }

        bool Writer::encode(std::vector<uint8_t>& data)
        {
            temp = data;

            std::cout << getSerialisationMethod() << " " << getSerialisationVersion() << " " << getAndIncrementNextUniqueID() << std::endl;

            capnp::MallocMessageBuilder builder;
            auto driver = builder.initRoot<Sophos::Journal::DriverEvent>();
            driver.setEventType(Sophos::Journal::DriverEvent::EventType::LOADED);

            auto serialised = messageToFlatArray(builder);
            auto bytes = serialised.asBytes();
            data.clear();
            data.insert(data.end(), bytes.begin(), bytes.end());

            return true;
        }

        bool Writer::insert(Subject subject, const std::vector<uint8_t>& data)
        {
            std::string subjectName = getSubjectName(subject);
            if (subjectName.empty())
            {
                return false;
            }

            auto directory = FileSystem::join(m_location, m_producer, subjectName);
            if (!FileSystem::fileSystem()->exists(directory))
            {
                FileSystem::fileSystem()->makedirs(directory);
            }

            auto existingFile = getExistingFile(subjectName);
            if (!existingFile.empty())
            {
                uint64_t existingUniqueID = readExistingUniqueID(existingFile);
                std::cout << "existingUniqueID=" << existingUniqueID << std::endl;
                if (existingUniqueID == 0)
                    return false;
                    //m_nextUniqueID.exchange(2);
                else
                    m_nextUniqueID.exchange(existingUniqueID+1);
            }

            time_t now = time(NULL);
            uint64_t producerUniqueID = getAndIncrementNextUniqueID();
            uint64_t timestamp = UNIXToWindowsFileTime(now);
            std::cout << "now=" << timestamp << std::endl;

            auto path = FileSystem::join(directory, getNewFilename(subjectName, producerUniqueID, timestamp));
            if (!existingFile.empty())
                path = existingFile;
            std::cout << path << std::endl;

            std::ios::openmode mode = std::ios::binary|std::ios::in|std::ios::out;
            if (FileSystem::fileSystem()->exists(path))
                mode |= std::ios::ate;
            else
                mode |= std::ios::trunc;
            std::fstream f(path, mode);


            std::vector<uint8_t> contents;
            if (existingFile.empty())
            {
                writeRIFFHeader(contents, subjectName);
            }
            writePbufHeader(contents, data.size(), producerUniqueID, timestamp);
            contents.insert(contents.end(), data.begin(), data.end());
            f.write(reinterpret_cast<const char*>(contents.data()), contents.size());


            uint32_t length = f.tellg();
            std::cout << "length=" << length << std::endl;
            length -= RIFF_HEADER_LENGTH;
            f.seekg(4, std::ios::beg);
            f.write(reinterpret_cast<const char*>(&length), sizeof(length));

            return true;
        }

        std::vector<uint8_t> Writer::encode(const Detection& detection)
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

        std::string Writer::getNewFilename(const std::string& subject, uint64_t uniqueID, uint64_t timestamp) const
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0') << std::setw(16) << uniqueID;

            return subject + "-" + oss.str() + "-" + std::to_string(timestamp) + ".bin";
        }

        std::string Writer::getExistingFile(const std::string& subject) const
        {
            auto subjectDirectory = FileSystem::join(m_location, m_producer, subject);
            if (FileSystem::fileSystem()->isDirectory(subjectDirectory))
            {
                auto files = FileSystem::fileSystem()->listFiles(subjectDirectory);
                for (const auto& file : files)
                {
                    if (UtilityImpl::StringUtils::endswith(file, ".bin"))
                    {
                        return file;
                    }
                }
            }

            return "";
        }

        uint64_t Writer::readExistingUniqueID(const std::string& file) const
        {
            std::ifstream f(file, std::ios::binary|std::ios::in);
            uint32_t fcc = 0;
            uint32_t length = 0;
            f.read(reinterpret_cast<char *>(&fcc), sizeof(fcc));
            f.read(reinterpret_cast<char *>(&length), sizeof(length));
            if (fcc == FCC_TYPE_RIFF)
                std::cout << "RIFF=" << length << std::endl;

            f.seekg(16, std::ios::beg);
            uint32_t sjrn_length = 0;
            f.read(reinterpret_cast<char *>(&sjrn_length), sizeof(sjrn_length));
                std::cout << "sjrn=" << sjrn_length << std::endl;

            return 0;
        }

        void Writer::writeRIFFHeader(std::vector<uint8_t>& data, const std::string& subject) const
        {
            const std::string serialisationMethod = getSerialisationMethod();
            const std::string serialisationVersion = getSerialisationVersion();

            std::cout << std::dec << m_producer.length() << " " << subject.length() << " " << serialisationMethod.length() << " " << serialisationVersion.length() << std::endl;
            uint32_t headerLength = RIFF_HEADER_LENGTH + SJRN_HEADER_LENGTH +
                m_producer.length()+1 +
                subject.length()+1 +
                serialisationMethod.length()+1 +
                serialisationVersion.length()+1;
            std::cout << headerLength << std::endl;
            if (headerLength & 0x00000007)
                headerLength = (headerLength & 0xfffffff8) + 8;
            std::cout << headerLength << std::endl;
            data.reserve(headerLength);

            uint8_t riffHeader[22];
            uint32_t v = FCC_TYPE_RIFF;
            memcpy(&riffHeader[0], &v, sizeof(v));
            v = 0; // length
            memcpy(&riffHeader[4], &v, sizeof(v));
            v = FCC_TYPE_SJRN;
            memcpy(&riffHeader[8], &v, sizeof(v));
            v = FCC_TYPE_HDR;
            memcpy(&riffHeader[12], &v, sizeof(v));
            v = headerLength - 16 - 4;
            memcpy(&riffHeader[16], &v, sizeof(v));
            uint16_t version = 1;
            memcpy(&riffHeader[20], &version, sizeof(version));
            data.insert(data.end(), riffHeader, riffHeader + sizeof(riffHeader));
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

        void Writer::writePbufHeader(std::vector<uint8_t>& data, uint32_t dataLength, uint64_t producerUniqueID, uint64_t timestamp) const
        {
            uint8_t pbuf[24];
            uint32_t v = FCC_TYPE_PBUF;
            memcpy(&pbuf[0], &v, sizeof(v));
            v = dataLength + 16;
            memcpy(&pbuf[4], &v, sizeof(v));
            memcpy(&pbuf[8], &producerUniqueID, sizeof(producerUniqueID));
            memcpy(&pbuf[16], &timestamp, sizeof(timestamp));

            data.insert(data.end(), pbuf, pbuf + sizeof(pbuf));
        }

        uint64_t Writer::getAndIncrementNextUniqueID()
        {
            return m_nextUniqueID.fetch_add(1);
        }

        std::string Writer::getSerialisationMethod() const
        {
            return "CapnProto";
        }

        std::string Writer::getSerialisationVersion() const
        {
            static const std::string version = std::to_string(CAPNP_VERSION_MAJOR) + "." +
                                               std::to_string(CAPNP_VERSION_MINOR) + "." +
                                               std::to_string(CAPNP_VERSION_MICRO);
            return version;
        }

    } // namespace EventJournalWrapper
} // namespace Common
