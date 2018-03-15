#! /bin/bash
set -e

script_dir="$( cd "$( dirname "${bash_source[0]}" )" && pwd )"
docker run --privileged -v $script_dir:/service -it includeos "$@"
