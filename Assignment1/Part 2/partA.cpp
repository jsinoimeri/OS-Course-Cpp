/*****************************************************
 *                                                   *
 *   The solution to the sleeping barber problem     *
 *   by forking three processes:                     *
 *                                                   *
 *   barber -> cuts hair or sleeps if no customers   *
 *                                                   *
 *   waiting room -> maintains a queue (FIFO)        *
 *                   of customers                    *
 *                                                   *
 *   customers -> wait to serviced or leave          *
 *                if no room                         *
 *                                                   *
 *                                                   *
 *   File Name: partA.cpp                            *
 *                                                   *
 *   By: Jeton Sinoimeri                             *
 *   Student Num: 100875046                          *
 *                                                   *
 *   Version: 1.6                                    *
 *   Created: Oct 5, 2014                            *
 *   Modified: Oct 29, 2014                          *
 *                                                   *
 *****************************************************/



#include <iostream>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>


using namespace std;




// function prototypes
void barber(int barberTOwaiting_r, int barberTOwaiting_w, int barberTOcustomer);
void waiting_room(int waitingTOcustomer_r, int waitingTOcustomer_w, int waitingTObarber_r, int waitingTObarber_w);
void create_customers(int customerTOwaiting_r, int customerTOwaiting_w, int customerTObarber);

void customer(int customerTOwaiting_r, int customerTOwaiting_w, int customerTObarber);
void pop(int * pid);
void push(int pid);



// linked list stucture
typedef struct node 
{
  int childpID = -1;        // value of pid to be stored
  node *next = NULL;        // pointer to next node
} Node;




// Global data constants
const int MAXNUMCHAIRS = 7;
const int SLEEPTIME = 5;


// Global data
int customerNum = 0;       // number of customers

Node * head,       // linked list head pointer
     * tail;       // linked list tail pointer


	




int main(void)
{
	
	int pipe_1[2],                   // variable for pipe 1 array
	 	  pipe_2[2],                   // variable for pipe 2 array
	    pipe_3[2],	                 // variable for pipe 3 array
	    pipe_4[2],                   // variable for pipe 4 array
	    pipe_5[2];                   // variable for pipe 5 array
	    
	    
	int barberProcess,               // variable to store process id for barber
	    waitingRoomProcess,          // variable to store process id for waiting room
	    customerCreationProcess;     // variable to store process id for creating customers
		
		
	// creating pipes for communication between processes	
	pipe(pipe_1);
  pipe(pipe_2);
  pipe(pipe_3);
  pipe(pipe_4);
  pipe(pipe_5);
  
  int barberTOwaiting_r = pipe_1[0];    // barber to waiting room reading pipe pointer
  int waitingTObarber_w = pipe_1[1];    // waiting room to barber writing pipe pointer
  
  int waitingTObarber_r = pipe_2[0];    // waiting room to barber reading pipe pointer
  int barberTOwaiting_w = pipe_2[1];    // barber to waiting room writing pipe pointer
  
  int waitingTOcustomer_r = pipe_3[0];  // waiting room to customer reading pipe pointer
  int customerTOwaiting_w = pipe_3[1];  // customer to waiting room writing pipe pointer
		
	int customerTObarber = pipe_4[0];     // customer to barber reading pipe pointer 
	int barberTOcustomer = pipe_4[1];	    // barber to customer writing pipe pointer
	
	int customerTOwaiting_r = pipe_5[0];  // customer to waiting room reading pipe pointer
	int waitingTOcustomer_w = pipe_5[1];  // waiting room to customer writing pipe pointer
		
		
		
	// create the barber process
	barberProcess = fork();
	
	// check if error occured
	if (barberProcess == -1)
	{
    perror("fork");
    exit(1);
	}
	
	// run the child process for the barber function
	if (barberProcess == 0)
	{
		cout << "Sucessfully created the barber process.\n";
		
		// close one end of the pipe communications
		close(customerTObarber);
		close(waitingTObarber_w);
		close(waitingTObarber_r);
		
		barber(barberTOwaiting_r, barberTOwaiting_w, barberTOcustomer);
	}
	
	
	// parent process runs
	else
	{
		
		// create the waiting room process
		waitingRoomProcess = fork();
		
		// check if error occured
		if (waitingRoomProcess == -1)
		{
   	  perror("fork");
   	  exit(1);
		}
	
		// run the child process for waiting room function
		if (waitingRoomProcess == 0 )
		{
			cout << "Sucessfully created the waiting room process.\n";	
			
			// close one end of pipe communications
			close(barberTOwaiting_r);
			close(barberTOwaiting_w);
			close(customerTOwaiting_r);
			close(customerTOwaiting_w);
					
			waiting_room(waitingTOcustomer_r, waitingTOcustomer_w, waitingTObarber_r, waitingTObarber_w);
		}
		
		
		// parent process runs
		else
		{
			// create the customer creation process	
			customerCreationProcess = fork();
			
			// check if error occured
			if (customerCreationProcess == -1)
			{
    		perror("fork");
    		exit(1);
			}	
		
			// run the child process for creating customers
			if (customerCreationProcess == 0)
			{
				cout << "Sucessfully created the customer creation process.\n";
				
				// close one end of pipe
				close(barberTOcustomer);
				close(waitingTOcustomer_r);
				close(waitingTOcustomer_w);
				
				create_customers(customerTOwaiting_r, customerTOwaiting_w, customerTObarber);
			}
	
	    // parent process wait
			else
				wait(NULL);
			
		}
		
	}
	
	// exit
	exit(0);
}



/***************************************************
 *                                                 *
 *                 FUNCTIONS                       *
 *                                                 *
 ***************************************************/




/*************************************
 *                                   *
 *   Pops the first element          *
 *   of the linked list and          *
 *   moves the head pointer of       *
 *   the list to the next element    *
 *   in the list.                    *
 *                                   *
 *   @arg *pid -> representing the   *
 *                address of where   *
 *                PID is stored      *
 *                                   *
 *************************************/
 
void pop(int *pid)
{
	*pid = head -> childpID;
	
	head = head -> next;
}



/*************************************
 *                                   *
 *   Pushes to the end of the        *
 *   the linked list.                *
 *                                   *
 *   If the list is empty, will      *
 *   assign the head of the list     *
 *   to the new node. Otherwise      *
 *   assigns it to the tail of the   *
 *   list and points the tail        *
 *   to the end of the list.         *
 *                                   *
 *                                   *
 *   @arg *pid -> representing the   *
 *                address of where   *
 *                PID is stored      *
 *                                   *
 *************************************/

void push(int pid)
{
	
	Node *new_node = new node;
	
	new_node -> childpID = pid;
	new_node -> next = NULL;
	
	// check if it is the first in the list
	if (head == NULL)
		head = new_node;
	
	
	// assign tail to point to the last element pointer
	tail = new_node;
	tail = tail -> next;
	
	
}



void barber(int barberTOwaiting_r, int barberTOwaiting_w, int barberTOcustomer)
{
	
	int timer,
	    cutTime,
	    barberReady,
	    customer_pid;	
		
		
	while (true)
	{
		barberReady = 1;
		
		// write to pipe to notify waiting room process
		write(barberTOwaiting_w, &barberReady, sizeof(barberReady));
		
		customer_pid = -1;
		timer = 0;
		
		// check if there are customers otherwise sleep
		while(customer_pid == -1)
		{
			read(barberTOwaiting_r, &customer_pid, sizeof(customer_pid));
			
			if (customer_pid == -1)
			{
			  cout << "No new customer. Barber sleep\n";
			  sleep(2);
		  } 
		  
	  }
	  
	  cout << "Barber received customer pid: " << customer_pid << endl;
		
		// cut hair	
		cutTime = rand() % 10 + 1;
		
		while (timer != cutTime)
		  timer ++;
			
		// done with customer
		cout << "Done cutting hair. Next customer.\n";
		
		
		// send message to customer process that barber is done
		write(barberTOcustomer, &customer_pid, sizeof(customer_pid));	
				
	}
	
}


void waiting_room(int waitingTOcustomer_r, int waitingTOcustomer_w, int waitingTObarber_r, int waitingTObarber_w)
{	
	
	// get info from customer - waiting room pipe
	
	int customer_pid,
	    barberReady;
	
	while (true)
	{
		
		// get info from barber - waiting room pipe
		barberReady = 0;
		
		cout << "Waiting for barber to be ready.\n";
		
		while (barberReady == 0)
		{
			customer_pid = -1;
		
			// waiting for customer pid to be received
			while (customer_pid == -1)
				read(waitingTOcustomer_r, &customer_pid, sizeof(customer_pid));
				
			
			cout << "Received from customer, this pid: " << customer_pid << endl;
			
			if (customerNum < MAXNUMCHAIRS)
			{
				customerNum ++;
			
			  // adding customer to queue
			  push(customer_pid);
			}
			
			else
				write(waitingTOcustomer_w, &customerNum, sizeof(customerNum));
				

			
			// get info from barber
			read(waitingTObarber_r, &barberReady, sizeof(barberReady));
		}
		
		cout << "Received signal from barber indicating he is ready. Sending customer to him.\n";
		
		// get first customer from waiting room
		pop(&customer_pid);
		
		// send info to barber - waiting room pipe
		write(waitingTObarber_w, &customer_pid, sizeof(customer_pid));
		
		cout << "Customer pid sent: " << customer_pid << endl;
		
		customerNum --;
  }
	
}


void create_customers(int customerTOwaiting_r, int customerTOwaiting_w, int customerTObarber)
{
	int timer = 0;
	
	int customerProcess;
	
	while (timer == 0)
	{
		int customerCreation = rand() % 60 + 1;
	
		while (timer != customerCreation)
			timer ++;
		
		timer = 0;
		//cout << timer << endl;
		
		
	  // create customer process
		customerProcess = fork();
		
			
		// run child process for customers	
		if (customerProcess == 0)
		{ 
			cout << "Successfully created customer process.\n";
			customer(customerTOwaiting_r, customerTOwaiting_w, customerTObarber);
		}
		
		// sleep for some time before creating another customer
		sleep(1);		
	}
				
}




void customer(int customerTOwaiting_r, int customerTOwaiting_w, int customerTObarber)
{
	
	cout << "customer created" << endl;
	
	int pid = getpid();
	
	
	// write to pipe sending pid
	write(customerTOwaiting_w, &pid, sizeof(pid));
	
	cout << "Customer with id: " << pid << " has entered the waiting room.\n";
	
	while (customerNum < MAXNUMCHAIRS)
	{
		  // gets message from waiting room in order to 
	   read(customerTOwaiting_r, &customerNum, sizeof(customerNum));
	   
		 // check if customerNum exceedes MAXNUMCHAIRS
	   if (customerNum > MAXNUMCHAIRS)
	   {
		   cout << "No chairs left in waiting room, customer with id: " << pid << " is leaving." << endl;
		   exit(0);
	   } 
	}
	
	
	cout << "Customer with id: " << pid << " is waiting in waiting room.\n";
	
	
	int pid_received;
	
	// gets message from barber in order to finish
	while (pid_received != pid)	
	   read(customerTObarber, &pid_received, sizeof(pid_received));
	
	
	cout << "Customer with id: "<< pid << " has left. Thank you. Bye bye\n";
	
	exit(0);
}
