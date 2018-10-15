import os

INST = os.environ.get('INST_DIR', '/tmp/sophos-spl')

def installDir():
    return INST

#base
def basePath():
    return os.path.join(installDir(), 'base')

#update
def updatePath():
    return os.path.join(basePath(), 'update')

def updateVarPath():
    return os.path.join(updatePath(), 'var')

#base/mcs
def mcsPath():
    return os.path.join(basePath(), 'mcs')

def rootCaPath():
    return os.path.join(mcsPath(), "certs/mcs_rootca.crt")

#base/mcs/action
def actionDir():
    return os.path.join(mcsPath(), 'action')

#base/mcs/event
def eventDir():
    return os.path.join(mcsPath(), 'event')

#base/mcs/policy
def policyDir():
    return os.path.join(mcsPath(), 'policy')

def mcsPolicyFile():
    return os.path.join(policyDir(), "MCS-25_policy.xml")

def sophosConfigFile():
    return os.path.join(policyDir(), "sophos.config")

#base/mcs/status
def statusDir():
    return os.path.join(mcsPath(), 'status')

#base/pluginRegistry
def pluginRegistryPath():
    return os.path.join(basePath(), "pluginRegistry")

#base/var/run/sophosspl
def getVarRunDir():
    return os.path.join(basePath(), "var", "run", "sophosspl")

def getUpdateLastEventFile():
    return os.path.join(getVarRunDir(), "update.last_event")

#etc
def etcDir():
    return os.path.join(basePath(), "etc")

def rootConfig():
    return os.path.join(etcDir(),"mcs.config")

def logConfFile():
    return os.path.join( etcDir(),"mcsrouter.log.conf")

def mcsrouterConf():
    return os.path.join( etcDir(),"mcsrouter.conf")

#etc/sophosspl
def sophos_etc_dir():
    return os.path.join(etcDir(), 'sophosspl')

def mcsPolicyConfig():
    return os.path.join(sophos_etc_dir(), "mcs_policy.config")

def sophossplConfig():
    return os.path.join(sophos_etc_dir(), "mcs.config")

#logs/base
def baseLogDir():
    return os.path.join(installDir(), "logs", "base")

def registerLog():
    return os.path.join(baseLogDir(), "registerCentral.log")

#logs/base/sophosspl
def sophossplLogDir():
    return os.path.join(baseLogDir(), "sophosspl")

def mcsrouterLog():
    return os.path.join(sophossplLogDir(), "mcsrouter.log")

def mcsenvelopeLog():
    return os.path.join(sophossplLogDir(), "mcs_envelope.log")

#var
def varDir():
    return os.path.join(installDir(), 'var')

#var/cache
def cacheDir():
    return os.path.join(varDir(), 'cache')


def fragmentedPoliciesDir():
    return os.path.join(cacheDir(), "mcs_fragmented_policies")

#var/lock-sophosspl
def lock_sophos():
    return os.path.join(varDir(), "lock-sophosspl")


def mcsrouterPidFile():
    return os.path.join(lock_sophos(), "mcsrouter.pid")

#tmp
def tempDir():
    return os.path.join(installDir(), 'tmp')

#base/lib:base/lib64
def getInstallationLibPath():
    return os.path.join(installDir(),"base","lib")+":"+os.path.join(installDir(),"base","lib64")

def wdctlBinPath():
    return os.path.join(installDir(), "bin", "wdctl")
