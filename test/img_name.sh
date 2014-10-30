#!/bin/bash


[ `git status -s | wc -l` -eq 0 ] && NAME=git rev-parse --short HEAD || NAME="DIRTY" 
echo $NAME
