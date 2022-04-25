
#include <Susi.h>

//#include <condition_variable>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>

static const int FILE_DESCRIPTOR_LIMIT = 150;
static const int NUMBER_OF_SCANS_PER_THREAD = 10; // 500;
static const int THREAD_COUNT = 100;

// NOLINTNEXTLINE
#define P(_X) std::cerr << _X << '\n'

#define ASSERT(_X) do { if (!(_X)) { P("!" #_X); abort(); } } while(false)

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

SusiCallbackTable my_susi_callbacks{ //NOLINT
    .version = SUSI_CALLBACK_TABLE_VERSION,
    .token = nullptr, //NOLINT
    .IsAllowlistedFile = isAllowlistedFile,
    .IsBlocklistedFile = IsBlocklistedFile,
    .IsTrustedCert = isTrustedCert,
    .IsAllowlistedCert = isAllowlistedCert
};

namespace
{
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
        ASSERT(res == SUSI_S_OK);
    }

    SusiGlobalHandler::~SusiGlobalHandler() noexcept
    {
        SusiResult res = SUSI_Terminate();
        P("Global Susi destroyed res=" << std::hex << res << std::dec);
        ASSERT(res == SUSI_S_OK);
    }

    class SusiHolder
    {
    public:
        explicit SusiHolder(const std::string& scannerConfig);
        ~SusiHolder();
        SusiScannerHandle m_handle;
    };
}

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

static thread_local int GL_THREAD_ID=0; //NOLINT

void mysusi_log_callback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    if (level == SUSI_LOG_LEVEL_DETAIL)
    {
        return;
    }
    std::string m(message);
    if (!m.empty())
    {
        // m already has new line
        std::cerr << GL_THREAD_ID << ": " << m;
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
        reset();
    }
    SusiScanResult* operator->() const
    {
        return m_result;
    }
    explicit operator bool() const
    {
        return m_result != nullptr;
    }
    void reset()
    {
        if (m_result != nullptr)
        {
            SUSI_FreeScanResult(m_result);
        }
        m_result = nullptr;
    }
};

static void scan(int thread, const std::string& scannerConfig, const char* filename)
{
    GL_THREAD_ID = thread;
    AutoFd fd(::open(filename, O_RDONLY));
    ASSERT(fd.fd() >= 0);

    SusiHolder susi(scannerConfig);
    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    AutoScanResult result;


    for (int i=0; i<NUMBER_OF_SCANS_PER_THREAD; i++)
    {
        P("THREAD:" << thread <<  ":" << i << ": Start scan");
        ASSERT(result.m_result == nullptr);
        ASSERT(!result);
        SusiResult res = SUSI_ScanHandle(susi.m_handle, metaDataJson.c_str(), filename, fd.fd(), &result.m_result);

        if (result)
        {
            P("THREAD:" << thread << ": Details: " << result->version << result->scanResultJson);
        }
        if (res == SUSI_E_INVALIDARG)
        {
            P("THREAD:" << thread << ":" << i << ": Scan result: SUSI_E_INVALIDARG: " << std::hex << res << std::dec);
            ASSERT(!"SUSI_E_INVALIDARG");
        }
        else if (res == SUSI_S_OK)
        {
            P("THREAD:" << thread << ":" << i << ":Clean");
        }
        else if (res == SUSI_I_THREATPRESENT)
        {
            P("THREAD:" << thread << ":" << i << ":ThreatFound");
        }
        else
        {
            P("THREAD:" << thread << ":" << i << ": Scan result " << std::hex << res << std::dec);
            ASSERT(!"Unknown scan result");
        }

        result.reset();
        ::lseek(fd.fd(), 0, SEEK_SET);
        std::this_thread::yield();
    }
    fd.close();
}

int main(int argc, char* argv[])
{
    // std::cout << "SUSI_E_INITIALISING=0x" << std::hex << SUSI_E_INITIALISING << std::dec << std::endl;

    const char* filename = "/etc/fstab";
    if (argc > 1)
    {
        filename = argv[1];
    }

    std::string libraryPath = "/opt/sophos-spl/plugins/av/chroot/susi/distribution_version";
    if (argc > 2)
    {
        libraryPath = argv[2];
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
    ASSERT(ret == 0);
    P("FD softlimit: "<< rlimitBuf.rlim_cur);
    P("FD hardlimit: "<< rlimitBuf.rlim_max);
    ASSERT(rlimitBuf.rlim_max >= FILE_DESCRIPTOR_LIMIT);
    rlimitBuf.rlim_cur = FILE_DESCRIPTOR_LIMIT;
    ret = setrlimit(RLIMIT_NOFILE, &rlimitBuf);
    ASSERT(ret == 0);

    SusiGlobalHandler global_susi(runtimeConfig);

    std::vector<std::thread> threads;

    for (int i=1; i<=THREAD_COUNT; ++i) // Thread 0 is main...
    {
        threads.emplace_back(scan, i, scannerConfig, filename);
    }
    std::this_thread::yield();

    for (auto& th : threads)
    {
        th.join();
    }

    return 0;
}
