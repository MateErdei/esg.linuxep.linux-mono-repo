##
## ALWAYS EDIT //version/product/savlinux/3rdparty/support/pathmgr.sh
##


## Setup PATHs to contain extra useful directories
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib:/lib

function lspath {
    echo $PATH | tr ':' '\n'
}

function makepath {
    unset NEW_PATH
    while read PATHEL; do
        NEW_PATH="$NEW_PATH:$PATHEL"
    done
    echo ${NEW_PATH#:}
}

function uniqpath {
    PATH=`lspath | awk '{seen[$0]++; if (seen[$0]==1){print}}' | makepath`
}

function cleanpath {
    uniqpath
    PATH=`lspath | sed -e 's|\/$||' -ne '/./p' | makepath`
}

function addpath {
    PATH=$1:$PATH
    cleanpath
}
