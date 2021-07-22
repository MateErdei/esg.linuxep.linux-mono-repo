/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <list>
#include <optional>
#include <lzma.h>
namespace Plugin
{


    class DiskManager
    {
    public:
        /**
        * compresses given file using xz format
        * @param filepath, file to be compressed
         */
        void compressFile(const std::string filepath);
        uint64_t getDirectorySize(const std::string dirpath);
        void deleteOldJournalFiles(const std::string dirpath,uint64_t limit);
        std::list<std::string> getSortedListOFCompressedJournalFiles(const std::string dirpath);
    private:
        bool init_encoder(lzma_stream *strm, uint32_t preset);
        bool compress(lzma_stream *strm, FILE *infile, FILE *outfile);
        static bool isJournalFileNewer(const std::string currentFile, const std::string newFile);


    };
}