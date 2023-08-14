#!/bin/bash
set -e
set -x

MERGE_DIR="/home/$USER/repo_merge"

function merge_in_repo() {
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
    git filter-repo --to-subdirectory-filter "$subdir"
    cd "$MERGE_DIR"

    if [ ! -d "$mono_repo_dir" ]; then
        cd "$MERGE_DIR"
        git clone "$mono_repo_remote"
    fi

    cd "$mono_repo_dir"
    # Make sure we have a merge branch
    git fetch
    git branch -a | grep merge-branch || git checkout -b merge-branch
    git checkout merge-branch
    git pull

    # Here we just use subdir as a name for the remote
    git remote add "$subdir" "${MERGE_DIR}/${repo_dir}"
    git fetch "$subdir" --tags
    git merge --no-edit --allow-unrelated-histories "$subdir"/develop # Merge in develop
    git remote remove "$subdir"
}

# When set to 1 we will remove the working merge dir, default this to off in case we have changes to keep.
FORCE_REMOVE=0

# When set to 1 the merge dir will not be removed, the script will try and merge in changes from the other repos
UPDATE=0

# Deal with arguments
while [[ $# -ge 1 ]]; do
    case $1 in
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

# Note - To make this smoother you can cache your ssh key passphrase using ssh-agent and adding your private key
# Start ssh-agent:
# eval `ssh-agent -s`
# Add private key:
# ssh-add ~/.ssh/id_alexcooper

mkdir -p "$MERGE_DIR"

# Target repo_dir
GIT_REMOTE_MONOREPO="git@github.com:sophos-internal/esg.linuxep.linux-mono-repo.git"

merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.everest-base.git" base
merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-edr-component.git" edr
merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-anti-virus.git" av
merge_in_repo "$GIT_REMOTE_MONOREPO" "git@github.com:sophos-internal/esg.linuxep.sspl-plugin-event-journaler.git" eventjournaler

echo "Merge dir: $MERGE_DIR"
