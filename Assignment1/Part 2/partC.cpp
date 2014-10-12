/*****************************************************
 *                                                   *
 *   The solution to the sleeping barber problem     *
 *   using SEMAPHORES to share information among     *
 *   the three processes:                            *
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
 *   File Name: partC.cpp                            *
 *                                                   *
 *   By: Jeton Sinoimeri                             *
 *                                                   *
 *   Version: 1.5                                    *
 *   Created: Oct 5, 2014                            *
 *   Modified: Oct 10, 2014                          *
 *                                                   *
 *****************************************************/



#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h> 

using namespace std;


#define SEM_MODE (0400 | 0200 | 044)
#define	IPC_ERROR	(-1)
#define BLOCK		0
#define DONT_BLOCK	IPC_NOWAIT

const int MAXNUMCHAIRS = 7;


int mutex,
    seats_avail = MAXNUMCHAIRS;


void Barber(int * waiting, int * barberReady, int *customerNum, int barberSem, int customerSem, int mutex);
void Customer(int * waiting, int * barberReady, int *customerNum, int barberSem, int customerSem, int mutex);
void create_customers(int * waiting, int * barberReady, int *customerNum,  int barberSem, int customerSem, int mutex);

void waiting_room(int * waiting, int * barberReady, int *customerNum,  int barberSem, int customerSem, int mutex);
void pop(int * pid);
void push(int pid);



// linked list stucture
typedef struct node 
{
  int childpID = -1;        // value of pid to be stored
  node *next = NULL;        // pointer to next node
} Node;

Node * head,       // linked list head pointer
     * tail;       // linked list tail pointer


int SemaphoreWait(int semid, int block )
{
	struct sembuf	sbOperation;

	sbOperation.sem_num = 0;
	sbOperation.sem_op = -1;
	sbOperation.sem_flg = block;

	return semop( semid, &sbOperation, 1 );
}

int SemaphoreSignal( int semid )
{
	struct sembuf	sbOperation;

	sbOperation.sem_num = 0;
	sbOperation.sem_op = +1;
	sbOperation.sem_flg = 0;

	return semop( semid, &sbOperation, 1 );
}

void SemaphoreRemove( int semid )
{
	if(semid != IPC_ERROR )
		semctl( semid, 0, IPC_RMID ,0);
}

int SemaphoreCreate(int value)
{
	int	semid;
	
	union semnum {

    int val;

    struct semid_ds *buf;

    unsigned short *array;

  }sulnitData;
  

	// get a semaphore 
	semid = semget( IPC_PRIVATE, 1, SEM_MODE );
	// check for errors 
	if( semid == IPC_ERROR ) 
		return semid;

	// initialize the semaphore
	sulnitData.val = value;
	
	// remove if error
	if(semctl( semid, 0, SETVAL, sulnitData) == IPC_ERROR )
	{
		SemaphoreRemove( semid );
		return IPC_ERROR;
	}

	return semid;
}
	



int main(void) 
{


 int shm_id,
     shm_id2,
     barberSem,
     customerSem,
     barberProcess,
     shm_key = 2222,
     shm_key2 = 4444,
     customerCreationProcess,
     waitingRoomProcess;
    
    
  
  shm_id = shmget(shm_key, sizeof(int), IPC_CREAT|0666);
  shm_id2 = shmget(shm_key2, sizeof(int), IPC_CREAT|0666);
  
  // check for errors
  if(shm_id <= 0)
  { 
     perror("Error");
     exit(1);
  } 
  
  // check for errors
  if( shm_id2 <= 0)
  {
    perror("Error");
    exit(1);
  }
  
  // shared memory
  int * waiting = (int *) shmat(shm_id, (char*) 0, 0);
  int * barberReady = (int *) shmat(shm_id, (char*) 0, 0);
  int * customerNum = (int *) shmat(shm_id, (char*) 0, 0);
 
  // check for errors
  if(waiting == (int*)-1)
  {
     perror("Error");
     exit(1);
  }

  
  
  // create the semaphores
  barberSem = SemaphoreCreate(0);
  //cout << barberSem << endl;
  customerSem = SemaphoreCreate(1);
  mutex = SemaphoreCreate(1);
  
  
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
		Barber(waiting, barberReady, customerNum, barberSem, customerSem, mutex);
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
			waiting_room(waiting, barberReady, customerNum,  barberSem, customerSem, mutex);
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
				create_customers(waiting, barberReady, customerNum,  barberSem, customerSem, mutex);
			}
	
	    // parent process wait
			else
				wait(NULL);
			
		}
		
	}
	
	
	// exit
	exit(0);
  
}


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



/*********************************************
 * child process calls the barber function   *
 *********************************************/

 
void Barber(int * waiting, int * barberReady, int *customerNum,  int barberSem, int customerSem, int mutex)
{
  int timer,
      cutTime;
      
   *waiting = MAXNUMCHAIRS;
      
  while (true)                  
  {
    SemaphoreWait(customerSem, BLOCK);             // Try to acquire a customer - if none is available, go to sleep.

    SemaphoreWait(mutex, BLOCK);
    	
    *waiting ++;
    
    SemaphoreSignal(barberSem);              // I am ready to cut.
    * barberReady = 1;
    SemaphoreSignal(mutex);       // Don't need the lock on the chairs anymore.
    
    timer = 0;
    
    cout << "cutting hair.\n";
    // cut hair	
		cutTime = rand() % 10 + 1;
		
		while (timer != cutTime)
		  timer ++;  
		  
		 cout << "finished cutting hair.\n";
		  
		*barberReady = -9;
		 
  }
}


/***************************************************
 * child process calls the waiting room function   *
 ***************************************************/

void waiting_room(int * waiting, int * barberReady, int *customerNum,  int barberSem, int customerSem, int mutex)
{	
	
	
	int customer_pid;

	
	while (true)
	{
		
		cout << "Waiting for barber to be ready.\n";
		
		while (*barberReady != 0)
		{
			customer_pid = *customerNum;
				
			
			cout << "Received from customer, this pid: " << customer_pid << endl;
					
			// adding customer to queue
			push(customer_pid);
		}
		
		cout << "Received signal from barber indicating he is ready. Sending customer to him.\n";
		
		// get first customer from waiting room
		pop(&customer_pid);
		
		// send info to barber
		*customerNum = customer_pid;
		
		cout << "Customer pid sent: " << customer_pid << endl;
		
  }
	
}
 


void create_customers(int * waiting, int * barberReady, int *customerNum,  int barberSem, int customerSem, int mutex)
{
	int timer = 0;
	
	int customerProcess;
	
	while (timer == 0)
	{
		int customerCreation = rand() % 60 + 1;
	
		while (timer != customerCreation)
			timer ++;
		
		timer = 0;
		
		
	  // create customer process
		customerProcess = fork();
		
			
		// run child process for customers	
		if (customerProcess == 0)
		{ 
			cout << "Successfully created customer process.\n";
			Customer(waiting, barberReady, customerNum,  barberSem, customerSem, mutex);
		}
		
		// sleep for some time before creating another customer
		sleep(2);		
	}
				
}



void Customer(int * waiting, int * barberReady, int *customerNum,  int barberSem, int customerSem, int mutex)
{
	
  while (true)                     
  {
    SemaphoreWait(mutex, BLOCK);        // Try to get access to the waiting room chairs.
    
    // check if there are any free seats
    if (seats_avail > 0)          
    {
      *waiting --;
      
      SemaphoreSignal(customerSem);         
      SemaphoreSignal(mutex);     
      SemaphoreWait(barberSem, BLOCK);       
      
      //*barberReady = 0;
      
      while (*barberReady != 0)
      {
      	wait(NULL);
      }
      
      if (*barberReady == -9)
      {
      	cout << "customer exiting\n";
      	exit(0);
      }
    }
    
    // if no seats leave
    else
    {                                    
      SemaphoreSignal(mutex);
      exit(0);
    }
    
  }
                                 
}
