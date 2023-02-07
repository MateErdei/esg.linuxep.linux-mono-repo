// Copyright 2023 Sophos Limited. All rights reserved.

#include "ZipUtils.h"

#include "Logger.h"

#include <Common/FileSystem/IFilePermissions.h>

#include <fstream>
#include <iostream>
#include <unzip.h>
//#include <zlib.h>
#include <zip.h>

namespace Common::ZipUtilities
{
    using namespace Common::FileSystem;

    void produceZip(const std::string& srcPath, const std::string& destPath)
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

                        if (0 == zipOpenNewFileInZip(zf, (fileName).c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, 6)) //6 =  Z_DEFAULT_COMPRESSION
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
//        //get file to unzip and open
//        unzFile zf = unzOpen(srcPath);
//        if (zf == NULL)
//        {
//            LOGWARN("Zip file: " << srcPath << " not found");
//        }
//
//        auto fs = Common::FileSystem::fileSystem();
//
//
//
//
//    }
} // namespace diagnose