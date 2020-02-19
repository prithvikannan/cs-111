#!/bin/bash

#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

rm -f profile.out
LD_PRELOAD=/usr/lib64/libprofiler.so \
CPUPROFILE=./temp.gperf \
./lab2_list --iterations=1000 --threads=12 --sync=s
pprof --text ./lab2_list ./temp.gperf > profile.out
pprof --list=newThreadFunction ./lab2_list ./temp.gperf >> profile.out
rm -f ./temp.gperf