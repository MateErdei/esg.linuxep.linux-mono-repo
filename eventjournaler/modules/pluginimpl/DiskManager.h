// Copyright 2021 Sophos Limited. All rights reserved.

#include "EventWriterWorkerLib/IEventWriterWorker.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"

#include <list>
#include <optional>

#include <lzma.h>

namespace Plugin
{


    class DiskManager
    {

    public:
        struct SubjectFileInfo
        {
            std::string filepath;
            u_int64_t fileId;
            u_int64_t size;
        };
        struct SubjectDirInfo
        {
            std::string subject;
            std::vector<SubjectFileInfo> fileset;
        };
        /**
        * compresses given file using xz format
        * @param filepath, file to be compressed
         */
        bool compressFile(const std::string& filepath, const std::string& temppath);
        uint64_t getDirectorySize(const std::string& dirpath);
        void deleteOldJournalFiles(const std::string& dirpath,uint64_t lowerLimit, uint64_t currentTotalSizeOnDisk);
        std::vector<SubjectFileInfo> getSortedListOFCompressedJournalFiles(const std::string& dirpath);
        void compressClosedFiles(const std::string& dirpath, std::shared_ptr<EventWriterLib::IEventWriterWorker> worker);
    private:
        bool init_encoder(lzma_stream *strm, uint32_t preset);
        bool compress(lzma_stream *strm, FILE *infile, FILE *outfile);
        static bool isJournalFileNewer(const SubjectFileInfo& currentInfo,const SubjectFileInfo& newInfo);


    };
}