#!/bin/bash

set -x

JOB_ID=$1
shift

IDENTITFIER=`hostname`-`date +%F`-`date +%H``date +%M`
[[ -n $STACK ]] || STACK=sspl-system-tests-${IDENTITFIER}-$(echo "$@"-${RANDOM} | md5sum | cut -f 1 -d " " )

## Start deleting old stacks
aws cloudformation delete-stack --stack-name $STACK --region eu-west-1 || failure "Unable to delete-stack: $?"

function failure()
{
    echo "$@"
    exit 1
}

function compress()
{
    python -c "import json,sys;i=open(sys.argv[1]);o=open(sys.argv[2],'w');json.dump(json.load(i),o,separators=(',',':'))" \
        "$1" \
        "$2" || failure "Unable to compress template $1: $?"
}

TEMPLATE=sspl-system.template
## Copy to template to s3
ARGS="$@"
sed "s:@ARGSGOHERE@:${ARGS}:g" $TEMPLATE > $TEMPLATE.with_args
compress $TEMPLATE.with_args template.temp
aws s3 cp template.temp "s3://sspl-testbucket/templates/$STACK.template" \
    || failure "Unable to copy $TEMPLATE to s3"
rm -f template.temp

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
REGION=eu-west-1
KEY_NAME=regressiontesting
BUILD_NAME=SSPL
## check if RunCloud is set
if [[ $RUNCLOUD == "true" ]]
then
    RUN_CLOUD=true
else
    RUN_CLOUD=false
fi

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
                     ParameterKey=RunCloud,ParameterValue="$RUN_CLOUD" \
                     ParameterKey=RunSome,ParameterValue="$RUN_SOME" \
                     ParameterKey=RunOne,ParameterValue="$RUN_ONE" \
                     ParameterKey=StackName,ParameterValue="${STACK}" \
                     ParameterKey=TarName,ParameterValue="${JOB_ID}" \
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
    failure "Unable to create-stack: $STACK"
fi

for instance in `aws ec2 describe-instances --filters "Name=instance.group-name,Values=$STACK*"\
    --query Reservations[*].Instances[*].InstanceId \
    | grep "i-" | cut -d\" -f 2`
do
    aws ec2 modify-instance-attribute \
        --instance-id $instance \
        --instance-initiated-shutdown-behavior terminate
done

aws s3 rm "s3://sspl-testbucket/templates/$STACK.template" \
    || failure "Unable to delete template file from s3 for $STACK"

if aws cloudformation list-stacks --stack-status-filter ROLLBACK_IN_PROGRESS | grep $STACK
then
    failure "Stack rolling back for $STACK"
fi

## Wait for termination

# Once all test runs have finished
cleanupStack() {
    echo "Beginning cleanup check for $STACK at $(date)" >&2
    python waitForTestRunCompletion.py "$STACK"
    echo "Beginning cleanup for $STACK at $(date)" >&2

    echo 'Ready to delete stack for $STACK:' >&2
    aws cloudformation delete-stack --stack-name $STACK --region eu-west-1 \
        || failure "Unable to delete-stack for $STACK: $?"

    echo "Delete unused volumes for $STACK:" >&2
    python $SCRIPT_DIR/DeleteUnusedVolumes.py \
        || failure "Unable to delete unused volumes for $STACK: $?"
}

cleanupStack

# TODO - think about what to clear up
# Get results back from the AWS test run and save them locally.
rm -rf ./results
mkdir ./results
aws s3 cp --recursive "s3://sspl-testbucket/test-results/${STACK}/" ./results
python3 delete_old_results.py ${STACK}
#aws s3 rm ${TAR_DESTINATION_FOLDER}/${TAR_BASENAME}
## exit
exit 0
