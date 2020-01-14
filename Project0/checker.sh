#! /bin/sh

#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

### ERROR 0 ###

#use the --input flag
`echo "abc" > in.txt`
`./lab0 --input in.txt > out.txt`
if [ $? -eq 0 ]
then 
    echo "Success: able to use --input"; 
else
    echo "Error: unable to use --input"
fi

#use the --output flag
`echo "abc" | ./lab0 > out.txt`
if [ $? -eq 0 ]
then 
    echo "Success: able to use --output"; 
else
    echo "Error: unable to use --output"
fi

#check to make sure that --input flag actually replaces stdin
`echo "abc" > in.txt`
`echo "123" | ./lab0 --input in.txt > out.txt`

`echo "abc" > in.txt`
`diff in.txt out.txt`
if [ $? -eq 0 ]
then
    echo "Success: --input replaces stdin"; 
else
    echo "Error: --input does not replace stdin"; 
fi

#check for correct copying
`diff in.txt out.txt`
if [ $? -eq 0 ]
then
    echo "Success: correct error code (0) for copied correctly"; 
else 
	echo "Error: did not copy correctly"
fi

### ERROR 1 ###

#test incorrect usage
`./lab0 --badflag 2> trash.txt`
if [ $? -eq 1 ]
then 
	echo "Success: correct error code (1) for bad flag"
else 
	echo "Error: did not catch bad flag"
fi

### ERROR 2 ###

#test bad input file
`./lab0 --input="sdfasdf" 2> trash.txt`
if [ $? -eq 2 ]
then 
	echo "Success: correct error code (2) for nonexistent input file"
else 
	echo "Error: did not catch bad input file"
fi

### ERROR 3 ###

#test write protected file
`touch nowrite.txt`
`chmod u-w nowrite.txt`
`echo "abc" | ./lab0 --output="nowrite.txt" 2> trash.txt`
if [ $? -eq 3 ]
then
	echo "Success: correct error code (3) for unable to write to file"
else
	echo "Error: did not catch unwriteable output"
fi

### ERROR 4 ###

#test segfault and catching
`./lab0 --segfault --catch 2> trash.txt`
if [ $? -eq 4 ]
then 
	echo "Success: correct error code (4) for segfault"
else 
	echo "Error: did not make and catch segfault"
fi

`rm -f trash.txt in.txt out.txt nowrite.txt`