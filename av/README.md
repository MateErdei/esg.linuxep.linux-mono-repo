# Example Plugin for Everest

Description of Example Plugin and Architecture: https://wiki.sophos.net/display/SAVLU/Example+Plugin+Architecture

Ensure you have setup SSH keys in Stash, and have permission to clone this repo and the Everest-SystemProductTests repo.
Email douglas.leeder@sophos.com to get repository access.

###Running the Example Plugin

Primarily, clone this repository and open it in CLion. Switch to the ExamplePlugin git branch.

The plugin should be run against the fake Management Agent script which is included in the Everest-SystemProductTests repository:

1. Clone the Everest-SystemProductTests repository.
2. In `./scripts/FakeManagementPath.sh` (ExamplePlugin Repository), modify `FAKEManagementDirPath` to point to your local `everest-systemproducttests/SupportFiles/FakeManagementAgent` directory.
3. Open a terminal and navigate to the root of the ExamplePlugin Repository.
4. Run `./scripts/startFakeManagementAgent.sh` in the terminal.

To run the Example Plugin in CLion:

1. Run the fake Management Agent using the steps above.
2. Ensure CLion is configured to run the exampleplugin Run Configuration.
3. Set `SOPHOS_INSTALL=/tmp/exampleplugin` in the Environment Variables section in CLion's Run Configuration.

###Exercising the Example Plugin
Once the Example Plugin and the Fake Management Agent are running, commands can be sent from the Management Agent to the Example Plugin using the following scripts from a terminal:

* ####Sending Policies

```bash
./scripts/sendPolicy.sh ./scripts/dataExamples/<SAV Policy>
```
`./scripts/dataExamples` contains three different SAV Policies which can be sent to the Plugin. The Plugin will queue this request and respond with a SAV status.


* ####Sending Actions

```bash
./scripts/sendAction.sh ./scripts/dataExamples/ScanNowAction.xml
```
This will cause the example plugin to queue a scan of the `/tmp` directory. Once completed, it will respond with virus detection events if it found any files containing the string "VIRUS".

* ####Get Status

```bash
./scripts/getStatus.sh
```

This will cause the example plugin to queue the status request and eventually respond with its current SAV status.

* ####Get Telemetry

```bash
./scripts/getTelemetry.sh
```

This will cause the example plugin to respond with its current telemetry.