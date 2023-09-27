#!/usr/bin/env bash
set -ex

BASE=$1
MODE=$2
Coverage=$3

if [[ ! -d ${BASE}/source/_input ]]; then
    >&2 echo "Run $0 with liveterminal diretory path. Eg. $0 <pathToliveterminalrepository>." 
    exit 1
fi
export BASE

#set the version for the rust agent
AutoVersionIni=${BASE}/source/linux/products/distribution/include/AutoVersioningHeaders/AutoVersion.ini
if [[ -f ${AutoVersionIni} ]]
then
  echo "Setting LiveTerminal agent version..."
  python3 ${BASE}/source/build/SetAgentVersion.py  -v ${AutoVersionIni}  -m ${BASE}/source/rust/sophos-live-terminal/Cargo.toml
else
  echo "Not setting LiveTerminal agent version, assuming a local build"
fi

if [[ ${Coverage} = "on" ]]; then
    export RUSTC_BOOTSTRAP=1
    export GRCOV=${BASE}/source/_input/installer/rust/cargo/bin/grcov
    chmod +x ${GRCOV}
fi

export CARGO_HOME=${BASE}/source/_input/installer/rust/cargo
export RUSTUP_HOME=${BASE}/source/_input/installer/rust/rustup
chmod +x ${CARGO_HOME}/bin/*
chmod +x ${RUSTUP_HOME}/toolchains/*-x86_64-unknown-linux-gnu/bin/*
export CARGO=${CARGO_HOME}/bin/cargo
export PATH=${PATH}:${CARGO_HOME}/bin


pushd ${BASE}/source/_input/rust
if [[ ! -d sophos-winpty-sys ]]; then
     #setup an empty project to for the non-applicable sophos-winpty-sys
     mkdir sophos-winpty-sys
     cd sophos-winpty-sys
     mkdir src 
     cat > Cargo.toml << EOF
[package]
name = "sophos-winpty-sys"
version = "0.4.4"

[dependencies]
cc = "1.0"
EOF
    touch src/lib.rs
fi
popd


pushd /build/redist/openssl/
# rust build expects the openssl to have a lib folder. But the exported one has lib64

    if [[ ! -s lib ]]; then
        ln -s lib64 lib
    fi
popd

pushd ${BASE}/source/rust

export OPENSSL_DIR=/build/redist/openssl/

BUILDOPTION="--release"
if [[ ${MODE} == 'debug' ]];then 
    BUILDOPTION=
fi

#-Clink-dead-code
if [[ ${Coverage} = "on" ]]; then
    echo "Coverage enabled for rust"
    export CARGO_INCREMENTAL=0
    export RUSTFLAGS="-Zprofile -Ccodegen-units=1 -Copt-level=0  -Coverflow-checks=off -Zpanic_abort_tests -Cpanic=abort"
    export RUSTDOCFLAGS="-Cpanic=abort"
fi


PATH=/build/input/gcc/bin/:${PATH} cargo build --offline ${BUILDOPTION}
LD_LIBRARY_PATH=${OPENSSL_DIR}/lib64 PATH=/build/input/gcc/bin/:${PATH} cargo test ${BUILDOPTION} --offline --tests --no-fail-fast -- --nocapture --test-threads=1

if [[ ${Coverage} = "on" ]]; then
    mkdir -p ${BASE}/source/_rustcov/cov_empty
    mkdir -p ${BASE}/source/_rustcov/cov_unittests
    mkdir -p ${BASE}/source/_rustcov/coverage_report

    find  ../_target | grep gcda | xargs cp -t ${BASE}/source/_rustcov/cov_unittests
    find  ../_target | grep gcno | xargs cp -t ${BASE}/source/_rustcov/cov_empty
    # results are uploaded to artifactory so GRCOV can be run on a machine with these outputs. Example of command to run:
    #grcov <path to unittest+empty outputs> -s ${BASE}/source/rust -t html --ignore-not-existing -o <path to output>
fi

popd


