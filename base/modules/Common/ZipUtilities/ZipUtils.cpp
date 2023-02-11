// Copyright 2023 Sophos Limited. All rights reserved.

#include "ZipUtils.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"

#include <fstream>

#include <zlib.h>
#include <unzip.h>

#include <unistd.h>
#include <utime.h>

#define WRITEBUFFERSIZE (8192)

namespace Common::ZipUtilities
{
    int ZipUtils::zip(const std::string& srcPath, const std::string& destPath, bool passwordProtected, const std::string& password)
    {
        int ret = ZIP_OK;
        zipFile zf = zipOpen(std::string(destPath.begin(), destPath.end()).c_str(), APPEND_STATUS_CREATE);
        if (zf == nullptr)
        {
            LOGWARN("Error opening zip file: " << destPath);
            return ENOENT;
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

                        unsigned long crcFile=0;
                        crcFile = crc32_z(crcFile, (unsigned char*)&buffer[0], size);
                        LOGDEBUG("Filename: " << fileName << ", crc: " << crcFile);

                        if (passwordProtected)
                        {
                            ret = zipOpenNewFileInZip3(zf, fileName.c_str(), &zfi, nullptr, 0, nullptr, 0, nullptr,
                                                       MZ_COMPRESS_METHOD_DEFLATE, MZ_COMPRESS_LEVEL_DEFAULT,
                                                       0,  -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, password.c_str(), crcFile);
                        }
                        else
                        {
                            ret = zipOpenNewFileInZip(zf, (fileName).c_str(), &zfi, nullptr, 0, nullptr, 0, nullptr,
                                                      MZ_COMPRESS_METHOD_DEFLATE, MZ_COMPRESS_LEVEL_DEFAULT);
                        }

                        if (ret == ZIP_OK)
                        {
                            ret = zipWriteInFileInZip(zf, size == 0 ? "" : &buffer[0], size);
                            if (ZIP_OK != ret)
                            {
                                LOGWARN("Error zipping up file: " << fileName);
                            }

                            ret = zipCloseFileInZip(zf);
                            if (ZIP_OK != ret)
                            {
                                LOGWARN("Error closing zip file: " << fileName);
                            }

                            file.close();
                            continue;
                        }
                    }
                    file.close();
                }
                LOGWARN("Could not open zip file: " << path);
            }
        }

        ret = zipClose(zf, nullptr);
        if (ZIP_OK != ret)
        {
            LOGWARN("Error closing zip file: " << destPath);
        }
        return ret;
    }

    static void changeFileDate(
        const char *filename,
        tm_unz tmu_date)
    {
        struct utimbuf ut;
        struct tm newdate;
        newdate.tm_sec = tmu_date.tm_sec;
        newdate.tm_min=tmu_date.tm_min;
        newdate.tm_hour=tmu_date.tm_hour;
        newdate.tm_mday=tmu_date.tm_mday;
        newdate.tm_mon=tmu_date.tm_mon;
        if (tmu_date.tm_year > 1900)
            newdate.tm_year=tmu_date.tm_year - 1900;
        else
            newdate.tm_year=tmu_date.tm_year ;
        newdate.tm_isdst=-1;

        ut.actime=ut.modtime=mktime(&newdate);
        utime(filename,&ut);
    }

    static int extractCurrentfile(
        unzFile uf,
        const char* password)
    {
        char filename_inzip[256];
        char* filename_withoutpath;
        char* p;
        FILE *fout = nullptr;
        void* buf;
        unsigned int size_buf;
        auto fs = Common::FileSystem::fileSystem();

        unz_file_info64 file_info;
        int err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
        if (err != UNZ_OK)
        {
            LOGWARN("Error getting current fil info from zipfile: " << std::to_string(err));
            return err;
        }

        size_buf = WRITEBUFFERSIZE;
        buf = (void*)malloc(size_buf);
        if (buf == nullptr)
        {
            LOGWARN("Error allocating memory");
            return UNZ_INTERNALERROR;
        }

        p = filename_withoutpath = filename_inzip;
        while ((*p) != '\0')
        {
            if (((*p)=='/') || ((*p)=='\\'))
            {
                filename_withoutpath = p + 1;
            }
            p++;
        }

        if ((*filename_withoutpath)=='\0')
        {
            LOGINFO("Creating directory: " << filename_inzip);
            fs->makedirs(filename_inzip);
        }
        else
        {
            const char* write_filename;

            write_filename = filename_inzip;

            err = unzOpenCurrentFilePassword(uf, password);
            if (err != UNZ_OK)
            {
                LOGWARN("Error opening zipfile with password: " << std::to_string(err));
            }
            else
            {
                fout = fopen64(write_filename, "wb");
                /* some zipfile don't contain directory alone before file */
                if ((fout == nullptr) && (filename_withoutpath != (char*)filename_inzip))
                {
                    char c = *(filename_withoutpath-1);
                    *(filename_withoutpath-1) = '\0';
                    fs->makedirs(write_filename);
                    *(filename_withoutpath-1) = c;
                    fout = fopen64(write_filename,"wb");
                }

                if (fout == nullptr)
                {
                    LOGWARN("Error opening: " << write_filename);
                }
            }

            if (fout != nullptr)
            {
                LOGINFO(" extracting: " << write_filename);

                do
                {
                    err = unzReadCurrentFile(uf, buf, size_buf);
                    if (err < 0)
                    {
                        LOGWARN("Error reading current file in zipfile: " << std::to_string(err));
                        break;
                    }
                    if (err > 0)
                    {
                        if (fwrite(buf, (unsigned)err, 1, fout) != 1)
                        {
                            LOGWARN("Error writing extracted file");
                            err = UNZ_ERRNO;
                            break;
                        }
                    }
                }
                while (err > 0);

                if (fout)
                {
                    fclose(fout);
                }

                if (err == 0)
                {
                    changeFileDate(write_filename, file_info.tmu_date);
                }
            }

            if (err == UNZ_OK)
            {
                err = unzCloseCurrentFile(uf);
                if (err != UNZ_OK)
                {
                    LOGWARN("Error closing current file in zipfile: " << std::to_string(err));
                }
            }
            else
            {
                unzCloseCurrentFile(uf);
            }
        }

        free(buf);
        return err;
    }

    static int extractAll(
        unzFile uf,
        const char* password)
    {
        unz_global_info64 gi;
        int err = unzGetGlobalInfo64(uf, &gi);
        if (err != UNZ_OK)
        {
            LOGWARN("Error getting global file info from zipfile: " << std::to_string(err));
            return err;
        }

        for (unsigned int i = 0; i < gi.number_entry; i++)
        {
            err = extractCurrentfile(uf, password);
            if (err != UNZ_OK)
            {
                break;
            }

            if ((i+1) < gi.number_entry)
            {
                err = unzGoToNextFile(uf);
                if (err != UNZ_OK)
                {
                    LOGWARN("Error getting next file in zipfile: " << std::to_string(err));
                    break;
                }
            }
        }
        return err;
    }

    int ZipUtils::unzip(const std::string& srcPath, const std::string& destPath, bool passwordProtected, const std::string& passwordStr)
    {
        int ret = UNZ_OK;
        unzFile uf = unzOpen64(srcPath.c_str());
        if (uf == nullptr)
        {
            LOGWARN("Error opening zip: " << srcPath.c_str());
            return ENOENT;
        }

        chdir(destPath.c_str());
        if (passwordProtected)
        {
            ret = extractAll(uf, passwordStr.c_str());
        }
        else
        {
            ret = extractAll(uf, nullptr);
        }

        unzClose(uf);
        return ret;
    }
} // namespace Common::ZipUtilities