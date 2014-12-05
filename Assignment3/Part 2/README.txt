/*************LIST OF FILES*******************/

Part1.docx
ATM.c
DBEditor.c
DBServer.c
db.txt
partB.c
partC.c


/************COMPILING AND RUNNING IN LINUX***********/

For part A:
The following commands are needed to run the three c files concurrently
note each command needs to be run in their own individual terminal.

gcc -o ATM.out ATM.c
./ATM.out

gcc -o DBEditor.out DbEditor.c
./DBEditor.out

gcc -o DBServer.out DBServer.c
./DBServer.out

For part b
The following command will compile and run the program.
gcc -o partB.out partB.c -pthread
./partB.out

For part c
The following command will compile and run the program.
gcc -o partC.out partC.c -pthread
./partC.out
