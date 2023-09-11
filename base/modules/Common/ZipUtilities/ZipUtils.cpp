// Copyright 2023 Sophos Limited. All rights reserved.

#include "ZipUtils.h"

#include "Logger.h"
#include "UnzipFileWrapper.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <sys/stat.h>

#include <fstream>
#include <memory>
#include <stdio.h>
#include <unistd.h>
#include <utime.h>
#include <zlib.h>

#define WRITEBUFFERSIZE (8192)

namespace
{
    void restoreFileInfo(const std::string& filepath, unz_file_info64 file_info)
    {
        struct utimbuf ut;
        struct tm newdate;
        tm_unz tmu_date = file_info.tmu_date;
        newdate.tm_sec = tmu_date.tm_sec;
        newdate.tm_min = tmu_date.tm_min;
        newdate.tm_hour = tmu_date.tm_hour;
        newdate.tm_mday = tmu_date.tm_mday;
        newdate.tm_mon = tmu_date.tm_mon;
        if (tmu_date.tm_year > 1900)
            newdate.tm_year = tmu_date.tm_year - 1900;
        else
            newdate.tm_year = tmu_date.tm_year;
        newdate.tm_isdst = -1;

        ut.actime = ut.modtime = mktime(&newdate);
        ::utime(filepath.c_str(), &ut);
        ::chmod(filepath.c_str(), file_info.external_fa);
        char f_mode[20];
        sprintf(f_mode, "%3o", file_info.external_fa & 0777);

        LOGDEBUG("Restoring mode to: " << f_mode);
    }

    void getFileInfo(const std::string& filepath, zip_fileinfo* zfi, unsigned long crcFile)
    {
        struct stat s;
        struct tm* filedate;
        time_t tm_t = 0;

        if (::stat(filepath.c_str(), &s) != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "Could not stat file: " << filepath;
            throw std::runtime_error(errorMessage.str());
        }
        tm_t = s.st_mtime;
        filedate = localtime(&tm_t);

        zfi->dosDate = 0;
        zfi->internal_fa = 0;
        zfi->external_fa = s.st_mode;
        zfi->tmz_date.tm_sec = filedate->tm_sec;
        zfi->tmz_date.tm_min = filedate->tm_min;
        zfi->tmz_date.tm_hour = filedate->tm_hour;
        zfi->tmz_date.tm_mday = filedate->tm_mday;
        zfi->tmz_date.tm_mon = filedate->tm_mon;
        zfi->tmz_date.tm_year = filedate->tm_year;

        char time_buf[256];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", filedate);

        char f_mode[20];
        sprintf(f_mode, "%3o", zfi->external_fa & 0777);
        LOGDEBUG(
            "Filename: " << filepath << ", crc: " << crcFile << ", timestamp: " << time_buf << ", mode: " << f_mode);
    }

    int extractCurrentfile(std::shared_ptr<Common::ZipUtilities::UnzipFileWrapper> uf, const char* password)
    {
        char filename_inzip[256];
        char* filename_withoutpath;
        char* p;
        FILE* fout = nullptr;
        void* buf;
        unsigned int size_buf;
        auto fs = Common::FileSystem::fileSystem();

        unz_file_info64 file_info;
        int err = unzGetCurrentFileInfo64(
            uf->get(), &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
        if (err != UNZ_OK)
        {
            LOGERROR("Error getting current file info from zipfile: " << std::to_string(err));
            return err;
        }

        size_buf = WRITEBUFFERSIZE;
        buf = (void*)malloc(size_buf);
        if (buf == nullptr)
        {
            LOGERROR("Error allocating memory");
            return UNZ_INTERNALERROR;
        }

        p = filename_withoutpath = filename_inzip;
        while ((*p) != '\0')
        {
            if (((*p) == '/') || ((*p) == '\\'))
            {
                filename_withoutpath = p + 1;
            }
            p++;
        }

        if ((*filename_withoutpath) == '\0')
        {
            LOGINFO("Creating directory: " << filename_inzip);
            fs->makedirs(filename_inzip);
        }
        else
        {
            const char* write_filename;

            write_filename = filename_inzip;

            err = unzOpenCurrentFilePassword(uf->get(), password);
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
                    char c = *(filename_withoutpath - 1);
                    *(filename_withoutpath - 1) = '\0';
                    fs->makedirs(write_filename);
                    *(filename_withoutpath - 1) = c;
                    fout = fopen64(write_filename, "wb");
                }

                if (fout == nullptr)
                {
                    LOGERROR("Error opening: " << write_filename);
                }
            }

            if (fout != nullptr)
            {
                LOGINFO(" extracting: " << write_filename);

                do
                {
                    err = unzReadCurrentFile(uf->get(), buf, size_buf);
                    if (err < 0)
                    {
                        LOGERROR("Error reading current file in zipfile: " << std::to_string(err));
                        break;
                    }
                    if (err > 0)
                    {
                        if (fwrite(buf, (unsigned)err, 1, fout) != 1)
                        {
                            LOGERROR("Error writing extracted file");
                            err = UNZ_ERRNO;
                            break;
                        }
                    }
                } while (err > 0);

                if (fout)
                {
                    fclose(fout);
                }

                if (err == 0)
                {
                    restoreFileInfo(write_filename, file_info);
                }
            }

            if (err == UNZ_OK)
            {
                err = unzCloseCurrentFile(uf->get());
                if (err != UNZ_OK)
                {
                    LOGERROR("Error closing current file in zipfile: " << std::to_string(err));
                }
            }
            else
            {
                unzCloseCurrentFile(uf->get());
            }
        }

        free(buf);
        return err;
    }

    int extractAll(std::shared_ptr<Common::ZipUtilities::UnzipFileWrapper> uf, const char* password)
    {
        unz_global_info64 gi;
        int err = unzGetGlobalInfo64(uf->get(), &gi);
        if (err != UNZ_OK)
        {
            LOGERROR("Error getting global file info from zipfile: " << std::to_string(err));
            return err;
        }

        for (unsigned int i = 0; i < gi.number_entry; i++)
        {
            err = extractCurrentfile(uf, password);
            if (err != UNZ_OK)
            {
                break;
            }

            if ((i + 1) < gi.number_entry)
            {
                err = unzGoToNextFile(uf->get());
                if (err != UNZ_OK)
                {
                    LOGERROR("Error getting next file in zipfile: " << std::to_string(err));
                    break;
                }
            }
        }
        return err;
    }
} // namespace

namespace Common::ZipUtilities
{
    int ZipUtils::zip(const std::string& srcPath, const std::string& destPath, bool ignoreFileError) const
    {
        return zip(srcPath, destPath, ignoreFileError, false, "");
    }

    int ZipUtils::zip(
        const std::string& srcPath,
        const std::string& destPath,
        bool ignoreFileError,
        bool passwordProtected) const
    {
        return zip(srcPath, destPath, ignoreFileError, passwordProtected, "");
    }

    int ZipUtils::zip(
        const std::string& srcPath,
        const std::string& destPath,
        bool ignoreFileError,
        bool passwordProtected,
        const std::string& password) const
    {
        int ret = ZIP_OK;
        zipFile zf = zipOpen(std::string(destPath.begin(), destPath.end()).c_str(), APPEND_STATUS_CREATE);
        if (zf == nullptr)
        {
            LOGERROR("Error opening zip file: " << destPath);
            return ENOENT;
        }

        auto fs = Common::FileSystem::fileSystem();
        std::vector<Path> filesToZip;
        bool fileOnly = false;
        if (fs->isFile(srcPath))
        {
            fileOnly = true;
            filesToZip.emplace_back(srcPath);
        }
        else if (fs->isDirectory(srcPath))
        {
            filesToZip = fs->listAllFilesInDirectoryTree(srcPath);
        }
        else
        {
            LOGWARN("No file or directory found at input path: " << srcPath);
            return ENOENT;
        }

        for (auto& fullFilePath : filesToZip)
        {
            if (!fs->isFile(fullFilePath))
            {
                continue;
            }

            std::string fileContents;
            try
            {
                // set maxsize of file we accept to 1GB
                fileContents = fs->readFile(fullFilePath, 1024 * 1024 * 1024);
            }
            catch (const std::exception& exception)
            {
                std::stringstream errorMessage;
                errorMessage << "Could not read contents of file: " << fullFilePath
                             << " with error: " << exception.what();
                LOGWARN(errorMessage.str());
                if (!ignoreFileError)
                {
                    throw std::runtime_error(errorMessage.str());
                }
            }

            zip_fileinfo zfi;
            std::string relativeFilePath;
            if (fileOnly)
            {
                relativeFilePath = Common::FileSystem::basename(srcPath);
            }
            else
            {
                std::string temp = srcPath;
                if (Common::UtilityImpl::StringUtils::endswith(srcPath, "/"))
                {
                    temp = temp.substr(0, temp.size() - 1);
                }

                relativeFilePath = fullFilePath.substr(temp.find_last_of("/") + 1, fullFilePath.size());
            }

            unsigned long crcFile = 0;
            crcFile = crc32_z(crcFile, (unsigned char*)&fileContents[0], fileContents.size());

            getFileInfo(fullFilePath, &zfi, crcFile);

            if (passwordProtected)
            {
                ret = zipOpenNewFileInZip3(
                    zf,
                    relativeFilePath.c_str(),
                    &zfi,
                    nullptr,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    MZ_COMPRESS_METHOD_DEFLATE,
                    MZ_COMPRESS_LEVEL_DEFAULT,
                    0,
                    -MAX_WBITS,
                    DEF_MEM_LEVEL,
                    Z_DEFAULT_STRATEGY,
                    password.c_str(),
                    crcFile);
            }
            else
            {
                ret = zipOpenNewFileInZip(
                    zf,
                    relativeFilePath.c_str(),
                    &zfi,
                    nullptr,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    MZ_COMPRESS_METHOD_DEFLATE,
                    MZ_COMPRESS_LEVEL_DEFAULT);
            }

            if (ret != ZIP_OK)
            {
                LOGWARN("Could not add file " << fullFilePath << " to zip file");
                continue;
            }

            ret = zipWriteInFileInZip(zf, &fileContents[0], fileContents.size());
            if (ZIP_OK != ret)
            {
                LOGERROR("Error zipping up file: " << relativeFilePath);
            }

            ret = zipCloseFileInZip(zf);
            if (ZIP_OK != ret)
            {
                LOGERROR("Error closing zip file: " << relativeFilePath);
            }
        }

        ret = zipClose(zf, nullptr);
        if (ZIP_OK != ret)
        {
            LOGERROR("Error closing zip file: " << destPath);
        }
        return ret;
    }

    int ZipUtils::unzip(const std::string& srcPath, const std::string& destPath) const
    {
        return unzip(srcPath, destPath, false, "");
    }

    int ZipUtils::unzip(
        const std::string& srcPath,
        const std::string& destPath,
        bool passwordProtected,
        const std::string& passwordStr) const
    {
        int ret = UNZ_OK;
        auto uf = std::make_shared<UnzipFileWrapper>(unzOpen64(srcPath.c_str()));
        if (uf->get() == nullptr)
        {
            if (Common::FileSystem::fileSystem()->isFile(srcPath))
            {
                LOGERROR("Error opening zip: " << srcPath << " as file found was not a zip file");
                return UNZ_BADZIPFILE;
            }
            else
            {
                LOGERROR("Error opening zip: " << srcPath);
                return ENOENT;
            }
        }

        std::ignore = chdir(destPath.c_str());
        if (passwordProtected)
        {
            ret = extractAll(uf, passwordStr.c_str());
        }
        else
        {
            ret = extractAll(uf, nullptr);
        }
        return ret;
    }
} // namespace Common::ZipUtilities
namespace
{
    std::unique_ptr<Common::ZipUtilities::IZipUtils> instance = std::make_unique<Common::ZipUtilities::ZipUtils>();
}
namespace Common::ZipUtilities
{
    IZipUtils& zipUtils()
    {
        return *instance;
    }
    /** Use only for test */
    void replaceZipUtils(std::unique_ptr<Common::ZipUtilities::IZipUtils> zipUtils)
    {
        instance = std::move(zipUtils);
    }

    void restoreZipUtils()
    {
        instance = std::make_unique<ZipUtils>();
    }
} // namespace Common::ZipUtilities