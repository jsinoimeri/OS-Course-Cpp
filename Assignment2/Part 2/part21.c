/*******************************************************
 *                                                     *
 *   CPU Scheduling First-Come, Frist Serve (FCFS)     *
 *                                                     *
 *   FileName: part2.c                                 *
 *                                                     *
 *   Name: Jeton Sinoimeri                             *
 *   Name: Varun Sriram                                *
 *                                                     *
 *   Version: 1.0                                      *
 *   Since: Nov 9, 2014                                *
 *                                                     *
 *******************************************************/


// throughput is the number of processors completed per time unit
// turnaround time is the amount of time to complete a process
// wait time is the amount of time that the process spends in the 


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <pthread.h>


// linked list stucture
typedef struct node 
{
  int pID;                  // value of pid to be stored
  
  time_t arrivalTime;       // value of arrival time
  
  int totalCPUTime,         // value of Total CPU time
      IOFrequency,          // value of I/O frequency
      IODuration;           // value of IO Duration
  
  double waitTime;          // value of wait time
  
  time_t completeTime;      // value of time for process to complete
  
  struct node *next;        // pointer to next node
} Node;



int jobcount = 0;            // the number of jobs in the system

pthread_t ThreadA;


// Function prototypes
void pop(Node **head, Node **item);
void push(int pid, int tCtime, int IOF, int IOD, double waitTime, double completeT, Node **head, Node **tail);
void readFile(FILE *ptr_file, Node **head, Node **tail);

void timer(Node *pcb, int timeSpent);
void IOExecution(Node *currentProcess, Node **head, Node **tail);
void CPUExecution(Node **head, Node **tail, Node **terHead, Node **terTail);

void pushGivenNode(Node *t, Node **head, Node **tail);
int LinkedListgetSize(Node *head, Node *tail);
double calcThroughPut(time_t startTime, time_t endTime);

double calcAvgWaitTime(Node *terHead);
double calcAvgTurnAroundTime(time_t startTime, Node *terHead);




int main(void)
{
	Node * headReady = NULL,            // linked list head pointer
       * tailReady = headReady,       // linked list tail pointer
       * headTerminated = NULL,
       * tailTerminated = headTerminated; 

	FILE *ptr_file = NULL;              // File pointer


	// read the file
	readFile(ptr_file, &headReady, &tailReady);
	
		time_t startSimTime,
				 endSimTime;
				 
				 
	time(&startSimTime);
	
	
	// start simulation
	while(headReady != NULL)
		CPUExecution(&headReady, &tailReady, &headTerminated, &tailTerminated);
	
	
	time(&endSimTime);
	
	
	double throughPut = calcThroughPut(startSimTime, endSimTime);
	double avgWaitTime = calcAvgWaitTime(headTerminated);
	double avgTurnAroundTime = calcAvgTurnAroundTime(startSimTime, headTerminated);
	
	printf("The size of the job queue is: %i\n", LinkedListgetSize(headReady, tailReady));
	printf("The size of the terminated queue is: %i\n", LinkedListgetSize(headTerminated, tailTerminated));
	
	
	printf("The throughput is: %lf\n", throughPut);
	printf("The avg wait time is: %lf\n", avgWaitTime);
	printf("The avg turn around time is: %lf\n", avgTurnAroundTime);

	
	return 0;
}







/*****************************************************
 *                                                   *
 *                     FUNCTIONS                     *
 *                                                   *
 *****************************************************/

/*
void printList(Node *head, Node *tail)
{
	Node *head2 = head;

	while(head2)
	{                                                                                        
		printf("pid 2: %i Arrive_time: %i CPUTIME: %i IOFreq: %i IOdur: %i\n", head2 -> pID, head2 -> arrivalTime, head2 -> totalCPUTime, head2 -> IOFrequency, head2 -> IODuration);
		head2 = head2 -> next;
	}
	
	
	while(head)
	{
		pop(&head, &head2);
		printf("pid 3: %i Arrive_time: %i CPUTIME: %i IOFreq: %i IOdur: %i\n", head2 -> pID, head2 -> arrivalTime, head2 -> totalCPUTime, head2 -> IOFrequency, head2 -> IODuration);
	}
}  
*/


double calcThroughPut(time_t startTime, time_t endTime)
{
	double throughPut = jobcount / difftime(endTime, startTime);
	
	return throughPut;
}


double calcAvgWaitTime(Node *terHead)
{
	double totalWaitTime = 0;
	
	while (terHead)
	{		
		totalWaitTime  += terHead -> waitTime;
		terHead = terHead -> next;
	}
	
	double avgWaitTime = totalWaitTime / jobcount;
	
	return avgWaitTime;
	
}


double calcAvgTurnAroundTime(time_t startTime, Node *terHead)
{
	double totalTurnAroundTime = 0;
	
	while (terHead)
	{
		totalTurnAroundTime += difftime(terHead -> completeTime, startTime);
		terHead = terHead -> next;
	}
	
	double avgTurnAroundTime = totalTurnAroundTime / jobcount;
	
	return avgTurnAroundTime;
	
}



int LinkedListgetSize(Node *head, Node *tail)
{
	int size = 0;
	
	while (head)
	{
		size ++;
		head = head -> next;
	}
	
	return size;
	
}


void CPUExecution(Node **head, Node **tail, Node **terHead, Node **terTail)
{
	Node *currentProcess;
	
	
	
	
	pop(head, &currentProcess);
	
	
	currentProcess -> waitTime += difftime(time(NULL), currentProcess -> arrivalTime);
	
	
	printf("The process ID: %i is entering CPUExecution\n", currentProcess -> pID);
	printf("This process waited for %lf\n", currentProcess -> waitTime);
	
	int ioFreq = currentProcess -> IOFrequency;
	
	
	if (currentProcess -> IOFrequency > 0 && currentProcess -> totalCPUTime > ioFreq)
	{
		currentProcess -> totalCPUTime = currentProcess -> totalCPUTime - currentProcess -> IOFrequency;
		
		timer(currentProcess, currentProcess -> IOFrequency);    // cpu exectution
		
		if (currentProcess -> totalCPUTime > 0)
		{
			IOExecution(currentProcess, head, tail);
		}
		
	}
	
	else
	{
		timer(currentProcess, currentProcess -> totalCPUTime);
		
		currentProcess -> completeTime = time(NULL);
		
		pushGivenNode(currentProcess, terHead, terTail);
		
		printf("The process ID: %i is complete CPUExecution\n", currentProcess -> pID);
		
		} 
}


void IOExecution(Node *currentProcess, Node **head, Node **tail)
{
	printf("PID: %i entering IOExecution\n", currentProcess -> pID);
	timer(currentProcess, currentProcess -> IODuration);
		
	push(currentProcess -> pID, currentProcess -> totalCPUTime, currentProcess -> IOFrequency, currentProcess -> IODuration,
	     currentProcess -> waitTime, currentProcess -> completeTime, head, tail);
	     
	printf("PID: %i leaving IOExecution\n", currentProcess -> pID);
}




/***************************************
 *                                     *
 *   Pops the first element of the     *
 *   linked list and moves the head    *
 *   pointer of the list to the next   *
 *   element in the list.              *
 *                                     *
 *   @arg **head -> representing       *
 *                  the address of     *
 *                  head of the        *
 *                  linked list        *
 *                                     *
 *   @arg **item -> representing       *
 *                  address of the     *
 *                  pointer which      *
 *                  will be assigned   *
 *                  the  first         *
 *                  element in the     *
 *                  linked list        *
 *                                     *
 ***************************************/
 
void pop(Node **head, Node **item)
{
	*item = *head;
	
	if (*head)
		*head = (*head) -> next;
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
 *   @arg pid -> representing the    *
 *               PID to be stored    *
 *                                   *
 *   @arg arTime -> representing     *
 *                  the arrival      *
 *                  time of the      *
 *                  process          *
 *                                   *
 *   @arg tCtime -> representing     *
 *                  the total CPU    *
 *                  time for the     *
 *                  process          *
 *                                   * 
 *   @arg IOF -> representing the    *
 *               I/O frequency       *
 *               request for the     *
 *               process             *
 *                                   *
 *   @arg IOD -> representing the    *
 *               amount of time      *
 *               process nees the    *
 *               I/O                 *
 *                                   *
 *   @arg **head -> representing     *
 *                  the address of   *
 *                  head of the      *
 *                  linked list      *
 *                                   *
 *   @arg **tail -> representing     *
 *                  the address of   *
 *                  tail of the      *
 *                  linked list      *
 *                                   *
 *************************************/

void push(int pid, int tCtime, int IOF, int IOD, double waitTime, double completeT, Node **head, Node **tail)
{
	
	// create a new node
	Node *new_node = (Node *)malloc(sizeof(Node));
	
	// assign the values
	new_node -> pID = pid;
	new_node -> arrivalTime = time(NULL);         
  new_node -> totalCPUTime = tCtime;         
  new_node -> IOFrequency = IOF;          
  new_node -> IODuration = IOD;  
  new_node -> waitTime = waitTime;
  new_node -> completeTime = completeT;     
	new_node -> next = NULL;
	
	
	// check if new node is the first in the list
	if (!*head)
	{
		*head = new_node;
		*tail = *head;
	}
	
	// assign tail to point to the last element pointer
	else
	{
		(*tail)-> next = new_node;
	  *tail = new_node;
  }
	
}


void pushGivenNode(Node *t, Node **head, Node **tail)
{	
	// check if new node is the first in the list
	if (!*head)
	{
		*head = t;
		*tail = *head;
	}
	
	// assign tail to point to the last element pointer
	else
	{
		(*tail)-> next = t;
	  *tail = t;
  }
}


/**************************************
 *                                    *
 *   Reads from a file and stores     *
 *   the information into a linked    *
 *   list.                            *
 *                                    *
 *   @arg *ptr_file -> representing   *
 *                    a pointer to a  *
 *                    file to be      *
 *                    read            *
 *                                    *
 *   @arg **head -> representing      *
 *                 the address of     *
 *                 head of the        *
 *                 linked list        *
 *                                    *
 *   @arg **tail -> representing      *
 *                 the address of     *
 *                 tail of the        *
 *                 linked list        *
 *                                    *
 **************************************/

void readFile(FILE *ptr_file, Node **head, Node **tail)
{
	
	int pid,               // value of pid to be stored
	    IOf,               // value of I/O frequency   
	    IOd,               // value of IO Duration 
      cpu_time;          // value of Total CPU time       
	
	
	// open file
	ptr_file = fopen("Input1.bat","r");
	
	
	// return if file cannot be opened
  if(!ptr_file)
  {
    fprintf(stderr,"Cant Open File\n");
    return;
  }


	// otherwise read, push the info into the linked list and increment the num of jobs until end of file
  while(fscanf(ptr_file,"%i %i %i %i ", &pid,  &cpu_time, &IOf, &IOd) != EOF)
  {
    push(pid,cpu_time,IOf,IOd, 0, 0, head, tail);
    jobcount ++;
  }
  
  
  // close the file
  fclose(ptr_file);

}


void timer(Node *pcb, int timeSpent)
{
	time_t startTime,
				 currentTime;
	
	time(&startTime);
	
	
	while(currentTime - startTime != timeSpent)
		time(&currentTime);
	
	
	double diff = difftime(currentTime,startTime);
	
	printf("time: %lf\n", diff);
	
}
