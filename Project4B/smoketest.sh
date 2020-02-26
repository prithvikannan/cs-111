#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

{ echo "START"; sleep 1; echo "STOP"; sleep 1; echo "START"; sleep 1; echo "SCALE=C"; sleep 1; echo "LOG"; sleep 1; echo "PERIOD=2" sleep 5; echo "OFF"; } | ./lab4b --log=temp.txt > output.txt

[ $? -ne 0 ] && echo "Return code should be 0...FAIL" || echo "Return code should be 0...PASS"

for command in START STOP SCALE LOG PERIOD OFF SHUTDOWN
do
    grep "$command" temp.txt > grep.txt
    [ $? -ne 0 ] && echo "Attempting to log $command...FAIL" || echo "Attempting to log $command...PASS"
done

rm -f temp.txt grep.txt output.txt