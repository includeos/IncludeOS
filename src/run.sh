#! /bin/bash
set -e
export JOBS=12
export SERVICE=test_service

. debug/run_test.sh $1 $2
