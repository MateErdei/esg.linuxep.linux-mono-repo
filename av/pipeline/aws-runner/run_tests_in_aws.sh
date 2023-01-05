#!/bin/bash

set -x

INCLUDE_TAG="$1"

SSHLocation=${SSHLocation:-"217.38.22.0/24"}

IDENTIFIER=$(hostname)-$(date +%F)-$(date +%H)$(date +%M)
[[ -n $STACK ]] || STACK=ssplav-system-tests-${IDENTIFIER}-$(echo "$*"-${RANDOM} | md5sum | cut -f 1 -d " " )

function failure()
{
    echo "$@"
    exit 1
}

function delete_stack_and_exit()
{
    echo "Failed to setup stack: $1 reason: $2"
    echo "Deleting failed stack.."
    aws cloudformation delete-stack --stack-name "$1" --region eu-west-1 \
        || failure "Unable to delete-stack for $1: $?"

    echo "Delete unused volumes for $STACK:" >&2
    pwd
    python3 ./DeleteUnusedVolumes.py \
        || failure "Unable to delete unused volumes for $STACK: $?"

    exit 1
}

function generate_uuid()
{
    local N B T
    for (( N=0; N < 16; ++N ))
    do
        B=$(( $RANDOM%255 ))
        if (( N == 6 ))
        then
            printf '4%x' $(( B%15 ))
        elif (( N == 8 ))
        then
            local C='89ab'
            printf '%c%x' ${C:$(( $RANDOM%${#C} )):1} $(( B%15 ))
        else
            printf '%02x' $B
        fi
        for T in 3 5 7 9
        do
            if (( T == N ))
            then
                printf '-'
                break
            fi
        done
    done
    echo
}

[[ -n "$TEST_PASS_UUID" ]] || TEST_PASS_UUID=$(generate_uuid)
export TEST_PASS_UUID

SCRIPT_DIR="${0%/*}"
[[ $SCRIPT_DIR == "$0" ]] && SCRIPT_DIR=.
cd $SCRIPT_DIR || exit 1

echo $STACK >stackName

# Install python requirements
if [[ -z "$SKIP_REQUIREMENTS" ]]
then
    STARTTIME=$(date +%s)
    python3 -m pip install --upgrade -r requirements.txt || failure "Unable to install python requirements: $?"
    ENDTIME=$(date +%s)
    TIME__TOTAL=$(( $ENDTIME - $STARTTIME ))
fi

# Upload basename needs to always be this - so that instances can file tarfile
TAR_BASENAME=ssplav-test-$STACK.tgz

TEST_TAR=${TEST_TAR:-./${TAR_BASENAME}}
export TEST_TAR


STARTTIME=$(date +%s)
## Gather files
if [[ -z "$SKIP_GATHER" ]]
then
    bash -x ./gather.sh || failure "Failed to gather test files: $?"
fi
[[ -f "$TEST_TAR" ]] || failure "Failed to gather test files: $TEST_TAR doesn't exist"

[[ -x $(which aws) ]] || failure "No aws command available"

ENDTIME=$(date +%s)
TIME__TOTAL=$(( $ENDTIME - $STARTTIME ))

export AWS_ACCESS_KEY_ID=AKIAIF23TRE42IG5IH4Q
aws configure set aws_access_key_id $AWS_ACCESS_KEY_ID || failure "Unable to configure access key: $?"
export AWS_SECRET_ACCESS_KEY="09/KeoBM/fhfj9AQOwaRpSXAwOATTcEe3PKL/V7v"
aws configure set aws_secret_access_key "$AWS_SECRET_ACCESS_KEY"
export AWS_REGION=eu-west-1
aws configure set default.region "$AWS_REGION"

## Start deleting old stacks
aws cloudformation delete-stack --stack-name $STACK --region $AWS_REGION || failure "Unable to delete-stack: $?"

## Create template

function compress()
{
    python -c "import json,sys;i=open(sys.argv[1]);o=open(sys.argv[2],'w');json.dump(json.load(i),o,separators=(',',':'))" \
        "$1" \
        "$2" || failure "Unable to compress template $1: $?"
}

machinecount=$(ls instances/ | wc -l)
if [[ -n $INCLUDE_TAG ]]
then
    IFS=' ' read -a tags <<< "$INCLUDE_TAG"
    tagcount=${#tags[*]}
    machinecount=$((machinecount*tagcount))
    echo -ne "Number of machines being started: $machinecount\n"
    printf '%s\n' $INCLUDE_TAG >argFile
fi

TEMPLATE=sspl-system.template
## Copy to template to s3
python ./combineTemplateJson.py
compress $TEMPLATE.with_args template.temp
aws s3 cp template.temp "s3://sspl-testbucket/templates/$STACK.template" \
    || failure "Unable to copy $TEMPLATE to s3"
rm -f template.temp

TAR_DESTINATION_FOLDER="s3://sspl-testbucket/ssplav"
## Upload test tarfile to s3
if [[ -z "$SKIP_TAR_COPY" ]]
then
    STARTTIME=$(date +%s)
    aws s3 cp "$TEST_TAR" ${TAR_DESTINATION_FOLDER}/${TAR_BASENAME} || failure "Unable to copy test tarfile to s3"
    ENDTIME=$(date +%s)
    TIME__TOTAL=$(( $ENDTIME - $STARTTIME ))

fi
# Only delete TEST_TAR if we just gathered it
[[ -z "$SKIP_GATHER" ]] && rm $TEST_TAR

function delete_stack()
{
    stack_status=''
    previous_status=''
    roll_counter=0
    aws cloudformation delete-stack --stack-name $STACK --region eu-west-1 || failure "Unable to delete-stack: $?"

    sleep 1

    while true
    do
        # Query the latest status
        stack_status=$(aws cloudformation describe-stacks --stack-name $STACK --query "Stacks[0].StackStatus" --output text 2>/dev/null)

        if [ "$previous_status" != "$stack_status" ]
        then
            # New status
            roll_counter=0
            previous_status="$stack_status"
        fi

        if [ "$stack_status" = "DELETE_IN_PROGRESS" ]
        then
            if (( roll_counter == 0 ))
            then
                echo -ne "Waiting for stack $STACK to be deleted"
            else
                echo -ne "."
            fi
        elif [ "$stack_status" = "DELETE_COMPLETE" ]
        then
            if (( roll_counter == 0 ))
            then
                echo -ne "Waiting for stack $STACK to be removed from AWS"
            else
                echo -ne "."
            fi
        elif [ "$stack_status" = "DELETE_FAILED" ]
        then
            echo "Deletion failed, forcing aws to delete stack"
            aws cloudformation delete-stack --stack-name $STACK --region eu-west-1 || failure "Unable to delete-stack: $?"
            roll_counter=0
        else
            break
        fi
        (( roll_counter++ ))
        sleep 5
    done
    echo ""
}

stack_name=$(aws cloudformation describe-stacks --stack-name $STACK --query "Stacks[0].StackName" --output=text 2>/dev/null)
if [[ "$stack_name" == "$STACK" ]]
then
    echo "Deleting old stack: $STACK"
    delete_stack
fi

## Trigger Cloud Formation
REGION=${AWS_REGION}
KEY_NAME=regressiontesting
BUILD_NAME=SSPLAV

if [[ -z $RUNSOME  ]]
then
    RUN_SOME=""
else
    RUN_SOME="$RUNSOME"
fi

if [[ -z $RUNONE  ]]
then
    RUN_ONE=""
else
    RUN_ONE=$RUNONE
fi
success="false"
counter=1
NUM_RETRIES=5
retry="false"
roll_counter=0

while (( counter <= $NUM_RETRIES )) && [ "$success" == "false" ]
do
    echo "Creating stack $STACK"
    stack_id=$( aws cloudformation create-stack \
        --template-url "https://sspl-testbucket.s3.amazonaws.com/templates/$STACK.template" \
        --stack-name $STACK \
        --region $REGION \
        --capabilities CAPABILITY_IAM \
        --parameters ParameterKey=KeyName,ParameterValue="$KEY_NAME" \
                     ParameterKey=BuildName,ParameterValue="$BUILD_NAME" \
                     ParameterKey=TestPassUUID,ParameterValue="$TEST_PASS_UUID" \
                     ParameterKey=RunSome,ParameterValue="$RUN_SOME" \
                     ParameterKey=RunOne,ParameterValue="$RUN_ONE" \
                     ParameterKey=StackName,ParameterValue="${STACK}" \
                     ParameterKey=SSHLocation,ParameterValue="${SSHLocation}" \
                     --output=text 2>&1)

    echo "Stack ID == ${stack_id}"

    roll_counter=0
    stack_status=""
    previous_status=""
    keep_checking="true"

    STARTTIME=$(date +%s)

    #Wait for instances to be created
    while [ "$keep_checking" == "true" ]
    do
        # Query the latest status
        # Relying on querying aws without -stack-status-filter is unreliable
        # Create array of states on keep querying until we have a response and then act accordingly
        stack_status=$(aws cloudformation describe-stacks --stack-name $STACK --query "Stacks[0].StackStatus" --output=text 2>/dev/null)

        if [ "$previous_status" != "$stack_status" ]
        then
            # New status
            roll_counter=0
            previous_status="$stack_status"
        fi

        # Depending on the status perform different operations
        if [ "$stack_status" == "CREATE_IN_PROGRESS" ]
        then
            if (( roll_counter == 0 ))
            then
                echo -ne "\nWaiting for instances to be created"
            else
                echo -ne "."
            fi
        elif [ "$stack_status" == "CREATE_COMPLETE" ]
        then
            echo "Stack has been created"
            ENDTIME=$(date +%s)
            TIME__TOTAL=$(( $ENDTIME - $STARTTIME ))
            echo "It took $(( $TIME__TOTAL / 60 ))m$(( $TIME__TOTAL % 60 ))s to create the stack"
            retry="false"
            keep_checking="false"
        elif [ "$stack_status" == "ROLLBACK_IN_PROGRESS" ]
        then
            if (( roll_counter == 0 ))
            then
                echo -ne "\nStack rolling back"
            else
                echo -ne "."
            fi
        elif [ "$stack_status" == "ROLLBACK_COMPLETE" ]
        then
            echo "Rollback completed"
            retry=true
            keep_checking="false"
        elif [ "$stack_status" == "ROLLBACK_FAILED" ]
        then
            echo "Rollback failed"
            retry=true
            keep_checking="false"
        else
            echo "Unhandled stack status: $stack_status"
            retry=true
            keep_checking="false"
        fi

        sleep 5
        (( roll_counter++ ))
    done


    if [ "$retry" == "true" ]
    then
        echo "There was a problem when creating stack $STACK"

        if (( counter < $NUM_RETRIES ))
        then
            delete_stack
            echo "Retrying to create stack $STACK"
        fi

        (( counter++ ))
    else
        success="true"
    fi
done

if [ "$success" == "false" ]
then
    delete_stack_and_exit "$STACK" "Unable to create-stack:"
fi

for instance in `aws ec2 describe-instances --filters "Name=instance.group-name,Values=$STACK*"\
    --query Reservations[*].Instances[*].InstanceId \
    | grep "i-" | cut -d\" -f 2`
do
    aws ec2 modify-instance-attribute \
        --instance-id $instance \
        --instance-initiated-shutdown-behavior terminate
    aws ec2 modify-instance-maintenance-options \
       --instance-id $instance \
       --auto-recovery disabled
done

aws s3 rm "s3://sspl-testbucket/templates/$STACK.template" \
    || delete_stack_and_exit "$STACK" "Unable to delete template file from s3 for"

if aws cloudformation list-stacks --stack-status-filter ROLLBACK_IN_PROGRESS | grep $STACK
then
    delete_stack_and_exit "$STACK" "Stack rolling back"
fi

# Get results back from the AWS test run and save them locally.
rm -rf ./results
mkdir ./results

SRC="s3://sspl-testbucket/test-results/${STACK}/"
DEST="./results"

## Wait for termination
# Once all test runs have finished
cleanupStack() {
    echo "Beginning cleanup check for $STACK at $(date)" >&2
    python waitForCompletionAndSync.py "$STACK" "${SRC}" "${DEST}"
    echo "Beginning cleanup for $STACK at $(date)" >&2

    echo 'Ready to delete stack for $STACK:' >&2
    aws cloudformation delete-stack --stack-name $STACK --region eu-west-1 \
        || failure "Unable to delete-stack for $STACK: $?"

    echo "Delete unused volumes for $STACK:" >&2
    pwd
    python3 ./DeleteUnusedVolumes.py \
        || failure "Unable to delete unused volumes for $STACK: $?"
}
STARTTIME=$(date +%s)
cleanupStack
ENDTIME=$(date +%s)
TIME__TOTAL=$(( $ENDTIME - $STARTTIME ))

STARTTIME=$(date +%s)
aws s3 sync "${SRC}" "${DEST}"
ENDTIME=$(date +%s)
TIME__TOTAL=$(( $ENDTIME - $STARTTIME ))

CORE_DIR=/opt/test/coredumps
if [[ $(id -u) != 0 ]]
then
    CORE_DIR=/tmp/coredumps
fi

rm -rf "${CORE_DIR}"
mkdir -p "${CORE_DIR}"
if [[ -d "${CORE_DIR}" ]]
then
  aws s3 sync "s3://sspl-testbucket/test-results/${STACK}/core-files" "${CORE_DIR}"
  # delete core dir if empty
  rmdir "${CORE_DIR}" || true
fi

python3 delete_old_results.py ${STACK}
aws s3 rm ${TAR_DESTINATION_FOLDER}/${TAR_BASENAME}

MISSING_RESULT=""
if ! [ "$(ls -A results)" ]
then
    echo "No results found!"
    exit 2
else
    resultcount=$(ls ./results/*output.xml | wc -l)
    if [ "$resultcount" != "$machinecount" ]
    then
        for fullfile in instances/*
        do
            file=$(basename -s .json "$fullfile")
            currenttag=1
            while [ $currenttag -le $tagcount ]
            do
                filetag="$file-$currenttag"
                if ! [ "$(find ./results/ -name $filetag'*output.xml')" ]
                then
                    MISSING_RESULT="$MISSING_RESULT\n$filetag"
                fi
                currenttag=$(( currenttag+1 ))
            done
        done
    fi
fi

combineResults()
{
  rm -rf results-combine-workspace
  mkdir results-combine-workspace
  for F in ./results/*-output.xml
  do
      local B=$(basename $F)
      python3 -m robot.rebot --merge -o ./results-combine-workspace/$B -l none -r none -N ${B%%-output.xml}  $F
  done

  python3 -m robot.rebot -l ./results/combined-log.html -r ./results/combined-report.html -N combined --removekeywords wuks ./results-combine-workspace/*
}
combineResults

RESULTS_DIR=${RESULTS_DIR:-/opt/test/results}
mkdir -p ${RESULTS_DIR}
cp ./results/*.html ${RESULTS_DIR}/
cp ./results/*.xml ${RESULTS_DIR}/

LOGS_DIR=${LOGS_DIR:-/opt/test/logs}
mkdir -p ${LOGS_DIR}
cp ./results/*.log ${LOGS_DIR}/

EXIT_FAIL=0
if [ -n "$MISSING_RESULT" ]
then
    echo -ne "Missing output.xml for $MISSING_RESULT\n"
    EXIT_FAIL=1
fi

# window.output["stats"] = [[{"elapsed":"04:55:20","fail":6,"label":"All Tests","pass":1082,"skip":0}]
FAIL_COUNT=$(sed -ne's/window\.output\["stats"\][^f]*"fail":\([0-9][0-9]*\).*/\1/p' ./results/combined-log.html)
echo "Failures: $FAIL_COUNT"
if (( FAIL_COUNT > 0 ))
then
    python3 extract_failed_tests.py ./results/*-output.xml
    EXIT_FAIL=1
fi

if (( EXIT_FAIL == 1 ))
then
    exit 1
fi

exit 0
