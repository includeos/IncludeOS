#!/bin/bash
source ../test_base

make SERVICE=Test_STL FILES=service.cpp
start Test_STL.img "Basic STL test"
