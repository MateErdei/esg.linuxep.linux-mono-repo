/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <optional>
#include <lzma.h>
namespace Plugin
{


    class PluginUtils
    {
    public:
        /**
        * compresses given file using xz format
        * @param filepath, file to be compressed
         */
        static void compressFile(const std::string filepath);
        static uint64_t getDirectorySize(const std::string dirpath);

    private:
        static bool init_encoder(lzma_stream *strm, uint32_t preset);
        static bool compress(lzma_stream *strm, FILE *infile, FILE *outfile);

    };
}