
#include <Susi.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

// NOLINTNEXTLINE
#define P(_X) std::cerr << _X << '\n';

static bool isWhitelistedFile(void *token, SusiHashAlg algorithm, const char *fileChecksum, size_t size)
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

static bool isWhitelistedCert(void *token, const char *fileTopLevelCert, size_t size)
{
    (void)token;
    (void)fileTopLevelCert;
    (void)size;

    P("isWhitelistedCert: " << size);

    return false;
}

static SusiCallbackTable my_susi_callbacks{ //NOLINT
    .version = CALLBACK_TABLE_VERSION,
    .token = nullptr, //NOLINT
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
    P("Global Susi constructed res=" << std::hex << res << std::dec);
    assert(res == SUSI_S_OK);
}

SusiGlobalHandler::~SusiGlobalHandler() noexcept
{
    SusiResult res = SUSI_Terminate();
    P("Global Susi destroyed res=" << std::hex << res << std::dec);
    assert(res == SUSI_S_OK);
}

class SusiHolder
{
public:
    explicit SusiHolder(const std::string& scannerConfig);
    ~SusiHolder();
    SusiScannerHandle m_handle;
};

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
    P("Susi scanner constructed");
}

SusiHolder::~SusiHolder()
{
    if (m_handle != nullptr)
    {
        SusiResult res = SUSI_DestroyScanner(m_handle);
        throwIfNotOk(res, "Failed to destroy SUSI Scanner");
        P("Susi scanner destroyed");
    }
}

void mysusi_log_callback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    std::string m(message);
    if (!m.empty())
    {
        P(level << ": " << m);
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
                "webArchive": @@WEB_ARCHIVES@@,
                "webEncoding": true,
                "media": true,
                "macintosh": true
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    })sophos", {
        {"@@SCAN_ARCHIVES@@", scanArchives?"true":"false"},
        {"@@WEB_ARCHIVES@@",  scanArchives?"true":"false"},
    });

    return scannerInfo;
}

static std::string create_runtime_config(const std::string& libraryPath, const std::string& scannerInfo)
{
    std::string versionNumber = "0.0.1";
    std::string customerId = "c1cfcf69a42311a6084bcefe8af02c8a";
    std::string endpointId = "66b8fd8b39754951b87269afdfcb285c";
    std::string runtimeConfig = orderedStringReplace(R"sophos({
    "library": {
        "libraryPath": "@@LIBRARY_PATH@@",
        "tempPath": "/tmp",
        "product": {
            "name": "SUSI_SPLAV",
            "context": "File",
            "version": "@@VERSION_NUMBER@@"
        },
        "globalReputation": {
            "customerID": "@@CUSTOMER_ID@@",
            "machineID":  "@@MACHINE_ID@@",
            "timeout": 5,
            "enableLookup": true
        }
    },
    @@SCANNER_CONFIG@@
})sophos", { { "@@LIBRARY_PATH@@", libraryPath },
            { "@@VERSION_NUMBER@@", versionNumber },
            { "@@CUSTOMER_ID@@", customerId },
            { "@@MACHINE_ID@@", endpointId },
            { "@@SCANNER_CONFIG@@", scannerInfo }
    });
    return runtimeConfig;
}

static std::string create_scanner_config(const std::string& scannerInfo)
{
    return "{"+scannerInfo+"}";
}

class AutoFd
{
    int m_fd;
public:
    explicit AutoFd()
        : m_fd(-1)
    {}
    explicit AutoFd(int fd)
        : m_fd(fd)
    {}

    ~AutoFd()
    {
        close();
    }

    [[nodiscard]] int fd() const
    {
        return m_fd;
    }

    void close()
    {
        if (m_fd >= 0)
        {
            ::close(m_fd);
        }
        m_fd = -1;
    }
};

class AutoScanResult
{
public:
    SusiScanResult* m_result = nullptr;
    ~AutoScanResult()
    {
        if (m_result != nullptr)
        {
            SUSI_FreeScanResult(m_result);
        }
    }
    SusiScanResult* operator->() const
    {
        return m_result;
    }
    explicit operator bool() const
    {
        return m_result != nullptr;
    }
};

static void scan(const std::string& scannerConfig, const char* filename)
{
    AutoFd fd(::open(filename, O_RDONLY));
    assert(fd.fd() >= 0);

    SusiHolder susi(scannerConfig);


    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    AutoScanResult result;
    SusiResult res = SUSI_ScanHandle(susi.m_handle, metaDataJson.c_str(), filename, fd.fd(), &result.m_result);
    fd.close();

    std::cerr << "Scan result " << std::hex << res << std::dec << std::endl;
    if (result)
    {
        std::cerr << "Details: "<< result->version << result->scanResultJson << std::endl;
    }
}

int main(int argc, char* argv[])
{
    // std::cout << "SUSI_E_INITIALISING=0x" << std::hex << SUSI_E_INITIALISING << std::dec << std::endl;

    std::string libraryPath = "/opt/sophos-spl/plugins/av/chroot/susi/distribution_version";
    if (argc > 1)
    {
        libraryPath = argv[1];
    }

    const char* filename = "/etc/fstab";
    if (argc > 2)
    {
        filename = argv[2];
    }

    static const std::string scannerInfo = create_scanner_info(true);
    static const std::string runtimeConfig = create_runtime_config(
        libraryPath,
        scannerInfo
    );
    static const std::string scannerConfig = create_scanner_config(scannerInfo);

    SusiResult susi_ret = SUSI_SetLogCallback(&GL_log_callback);
    throwIfNotOk(susi_ret, "Failed to set log callback");

    // Get and set file descriptor limit
    struct rlimit rlimitBuf{};
    int ret = getrlimit(RLIMIT_NOFILE, &rlimitBuf);
    assert(ret == 0);
    P("FD softlimit: "<< rlimitBuf.rlim_cur);
    P("FD hardlimit: "<< rlimitBuf.rlim_max);
    static const int MAX_FD = 100;
    assert(rlimitBuf.rlim_max >= MAX_FD);
    rlimitBuf.rlim_cur = MAX_FD;
    ret = setrlimit(RLIMIT_NOFILE, &rlimitBuf);
    assert(ret == 0);

    SusiGlobalHandler global_susi(runtimeConfig);

    scan(scannerConfig, filename);

    return 0;
}
