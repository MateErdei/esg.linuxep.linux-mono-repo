#!/bin/bash
set -e
#set -x

MERGE_DIR="/home/$USER/repo_merge"
BRANCH_CLEAN=merge-branch-clean
BRANCH_MERGE=merge-branch-rebased

function merge_in_repo()
{
    mono_repo_remote=$1
    remote=$2
    subdir=$3

    repo_dir=$(python -c "print('$remote'.split('/')[1].split('.git')[0])")
    mono_repo_dir=$(python -c "print('$mono_repo_remote'.split('/')[1].split('.git')[0])")
    echo "Remote = $remote"
    echo "Repo dir = $repo_dir"

    cd "$MERGE_DIR"
    rm -rf "$repo_dir"
    git clone "$remote"
    cd "$repo_dir"
    # TODO we should check we're on develop.
    git filter-repo --to-subdirectory-filter "$subdir"
    cd "$MERGE_DIR"

    if [ ! -d "$mono_repo_dir" ]; then
        cd "$MERGE_DIR"
        git clone "$mono_repo_remote"
    fi

    cd "$mono_repo_dir"
    # Make sure we have a merge branch
    git fetch
    git branch -a | grep $BRANCH_CLEAN || git checkout -b $BRANCH_CLEAN
    git checkout $BRANCH_CLEAN

    # Here we just use subdir as a name for the remote
    git remote add "$subdir" "${MERGE_DIR}/${repo_dir}"
    git fetch "$subdir" --tags
    git merge --no-edit --allow-unrelated-histories "$subdir"/develop # Merge in develop
    git remote remove "$subdir"
}

# rebase does not work well in this scenario, it duplicates commits.
# Leaving it here in case someone else wants to use it.
#function rebase()
#{
#    mono_repo_remote=$1
#    mono_repo_dir=$(python -c "print('$mono_repo_remote'.split('/')[1].split('.git')[0])")
#    cd "$MERGE_DIR/$mono_repo_dir"
#    branch_to_rebase=$2
#    branch_to_rebase_onto=$3
#    echo "Rebase $branch_to_rebase onto $branch_to_rebase_onto"
#    echo git checkout "$branch_to_rebase"
#    echo "pulling $branch_to_rebase"
#    echo git pull
#    echo git rebase "$branch_to_rebase_onto"
#}

# When set to 1 we will remove the working merge dir, default this to off in case we have changes to keep.
FORCE_REMOVE=0

# When set to 1 the merge dir will not be removed, the script will try and merge in changes from the other repos
UPDATE=0

function print_help()
{
    echo "This script merges Base, AV, EDR, event journaler and spl-tools into a clean branch in the linux mono repo and can update your merge branch."
    echo "> no args = running this for first time will create a dir in /home/$USER, check out all repos and merge them."
    echo "> no args = running this again will warn that you need to manually remove the merge dir if you haven't already."
    echo "> --force-remove = This will delete all the component repos and the mono repo merge dir."
    echo "> --update = This is probably what you want, assuming this script has run successfully once it will update the mono repo clean branch and merge that back into the the merge branch."
}

# Deal with arguments
while [[ $# -ge 1 ]]; do
    case $1 in
     --help)
        print_help
        exit 0
        ;;
    --force-remove)
        FORCE_REMOVE=1
        ;;
    --update)
        UPDATE=1
        ;;
    *)
        echo "unknown argument $1"
        exit 1
        ;;
    esac
    shift
done

if [[ $FORCE_REMOVE == 1 ]]; then
    echo "Removing $MERGE_DIR"
    sleep 3 # Give someone a chance to cancel.
    rm -rf "$MERGE_DIR"
fi

if [[ $UPDATE == 1 ]]; then
    echo "Updating $MERGE_DIR"
else
    if [ -d "$MERGE_DIR" ]; then
        echo "$MERGE_DIR already exists, please delete it. Script will not automatically remove it in case of changes."
        exit 1
    fi
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

mkdir -p "$MERGE_DIR"

# Target repo_dir
GIT_REMOTE_MONOREPO="git@github.com:sophos-internal/esg.linuxep.linux-mono-repo.git"

merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.everest-base.git" base
merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-edr-component.git" edr
merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-anti-virus.git" av
merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-event-journaler.git" eventjournaler
merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-tools.git" spl-tools

#rebase "$GIT_REMOTE_MONOREPO" "$BRANCH_MERGE" "$BRANCH_CLEAN"

# Now merge the clean mono repo branch into the merge branch to update it.
git checkout $BRANCH_MERGE

echo "Merge dir: $MERGE_DIR"
ls "$MERGE_DIR"
echo "Mono repo merge dir: $PWD"

echo "Will now try merging $BRANCH_CLEAN into $BRANCH_MERGE"
sleep 4
git merge $BRANCH_CLEAN

