// Copyright 2023 Sophos Limited. All rights reserved.

#include "ZipUtils.h"

#include "Logger.h"

#include <Common/FileSystem/IFilePermissions.h>

#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unzip.h>

namespace Common::ZipUtilities
{
    using namespace Common::FileSystem;

    void ZipUtils::produceZip(const std::string& srcPath, const std::string& destPath) const
    {
        zipFile zf = zipOpen(std::string(destPath.begin(), destPath.end()).c_str(), APPEND_STATUS_CREATE);
        if (zf == NULL)
        {
            LOGWARN("Error opening zip :" << destPath);
        }

        auto fs = Common::FileSystem::fileSystem();

        std::vector<Path> filesToZip = fs->listAllFilesInDirectoryTree(srcPath);

        for (auto& path : filesToZip)
        {
            if (fs->isFile(path))
            {
                std::fstream file(path.c_str(), std::ios::binary | std::ios::in);
                if (file.is_open())
                {
                    file.seekg(0, std::ios::end);
                    long size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    std::vector<char> buffer(size);
                    if (size == 0 || file.read(&buffer[0], size))
                    {
                        zip_fileinfo zfi;

                        std::string fileName = path.substr(srcPath.size()+1);

                        if (0 == zipOpenNewFileInZip(zf, (fileName).c_str(), &zfi, NULL, 0, NULL, 0, NULL, MZ_COMPRESS_METHOD_DEFLATE, MZ_COMPRESS_LEVEL_DEFAULT))
                        {
                            if (ZIP_OK != zipWriteInFileInZip(zf, size == 0 ? "" : &buffer[0], size))
                            {
                                LOGWARN("Error zipping up file: " << fileName);
                            }

                            if (ZIP_OK != zipCloseFileInZip(zf))
                            {
                                LOGWARN("Error closing zip file: " << fileName);
                            }

                            file.close();
                            continue;
                        }
                    }
                    file.close();
                }
                LOGWARN("Could not open file: " << path);
            }
        }

        if (ZIP_OK != zipClose(zf, NULL))
        {
            LOGWARN("Error closing zip :" << destPath);
        }

    }

//    void unzipFolder(const char *srcPath)
//    {
//        unzFile zf = unzOpen(srcPath);
//        if (zf == NULL)
//        {
//            LOGERROR("Zip file: " << srcPath << " not found");
//        }
//
//        auto fs = Common::FileSystem::fileSystem();
//
//        unz_global_info zfGlobalInfo;
//        if (unzGetGlobalInfo(zf, &zfGlobalInfo) != UNZ_OK)
//        {
//            LOGERROR("Could not read global info of: " << srcPath);
//            unzClose(zf);
//        }
//
//        char read_buffer[zfGlobalInfo.size_comment];
//
//        ulong i;
//        int maxNameLength = 0;
//        for (i = 0; i < zfGlobalInfo.number_entry; ++i)
//        {
//            unz_file_info fileInfo;
//            if (fileInfo.size_filename > maxNameLength)
//            {
//                maxNameLength = fileInfo.size_filename;
//            }
//            char fileName[maxNameLength];
//            if (unzGetCurrentFileInfo(
//                    zf,
//                    &fileInfo,
//                    fileName,
//                    maxNameLength,
//                    NULL, 0, NULL, 0) != UNZ_OK)
//            {
//                LOGERROR("Could not read file: " << srcPath << "/" << fileName);
//                unzClose(zf);
//            }
//
//            const size_t nameLength = strlen(fileName);
//            if (fileName[ nameLength-1 ] = "/") //fix this
//            {
//                mkdir(fileName, 0600); //dont know permissions
//                LOGINFO("Extracted directory: " << srcPath << "/" << fileName);
//                //error catch?
//            }
//            else
//            {
//                if (unzOpenCurrentFile(zf) != UNZ_OK)
//                {
//                    LOGERROR("Could not open file: " << srcPath);
//                    unzClose(zf);
//                }
//                FILE *out = fopen(fileName, "wb");
//                if (out == NULL )
//                {
//                    LOGERROR("Could not open: " << srcPath << "/" << fileName); //what if in a dir in zip?
//                    unzCloseCurrentFile(zf);
//                    unzClose(zf);
//                }
//
//                int error = UNZ_OK;
//                do
//                {
//                    error = unzReadCurrentFile(zf, read_buffer, zfGlobalInfo.size_comment);
//                    if (error < 0)
//                    {
//                        LOGERROR("tbd"); //UNZ_ERRNO for IO error, or zLib error for uncompress error
//                        unzCloseCurrentFile(zf);
//                        unzClose(zf);
//                    }
//                    if ( error > 0 )
//                    {
//                        size_t count = 1;
//                        fwrite( read_buffer, error, count, out );
//                        if (fwrite != count) //??
//                        {
//                            LOGERROR("tbd");
//                        }
//                    }
//                } while (error > 0);
//
//                fclose(out);
//            }
//
//            unzCloseCurrentFile(zf);
//            if ((i+1) < zfGlobalInfo.number_entry)
//            {
//                if (unzGoToNextFile(zf) != UNZ_OK)
//                {
//                    LOGERROR("Could not read file: " << srcPath << "/" << fileName);
//                    unzClose(zf);
//                }
//            }
//        }
//        unzClose(zf);
//    }
}