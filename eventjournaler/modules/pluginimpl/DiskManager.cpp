// Copyright 2021 Sophos Limited. All rights reserved.

#include "DiskManager.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <sys/stat.h>

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

    bool DiskManager::compressFile(const std::string& filepath, const std::string& destPath)
    {
        lzma_stream strm = LZMA_STREAM_INIT;

        // Initialize the encoder. If it succeeds, compress from
        // stdin to stdout. the should be file handles fro the file to compressed and the output file
        if (!Common::FileSystem::fileSystem()->isFile(filepath))
        {
                LOGWARN("File does not exist, " << filepath );
                return false;
        }

        FILE *myfile = fopen (filepath.c_str() , "r");
        if (myfile == nullptr)
        {
            LOGWARN("Failed to open file, " << filepath );
            return false;
        }

        FILE *output = fopen (destPath.c_str() , "wb");
        if (output == nullptr)
        {
            LOGWARN("Failed to create file, " << destPath );
            fclose(myfile);
            return false;
        }

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
            return false;
        }
        return true;
    }

    uint64_t DiskManager::getDirectorySize(const std::string& dirpath)
    {
        auto fs = Common::FileSystem::fileSystem();
        uint64_t totalDirectorySize = 0;
        if (fs->isDirectory(dirpath))
        {
            std::vector<Path> filesCollection = fs->listAllFilesInDirectoryTree(dirpath);
            for (auto& file : filesCollection)
            {
                totalDirectorySize += fs->fileSize(file);
            }
            return totalDirectorySize;
        }
        return totalDirectorySize;

    }

    void DiskManager::deleteOldJournalFiles(const std::string& dirpath, uint64_t lowerLimit, uint64_t currentTotalSizeOnDisk)
    {
        if (currentTotalSizeOnDisk <= lowerLimit)
        {
            return;
        }

        std::vector<SubjectDirInfo> fileset;
        auto fs = Common::FileSystem::fileSystem();
        std::vector<std::string> producers = fs->listDirectories(dirpath);
        std::vector<std::string> subjects;
        for (const auto& producer: producers)
        {
            std::vector<std::string> list = fs->listDirectories(producer);
            subjects.insert(subjects.end(),list.begin(),list.end());
        }

        if (subjects.size() == 0 )
        {
            return;
        }

        uint64_t totalSizeToDelete = currentTotalSizeOnDisk - lowerLimit;

        for (const auto& subject : subjects)
        {
            SubjectDirInfo dirInfo;
            dirInfo.fileset = getSortedListOFCompressedJournalFiles(subject);
            dirInfo.subject = subject;
            fileset.push_back(dirInfo);
        }

        uint64_t sizeOfDeletedFiles = 0;
        std::vector<std::string> filesToDelete;
        size_t retry = 0;

        while (sizeOfDeletedFiles < totalSizeToDelete && retry <= fileset.size())
        {
            retry ++;
            std::vector<SubjectDirInfo> listOfSubjectsThatHaveFiles;
            for (auto& subjectList: fileset)
            {
                if (subjectList.fileset.size() != 0)
                {
                    listOfSubjectsThatHaveFiles.push_back(subjectList);
                }
            }
            if (listOfSubjectsThatHaveFiles.size() == 0)
            {
                break;
            }
            uint64_t sizeToDeleteFromEachSubject = (totalSizeToDelete-sizeOfDeletedFiles) / listOfSubjectsThatHaveFiles.size();
            if (sizeToDeleteFromEachSubject == 0)
            {
                break;
            }

            for (auto& subjectList: listOfSubjectsThatHaveFiles)
            {
                uint64_t  sizeOfDeletedFilesPerSubject = 0;
                std::vector<SubjectFileInfo> list = subjectList.fileset;
                for (auto& subjectFile : list)
                {
                   sizeOfDeletedFiles += subjectFile.size;
                   sizeOfDeletedFilesPerSubject += subjectFile.size;
                   filesToDelete.push_back(subjectFile.filepath);
                   subjectList.fileset.erase(std::next(subjectList.fileset.begin()));
                   if (sizeOfDeletedFilesPerSubject >= sizeToDeleteFromEachSubject)
                   {
                       break;
                   }
                }
            }
        }

        for (const auto& file: filesToDelete)
        {
            fs->removeFile(file);
        }
    }

    std::vector<DiskManager::SubjectFileInfo> DiskManager::getSortedListOFCompressedJournalFiles(const std::string& dirpath)
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
                        std::string cTime = fileNameParts[3];
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
            LOGWARN("No files in directory :" << dirpath << " to be sorted");
        }
        std::vector<SubjectFileInfo> sortedFiles;
        for (const auto& file : list)
        {
            sortedFiles.insert(sortedFiles.begin(),file);
        }
        return sortedFiles;
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

    void DiskManager::compressClosedFiles(const std::string& dirpath, std::shared_ptr<EventWriterLib::IEventWriterWorker> worker)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::vector<Path> filesCollection = fs->listAllFilesInDirectoryTree(dirpath);
        for (const auto& path : filesCollection)
        {
            if (Common::UtilityImpl::StringUtils::endswith(path,".bin"))
            {
                //closed file name should be in format  subject-uniqueID1-uniqueID2-timestamp1-timestamp2.xz
                std::vector<std::string> fileNameParts =  Common::UtilityImpl::StringUtils::splitString( Common::FileSystem::basename(path),"-");

                if (fileNameParts.size() == 5)
                {
                    std::string compressedFileFinalPath  = path.substr(0,path.find_first_of(".")) + ".xz";
                    std::string compressFileTempPath =
                        Common::FileSystem::join(Common::ApplicationConfiguration::applicationPathManager().getTempPath(), "journal.xz");

                    if (fs->exists(compressFileTempPath))
                    {
                        fs->removeFile(compressFileTempPath);
                    }

                    worker->checkAndPruneTruncatedEvents(path);

                    if (fs->exists(path))
                    {
                        if (compressFile(path, compressFileTempPath))
                        {
                            try
                            {
                                Common::FileSystem::filePermissions()->chmod(compressFileTempPath, S_IRUSR | S_IWUSR | S_IRGRP);
                                fs->moveFile(compressFileTempPath,compressedFileFinalPath);
                                fs->removeFile(path);
                            }
                            catch (Common::FileSystem::IFileSystemException& exception)
                            {
                                LOGWARN("Failed to clean up file " << path << " after compressing due to error: " << exception.what());
                            }
                        }

                        LOGINFO("Compressed file: " << path);
                    }
                }
            }
        }
    }

}