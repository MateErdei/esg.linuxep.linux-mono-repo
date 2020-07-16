
#include <iostream>
#include <Common/HttpSenderImpl/HttpSender.h>
#include <Common/HttpSenderImpl/RequestConfig.h>

#include <unistd.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>


using namespace Common::HttpSenderImpl;

int main(/*int argc, char *argv[]*/)
{
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp/testchroot2");

    Common::Logging::FileLoggingSetup setup("chrootTest");

//    const char *defaultServer = "t1.sophosupd.com";
    const char *defaultServer2 = "https://www.google.com/";
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


    std::string chrootDir = "/tmp/testchroot2/";
    std::string rootDir = "/";
    auto fs = Common::FileSystem::fileSystem();
    auto ifperms = Common::FileSystem::FilePermissionsImpl();
    fs->makedirs(chrootDir);


    std::vector<std::pair<std::string, std::string>> mountSources = {
            {"/lib",                               "lib"},
            {"/usr/lib",                           "usr/lib"},
            {"/etc/ssl/certs/ca-certificates.crt", "certs/ca-certificates.crt"},
            {"/etc/hosts",                         "etc/hosts"},
            {"/etc/resolv.conf",                   "etc/resolv.conf"}};

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
            ifperms.chmod(dirpath, 0775);
            //ifperms.chmod(targetPath, 664);

            Common::SecurityUtils::bindMountFiles(sourcePath, targetPath);
        }
        else
        {
            fs->makedirs(targetPath);
            ifperms.chmod(targetPath, 0775);
            Common::SecurityUtils::bindMountDirectory(sourcePath, targetPath);
        }

    }

    //make fake /tmp

    auto fakeTmp = Common::FileSystem::join(chrootDir, "tmp");
    fs->makedirs(fakeTmp);
    ifperms.chmod(fakeTmp, 0777);
    fs->writeFile(Common::FileSystem::join(fakeTmp, "test.txt"), "testcontent");


    auto libs = fs->listFilesAndDirectories(Common::FileSystem::join(chrootDir, "lib"));
    std::cout << "Libs mount be4 chroot" << "\n";
    for (const auto& lib : libs)
    {
        std::cout << lib << "\n";
    }


    Common::SecurityUtils::chrootAndDropPrivileges("lp", "lp", chrootDir);


    fs->writeFile("/tmp/newtest.txt", "newtestcontent");

    Path defaultCertPathAfterGoogle = "/certs/ca-certificates.crt";

    RequestConfig req(RequestType::GET, additionalHeaders, defaultServer2, defaultPort, defaultCertPathAfterGoogle,
                      "");
    auto after_chroot = httpSender->doHttpsRequest(req);
    std::cout << "return code after_chroot" << after_chroot << std::endl;

    //test unmount
    for (const auto& target : mountSources)
    {
        auto sourcePath = Common::FileSystem::join(rootDir, target.second);
        Common::SecurityUtils::unMount(sourcePath);
    }

//Path defaultCertPathAfterSophos = Common::FileSystem::join("/etc", "telemetry_cert.pem");
//    RequestConfig req2(RequestType::GET, additionalHeaders, defaultServer, defaultPort, defaultCertPathAfterSophos,
//                       "linux/prod");
//    auto after_chroot = httpSender->doHttpsRequest(req2);
//    std::cout << "return code" << after_chroot << std::endl;
    return 0;
}
