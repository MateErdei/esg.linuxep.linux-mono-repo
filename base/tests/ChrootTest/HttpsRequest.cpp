
#include <iostream>
#include <Common/HttpSenderImpl/HttpSender.h>
#include <Common/HttpSender/RequestConfig.h>

#include <unistd.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>


using namespace Common::HttpSenderImpl;
using namespace Common::HttpSender;

int main(/*int argc, char *argv[]*/)
{
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp/testchroot2");

    Common::Logging::FileLoggingSetup setup("chrootTest");

//    const char *defaultServer = "t1.sophosupd.com";
    const char *defaultServer2 = "google.com/";
    const int defaultPort = 443;

    std::shared_ptr<HttpSender> httpSender;
    std::vector<std::string> additionalHeaders;

    std::string m_defaultCertPath;

    std::cout << "Starting doRequest in chroot" << std::endl;
    std::string argument;

    std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
            std::make_shared<Common::HttpSenderImpl::CurlWrapper>();

    httpSender = std::make_shared<HttpSender>(curlWrapper);

    additionalHeaders.emplace_back("header1");


    std::string chrootDir = "/tmp/testchroot/";

    auto fs = Common::FileSystem::fileSystem();
    auto ifperms = Common::FileSystem::FilePermissionsImpl();
    fs->makedirs(chrootDir);
    ifperms.chown(chrootDir, "pair", "pair");


    std::vector<std::pair<std::string, std::string>> mountSources = {
            {"/lib",                               "lib"},
            {"/usr/lib",                           "usr/lib"},
            {"/etc/ssl/certs/ca-certificates.crt", "certs/ca-certificates.crt"},
            {"/etc/hosts",                         "etc/hosts"},
            {"/etc/resolv.conf",                   "etc/resolv.conf"}};

//    std::vector<std::pair<std::string, std::string>> mountSources = {
//            {"/tmp/testdir",  "testdir"},
//            {"/etc/hosts.bk", "etc/hosts.bk"},
//    };

    for (const auto& target : mountSources)
    {
        auto sourcePath = target.first;
        auto targetPath = Common::FileSystem::join(chrootDir, target.second);
        if (fs->isFile(sourcePath)&&!fs->isFile(targetPath))
        {
            auto dirpath = Common::FileSystem::dirName(targetPath);
            std::cout << "dirname " << dirpath << std::endl;
            fs->makedirs(dirpath);
            fs->writeFile(targetPath, "");
            ifperms.chown(targetPath, "pair", "pair"); //test-spl-user //test-spl-grp
            ifperms.chmod(dirpath, 0755);
            //ifperms.chmod(targetPath, 0664);

            Common::SecurityUtils::bindMountReadOnly(sourcePath, targetPath);
        }
        else
        {
            fs->makedirs(targetPath);
            ifperms.chown(targetPath, "pair", "pair");
            ifperms.chmod(targetPath, 0700);
            Common::SecurityUtils::bindMountReadOnly(sourcePath, targetPath);
        }

    }

    //make fake /tmp

    auto fakeTmp = Common::FileSystem::join(chrootDir, "tmp");
    fs->makedirs(fakeTmp);
    ifperms.chown(fakeTmp, "pair", "pair");
    ifperms.chmod(fakeTmp, 0700);
    fs->writeFile(Common::FileSystem::join(fakeTmp, "test.txt"), "testcontent");

    ifperms.chown(Common::FileSystem::join(fakeTmp, "test.txt"), "pair", "pair");
    ifperms.chmod(Common::FileSystem::join(fakeTmp, "test.txt"), 0700);

    Common::SecurityUtils::chrootAndDropPrivileges("pair", "pair", chrootDir);

    fs->writeFile("/tmp/newtest.txt", "newtestcontent");

    Path defaultCertPathAfterGoogle = "/certs/ca-certificates.crt";

    RequestConfig req(RequestType::GET, additionalHeaders, defaultServer2, defaultPort, defaultCertPathAfterGoogle,
                      "");
    auto after_chroot = httpSender->doHttpsRequest(req);
    std::cout << "return code after_chroot" << after_chroot << std::endl;


//Path defaultCertPathAfterSophos = Common::FileSystem::join("/etc", "telemetry_cert.pem");
//    RequestConfig req2(RequestType::GET, additionalHeaders, defaultServer, defaultPort, defaultCertPathAfterSophos,
//                       "linux/prod");
//    auto after_chroot = httpSender->doHttpsRequest(req2);
//    std::cout << "return code" << after_chroot << std::endl;
    return 0;
}