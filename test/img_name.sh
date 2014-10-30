#!/bin/bash


if [ `git status -s | wc -l` -eq 0 ] 
then 
    NAME=`git rev-parse --short HEAD`
else
    NAME="DIRTY" 
fi
echo $NAME
