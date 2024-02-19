// Copyright 2024 Sophos Limited. All rights reserved.

#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Exceptions/IException.h"

#include <nlohmann/json.hpp>

class SafeStoreObjectException : public Common::Exceptions::IException
{
public:
    using Common::Exceptions::IException::IException;
};

namespace safestore
{

    class Extractor
    {
    public:
        Extractor(std::unique_ptr<safestore::SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper);



        [[nodiscard]] std::string extractQuarantinedFile(const std::string& path, const std::string& objectId);

        int extract(const std::string& pathFilter, const std::string& sha, const std::string& password, const std::string& dest);
    private:
        std::optional<std::string>loadPassword();
        void cleanupUnpackDir(bool failedToCleanUp);
        bool doesThreatMatch(const std::string& matchSha, const std::string& threatSha);
        void sendResponse(const std::string& errorMsg);
        std::unique_ptr<safestore::SafeStoreWrapper::ISafeStoreWrapper> safeStore_;
        static constexpr int USER_ERROR = 1;
        static constexpr int INTERNAL_ERROR = 3;
        nlohmann::json response_;
        Path directory_;
    };

}
