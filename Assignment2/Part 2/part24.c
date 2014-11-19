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


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>


// linked list stucture
typedef struct node 
{
  int pID,                  // value of pid to be stored
      arrivalTime;          // value of arrival time
  
  time_t readyTime;       // value of ready queue time
  
  int totalCPUTime,         // value of Total CPU time
      IOFrequency,          // value of I/O frequency
      IODuration;           // value of IO Duration
  
  double waitTime;          // value of wait time
  
  time_t completeTime;      // value of time for process to complete
  
  int priority;
  
  struct node *next;        // pointer to next node
} Node;


// linked list stucture
typedef struct args 
{
	struct node *current;
  struct node **head;
  struct node **tail;        // pointer to next node
} Args;


// linked list stucture
typedef struct args1 
{
	struct node **head1,
              **head2,
              **tail1,      // pointer to next node
              **head3,
              **tail2;
} Args1; 



int jobcount = 0;            // the number of jobs in the system


pthread_mutex_t mutexNew = PTHREAD_MUTEX_INITIALIZER,
								mutexReady = PTHREAD_MUTEX_INITIALIZER,
								mutexTerminated = PTHREAD_MUTEX_INITIALIZER;



// Function prototypes
void pop(Node **head, Node **item);
void push(int pid, int arTime, int tCtime, int IOF, int IOD, double waitTime, int priority, Node **head, Node **tail);
void readFile(FILE *ptr_file, Node **head, Node **tail);

void timer(Node *pcb, int timeSpent);
void *IOExecution(void *args);
void CPUExecution(Node **head, Node **tail, Node **terHead, Node **terTail);

void pushGivenNode(Node *t, Node **head, Node **tail);
int LinkedListgetSize(Node *head, pthread_mutex_t *mutex);
double calcThroughPut(time_t startTime, time_t endTime);

double calcAvgWaitTime(Node *terHead);
double calcAvgTurnAroundTime(time_t startTime, Node *terHead);
void *admitted(void *arg);

void bubble_sort(Node *node_array[], int node_array_size);
void reorderReadyQueue(Node **head, Node **tail, int size);


int main(void)
{
	Node * headNew = NULL,
			 * tailNew = headNew,
			 * headReady = NULL,            // linked list head pointer
       * tailReady = headReady,       // linked list tail pointer
       * headTerminated = NULL,
       * tailTerminated = headTerminated; 


	pthread_mutex_init(&mutexNew, NULL);
	pthread_mutex_init(&mutexReady, NULL);
	pthread_mutex_init(&mutexTerminated, NULL);

	FILE *ptr_file = NULL;              // File pointer


	// read the file
	readFile(ptr_file, &headNew, &tailNew);
	
	
	time_t startSimTime,
				 endSimTime;
				 
				 
	time(&startSimTime);
	
	pthread_t ThreadA;
	
	
	while(LinkedListgetSize(headTerminated, &mutexTerminated) != jobcount)
	{
		if (headNew)
		{			
			Args1 *args1 = (Args1 *)malloc(sizeof(Args1));
			args1 -> head1 = &headNew;
			args1 -> head2 = &headReady;
			args1 -> tail1 = &tailReady;
			args1 -> head3 = &headTerminated;
			args1 -> tail2 = &tailTerminated;
				
			pthread_create(&ThreadA, NULL, admitted, (void *)args1);
		}
		
		CPUExecution(&headReady, &tailReady, &headTerminated, &tailTerminated);
	}		
	
	
	time(&endSimTime);
	
	
	double throughPut = calcThroughPut(startSimTime, endSimTime);
	double avgWaitTime = calcAvgWaitTime(headTerminated);
	double avgTurnAroundTime = calcAvgTurnAroundTime(startSimTime, headTerminated);
	
	printf("The size of the job queue is: %i\n", LinkedListgetSize(headReady, &mutexReady));
	printf("The size of the terminated queue is: %i\n", LinkedListgetSize(headTerminated, &mutexTerminated));
	
	
	printf("\nThe throughput is: %lf\n", throughPut);
	printf("The avg wait time is: %lf\n", avgWaitTime);
	printf("The avg turn around time is: %lf\n", avgTurnAroundTime);

	pthread_mutex_destroy(&mutexNew);
	pthread_mutex_destroy(&mutexReady);
	
	return 0;
}







/*****************************************************
 *                                                   *
 *                     FUNCTIONS                     *
 *                                                   *
 *****************************************************/





void *admitted(void *arg)
{
	
	Node *currentProcess;
	
	Args1 *args1 = (Args1 *)arg;
	
	pthread_mutex_lock(&mutexNew);
	
	pop(args1 -> head1, &currentProcess);
	
	pthread_mutex_unlock(&mutexNew);	
	
	if(currentProcess)
	{
		//printf("Pid in new state: %i\n", currentProcess -> pID);
		
		timer(currentProcess, currentProcess -> arrivalTime);
		
		time_t time1;
		time(&time1);
		printf("PID: %i\nTime of Transition: %sOld State: New\nNew State: Ready\n\n", currentProcess -> pID, ctime(&time1));	
			
		pthread_mutex_lock(&mutexReady);
		
		push(currentProcess -> pID, currentProcess -> arrivalTime, currentProcess -> totalCPUTime, currentProcess -> IOFrequency, 
		     currentProcess -> IODuration, currentProcess -> waitTime, currentProcess -> priority, args1 -> head2, args1 -> tail1);
		
		pthread_mutex_unlock(&mutexReady);
		
		int size = LinkedListgetSize(*(args1 -> head2), &mutexReady);
	
		//reorderReadyQueue(args1 -> head2, args1 -> tail1, size);
		
		//printf("Pid in ready state: %i\n", currentProcess -> pID);
	}
	
	
	
	return NULL;
}


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



int LinkedListgetSize(Node *head, pthread_mutex_t *mutex)
{
	int size = 0;

	pthread_mutex_lock(mutex);
	
	while (head)
	{
		size ++;
		head = head -> next;
	}
	
	pthread_mutex_unlock(mutex);
	
	return size;
	
}


void CPUExecution(Node **head, Node **tail, Node **terHead, Node **terTail)
{
	Node *currentProcess;
	
	pthread_mutex_lock(&mutexReady);
	
	pop(head, &currentProcess);
	
	pthread_mutex_unlock(&mutexReady);
	
	
	time_t time1;
	time(&time1);
	
	if (currentProcess)
	{
	
		currentProcess -> waitTime += difftime(time(NULL), currentProcess -> readyTime);
		
		
		//printf("The process ID: %i is entering CPUExecution\n", currentProcess -> pID);
		//printf("This process waited for %lf milliseconds\n", currentProcess -> waitTime);
		
		int ioFreq = currentProcess -> IOFrequency;
		
		
		if (currentProcess -> IOFrequency > 0 && currentProcess -> totalCPUTime > ioFreq)
		{
			currentProcess -> totalCPUTime = currentProcess -> totalCPUTime - currentProcess -> IOFrequency;
			
			printf("PID: %i\nTime of Transition: %sOld State: Ready\nNew State: Running\n\n", currentProcess -> pID, ctime(&time1));
			
			timer(currentProcess, currentProcess -> IOFrequency);    // cpu exectution
			
			if (currentProcess -> totalCPUTime > 0)
			{
				pthread_t ThreadB;
				
				time(&time1);
				printf("PID: %i\nTime of Transition: %sOld State: Running\nNew State: IO\n\n", currentProcess -> pID, ctime(&time1));
				
				Args *args1 = (Args *)malloc(sizeof(Args));
				args1 -> current = currentProcess;
				args1 -> head = head;
				args1 -> tail = tail;
				
				pthread_create(&ThreadB, NULL, IOExecution, (void *)args1);
			}
			
		}
		
		else
		{
			timer(currentProcess, currentProcess -> totalCPUTime);
			
			
			pthread_mutex_lock(&mutexTerminated);
			
			push(currentProcess -> pID, currentProcess -> arrivalTime, currentProcess -> totalCPUTime, currentProcess -> IOFrequency, 
	     currentProcess -> IODuration, currentProcess -> waitTime, currentProcess -> priority, terHead, terTail);
			
			pthread_mutex_unlock(&mutexTerminated);
			
			time(&time1);
			printf("PID: %i\nTime of Transition: %sOld State: Running\nNew State: Terminated\n\n", currentProcess -> pID, ctime(&time1));
			
			//printf("The process ID: %i is complete CPUExecution\n", currentProcess -> pID);
			
		}
	}
}


void *IOExecution(void *args)
{
	Args *args1 = (Args *)args;
	Node *currentProcess = args1 -> current;
	
	 
	//printf("PID: %i entering IOExecution\n", currentProcess -> pID);
	timer(currentProcess, currentProcess -> IODuration);
	
	pthread_mutex_lock(&mutexReady);
		
	push(currentProcess -> pID, currentProcess -> arrivalTime, currentProcess -> totalCPUTime, currentProcess -> IOFrequency, 
	     currentProcess -> IODuration, currentProcess -> waitTime, currentProcess -> priority, args1 -> head, args1 -> tail);
	     
	pthread_mutex_unlock(&mutexReady);
	
	int size = LinkedListgetSize(*(args1 -> head), &mutexReady);
	
	//reorderReadyQueue(args1 -> head, args1 -> tail, size);
	     
	time_t time1;
	time(&time1);
	
	printf("PID: %i\nTime of Transition: %sOld State: IO\nNew State: Ready\n\n", currentProcess -> pID, ctime(&time1));
	//printf("PID: %i leaving IOExecution\n", currentProcess -> pID);
	
	return NULL;
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

void push(int pid, int arTime, int tCtime, int IOF, int IOD, double waitTime, int priority, Node **head, Node **tail)
{
	
	// create a new node
	Node *new_node = (Node *)malloc(sizeof(Node));
	
	// assign the values
	new_node -> pID = pid;
	new_node -> arrivalTime = arTime; 
	new_node -> readyTime = time(NULL);        
  new_node -> totalCPUTime = tCtime;         
  new_node -> IOFrequency = IOF;          
  new_node -> IODuration = IOD;  
  new_node -> waitTime = waitTime;
  new_node -> completeTime = time(NULL);  
  new_node -> priority = priority;
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
	  *tail = (*tail) -> next;
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
	    arT,               // value of arrival time to the ready queue
	    IOf,               // value of I/O frequency   
	    IOd,               // value of IO Duration 
      cpu_time,          // value of Total CPU time
      priority;          // value of priority   
	
	
	// open file
	ptr_file = fopen("Input.txt","r");
	
	
	// return if file cannot be opened
  if(!ptr_file)
  {
    fprintf(stderr,"Cant Open File\n");
    return;
  }


	// otherwise read, push the info into the linked list and increment the num of jobs until end of file
  while(fscanf(ptr_file,"%i %i %i %i %i %i", &pid, &arT, &cpu_time, &IOf, &IOd, &priority) != EOF)
  {
  	// create a new node
		Node *new_node = (Node *)malloc(sizeof(Node));
	
		// assign the values
		new_node -> pID = pid;
		new_node -> arrivalTime = arT; 
		new_node -> readyTime = time(NULL);        
	  new_node -> totalCPUTime = cpu_time;         
	  new_node -> IOFrequency = IOf;          
	  new_node -> IODuration = IOd;  
	  new_node -> waitTime = 0.0;
	  new_node -> completeTime = time(NULL);
	  new_node -> priority = priority;  
		new_node -> next = NULL;
  	
  	
  	pushGivenNode(new_node, head, tail);
  	
    jobcount ++;
  }
  
  
  // close the file
  fclose(ptr_file);

}





   /*****************************************
    *                                       * 
    *   Sorts the int array from largest    *
    *   to smallest using the bubble sort   *
    *   algorithim                          *
    *                                       *
    *****************************************/

void bubble_sort(Node *node_array[], int node_array_size)
{

   Node *temp_node = (Node *)malloc(sizeof(Node));

   for (int i = 0; i < node_array_size; i++)
   {
      int x = node_array_size - i - 1;

      for(int j = 0; j < x; j++)
      {
         if (node_array[j + 1] -> priority > node_array[j] -> priority)
         {
            temp_node = node_array[j + 1];
            node_array[j + 1] = node_array[j];
            node_array[j] = temp_node;
         }
      }
   }
}



void reorderReadyQueue(Node **head, Node **tail, int size)
{
	Node *nodeArrays[size];
	Node *currentProcess;
	
	pthread_mutex_lock(&mutexReady);
	
	for (int i = 0; i < size; i++)
	{
		pop(head, &currentProcess);
		nodeArrays[i] = currentProcess;
	}
	
	//bubble_sort(&nodeArrays, size);
	
	
	 Node *temp_node;

   for (int i = 0; i < size; i++)
   {
      int x = size - i - 1;

      for(int j = 0; j < x; j++)
      {
         if (nodeArrays[j + 1] -> priority > nodeArrays[j] -> priority)
         {
            temp_node = nodeArrays[j + 1];
            nodeArrays[j + 1] = nodeArrays[j];
            nodeArrays[j] = temp_node;
         }
      }
   }
	
	
	for (int i = 0; i < size; i++)
	{
		nodeArrays[i] = currentProcess;
		push(currentProcess -> pID, currentProcess -> arrivalTime, currentProcess -> totalCPUTime, currentProcess -> IOFrequency, 
	     currentProcess -> IODuration, currentProcess -> waitTime, currentProcess -> priority, head, tail);
	}
	
	printf("Head is now pointing to pid: %i\n", (*head) -> pID);
	pthread_mutex_unlock(&mutexReady);
}


void timer(Node *pcb, int timeSpent)
{
	time_t startTime,
				 currentTime;
	
	time(&startTime);
	
	
	while(currentTime - startTime != timeSpent)
		time(&currentTime);
	
	
	double diff = difftime(currentTime,startTime);
	
	//printf("pid: %i time: %lf\n", pcb -> pID, diff);
	
}
