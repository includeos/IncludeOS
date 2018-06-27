#!/bin/bash

sudo arp -d 10.0.0.10 2>&1 > /dev/null
sudo arp -d 10.0.0.11 2>&1 > /dev/null
sudo arp -d 10.0.0.12 2>&1 > /dev/null
