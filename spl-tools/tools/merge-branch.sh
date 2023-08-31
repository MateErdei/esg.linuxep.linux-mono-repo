#!/bin/bash
set -e
#set -x

MERGE_DIR="/home/$USER/repo_merge-branches"

BASE=0
EDR=0
AV=0
EJ=0
SPL_TOOLS=0
BRANCH_TO_MOVE=""

function merge_branch()
{
    mono_repo_remote="$1"
    remote="$2"
    subdir="$3"
    branch_to_move="$4"

    repo_dir=$(python -c "print('$remote'.split('/')[1].split('.git')[0])")
    mono_repo_dir=$(python -c "print('$mono_repo_remote'.split('/')[1].split('.git')[0])")
    echo "Remote = $remote"
    echo "Repo dir = $repo_dir"

    cd "$MERGE_DIR"
    rm -rf "$repo_dir"
    git clone "$remote" # e.g. Base repo
    cd "$repo_dir"

    git filter-repo --to-subdirectory-filter "$subdir"
    git checkout "$branch_to_move"

    cd "$MERGE_DIR"

    if [ ! -d "$mono_repo_dir" ]; then
        cd "$MERGE_DIR"
        git clone "$mono_repo_remote"
    fi

    cd "$mono_repo_dir"
    git fetch
    git checkout -b "$branch_to_move"

    # Here we just use subdir as a name for the remote
    git remote add "$subdir" "${MERGE_DIR}/${repo_dir}"
    git fetch "$subdir" --tags
    git merge --no-edit --allow-unrelated-histories "$subdir/$branch_to_move"  # Merge in branch_to_move
    git remote remove "$subdir"
    git push --set-upstream origin "$BRANCH_TO_MOVE"
    echo "Your branch has been moved and pushed to the mono repo with the same name: $BRANCH_TO_MOVE"
}

function print_help()
{
    echo "This script merges in branches from Base, AV, EDR, event journaler and spl-tools into a branch of the same name in the linux mono repo."
    echo "> --branch = The name of the branch you want to move."
    echo "> --base = If you're moving a Base branch."
    echo "> --av = If you're moving an AV branch."
    echo "> --edr = If you're moving an EDR branch."
    echo "> --ej = If you're moving an Event Journaler branch."
    echo "> --spl-tools = If you're moving a SPL Tools branch."
}

# Deal with arguments
while [[ $# -ge 1 ]]; do
    case $1 in
     --help)
        print_help
        exit 0
        ;;
    --branch)
        shift
        BRANCH_TO_MOVE="$1"
        ;;
    --base)
        BASE=1
        ;;
    --edr)
        EDR=1
        ;;
    --av)
        AV=1
        ;;
    --ej)
        EJ=1
        ;;
    --spl-tools)
        SPL_TOOLS=1
        ;;
    *)
        echo "unknown argument $1"
        exit 1
        ;;
    esac
    shift
done

if [[ $BRANCH_TO_MOVE == "" ]] ; then
    echo "Specify the branch name you want to move"
    print_help
    exit 1
fi

# Enable rerere so git remembers previous rebase conflict resolutions
# https://git-scm.com/book/en/v2/Git-Tools-Rerere
git config --global rerere.enabled true

# Note - To make this smoother you can cache your ssh key passphrase using ssh-agent and adding your private key
# Start ssh-agent:
if [[ -n $SSH_AGENT_PID ]]
then
   echo "ssh-agent is already running"
   # Do something knowing the pid exists, i.e. the process with $PID is running
else
    eval `ssh-agent -s`

    if [ -f ./path_to_git_ssh_key ]
    then
        sshkey=$(cat ./path_to_git_ssh_key)
    else
        read -p 'git ssh key path (e.g. "/home/user/.ssh/id_my_key"): ' sshkey
        echo -n "$sshkey" > ./path_to_git_ssh_key
    fi

    # Add private key (e.g. ssh-add /home/alex/.ssh/id_alexcooper)
    [ -f "$sshkey" ] || exit 1
    ssh-add "$sshkey"
fi
echo "SSH_AGENT_PID = $SSH_AGENT_PID"


GIT_REMOTE_MONOREPO="git@github.com:sophos-internal/esg.linuxep.linux-mono-repo.git"

rm -rf "$MERGE_DIR"
mkdir -p "$MERGE_DIR"

if [[ $BASE == 1 ]]; then
    echo "Moving Base branch: $BRANCH_TO_MOVE"
    merge_branch "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.everest-base.git" base "$BRANCH_TO_MOVE"
elif [[ $AV == 1 ]]; then
    echo "Moving AV branch: $BRANCH_TO_MOVE"
    merge_branch "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-anti-virus.git" av "$BRANCH_TO_MOVE"
elif [[ $EDR == 1 ]]; then
    echo "Moving EDR branch: $BRANCH_TO_MOVE"
    merge_branch "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-edr-component.git" edr "$BRANCH_TO_MOVE"
elif [[ $EJ == 1 ]]; then
    echo "Moving Event Journaler branch: $BRANCH_TO_MOVE"
    merge_branch "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-edr-component.git" eventjournaler "$BRANCH_TO_MOVE"
elif [[ $SPL_TOOLS == 1 ]]; then
    echo "Moving SPL Tools branch: $BRANCH_TO_MOVE"
    merge_branch "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-tools.git" spl-tools "$BRANCH_TO_MOVE"
else
    echo "Set which repo you're moving the branch from."
    print_help
    exit 1
fi
echo "When you're done you can remove $MERGE_DIR"