#!/bin/bash

set -e
set -u

find '(' -name '*.o' -or -name 'uneven' ')' -print0 | xargs -0 rm -f

gcc -O2 -Wall -o uneven uneven.c -lpulse
