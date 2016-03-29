#!/bin/bash
source ../test_base
make SERVICE=Test FILES=service.cpp
start test_transmit.img "Network Transmission Tests"
