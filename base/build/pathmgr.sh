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

lpath=/usr/local
if [ -d "$lpath" ]; then
    addpath $lpath/bin
    if [ "$PLATFORM" = "sunos" ]; then
      export LD_LIBRARY_PATH=$lpath/lib:$LD_LIBRARY_PATH
    else
      export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$lpath/lib
    fi
    export LINK_FLAGS="$LINK_FLAGS -L$lpath/lib"
fi

if [[ -z $NO_TOOLS ]]
then
    lpath=/opt/tools
    if [ -d "$lpath" ]; then
        addpath $lpath/bin
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$lpath/lib
        export LINK_FLAGS="$LINK_FLAGS -L$lpath/lib"
    fi
fi

lpath=/opt/sophos
if [ -d "$lpath" ]; then
    addpath $lpath/bin
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$lpath/lib
    export LINK_FLAGS="-L$lpath/lib $LINK_FLAGS"
fi

lpath=/opt/gcc-3.4.6
if [ -d "$lpath" ]; then
    addpath $lpath/bin
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$lpath/lib
    export LINK_FLAGS="-L$lpath/lib $LINK_FLAGS"
fi

## AIX xlC compiler - new version
lpath=/opt/IBM/xlC/13.1.3
if [ -d "$lpath" ]; then
    addpath $lpath/bin
    export LD_LIBRARY_PATH=$lpath/lib:$LD_LIBRARY_PATH
    export LINK_FLAGS="-L$lpath/lib $LINK_FLAGS"
fi

## AIX xlC compiler
lpath=/usr/vacpp
if [ -d "$lpath" ]; then
    addpath $lpath/bin
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$lpath/lib
    export LINK_FLAGS="-L$lpath/lib $LINK_FLAGS"
fi

if [[ -z $NO_TOOLCHAIN ]]
then
    lpath=/opt/toolchain
    if [ -d "$lpath" ]; then
        addpath $lpath/bin
        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$lpath/lib:$lpath/lib64
    fi
fi

lpath=/opt/csw
if [ -d "$lpath" ]; then
    addpath $lpath/bin
    addpath $lpath/gnu
    export LD_LIBRARY_PATH=$lpath/lib:$LD_LIBRARY_PATH
fi

# On AIX, msgfmt requires that /opt/pware/lib is in the search path before /usr/lib.
# However, it must not be added globally else other application (e.g. bash) will fail.
lpath=/opt/pware/lib
if [ -d "$lpath" ]; then
	export PWARE_LIB_PATH=$lpath
fi
