
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
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp/testchroot");

    Common::Logging::FileLoggingSetup setup("chrootTest");

//    const char *defaultServer = "t1.sophosupd.com";
    const char *defaultServer2 = "google.com";
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
    std::string rootDir = "/";
    auto fs = Common::FileSystem::fileSystem();
    auto ifperms = Common::FileSystem::FilePermissionsImpl();
    fs->makedirs(chrootDir);


    std::vector<std::string> mountSources = {
            "lib", "usr/lib", "etc/ssl/certs/ca-certificates.crt", "etc/hosts", "etc/resolv.conf"
    };

    for (const auto& target : mountSources)
    {
        auto sourcePath = Common::FileSystem::join(rootDir, target);
        auto targetPath = Common::FileSystem::join(chrootDir, target);
        if (fs->isFile(sourcePath)&&!fs->isFile(targetPath))
        {
            auto dirpath = Common::FileSystem::dirName(targetPath);
            std::cout << "dirname " << dirpath << std::endl;
            fs->makedirs(dirpath);
            fs->writeFile(targetPath, "");
            ifperms.chmod(dirpath, 0775);
            ifperms.chmod(targetPath, 664);

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
    ifperms.chmod(Common::FileSystem::join(fakeTmp, "test.txt"), 0664);


    auto libs = fs->listFilesAndDirectories("/tmp/testchroot/lib");
    std::cout << "Libs mount be4 chroot" << "\n";
    for (const auto& lib : libs)
    {
        std::cout << lib << "\n";
    }


    Common::SecurityUtils::chrootAndDropPrivileges("lp", "lp", chrootDir);


    fs->readFile("/tmp/test.txt");
    fs->writeFile("/tmp/newtest.txt", "newtestcontent");
    fs->removeFile("/tmp/test.txt");

    auto dirsInLibs = fs->listDirectories("/etc");
    for (auto item : dirsInLibs)
    {
        std::cout << item << std::endl;
    }

    Path defaultCertPathAfterGoogle = "etc/ssl/certs/ca-certificates.crt";

    RequestConfig req3(RequestType::GET, additionalHeaders, defaultServer2, defaultPort, defaultCertPathAfterGoogle,
                       "");
    auto after_chroot2 = httpSender->doHttpsRequest(req3);
    std::cout << "return code" << after_chroot2 << std::endl;

    //test unmount
    for (const auto& target : mountSources)
    {
        auto sourcePath = Common::FileSystem::join(rootDir, target);
        Common::SecurityUtils::unMount(sourcePath);
    }

//Path defaultCertPathAfterSophos = Common::FileSystem::join("/etc", "telemetry_cert.pem");
//    RequestConfig req2(RequestType::GET, additionalHeaders, defaultServer, defaultPort, defaultCertPathAfterSophos,
//                       "linux/prod");
//    auto after_chroot = httpSender->doHttpsRequest(req2);
//    std::cout << "return code" << after_chroot << std::endl;
    return 0;
}
