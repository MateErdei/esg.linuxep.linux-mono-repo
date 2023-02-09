// Copyright 2023 Sophos Limited. All rights reserved.

#include "ZipUtils.h"

#include "Logger.h"

#include <Common/FileSystem/IFilePermissions.h>

#include <fstream>

#include <unzip.h>
#include <mz_compat.h>

# include <unistd.h>
# include <utime.h>

#define WRITEBUFFERSIZE (8192)

namespace Common::ZipUtilities
{
    using namespace Common::FileSystem;

    void ZipUtils::zip(const std::string& srcPath, const std::string& destPath) const
    {
        zipFile zf = zipOpen(std::string(destPath.begin(), destPath.end()).c_str(), APPEND_STATUS_CREATE);
        if (zf == NULL)
        {
            LOGWARN("Error opening zip file: " << destPath);
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
                LOGWARN("Could not open zip file: " << path);
            }
        }

        if (ZIP_OK != zipClose(zf, NULL))
        {
            LOGWARN("Error closing zip file: " << destPath);
        }

    }

    static void changeFileDate(
        const char *filename,
        unsigned long dosdate,
        tm_unz tmu_date)
    {
        (void)dosdate;
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
        FILE *fout=NULL;
        void* buf;
        unsigned int size_buf;
        auto fs = Common::FileSystem::fileSystem();

        unz_file_info64 file_info;
        int err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
        if (err != UNZ_OK)
        {
            LOGWARN("Error getting current fil info from zipfile: " << err);
            return err;
        }

        size_buf = WRITEBUFFERSIZE;
        buf = (void*)malloc(size_buf);
        if (buf == NULL)
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
            int skip=0;

            write_filename = filename_inzip;

            err = unzOpenCurrentFilePassword(uf, password);
            if (err != UNZ_OK)
            {
                LOGWARN("Error opening zipfile with password: " << err);
            }

            if ((skip==0) && (err==UNZ_OK))
            {
                fout = fopen64(write_filename, "wb");
                /* some zipfile don't contain directory alone before file */
                if ((fout == NULL) && (filename_withoutpath != (char*)filename_inzip))
                {
                    char c = *(filename_withoutpath-1);
                    *(filename_withoutpath-1) = '\0';
                    fs->makedirs(write_filename);
                    *(filename_withoutpath-1) = c;
                    fout = fopen64(write_filename,"wb");
                }

                if (fout == NULL)
                {
                    LOGWARN("Error opening: " << write_filename);
                }
            }

            if (fout != NULL)
            {
                LOGINFO(" extracting: " << write_filename);

                do
                {
                    err = unzReadCurrentFile(uf, buf, size_buf);
                    if (err < 0)
                    {
                        LOGWARN("Error reading current file in zipfile: " << err);
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
                    changeFileDate(write_filename, file_info.dosDate, file_info.tmu_date);
                }
            }

            if (err == UNZ_OK)
            {
                err = unzCloseCurrentFile(uf);
                if (err != UNZ_OK)
                {
                    LOGWARN("Error closing current file in zipfile: " << err);
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

    static void extractAll(
        unzFile uf,
        const char* password)
    {
        unz_global_info64 gi;
        int err = unzGetGlobalInfo64(uf, &gi);
        if (err != UNZ_OK)
        {
            LOGWARN("Error getting global file info from zipfile: " << err);
        }

        for (unsigned int i = 0; i < gi.number_entry; i++)
        {
            if (extractCurrentfile(uf, password) != UNZ_OK)
            {
                break;
            }

            if ((i+1) < gi.number_entry)
            {
                err = unzGoToNextFile(uf);
                if (err != UNZ_OK)
                {
                    LOGWARN("Error getting next file in zipfile: " << err);
                    break;
                }
            }
        }
    }

    void ZipUtils::unzip(const std::string& srcPath, const std::string& destPath)
    {
        const char *password = NULL;

        unzFile uf = unzOpen64(srcPath.c_str());
        if (uf == NULL)
        {
            LOGWARN("Error opening zip: " << srcPath.c_str());
        }

        chdir(destPath.c_str());
        extractAll(uf, password);

        unzClose(uf);
    }
} // namespace Common::ZipUtilities