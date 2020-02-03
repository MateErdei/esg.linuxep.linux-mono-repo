from os.path import normpath, isfile, isdir, join
import sys
from os import environ, mkdir, makedirs
import os
import subprocess
import base64

class Sign:

    def __init__( self, logger ):
        self.logger = logger

    def signManifest( self, chksumfile, signPrivKey, signPubCert,
                      manifest, opensslExe, interCert ):

        signPubCertData = None
        interCertData = None
        checksum = open(chksumfile, 'rb')

        self.forceUnlink(manifest)
        self.dos2unix(chksumfile, manifest)

        self.logger.statusMessage( "Preparing to sign manifest" )

        sig = self.sign(chksumfile, signPrivKey, opensslExe)

        append = open(manifest, "a+b")
        append.write("-----BEGIN SIGNATURE-----\012")
        append.write(sig)
        append.write("-----END SIGNATURE-----\012")
        append.close()
        signPubCertData = self.dos2unix2(signPubCert, manifest)
        interCertData = self.dos2unix2(interCert, manifest)

        return sig, signPubCertData, interCertData

    def sign( self, datafile, certfile, opensslExe ):
        cmd = [opensslExe,'sha1','-sign',certfile,datafile]
        #~ cmd = '%s sha1 -sign "%s" "%s" | %s base64' % \
                #~ (opensslExe, certfile, datafile, opensslExe)
        self.logger.statusMessage( "Using command: '%s'" % ( str(cmd) ) )
        proc = subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        (stdout,error) = proc.communicate()
        exitStatus = proc.wait()

        if error != "" or exitStatus > 0:
            self.logger.errorMessage( "Failed to sign manifest with exit=%d error='%s'" % ( exitStatus,error ) )
            self.logger.errorMessage( "LD_LIBRARY_PATH='%s'" % (os.environ.get("LD_LIBRARY_PATH","<NONE>")))
            self.logger.errorMessage( "PATH='%s'" % (os.environ.get("PATH","<NONE>") ) )
            raise Exception("Failed to run openssl exit=%d error=%s"%(exitStatus,error))
        else:
            self.logger.statusMessage( "Success" )
            return base64.b64encode(stdout)

    def forceUnlink( self, file ):
        if os.path.exists(file) :
            os.unlink(file)

    def dos2unix( self, filename, output, append = 1 ):
        try:
            os.makedirs(os.path.dirname(output))
        except EnvironmentError:
            pass

        if append:
            out = open(output, "a+b")
        else:
            out = open(output, "w+b")
        for line in open(filename,'rb').readlines():
            out.write(line[:-1] + '\012')
            output = output + line

        return output

    def dos2unix2( self, filename ,output, append = 1 ):
        if append:
            out = open(output, "a+b")
        else:
            out = open(output, "w+b")
        for line in open(filename, 'r').readlines():
            out.write(line[:-1] + '\012')
            output = output + line

        return output

class sdds_platform_class:

    def __init__( self ):

        self.cwd = os.getcwd()
        self.scriptdir = os.path.dirname(sys.argv[0])

        self.PRIVATEKEY_PEM = self.examine("./Authentication/privatekey.pem")
        self.PUBLICCERT_CRT = self.examine("./Authentication/publiccert.crt")
        self.MANIFEST_DAT = self.examine("./Authentication/manifest.dat")
        self.INTERCERT_CRT = self.examine("./Authentication/intercert.crt")

        self.TEMP_DIR = self._getTempDir()
        self.OPENSSL = self._getOpenSSL()

    def examine( self, path ):
        p = os.path.normpath(os.path.join(self.cwd,path))
        if os.path.exists(p): return p

        p2 = os.path.normpath(os.path.join(self.scriptdir,path))
        if os.path.exists(p2): return p2

        return p

    def __setattr__( self, name, value ):
        '''
        Make sure directory structure is prepared for Data files
        '''
        self.__dict__[name] = value
        if name is 'DATA_DIRECTORY':
            try:
                makedirs(value)
            except Exception, e:
                pass

    def _getTempDir( self ):
        '''
        Gets temp directory location (creates it if necessary)
        Default directory is environ(TMPDIR) or environ(TMP) or environ(TEMP)
        Fallback is ./FileDumps (was default so far)
        '''
        try:
            # Regression suite sets this for temp folder
            for envvar in ['TMPDIR', 'TEMP', 'TMP']:
                tmpdir = environ.get(envvar)
                if tmpdir:
                    break

            if not tmpdir:
                tmpdir = '.'

            tmpdir = join(tmpdir, 'FileDump')

            if not isdir(tmpdir):
                mkdir(tmpdir)
        except:
            return '.'

        return tmpdir


    def _getOpenSSL( self ):
        '''
        Get platform dependent location of OpenSSL
        '''
        windows_platforms = ['win32', 'win64', 'cygwin']

        if sys.platform in windows_platforms:
            try:
                win_openssl = environ['SOPHOS_OPENSSL']
            except:
                return None

            if isfile(win_openssl):
                return win_openssl
        else:
            # check linux/unix environment
            try:
                return environ['OPENSSL_PATH']
            except KeyError:
                # fallback to default openssl
                return 'openssl'

sdds_platform = sdds_platform_class()
