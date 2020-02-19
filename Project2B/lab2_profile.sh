#!/bin/bash

#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

rm -f profile.gperf profile.out
LD_PRELOAD=/usr/lib64/libprofiler.so \
CPUPROFILE=./raw.gperf \
./lab2_list --iterations=1000 --threads=12 --sync=s
pprof --text ./lab2_list ./raw.gperf > profile.out
pprof --list=newThreadFunction ./lab2_list ./raw.gperf >> profile.out
rm -f ./raw.gperf