
#include <SusiTypes.h>
#include <Susi.h>

#include <string>
#include <iostream>

#include <cassert>

static bool isWhitelistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)fileChecksum;
    (void)size;
    return false;
}

static SusiCertTrustType isTrustedCert(void *token, SusiHashAlg algorithm, const char *pkcs7, size_t size)
{
    (void)token;
    (void)algorithm;
    (void)pkcs7;
    (void)size;

    return SUSI_TRUSTED;
}

static bool isWhitelistedCert(void *token, const char *fileTopLevelCert, size_t size)
{
    (void)token;
    (void)fileTopLevelCert;
    (void)size;

    return false;
}

static SusiCallbackTable my_susi_callbacks{
        .version = CALLBACK_TABLE_VERSION,
        .token = nullptr,
        .IsWhitelistedFile = isWhitelistedFile,
        .IsTrustedCert = isTrustedCert,
        .IsWhitelistedCert = isWhitelistedCert
};

class SusiGlobalHandler
{
public:
    explicit SusiGlobalHandler(const std::string& json_config);
    ~SusiGlobalHandler() noexcept;
};

SusiGlobalHandler::SusiGlobalHandler(const std::string& json_config)
{
    my_susi_callbacks.token = this;

    SusiResult res = SUSI_Initialize(json_config.c_str(), &my_susi_callbacks);
    std::cerr << "Global Susi constructed res=" << std::hex << res << std::dec << std::endl;
    assert(res == SUSI_S_OK);
}

SusiGlobalHandler::~SusiGlobalHandler()
{
    SusiResult res = SUSI_Terminate();
    std::cerr << "Global Susi destroyed res=" << std::hex << res << std::dec << std::endl;
    assert(res == SUSI_S_OK);
}

class SusiHolder
{
public:
    explicit SusiHolder(const std::string& scannerConfig);
    ~SusiHolder();
    SusiScannerHandle m_handle;
};

SusiHolder::SusiHolder(const std::string& scannerConfig)
    : m_handle(nullptr)
{
    SusiResult res = SUSI_CreateScanner(scannerConfig.c_str(), &m_handle);
    assert(res == SUSI_S_OK);
    std::cerr << "Susi scanner constructed" << std::endl;
}

SusiHolder::~SusiHolder()
{
    if (m_handle != nullptr)
    {
        SusiResult res = SUSI_DestroyScanner(m_handle);
        assert(res == SUSI_S_OK);
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

int main(int argc, char* argv[])
{
    // std::cout << "SUSI_E_INITIALISING=0x" << std::hex << SUSI_E_INITIALISING << std::dec << std::endl;

    SusiResult ret;
    ret = SUSI_SetLogCallback(&GL_log_callback);
    assert(ret == SUSI_S_OK);

    static const std::string config = R"({
    "library": {
        "libraryPath": "/home/pair/gitrepos/susi_experiment/susi",
        "tempPath": "/tmp",
        "product": {
            "name": "DLCL_Experiment",
            "context": "File",
            "version": "1.0.0"
        },
        "customerID": "0123456789abcdef",
        "machineID": "fedcba9876543210"
    },
    "scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": true,
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
                "puaDetection": true,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    }
})";
    SusiGlobalHandler global_susi(config);

    static const std::string scannerConfig = R"({
    "scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": true,
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
                "puaDetection": true,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    }
})";

    SusiHolder susi(scannerConfig);

    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    const char* filename = "/etc/fstab";
    if (argc > 1)
    {
        filename = argv[1];
    }

    SusiScanResult* result = nullptr;
    SusiResult res = SUSI_ScanFile(susi.m_handle, metaDataJson.c_str(), filename, &result);

    std::cerr << "Scan result " << std::hex << res << std::dec << std::endl;
    if (result != nullptr)
    {
        std::cerr << "Details: "<< result->version << result->scanResultJson << std::endl;
    }

    SUSI_FreeScanResult(result);

    return 0;
}
