#!/bin/bash

# Runs the IncludeOS demo service, and tests it by doing a curl.

NAME=demo_service
NET_IP=10.0.0.42
NET_DEVICE=tap100
DISK_DEVICE=dummy.img

INCLUDEOS_SRC=${INCLUDEOS_SRC-$HOME/IncludeOS}
UNIKERNEL_SRC=${INCLUDEOS_SRC}/examples/demo_service
UNIKERNEL_BUILD=${UNIKERNEL_SRC}/build_solo5-hvt
UNIKERNEL_IMG=${UNIKERNEL_BUILD}/demo
ARCH=${ARCH:-x86_64}

mkdir -p build
pushd build
conan install .. -pr clang-6.0-linux-x86_64
popd
source build/activate.sh
die_error ()
{
  source build/deactivate.sh
}
trap deactivate 0 INT TERM

SOLO5_TENDER=solo5-hvt

die_error ()
{
    echo $0: "$@" 1>&2
    exit 1
}

die_info ()
{
    echo $0: "$@" 1>&2
    exit 0
}

SYSTEM=`uname -a`
[[ ! $SYSTEM =~ .*[L|l]inux.* ]] && die_info "Solo5 is currently only supported on Linux."

trap nuketmpdir 0 INT TERM
TMPDIR=$(mktemp -d)
[ $? -ne 0 ] && die_error "Error creating temporary directory."

nuketmpdir ()
{
    [ -n "${PRESERVE_TMPDIR}" ] && return
    [ -z "${TMPDIR}" ] && return
    [ ! -d "${TMPDIR}" ] && return
    rm -rf ${TMPDIR}
}

logto ()
{
    LOG=${TMPDIR}/$1
    exec >>${LOG} 2>&1 </dev/null
}

setup()
{(
    logto ${NAME}.log.0

    mkdir -p ${UNIKERNEL_BUILD}
    pushd ${UNIKERNEL_BUILD}
        PLATFORM=x86_solo5 cmake .. || die_error "cmake failed"
        make || die_error "make failed"
    popd
    [ -f ${UNIKERNEL_IMG} ] || die_error "The unikernel was not built"
    [ -s ${UNIKERNEL_IMG} ] || die_error "The unikernel image is zero size"

    # The default solo5-hvt needs a disk, even if it's a dummy 0 byte one.
    # If you want solo5-hvt with just the net module, you need to re-build it.
    touch ${TMPDIR}/${DISK_DEVICE}
    #${INCLUDEOS_SRC}/etc/scripts/create_bridge.sh || true
    # Create a tap100 device
    #${INCLUDEOS_SRC}/etc/scripts/solo5-ifup.sh || true

    return 0
)}

retry_command ()
{
    local N
    local STATUS

    COMMAND=$1
    MAX_RETRIES=$2

    N=0
    STATUS=1
    until [ $N -ge $MAX_RETRIES ]
    do
        ${COMMAND} && STATUS=0 && break
        N=$[$N+1]
        sleep 1
    done
    return $STATUS
}

run_curl_test ()
{(
    local TENDER
    local UNIKERNEL
    local PID_TENDER

    logto ${NAME}.log.1
    TENDER="${SOLO5_TENDER} --disk=${TMPDIR}/${DISK_DEVICE} --net=${NET_DEVICE}"

    # If we can't run solo5-hvt, just return code 98 (skipped)
    [ -c /dev/kvm -a -w /dev/kvm ] || return 98

    ${TENDER} -- ${UNIKERNEL_IMG} &
    PID_TENDER=$!

    retry_command "curl -m 1 ${NET_IP}" 30
    STATUS=$?

    kill -9 ${PID_TENDER} || true
    return ${STATUS}
)}

dumplogs ()
{
    LOGS=$(find ${TMPDIR} -type f -name $1.log.\*)
    for F in ${LOGS}; do
        echo "$2${F}: $3"
        cat ${F} | sed "s/^/$2>$3 /"
    done
}


ARGS=$(getopt v $*)
[ $? -ne 0 ] && exit 1
set -- $ARGS
VERBOSE=
while true; do
    case "$1" in
    -v)
        VERBOSE=1
        shift
        ;;
    --)
        shift; break
        ;;
    esac
done

if [ -t 1 ]; then
    TRED=$(tput setaf 1)
    TGREEN=$(tput setaf 2)
    TYELL=$(tput setaf 3)
    TOFF=$(tput sgr0)
else
    TRED=
    TGREEN=
    TYELL=
    TOFF=
fi

STATUS=
setup && run_curl_test
case $? in
0)
    STATUS=0
    echo "${TGREEN}PASSED${TOFF}"
    [ -n "${VERBOSE}" ] && dumplogs ${NAME} ${TGREEN} ${TOFF}
    ;;
98)
    STATUS=0
    # can't run solo5 tender (no KVM support)
    echo "${TYELL}SKIPPED${TOFF}"
    [ -n "${VERBOSE}" ] && dumplogs ${NAME} ${TGREEN} ${TOFF}
    ;;
*)
    STATUS=1
    echo "${TRED}ERROR${TOFF}"
    ;;
esac

[ ${STATUS} -ne 0 ] && dumplogs ${NAME} ${TRED} ${TOFF}

exit ${STATUS}
