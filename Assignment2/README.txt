/******************List of Files************************************/

part2.c
README.txt
Input.txt
Part1.docx
Report.docx


/****************** INPUT FILE FORMATS ****************************/

The input file contains a series of numbers 5 numbers (6 if using external priorities) per line.
Each line represents one Process. As shown in the example below. please also
refer to the various input.txt files given in our submission.

eg.

1 2 3 4 5 -----> this is a process with a pid of 1, an arrival time of 2, cpu duration of 3, I/0 freq of 4 and I/O duration of 5 
2 3 4 5 5	  	
3 8 9 0 9
5 0 20 34 5


OR if using external priorities

1 2 3 4 5 6  -----> this is a process with a pid of 1, an arrival time of 2, cpu duration of 3, I/0 freq of 4 and I/O duration of 5 
2 3 4 5 5 100       and a Priority of 6.
3 4 5 45 45 3
4 3 2 3 34 2 


the first number represents the pid of the process
the second number is the arrival time of the process



the third number is the CPU duration of that process
the fourth number is the I/O Frequency 
the 5th number is the I/O duration.

the 6th optional number is the priority value 
(higher it is the higher the priority of the process)


/*****************COMPILING THE PROGRAM IN LINUX*****************/

The Scheduler must be compiled using Linux as it uses threads to function. The following Linux commands will allow the user
to compile and run the code.

gcc -o part2.out part2.c -pthread -std=c99
./part2.out


/*****************TESTING THE PROGRAM****************************/

Our program is  both the non I/O version and I/O version of the scheduler
simulator in one. If you want to test the program to run without I/O
please make sure that the I/0 frequency value on all processes is 0 or higher than
than cpu duration in the input file. This will allow our program to ignore 
I/O executions.

The program and given contraints with the time.h library each milisecond we represent in the 
input file coresponds to 1 second in real life. so the program could take a minute or two to complete
its fetch and excecute, and I/O cycles depending on how large the I/O durrations and CPU time is 
given to each process. The program will print to the terminal as it is running then output a txt file 
when complete.
