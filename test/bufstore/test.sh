#!/bin/bash
source ../test_base

make
start test_bufstore.img "Test bufstore"
