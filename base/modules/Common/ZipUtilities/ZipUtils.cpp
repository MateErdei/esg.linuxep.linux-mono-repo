// Copyright 2023 Sophos Limited. All rights reserved.

#include "ZipUtils.h"

#include "Logger.h"

#include <Common/FileSystem/IFilePermissions.h>

#include <fstream>

#include <unzip.h>
#include <mz.h>
#include <mz_os.h>
#include <mz_strm.h>
#include <mz_strm_buf.h>
#include <mz_strm_split.h>
#include <mz_zip.h>

namespace Common::ZipUtilities
{
    using namespace Common::FileSystem;

    void ZipUtils::produceZip(const std::string& srcPath, const std::string& destPath) const
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
                LOGWARN("Could not open zip file: " << path);
            }
        }

        if (ZIP_OK != zipClose(zf, NULL))
        {
            LOGWARN("Error closing zip file: " << destPath);
        }

    }

    int32_t minizip_extract_currentfile(void *handle, const char *destination, const char *password)
    {
        mz_zip_file *file_info = NULL;
        uint8_t buf[INT16_MAX];
        int32_t read = 0;
        int32_t written = 0;
        int32_t err = MZ_OK;
        int32_t err_close = MZ_OK;
        void *stream = NULL;
        const char *filename = NULL;
        char out_name[512];
        char out_path[512];
        char directory[512];

        if (mz_zip_entry_get_info(handle, &file_info) != MZ_OK)
        {
            LOGWARN("Error getting entry info in zip file: " << err);
            return err;
        }

        if (mz_path_get_filename(file_info->filename, &filename) != MZ_OK)
        {
            filename = file_info->filename;
        }

        // Construct output path
        out_path[0] = 0;
        if ((*file_info->filename == '/') || (file_info->filename[1] == ':'))
        {
            strncpy(out_path, file_info->filename, sizeof(out_path));
        }
        else
        {
            if (destination != NULL)
            {
                mz_path_combine(out_path, destination, sizeof(out_path));
            }

            err = mz_path_resolve(file_info->filename, out_name, sizeof(out_name));
            if (err != MZ_OK)
            {
                LOGWARN("Error resolving output path: " << err);
                return err;
            }
            mz_path_combine(out_path, out_name, sizeof(out_path));
        }
        // If zip entry is a directory then create it on disk
        if (mz_zip_attrib_is_dir(file_info->external_fa, file_info->version_madeby) == MZ_OK)
        {
            LOGDEBUG("Creating directory: " << out_path);
            mz_os_make_dir(out_path);
            return err;
        }
        err = mz_zip_entry_read_open(handle, 0, password);
        if (err != MZ_OK)
        {
            LOGWARN("Error opening entry in zip file: " << err);
            return err;
        }
        mz_stream_raw_create(&stream);
        // Create the file on disk so we can unzip to it
        if (err == MZ_OK)
        {
            // Some zips don't contain directory alone before file
            err = mz_stream_raw_open(stream, out_path, MZ_OPEN_MODE_CREATE);
            if ((err != MZ_OK) && (filename != file_info->filename))
            {
                // Create the directory of the output path
                strncpy(directory, out_path, sizeof(directory));
                mz_path_remove_filename(directory);
                mz_os_make_dir(directory);
                err = mz_stream_raw_open(stream, out_path, MZ_OPEN_MODE_CREATE);
            }
        }
        // Read from the zip, unzip to buffer, and write to disk
        if (err == MZ_OK)
        {
            LOGDEBUG("Extracting: " << out_path);
            while (1)
            {
                read = mz_zip_entry_read(handle, buf, sizeof(buf));
                if (read < 0)
                {
                    err = read;
                    LOGWARN("Error reading entry in zip file: " << err);
                    break;
                }
                if (read == 0)
                {
                    break;
                }
                written = mz_stream_raw_write(stream, buf, read);
                if (written != read)
                {
                    err = mz_stream_raw_error(stream);
                    LOGWARN("Error in writing extracted file: " << err);
                    break;
                }
            }
            mz_stream_raw_close(stream);
            // Set the time of the file that has been unzipped
            if (err == MZ_OK)
            {
                mz_os_set_file_date(out_path, file_info->modified_date, file_info->accessed_date, file_info->creation_date);
            }
        }
        else
        {
            LOGWARN("Error opening " << out_path << " (" << err << ")");
        }
        mz_stream_raw_delete(&stream);
        err_close = mz_zip_entry_close(handle);
        if (err_close != MZ_OK)
        {
            LOGWARN("Error closing entry in zip file: " << err_close);
            err = err_close;
        }
        return err;
    }

    int32_t minizip_extract_all(void *handle, const char *destination, const char *password)
    {
        int32_t err = MZ_OK;
        err = mz_zip_goto_first_entry(handle);
        if (err == MZ_END_OF_LIST)
        {
            return MZ_OK;
        }
        if (err != MZ_OK)
        {
            LOGWARN("Error going to first entry in zip file: " << err);
        }
        while (err == MZ_OK)
        {
            err = minizip_extract_currentfile(handle, destination, password);
            if (err != MZ_OK)
            {
                break;
            }
            err = mz_zip_goto_next_entry(handle);
            if (err == MZ_END_OF_LIST)
            {
                return MZ_OK;
            }
            if (err != MZ_OK)
            {
                LOGWARN("Error going to next entry in zip file: " << err);
            }
        }
        return err;
    }

    bool ZipUtils::unzip(const std::string& srcPath, const std::string& destPath)
    {
        void *handle = NULL;
        void *file_stream = NULL;
        void *split_stream = NULL;
        void *buf_stream = NULL;
        char *password = NULL;
//        minizip_opt options;
//        options.overwrite = 1;
//        options.compress_method = MZ_COMPRESS_METHOD_DEFLATE;
//        options.compress_level = MZ_COMPRESS_LEVEL_DEFAULT;
        std::int64_t disk_size = 0;
        int32_t mode = MZ_OPEN_MODE_CREATE;

        mz_stream_raw_create(&file_stream);
        mz_stream_buffered_create(&buf_stream);
        mz_stream_split_create(&split_stream);
        //mz_stream_set_base(buf_stream, file_stream);
        mz_stream_set_base(split_stream, file_stream);
        mz_stream_split_set_prop_int64(split_stream, MZ_STREAM_PROP_DISK_SIZE, disk_size);
        auto err = mz_stream_open(split_stream, srcPath.c_str(), mode);
        if (err != MZ_OK)
        {
            LOGWARN("Error opening file " << srcPath.c_str());
        }
        else
        {
            err = mz_zip_open(handle, split_stream, mode);
            if (handle == NULL)
            {
                LOGWARN("Error opening zip: " << srcPath.c_str());
                err = MZ_FORMAT_ERROR;
            }

            auto err = minizip_extract_all(handle, destPath.c_str(), password);
            if (err != MZ_OK)
            {
                LOGWARN("Error extracting " << srcPath.c_str() << "(" << err << ")");
            }

            auto err_close = mz_zip_close(handle);
            if (err_close != MZ_OK)
            {
                LOGWARN("Error in closing " << srcPath.c_str() << "(" << err_close << ")");
                err = err_close;
            }
            mz_stream_close(split_stream);
        }
        mz_stream_split_delete(&split_stream);
        mz_stream_buffered_delete(&buf_stream);
        mz_stream_raw_delete(&file_stream);
        //return err;
        return true;
    }
} // namespace Common::ZipUtilities