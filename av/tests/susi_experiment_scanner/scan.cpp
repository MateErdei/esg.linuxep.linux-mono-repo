
#include <Susi.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

// NOLINTNEXTLINE
#define P(_X) std::cerr << _X << '\n';

static bool isAllowlistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)fileChecksum;
    (void)size;
    P("isWhitelistedFile: " << fileChecksum);
    return false;
}

static SusiCertTrustType isTrustedCert(void *token, SusiHashAlg algorithm, const char *pkcs7, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)pkcs7;
    (void)size;

    P("isTrustedCert: " << size);

    return SUSI_TRUSTED;
}


static bool isAllowlistedCert(void *token, const char *fileTopLevelCert, size_t size)
{
    (void)token;
    (void)fileTopLevelCert;
    (void)size;

    P("isWhitelistedCert: " << size);

    return false;
}

static bool IsBlocklistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)fileChecksum;
    (void)size;
    P("IsBlocklistedFile: " << fileChecksum);
    return false;
}

namespace
{
    SusiCallbackTable my_susi_callbacks{
        .version = SUSI_CALLBACK_TABLE_VERSION,
        .token = nullptr, //NOLINT
        .IsAllowlistedFile = isAllowlistedFile,
        .IsBlocklistedFile = IsBlocklistedFile,
        .IsTrustedCert = isTrustedCert,
        .IsAllowlistedCert = isAllowlistedCert
    };

    class SusiGlobalHandler
    {
    public:
        explicit SusiGlobalHandler(const std::string& json_config);
        ~SusiGlobalHandler() noexcept;
    };
} // namespace

SusiGlobalHandler::SusiGlobalHandler(const std::string& json_config)
{
    my_susi_callbacks.token = this;

    SusiResult res = SUSI_Initialize(json_config.c_str(), &my_susi_callbacks);
    std::cerr << "Global Susi constructed res=" << std::hex << res << std::dec << std::endl;
    assert(res == SUSI_S_OK);
}

SusiGlobalHandler::~SusiGlobalHandler() noexcept
{
    SusiResult res = SUSI_Terminate();
    std::cerr << "Global Susi destroyed res=" << std::hex << res << std::dec << std::endl;
    assert(res == SUSI_S_OK);
}

namespace
{
    class SusiHolder
    {
    public:
        explicit SusiHolder(const std::string& scannerConfig);
        ~SusiHolder();
        SusiScannerHandle m_handle;
    };
} // namespace

static void throwIfNotOk(SusiResult res, const std::string& message)
{
    if (res != SUSI_S_OK)
    {
        throw std::runtime_error(message);
    }
}

SusiHolder::SusiHolder(const std::string& scannerConfig)
    : m_handle(nullptr)
{
    SusiResult res = SUSI_CreateScanner(scannerConfig.c_str(), &m_handle);
    throwIfNotOk(res, "Failed to create SUSI Scanner");
    std::cerr << "Susi scanner constructed" << std::endl;
}

SusiHolder::~SusiHolder()
{
    if (m_handle != nullptr)
    {
        SusiResult res = SUSI_DestroyScanner(m_handle);
        throwIfNotOk(res, "Failed to destroy SUSI Scanner");
        std::cerr << "Susi scanner destroyed" << std::endl;
    }
}

void mysusi_log_callback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    std::string m(message);
    if (!m.empty())
    {
        std::cerr << level << ": " << m << std::endl;
    }
}

static const SusiLogCallback GL_log_callback{
    .version = SUSI_LOG_CALLBACK_VERSION,
    .token = nullptr,
    .log = mysusi_log_callback,
    .minLogLevel = SUSI_LOG_LEVEL_DETAIL
};

using KeyValueCollection = std::vector<std::pair<std::string, std::string>>;

static std::string orderedStringReplace(const std::string& pattern, const KeyValueCollection& keyvalues)
{
    std::string result;
    size_t beginPos = 0;

    for (const auto& keyvalue : keyvalues)
    {
        const auto& key = keyvalue.first;
        size_t pos = pattern.find(key, beginPos);
        if (pos == std::string::npos)
        {
            break;
        }
        result += pattern.substr(beginPos, pos - beginPos);
        result += keyvalue.second;
        beginPos = pos + key.length();
    }

    result += pattern.substr(beginPos);
    return result;
}

static std::string create_scanner_info(bool scanArchives)
{
    std::string scannerInfo = orderedStringReplace(R"sophos("scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": @@SCAN_ARCHIVES@@,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": true,
                "webEncoding": true,
                "media": true,
                "macintosh": true
            },
            "scanControl": {
                "trueFileTypeDetection": true,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    })sophos", {{"@@SCAN_ARCHIVES@@", scanArchives?"true":"false"}});

    return scannerInfo;
}

static std::string create_runtime_config(const std::string& libraryPath, const std::string& scannerInfo)
{
    std::string runtimeConfig = orderedStringReplace(R"sophos({
    "library": {
        "libraryPath": "@@LIBRARY_PATH@@",
        "tempPath": "/tmp",
        "product": {
            "name": "SSPL AV Plugin",
            "context": "File",
            "version": "1.0.0"
        },
        "customerID": "0123456789abcdef",
        "machineID": "fedcba9876543210"
    },
    @@SCANNER_CONFIG@@
})sophos", {{"@@LIBRARY_PATH@@", libraryPath},
            {"@@SCANNER_CONFIG@@", scannerInfo}
    });
    return runtimeConfig;
}

static std::string create_scanner_config(const std::string& scannerInfo)
{
    return "{"+scannerInfo+"}";
}

int main(int argc, char* argv[])
{
    // std::cout << "SUSI_E_INITIALISING=0x" << std::hex << SUSI_E_INITIALISING << std::dec << std::endl;

    std::string libraryPath = "/home/pair/gitrepos/sspl-tools/sspl-plugin-mav-susi-component/sspl-plugin-mav-susi-component-build/output/susi";
    if (argc > 1)
    {
        libraryPath = argv[1];
    }

    const char* filename = "/etc/fstab";
    if (argc > 2)
    {
        filename = argv[2];
    }

    int fd = ::open(filename, O_RDONLY);
    assert(fd >= 0);

    static const std::string scannerInfo = create_scanner_info(true);
    static const std::string runtimeConfig = create_runtime_config(
        libraryPath,
        scannerInfo
    );

    SusiResult ret = SUSI_SetLogCallback(&GL_log_callback);
    throwIfNotOk(ret, "Failed to set log callback");

    SusiGlobalHandler global_susi(runtimeConfig);

    static const std::string scannerConfig = create_scanner_config(scannerInfo);
    SusiHolder susi(scannerConfig);

    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    SusiScanResult* result = nullptr;
    SusiResult res = SUSI_ScanHandle(susi.m_handle, metaDataJson.c_str(), filename, fd, &result);

    ::close(fd);

    std::cerr << "Scan result " << std::hex << res << std::dec << std::endl;
    if (result != nullptr)
    {
        std::cerr << "Details: "<< result->version << result->scanResultJson << std::endl;
    }

    SUSI_FreeScanResult(result);

    return 0;
}
