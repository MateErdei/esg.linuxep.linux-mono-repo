#include <iostream>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <chrono>
#include <thread>
#include <string.h>
#include <sys/sysmacros.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <signal.h>

/** This process is to be executed by the JailBreakerTest. 
 *  It draws inspiration from https://github.com/earthquake/chw00t/blob/master/chw00t.c to check possible techiniques to escape chroot. 
 *  
 *  /opt/sophos-spl/base/bin/JailBreakerTest sophos-spl-local sophos-spl-network path-to-JailScapeTechiniques
 * Example: 
 * /opt/sophos-spl/base/bin/JailBreakerTest sophos-spl-local sophos-spl-network /vagrant/everest-base/tests/manualTools/JailScapeTechiniques
 *  
 *  The files will be created in /tmp/testBreak/jail and can be inspected aftewards. 
 * 
 *  Also, as some of the techiniques require that other processes are to be running as the same user in the jail (with the above command sophos-spl-network)
 *  But this command does not receive arguments, this is what you should do. 
 *  
 *  1. Run a bash command as sophos-spl-network
 *     su sophos-spl-network -s /bin/bash
 *  2. Get the pid of the bash command running
 *     type ps and check the pid of the entry bash. 
 *  3. Create a file /tmp/testBreak/jail/sibling.pid with the given pid as the first line. 
 * 
 *  For example: 
root@vagrant:~# su sophos-spl-network -s /bin/bash
sophos-spl-network@vagrant:/root$ echo $$ > /tmp/testBreak/jail/sibling.pid
sophos-spl-network@vagrant:/root$
 * 
 *  Some of the techiniques will try to ptrace into that process or kill that process. 
 *  
 *  To build this application, you must link it statically, do the following: 
 * 
 *  g++ -std=c++11 -static JailScapeTechiniques.cpp -o JailScapeTechiniques
 *  
 * */


/** Return the first integer in the file /sibling.pid or 0 if not found of file not present. As there is no pid with number 0, this can be used to verify if it is a valid pid*/
int pidOfSibling()
{
    std::ifstream input{"/sibling.pid"}; 
    int pid; 
    input >> pid; 
    return pid;  
}

/** The principle here is that the parent of / is /. The path: /../ is the same as /. 
 *   Usually, this can be used to get a 'fullpath' using relative path. 
 * */
int movetotheroot()
{
    int i;

    for (i = 0; i < 255; i++)
    {
	if (chdir(".."))
	    return 0xDEADBEEF;
    }

    return 0;
}

/** Auxiliary method to list all the entries in the directory to allow for inspection of what the process can see. 
 * */
int listDirectories(const std::string & directoryPath, std::string & out )
{
    static std::string dot{ "." };
    static std::string dotdot{ ".." };

    DIR* directoryPtr;
    struct dirent* outDirEntity;
    directoryPtr = opendir(directoryPath.c_str());

    if (!directoryPtr)
    {
        return errno; 
    }
    while (true)
    {
        errno = 0;
        outDirEntity = readdir(directoryPtr);

        if (errno != 0 || outDirEntity == nullptr)
        {
            break;
        }

        if (outDirEntity->d_name == dot || outDirEntity->d_name == dotdot)
        {
            continue;
        }

        std::string fullPath = directoryPath + "/" +  outDirEntity->d_name;
        out += fullPath + "\n"; 
    }

    (void)closedir(directoryPtr);
    return 0; 
}

/**If the jailed process were to restore root capability it would be able to circunvent restrictions*/
void verifyJailedProcessCanNotChangeUID()
{    
    std::stringstream report; 
    report << "\n\n------------------------------\n";
    report << "Test Can change UID\n\n"; 
    bool allStepsPassed = false; 
    if ( setreuid(0,0) == -1)
    {
        if ( errno != EPERM)
            report << "setreuid passed: " << strerror(errno) << "\n"; 
        if ( setuid(0) == -1 )
        {
            if ( errno != EPERM)
                report << "setuid passed: " << strerror(errno) << "\n"; 
            if ( seteuid(0) == -1 )
            {
                if ( errno != EPERM)
                    report << "seteuid passed: " << strerror(errno) << "\n"; 
                if (setresuid(0, 0, 0 ))
                {
                    if ( errno != EPERM)
                        report << "setresuid passed: " << strerror(errno) << "\n"; 
                    allStepsPassed = true; 
                }
            }
        }

    }
    std::cout << report.str(); 
    if (allStepsPassed)
    {
        std::cout << "\n\nPASSED: Jailed process can not change UID. " << std::endl; 
    }
    else
    {
        std::cerr << "\n\nFAILED: Jailed process can change UID. " << std::endl; 
    }
}


void verifyJailedProcessCanNotUseMknod()
{
    bool allStepsPassed=true; 
    std::stringstream report; 
    report << "\n\n------------------------------\n";
    report << "Test Can use mknod\n\n"; 
    if ( mkdir("/chwroot", -1) == -1 )
    {
        if ( errno != EEXIST)
            report << "could not create chwroot dir :" << strerror(errno) << std::endl; 
    }

    for(int i=0; i<196; i++)
    {
        if (mknod("/chwroot/test", S_IFBLK, makedev(8, i)) != 0)
        {
            int err = errno; 
            if (err != EPERM)
            {
                allStepsPassed = false; 
                report << "Not expected reason for failed to create mknod. "<< err << " " << strerror(err) << "\n"; 
            }
        }
        else
        {
            report << "Managed to create mknod entry for makedev(8,"<< i<< ")\n"; 
            allStepsPassed = false; 
        }
        
    }
    std::cout << report.str(); 
    if (allStepsPassed)
    {
        std::cout << "\n\nPASSED: Jailed process can not use mknod. " << std::endl; 
    }
    else
    {
        std::cerr << "\n\nFAILED: Jailed process can create mknod entries. " << std::endl; 
    }    
}


void verifyJailedProcessCanNotMountProc()
{
    bool allStepsPassed=true; 
    std::stringstream report; 
    report << "\n\n------------------------------\n";
    report << "Test Can Mount Proc\n\n"; 

    if ( mkdir("/proc", -1) == -1 )
    {
        if ( errno != EEXIST)
            report << "could not create proc dir :" << strerror(errno) << std::endl; 
    }

    // using mount
    if (mount("proc", "/proc", "proc", 0, NULL) == -1 )
    {        
        if (errno != EPERM)
        {
            report << "Failed to mount proc for different reason: " << strerror(errno) << "\n";
            allStepsPassed = false;  
        }
    }
    else
    {
        allStepsPassed = false; 
        std::cout << "proc mounted" << std::endl; 
    }


    std::cout << report.str(); 
    if (allStepsPassed)
    {
        std::cout << "\n\nPASSED: Jailed process can not mount proc. " << std::endl; 
    }
    else
    {
        std::cerr << "\n\nFAILED: Jailed process can mount proc. " << std::endl; 
    }    
}


void verifyJailedProcessCanNotUsePtrace()
{
    bool allStepsPassed=true; 
    bool validSibling = true; 
    std::stringstream report; 
    report << "\n\n------------------------------\n";
    report << "Test Cannot use ptrace\n\n"; 

    int pid = pidOfSibling(); 
    if ( pid == 0 )
    {   
        validSibling = false; 
        report << "No sibling pid avaliable. Please, make sure that /sibling.pid has a valid pid of a process running as the jailed user.\n"; 
    }
    else
    {
        if (ptrace(PTRACE_ATTACH, pid, 0, 0) == -1)
        {
            if ( errno == ESRCH)
            {
                validSibling = false; 
                report << "Sibling pid: " << pid << " is not a valid pid";                 
            }
            else if ( errno != EPERM )
            {
                report << "Failed to attach ptrace: " << errno << ": " << strerror(errno) << "\n";
                allStepsPassed = false;  
            }            
        }    
        else
        {
            report << "Managed to start a ptrace to pid: " << pid << "\n"; 
            allStepsPassed = false; 
        }
    }

    std::cout << report.str(); 
    if ( !validSibling)
    {
        std::cout << "\n\nSKIPPED: Can not check ptrace. " << std::endl; 
        return;
    }
    if (allStepsPassed)
    {
        std::cout << "\n\nPASSED: Jailed process can not use ptrace. " << std::endl; 
    }
    else
    {
        std::cerr << "\n\nFAILED: Jailed process can use ptrace. " << std::endl; 
    }    
}


void verifyJailedProcessCanNotSendKillSignalToOtherProcesses()
{
    bool allStepsPassed=true;
    bool validSibling = true; 
    std::stringstream report; 
    report << "\n\n------------------------------\n";
    report << "Test Cannot send kill to other processes\n\n"; 

    int pid = pidOfSibling(); 
    if ( pid == 0 )
    {   
        validSibling = false; 
        report << "No sibling pid avaliable. Please, make sure that /sibling.pid has a valid pid of a process running as the jailed user.\n"; 
    }
    else
    {
        if (kill(pid, SIGHUP) == -1)
        {
            if (errno == ESRCH)
            {
                report << "Can not verify that the process is present. Pid: " << pid << std::endl; 
                validSibling = false;
            }
            else
            {
                report << "Failed to send kill to pid " << pid << " for the following reason: " << errno << strerror(errno); 
            }
        }
        else
        {
            if ( kill(-1, SIGTERM) == -1 )
            {                
                // this is good, but not the behaviour right now. 
                report << "Failed to send kill signal for the following reason: " << strerror(errno) << std::endl; 
            }
            else
            {
                report << "Warning: it managed to send kill signal to -1. Other processes runing as the same user could be killed in a denail of service.\n"; 
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }

            if ( kill(pid, SIGTERM) == -1 )
            {
                if ( errno==ESRCH)
                {
                    report << "Process " << pid << " that was running before, was killed with the -1 'attack'" << std::endl; 
                    allStepsPassed = false; 
                }
                else
                {
                    report << " Failed to send kill to " << pid << " for the following reason: " << errno << " " << strerror(errno) << std::endl; 
                }
                
            }

        }

    }


    std::cout << report.str(); 
    if ( !validSibling)
    {
        std::cout << "\n\nSKIPPED: Can not check sending kill. " << std::endl; 
        return;
    }

    if (allStepsPassed)
    {
        std::cout << "\n\nPASSED: Jailed process can not send kill. " << std::endl; 
    }
    else
    {
        std::cerr << "\n\nFAILED: Jailed process can send kill signals. " << std::endl; 
    }    
}

void verifyNoOpenDirectoryIsAvailableToUseToEscapeRoot()
{
    bool allStepsPassed=true; 
    std::stringstream report; 
    report << "\n\n------------------------------\n";
    report << "Test Cannot use open directory to escape root\n\n"; 
  
    for(int i=0; i<255; i++)
    {
        DIR* directoryPtr = fdopendir(i); 
        if (directoryPtr ==nullptr)
        {
            int errcode = errno; 
            if ( ! (errcode == ENOTDIR || errcode == EBADF))
            {
                report << "Found an opened directory as file descriptor. Number: " << i  << " errorcode: " << errcode << ": " << strerror(errcode); 
                allStepsPassed = false; 
            }
        }
    }

    std::cout << report.str(); 
    if (allStepsPassed)
    {
        std::cout << "\n\nPASSED: Jailed process can not leverage previous opened directory. " << std::endl; 
    }
    else
    {
        std::cerr << "\n\nFAILED: Jailed process can leverage previous opened directory. " << std::endl; 
    }    

}



void verifyJailedProcessCannotScapeChroot()
{
    bool allStepsPassed=true; 
    std::stringstream report; 
    report << "\n\n------------------------------\n";
    report << "Test Cannot escape chroot\n\n"; 

    movetotheroot(); 
    std::string listfiles; 
    int errcode =  listDirectories("/tmp", listfiles); 
    if (errcode != ENOENT)
    {
        report << "Managed to escape the chroot. Errcode: " << errcode << " " << strerror(errcode) << " Proof: list of files in /tmp\n" ; 
        report << listfiles; 
        allStepsPassed = false; 
    }

    chdir("/lib"); 
    errcode =  listDirectories("/tmp", listfiles); 
    if (errcode != ENOENT)
    {
        report << "Managed to escape the chroot. Errcode: " << errcode << " " << strerror(errcode) << " Proof: list of files in /tmp\n" ; 
        report << listfiles; 
        allStepsPassed = false; 
    }


    std::cout << report.str(); 
    if (allStepsPassed)
    {
        std::cout << "\n\nPASSED: Jailed process can not escape chroot. " << std::endl; 
    }
    else
    {
        std::cerr << "\n\nFAILED: Jailed process can not escape chroot. " << std::endl; 
    }    
}



int main(int argc, char* argv[])
{
    std::cout << "Running Tests: \n\n-------------------------------------------\n"; 
    verifyJailedProcessCanNotChangeUID(); 

    verifyJailedProcessCanNotUseMknod(); 

    verifyJailedProcessCanNotMountProc(); 

    verifyJailedProcessCanNotUsePtrace(); 
    verifyJailedProcessCanNotSendKillSignalToOtherProcesses();

    verifyNoOpenDirectoryIsAvailableToUseToEscapeRoot(); 

    verifyJailedProcessCannotScapeChroot(); 

    return 0;
}
