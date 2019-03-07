
These scripts are designed to gather remote ssh data for specific cases of authentication. These match the data
collected in sspl-tools for SSH with localhost done via the auditdScript.py


1.  Copy the keys from:
        /redist/binaries/linux11/input/auditDTestFiles/id_rsa_vagrant
        /redist/binaries/linux11/input/auditDTestFiles/id_rsa_vagrant.pub
    Into this folder

2.  Copy this folder to a machine which you want to get remote ssh logs for.
        scp -r ../ManualRemoteSSH <user>@<ip address of target>:

3.  Open two terminals, one in this folder and one on the target machine.

NOT AMAZON

4.  In the terminal of the target machine become root (sudo su) and then run:
        ./CollectData.sh <ip address of target>

AMAZON

4.  In the terminal of the target machine become root (sudo su) and then run:
        ./CollectData.sh <ip address of target> AWS

    The second argument can be anything, but its presence informs the script that it should run in aws mode

ALL

5.  Follow the instructions which will be of the form:
        Run
        ./Local-failed_ssh_multiple_attempts_with_key.sh <ip address or amazon address of target>
        Have you run the script

    i.e. copy the command after run into the local terminal, run it and then return to the target machine and press enter after
    the script is run on the local machine

    Each command run locally will be matched with a clearing of the logs and gathering of the logs on the remote machine

    NOTE if you make a mistake you can rerun the script and skip collecting data on any step by entering anything after
    "Have you run the script" and it won't gather the logs
    e.g.
        Have you run the script<anystringhere> <return>
    Will not collect or over write any logs


6.  Copy the *.log and *.bin files into a folder with the platform name
    mkdir rhel
    cp *.log *.bin rhel/

7.  Copy the files back to the local machine and check them into

    sspl-tools/everest-systemproducttests/SupportFiles/auditd_data/<platform>

    And check them in!

