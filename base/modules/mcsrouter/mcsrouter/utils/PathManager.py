import os

INST = os.environ.get('INST_DIR', '/tmp/sophos-spl')

def installDir():
    return INST

def basePath():
    return os.path.join(INST, 'base')

def mcsPath():
    return os.path.join(basePath(), 'mcs')

def baseLogDir():
    return os.path.join(installDir(), "logs", "base")

def registerLog():
    return os.path.join(baseLogDir(), "registerCentral.log")

def sophossplLogDir():
    return os.path.join(baseLogDir(), "sophosspl")

def mcsrouterLog():
    return os.path.join(sophossplLogDir(), "mcsrouter.log")

def mcsenvelopeLog():
    return os.path.join(sophossplLogDir(), "mcs_envelope.log")

def varDir():
    return os.path.join(installDir(), 'var')

def cacheDir():
    return os.path.join(varDir(), 'cache')

def fragmentedPoliciesDir():
    return os.path.join(cacheDir(), "mcs_fragmented_policies")

def lock_sophos():
    return os.path.join(varDir(), "lock-sophosspl")

def sophos_etc_dir():
    return os.path.join(installDir(), 'etc', 'sophosspl')

def policyDir():
    return os.path.join(mcsPath(), 'policy')

def eventDir():
    return os.path.join(mcsPath(), 'event')

def actionDir():
    return os.path.join(mcsPath(), 'action')

def statusDir():
    return os.path.join(mcsPath(), 'status')

def tempDir():
    return os.path.join(installDir(), 'tmp')

def getInstallationLibPath():
    return os.path.join(installDir(),"base","lib")+":"+os.path.join(installDir(),"base","lib64")

def getVarRunDir():
    return os.path.join(basePath(), "var", "run", "sophosspl")

def pluginRegistryPath():
    return os.path.join(basePath(), "pluginRegistry")

# files

def rootConfig():
    return os.path.join(installDir(),"etc","mcs.config")

def mcsPolicyConfig():
    return os.path.join(sophos_etc_dir(), "mcs_policy.config")
def sophossplConfig():
    return os.path.join(sophos_etc_dir(), "mcs.config")
def rootCaPath():
    return os.path.join(mcsPath(),"mcs_rootca.crt")

def logConfFile():
    return os.path.join( installDir(),"etc","mcsrouter.log.conf")

def mcsrouterConf():
    return os.path.join( installDir(),"etc","mcsrouter.conf")