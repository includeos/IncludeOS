#!/bin/sh
# Copyright (c) 2015-2017 Contributors as noted in the AUTHORS file
#
# This file is part of Solo5, a unikernel base layer.
#
# Permission to use, copy, modify, and/or distribute this software
# for any purpose with or without fee is hereby granted, provided
# that the above copyright notice and this permission notice appear
# in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#
# Convention is: tap interface named 'tap100'
#

# The name of the bridge VM's are added to
BRIDGE=bridge43

if [ $(id -u) -ne 0 ]; then
    echo "$0: must be root" 1>&2
    exit 1
fi

case `uname -s` in
Linux)
    if ip tuntap | grep -q tap100; then
        echo "tap100 already exists"
    else
        set -xe
        echo "Creating tap100 for solo5 tender"
        ip tuntap add tap100 mode tap
        ip link set dev tap100 up
        brctl addif $BRIDGE tap100
    fi
    ;;
FreeBSD)
    kldload vmm
    kldload if_tap
    kldload nmdm
    sysctl -w net.link.tap.up_on_open=1
    ifconfig $BRIDGE addm tap100
    ;;
*)
    exit 1
    ;;
esac
