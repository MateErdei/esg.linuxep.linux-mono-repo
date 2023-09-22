// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionStructs.h"

#include "Common/HttpRequests/IHttpRequester.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"

#include <json.hpp>
#include <string>

#ifndef TEST_PUBLIC
#    define TEST_PUBLIC private
#endif

namespace ResponseActionsImpl
{
    class DownloadFileAction
    {
    public:
        DownloadFileAction(std::shared_ptr<Common::HttpRequests::IHttpRequester> requester);
        [[nodiscard]] nlohmann::json run(const std::string& actionJson);

        TEST_PUBLIC : bool assessSpaceInfo(const DownloadInfo& info);

    private:
        bool initialChecks(const DownloadInfo& info);
        void download(const DownloadInfo& info);
        void handleHttpResponse(const Common::HttpRequests::Response& httpresponse);
        bool verifyFile(const DownloadInfo& info);
        void decompressAndMoveFile(const DownloadInfo& info);

        bool makeDestDirectory(const Path& destDir);
        bool fileAlreadyExists(const Path& destPath);
        Path getSubDirsInTmpDir(const Path& filePath);

        Path findBaseDir(const Path& path); // To be made more robust for general use and moved to Filesystem
        void removeTmpFiles();

        /**
         *  @return Will return true if the move was successful, false if not
         */
        bool moveFile(const Path& destDir, const Path& fileName, const Path& filePathToMove);

        /**
         *  @return Will return true if the directory existed, false if not
         */
        bool removeDestDir(const std::string& destPath);

        bool createExtractionDirectory();
        void handleUnZipFailure(const int& unzipReturn);
        void handleMovingArchive(const Path& targetPath);
        void handleMovingSingleExtractedFile(const Path& destDir, const Path& targetPath, const Path& extractedFile);
        void handleMovingMultipleExtractedFile(
            const Path& destDir,
            const Path& targetPath,
            const std::vector<std::string>& extractedFiles);

        const Path m_raTmpDir = Common::ApplicationConfiguration::applicationPathManager().getResponseActionTmpPath();
        const Path m_tmpDownloadFile = m_raTmpDir + "/tmp_download.zip";
        const Path m_tmpExtractPath = m_raTmpDir + "/extract";
        const Path m_archiveFileName = "download.zip";

        std::shared_ptr<Common::HttpRequests::IHttpRequester> m_client;
        Common::FileSystem::IFileSystem* m_fileSystem = Common::FileSystem::fileSystem();
        nlohmann::json m_response;
    };

} // namespace ResponseActionsImpl