#!/bin/bash
cat bla.txt | ncat 10.0.0.42 1338 --send-only
