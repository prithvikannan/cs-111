#!/bin/bash

#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

rm -f *.csv

for numThreads in 1, 2, 4, 8, 12, 16, 24
do
	./lab2_list --iterations=1000 --threads=$numThreads --sync=m >> lab2b_list.csv
done

for numThreads in 1, 2, 4, 8, 12, 16, 24
do
        ./lab2_list --iterations=1000 --threads=$numThreads --sync=s >> lab2b_list.csv
done

for numThreads in 1, 4, 8, 12, 16
do
	for numIter in 1, 2, 4, 8, 16
	do
		./lab2_list --iterations=$numIter --threads=$numThreads --yield=id --lists=4 >> lab2b_list.csv
	done
done

for numThreads in 1, 4, 8, 12, 16
do
        for numIter in 10, 20, 40, 80
        do
                ./lab2_list --iterations=$numIter --threads=$numThreads --yield=id --sync=m --lists=4 >> lab2b_list.csv
        done
done

for numThreads in 1, 4, 8, 12, 16
do
        for numIter in 10, 20, 40, 80
        do
                ./lab2_list --iterations=$numIter --threads=$numThreads --yield=id --sync=s --lists=4 >> lab2b_list.csv
        done
done

for numThreads in 1, 2, 4, 8, 12
do
        for numIter in 4, 8, 16
        do
                ./lab2_list --iterations=1000 --threads=$numThreads --sync=m --lists=$numIter >> lab2b_list.csv
        done
done

for numThreads in 1, 2, 4, 8, 12
do
        for numIter in 4, 8, 16
        do
                ./lab2_list --iterations=1000 --threads=$numThreads --sync=s --lists=$numIter >> lab2b_list.csv
        done
done