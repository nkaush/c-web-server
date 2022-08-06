#!/usr/bin/env bash

set -x

DUMPFILE=dump
OUTFILE=data
CSVFILE=data.csv

make server-debug

sudo dtrace -s trace/fcalls.d -c './server-debug 80' > $DUMPFILE

cat $DUMPFILE | tr -s ' ' > $OUTFILE

rm $DUMPFILE

python3 trace/profile_create.py > $CSVFILE

rm $OUTFILE

python3 trace/profile_analyze.py