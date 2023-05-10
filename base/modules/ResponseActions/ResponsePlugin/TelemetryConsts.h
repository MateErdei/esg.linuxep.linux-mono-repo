// Copyright 2023 Sophos Limited. All rights reserved.

#include <vector>

namespace ResponsePlugin
{
    namespace Telemetry
    {
        const char *const version = "version";
        const char* const pluginHealthStatus = "health";

        //Run Command
        constexpr auto RUN_COMMAND_COUNT = "run-command-actions";
        constexpr auto RUN_COMMAND_FAILED_COUNT = "run-command-failed";
        constexpr auto RUN_COMMAND_TIMEOUT_COUNT = "run-command-timeout-actions";
        constexpr auto RUN_COMMAND_EXPIRED_COUNT = "run-command-expired-actions";

        //Upload File
        constexpr auto UPLOAD_FILE_COUNT = "upload-file-count";
        constexpr auto UPLOAD_FILE_FAILED_COUNT = "upload-file-overall-failures";
        constexpr auto UPLOAD_FILE_TIMEOUT_COUNT = "upload-file-timeout-failures";
        constexpr auto UPLOAD_FILE_EXPIRED_COUNT = "upload-file-expiry-failures";

        //Upload Folder
        constexpr auto UPLOAD_FOLDER_COUNT = "upload-folder-count";
        constexpr auto UPLOAD_FOLDER_FAILED_COUNT = "upload-folder-overall-failures";
        constexpr auto UPLOAD_FOLDER_TIMEOUT_COUNT = "upload-folder-timeout-failures";
        constexpr auto UPLOAD_FOLDER_EXPIRED_COUNT = "upload-folder-expiry-failures";

        //Download File
        constexpr auto DOWNLOAD_FILE_COUNT = "download-file-count";
        constexpr auto DOWNLOAD_FILE_FAILED_COUNT = "download-file-overall-failures";
        constexpr auto DOWNLOAD_FILE_TIMEOUT_COUNT = "download-file-timeout-failures";
        constexpr auto DOWNLOAD_FILE_EXPIRED_COUNT = "download-file-expiry-failures";

        //Vector for getTelemetry
        const std::vector<std::string> TELEMETRY_FIELDS {
            RUN_COMMAND_COUNT, RUN_COMMAND_FAILED_COUNT, RUN_COMMAND_TIMEOUT_COUNT, RUN_COMMAND_EXPIRED_COUNT,
            UPLOAD_FILE_COUNT, UPLOAD_FILE_FAILED_COUNT, UPLOAD_FILE_TIMEOUT_COUNT, UPLOAD_FILE_EXPIRED_COUNT,
            UPLOAD_FOLDER_COUNT, UPLOAD_FOLDER_FAILED_COUNT, UPLOAD_FOLDER_TIMEOUT_COUNT, UPLOAD_FOLDER_EXPIRED_COUNT,
            DOWNLOAD_FILE_COUNT, DOWNLOAD_FILE_FAILED_COUNT, DOWNLOAD_FILE_TIMEOUT_COUNT, DOWNLOAD_FILE_EXPIRED_COUNT
        };

    } // namespace Telemetry
} // namespace ResponsePlugin