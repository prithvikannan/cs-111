#! /usr/bin/gnuplot
#
#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#   lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
#   lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#   lab2b_3.png ... successful iterations vs. threads for each synchronization method.
#   lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists.
#   lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.
#

# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "Throughput vs. number of threads for mutex and spin-lock synchronized list operations"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ops/s)"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title 'With Mutex' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
    title 'With Spin Lock' with linespoints lc rgb 'green'

set title "Mean time per mutex wait and mean time per operation for mutex-synchronized list operations"
set xlabel "Thread #"
set logscale x 2
set xrange [1:32]
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
    title 'Total Completion Time' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
    title 'Lock Wait Time' with linespoints lc rgb 'red'
     
set title "Successful iterations vs. threads for each synchronization method"
set logscale x 2
set xrange [0.5:]
set xlabel "Threads"
set ylabel "successful iterations"
set logscale y 10
set key right top
set output 'lab2b_3.png'

plot \
    "< grep -e 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    with points lc rgb 'red' title 'unprotected', \
    "< grep -e 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    with points lc rgb 'blue' title 'mutex protected', \
    "< grep -e 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
    with points lc rgb 'green' title 'spin-lock protected', \

#
# "no valid points" is possible if even a single iteration can't run
#

set title "Throughput vs. number of threads for mutex synchronized partitioned lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/s)"
set logscale y 10
set output 'lab2b_4.png'
set key right top

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'red' title '1 list', \
     "< grep -e 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'green' title '4 lists', \
     "< grep -e 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'blue' title '8 lists', \
     "< grep -e 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'orange' title '16 lists'

set title "Throughput vs. number of threads for spin-lock-synchronized partitioned lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/s)"
set logscale y 10
set output 'lab2b_5.png'
set key right top

plot \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'red' title '1 list', \
     "< grep -e 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'green' title '4 lists', \
     "< grep -e 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'blue' title '8 lists', \
     "< grep -e 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with linespoints lc rgb 'orange' title '16 lists'