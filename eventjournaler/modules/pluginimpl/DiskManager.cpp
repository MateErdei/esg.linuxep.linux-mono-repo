/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DiskManager.h"

#include "Logger.h"

#include <Common/UtilityImpl/StringUtils.h>

#include <cstring>
#include <lzma.h>
namespace Plugin
{
    bool DiskManager::init_encoder(lzma_stream *strm, uint32_t preset)
    {
        // Initialize the encoder using a preset.
        lzma_ret ret = lzma_easy_encoder(strm, preset, LZMA_CHECK_CRC64);

        // Return successfully if the initialization went fine.
        if (ret == LZMA_OK)
            return true;

        // Something went wrong.
        const char *msg;
        switch (ret) {
            case LZMA_MEM_ERROR:
                msg = "Memory allocation failed";
                break;

            case LZMA_OPTIONS_ERROR:
                msg = "Specified preset is not supported";
                break;

            case LZMA_UNSUPPORTED_CHECK:
                msg = "Specified integrity check is not supported";
                break;

            default:
                msg = "Unknown error, possibly a bug";
                break;
        }

        LOGERROR( "Error initializing the encoder: " << msg << " with error code: " << ret);
        return false;
    }


    bool DiskManager::compress(lzma_stream *strm, FILE *infile, FILE *outfile)
    {
        // This will be LZMA_RUN until the end of the input file is reached.
        // This tells lzma_code() when there will be no more input.
        lzma_action action = LZMA_RUN;

        // Buffers to temporarily hold uncompressed input
        // and compressed output.
        uint8_t inbuf[BUFSIZ];
        uint8_t outbuf[BUFSIZ];

        strm->next_in = NULL;
        strm->avail_in = 0;
        strm->next_out = outbuf;
        strm->avail_out = sizeof(outbuf);

        // Loop until the file has been successfully compressed or until
        // an error occurs.
        while (true) {
            // Fill the input buffer if it is empty.
            if (strm->avail_in == 0 && !feof(infile)) {
                strm->next_in = inbuf;
                strm->avail_in = fread(inbuf, 1, sizeof(inbuf),
                                       infile);

                if (ferror(infile)) {
                    LOGERROR("Read error: " << strerror(errno));
                    return false;
                }


                if (feof(infile))
                    action = LZMA_FINISH;
            }

            lzma_ret ret = lzma_code(strm, action);


            if (strm->avail_out == 0 || ret == LZMA_STREAM_END) {

                size_t write_size = sizeof(outbuf) - strm->avail_out;

                if (fwrite(outbuf, 1, write_size, outfile)
                    != write_size) {
                    LOGERROR("Write error:"<< strerror(errno));
                    return false;
                }

                // Reset next_out and avail_out.
                strm->next_out = outbuf;
                strm->avail_out = sizeof(outbuf);
            }

            // Normally the return value of lzma_code() will be LZMA_OK
            // until everything has been encoded.
            if (ret != LZMA_OK) {

                if (ret == LZMA_STREAM_END)
                    return true;

                // It's not LZMA_OK nor LZMA_STREAM_END,
                // so it must be an error code.
                const char *msg;
                switch (ret) {
                    case LZMA_MEM_ERROR:
                        msg = "Memory allocation failed";
                        break;
                    case LZMA_DATA_ERROR:
                        msg = "File size limits exceeded";
                        break;

                    default:
                        msg = "Unknown error, possibly a bug";
                        break;
                }

                LOGERROR("Encoder error: " << msg << " with error code: " << ret);;
                return false;
            }
        }
    }

    void DiskManager::compressFile(const std::string filepath)
    {
        lzma_stream strm = LZMA_STREAM_INIT;

        // Initialize the encoder. If it succeeds, compress from
        // stdin to stdout. the shoudl be file handles fro the file to compressed and the output file
        FILE *myfile = fopen (filepath.c_str() , "r");
        std::string destPath = filepath.substr(0,filepath.find_first_of(".")) + ".xz";
        std::cout << destPath << std::endl;
        FILE *output = fopen (destPath.c_str() , "wb");
        //FILE *output = fopen ("/tmp/myfile.xz" , "wb");

        bool success = init_encoder(&strm, 7);
        if (success)
        {
            compress(&strm, myfile, output);
        }


        // Free the memory allocated for the encoder. If we were encoding

        lzma_end(&strm);

        // Close stdout to catch possible write errors that can occur
        // when pending data is flushed from the stdio buffers.
        fclose(myfile);
        if (fclose(output)) {
            LOGERROR( "Write error: " << strerror(errno));

        }
    }
    uint64_t DiskManager::getDirectorySize(const std::string dirpath)
    {
        auto fs = Common::FileSystem::fileSystem();
        uint64_t totalDirectorySize = 0;
        if (fs->isDirectory(dirpath))
        {
            std::vector<Path> filesCollection = fs->listAllFilesInDirectoryTree(dirpath);
            for(auto& file : filesCollection)
            {
                totalDirectorySize += fs->fileSize(file);
            }
            return totalDirectorySize;
        }
        return totalDirectorySize;

    }

    void DiskManager::deleteOldJournalFiles(const std::string dirpath, uint64_t dataDeletionSize)
    {
        std::list<SubjectFileInfo> files = getSortedListOFCompressedJournalFiles(dirpath);
        auto fs = Common::FileSystem::fileSystem();
        uint64_t sizeOfDeletedFiles =0;
        std::vector<std::string> filesToDelete;
        for (const auto& file: files)
        {
            sizeOfDeletedFiles += file.size;
            filesToDelete.push_back(file.filepath);
            if (sizeOfDeletedFiles > dataDeletionSize)
            {
                break;
            }

        }
        for (const auto& file: filesToDelete)
        {
            fs->removeFile(file);
        }
        if (filesToDelete.size() == files.size())
        {
            LOGERROR("We have deleted all compressed files in directory: "<< dirpath << "but the data in that directory is still above the limit");
        }
    }

    std::list<DiskManager::SubjectFileInfo> DiskManager::getSortedListOFCompressedJournalFiles(const std::string dirpath)
    {
        std::list<SubjectFileInfo> list;
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isDirectory(dirpath))
        {
            std::vector<Path> filesCollection = fs->listAllFilesInDirectoryTree(dirpath);

            for (const auto& path : filesCollection)
            {
                if (Common::UtilityImpl::StringUtils::endswith(path,".xz"))
                {
                    SubjectFileInfo info;
                    info.filepath = path;
                    info.size = fs->fileSize(path);
                    std::vector<std::string> fileNameParts = Common::UtilityImpl::StringUtils::splitString( Common::FileSystem::basename(path),"-");
                    if (fileNameParts.size() != 5)
                    {
                        // this file name is malformed this should go to the top of the list
                        info.fileId = 0;
                    }
                    else
                    {
                        std::string cTime= fileNameParts[3];

                        try
                        {
                            info.fileId = std::stoul(cTime);
                        }
                        catch (std::exception& exception)
                        {
                            // this timestamp is malformed this should go to the top of the list
                            info.fileId = 0;
                        }
                    }
                    list.push_back(info);
                }
            }
            list.sort(isJournalFileNewer);
        }
        else
        {
            LOGWARN("No files to delete");
        }
        return list;
    }

    bool DiskManager::isJournalFileNewer(const SubjectFileInfo& currentInfo,const SubjectFileInfo& newInfo)
    {
        if (currentInfo.fileId == 0)
        {
            return false;
        }
        if (currentInfo.fileId > newInfo.fileId)
        {
            return true;
        }
        return false;
    }

}