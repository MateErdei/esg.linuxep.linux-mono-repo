# Linux Live Response Plugin

This plugin provides the following functionality:

- Live Response (a root remote terminal)  


### How to run and test the rust agent

1. Install base & plugin
2. The Mock LTServer is in liveterminal/ta/scripts/utils/websocket_server
    * python3.7 LTServer.py
        * launch wih python 3.7 and above. its stand alone so grab the whole webscoket_server folder.

3. launch the rust agent 
    * create a json file with named id1*:
        {
            "url":"https://localhost/id1",   
            "thumbprint":"2D03C43BFC9EBE133E0C22DF61B840F94B3A3BD5B05D1D715CC92B2DEBCB6F9D"
        }
    * put the attached file id1 in the plugin install path at  plugins/liveresponse/var/id1:
    * /opt/sophos-spl/plugin/liveresponse/bin/sophos-live-terminal -i id1

4. To send messages from the LTServer command prompt.
    /id1
    ls /tmp
    /id1
    newline

    * The response should be received and displayed in the terminal
5. Other verifications:
    * see the plugins/liveresponse/LiveTerminal.log for the  connection success and to see that linux pty is launched.
    * to test logging manually make changes to base/etc/logger.conf and restart the rust agent it will pick keys:
        * [sophos-liveresponse] & [liveresponse]
        

##How to run cppcheck static analysis
 1. On CI build portal:
    * build-with-parameters: mode=linux-coverage  (static analysis build is bundled with coverage for efficiency)
    * html results published to:    (look at index.html)
        * Artifactory: build artifact analysis.zip 
        * filer6: output/coverage/cppcheck
 2. if ever you need to do this locally 
     * replace line 195 & 196 in "build-files/build.sh" 
        * yum -y install cppcheck -> apt -y install cppcheck
     * build-files/build.sh --analysis 
     * results output/coverage/cppcheck/index.html
