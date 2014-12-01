#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


/****************************************************
 *                                                  *
 *    This file contains the optional part of the   *
 *    assignment. The modification of partB.c was   *
 *    the ability for the user to transfer funds    *
 *    from one account to another, provided that    *
 *    both accounts existed in the database.        *
 *                                                  *
 *    File: partC.c                                 *
 *                                                  *
 *    Name: Jeton Sinoimeri                         *
 *    Name: Varun Sriram                            *
 *                                                  *
 *    Created: Nov 27, 2014                         *
 *                                                  *
 ****************************************************/



// struct for the accounts
struct account
{
    char accountNo[6];
    char pin[4];
    float funds;
    int numOfFailedAttempts;
};


// struct for the message to be passed between threads
struct alltypesmessage
{
    long message_type;
    char accountNo[6];
    char accountNo2[6];
    char pin[4];
    float funds;
    char message[100];
    
};


// struct for the arguments to be passed to the threads
struct args
{
	int toDBServer_message;
	int dbServerTOatm_message;
	int dbServerProcess;
};


// message queue keys
key_t toDBServer_key = (key_t)1235,
      dbServerTOatm_key = (key_t)1236;



// function prototypes
void *atm(void *arg);
void *dbEditor(void *arg);
void dbServer(struct account accounts[], int toDBServer_message, int dbServerTOatm_message, int atmProcess);

void encrypt(char x[], char encryption[]);
void decrypt(char x[], char decryption[]);
int checkPin(struct account accounts[], char accountNo[], char pin[]);

int search(struct account accounts[], char accountNo[]);
void writeFile(FILE *file, struct account accounts[]);



/*****************************************************
 *                                                   *
 *   Main function reponsible for initializing the   *
 *   database and running the program.               *
 *                                                   *
 *   Returns:                                        *
 *            0: for successful termination          *
 *                                                   *
 *            1: for termination with error          *
 *                                                   *
 *****************************************************/

int main (void)
{
    // create the database of accounts
    struct account accounts[3];
    strcpy(accounts[0].accountNo, "00001");
    strcpy(accounts[0].pin, "218");
    accounts[0].funds = 3443.22;
    accounts[0].numOfFailedAttempts = 0;

    strcpy(accounts[1].accountNo, "00011");
    strcpy(accounts[1].pin, "434");
    accounts[1].funds = 10089.97;
    accounts[1].numOfFailedAttempts = 0;

    strcpy(accounts[2].accountNo, "00117");
    strcpy(accounts[2].pin, "360");
    accounts[2].funds = 112.00;
    accounts[2].numOfFailedAttempts = 0;
      
      
    // declaration of atm and dbEditor thread variables  
    pthread_t atmThread,
              dbEditorThread;
    
    
    // create the message queues      
 		int toDBServer_message = msgget(toDBServer_key, IPC_CREAT|0600);
    int dbServerTOatm_message = msgget(dbServerTOatm_key, IPC_CREAT|0600);
    
    
    // check if toDBServer message queue failed to be created
    if (toDBServer_message == -1)
    {
        perror("toDBServer_message failed");
        exit(1);
    }
    
    
    // check if dbServerTOatm message queue failed to be created
    if (dbServerTOatm_message == -1)
    {
        perror("dbServerTOatm_message failed");
        exit(1);
    }
    
    
    else
    {
    	// variable for the choice that user will enter
    	int choice;
    	
    	// ask for user input
    	printf("Enter which operation you wish to do, 1 for ATM or 2 for db editor: ");
    	scanf("%i", &choice);
    	
    	
    	// create a struct of arguments to pass to the threads
    	struct args *arguments = (struct args*)malloc(sizeof(struct args));
    	arguments -> toDBServer_message = toDBServer_message;
    	arguments -> dbServerTOatm_message = dbServerTOatm_message;
    	arguments -> dbServerProcess = 1;
    	
    	
    	// create atm thread
    	if (choice == 1)
    		pthread_create(&atmThread, NULL, atm, (void *) arguments);
    	
    		
    	// create dbEditor thread	
    	else if (choice == 2)
    		pthread_create(&dbEditorThread, NULL, dbEditor, (void *) arguments);
    	
    	
    	// main process runs dbServer
      dbServer(accounts, toDBServer_message, dbServerTOatm_message, 2);

    }
    
    
    // remove the message queues
    msgctl(toDBServer_message, IPC_RMID, 0);
    msgctl(dbServerTOatm_message, IPC_RMID, 0);
    
    
    return 0;
}



/*********************************************************
 *                                                       *
 *   Searches through the array of accounts to           *
 *   find the account number.                            *
 *                                                       *
 *   parameter accounts[]: an array of account           *
 *                         structs representing          *
 *                         the database containing       *
 *                         account information.          *
 *                                                       *
 *   parameter accountNo[]: char array representing      *
 *                          the account number to be     *
 *                          searched in the accounts[]   *
 *                                                       *
 *   Returns:                                            *
 *             i: representing the index of where the    *
 *                account is in the accounts[] if it     *
 *                exists.                                *
 *                                                       *
 *            -1: if the account number does not exist   *
 *                                                       *
 *********************************************************/

int search(struct account accounts[], char accountNo[])
{
	  int i,
        size = sizeof(struct account) / sizeof(accounts) + 1;
    
    for(i = 0; i < size; i++)
    {
        if (!strcmp(accounts[i].accountNo, accountNo))
            return i;
    }
    
    return -1;
}



/*****************************************************
 *                                                   *
 *   Writes the account information to a file.       *
 *                                                   *
 *   parameter *file: FILE pointer representing      *
 *                    the address of the file to     *
 *                    be written.                    *
 *                                                   *
 *   parameter accounts[]: an array of account       *
 *                         structs representing      *
 *                         the database containing   *
 *                         account information.      *
 *                                                   *
 *****************************************************/


void writeFile(FILE *file, struct account accounts[])
{
    // open file
    file = fopen("db.txt","w");
    
    int i,
        size = sizeof(struct account) / sizeof(accounts) + 1;
    
    for(i = 0; i < size; i++)
        fprintf(file, "%s,%s,%f\n", accounts[i].accountNo, accounts[i].pin, accounts[i].funds);
    
    fclose(file);
}



/*********************************************************
 *                                                       *
 *   Checks that the pin provided be the user is         *
 *   correct.                                            *
 *                                                       *
 *   parameter accounts[]: an array of account           *
 *                         structs representing          *
 *                         the database containing       *
 *                         account information.          *
 *                                                       *
 *   parameter accountNo[]: char array representing      *
 *                          the account number to be     *
 *                          searched in the accounts[]   *
 *                                                       *
 *   parameter pin[]: char array representing the        *
 *                    user's pin                         *
 *                                                       *
 *   Returns:                                            *
 *            1: if the pin is correct                   *
 *                                                       *
 *            0: if the pin is incorrect                 *
 *                                                       *
 *           -1: if the account does not exist           *
 *                                                       *
 *********************************************************/

int checkPin(struct account accounts[], char accountNo[], char pin[])
{
    int index = search(accounts, accountNo);
    
    if (index != -1)
    {
        char encTest[4];
        
        decrypt(accounts[index].pin, encTest);

        if (!strcmp(encTest, pin))
            return 1;
            
        return 0;
    }
            
    return index;
}


/**************************************************************************
 *                                                                        *
 *   dbServer is responsible for receieving, processing information       *
 *   received from the atm and db editor as well as sending information   *
 *   back to the atm depending on the request receieved.                  *
 *                                                                        *
 *   parameter accounts[]: an array of account structs representing the   *
 *                         database containingaccount information.        *
 *                                                                        *
 *   parameter toDBServer_message: integer representing the id of the     *
 *                                 toDBServer message queue that the      *
 *                                 server receieves messages from         *
 *                                                                        *
 *   parameter dbServerTOatm_message: integer representing the id of      *
 *                                    the dbServerTOatm message queue     *
 *                                    that the server sends messages to   *
 *                                    the atm                             *
 *                                                                        *
 *   parameter atmProcess: integer representing the channel the atm is    *
 *                         listening to on the dbServerTOatm message      *
 *                         queue                                          *
 *                                                                        *
 **************************************************************************/

void dbServer(struct account accounts[], int toDBServer_message, int dbServerTOatm_message, int atmProcess)
{
	  // create and initialize the file pointer to null
    FILE *file = NULL;
    
    // declear the message struct
    struct alltypesmessage messages;
    
    while (1)
    {	
    	  // find the length of the message
        int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
        
        // attempt to receive messages and if an error occurs exit, otherwise continue
        if(msgrcv(toDBServer_message, &messages, msgLength, 1, 0) == -1)
        {
            perror("Server: receiving message failed.");
            exit(1);
        }
        
        
				// check what the message contains
        else
        {
            printf("Server: recieved message.\n");
            
						// if it is pin message
            if (!strcmp("PIN", messages.message))
            {
                // get the index of the account list
                int index = search(accounts, messages.accountNo);
                
                // temp char to see if it is a blocked account
                char temp[6];
                
                // store what was passed in to temp
                strcpy(temp,messages.accountNo);
                
                // replace the first char with X
                temp[0] = 'X';
               
                // if it is not a blocked
                if(search(accounts,temp) == -1)
                {
										// check the pin
                    int correctPin = checkPin(accounts, messages.accountNo, messages.pin);
                    
										// account does not exit
                    if (correctPin == -1)
                    {
                        printf("Server: account number does not exist\n");
                        strcpy(messages.message, "NOT OK");
                    }
                    
                    // incorrect pin
                    else if (correctPin == 0)
                    {
                    	  // check if account number exists
                        if(index != -1)
                        {
                        	  // check if account number is going to be blocked
                            if(accounts[index].numOfFailedAttempts==2)
                            {
                                accounts[index].accountNo[0] = 'X';
                                writeFile(file, accounts);

                                strcpy(messages.message, "Blocked");
                                messages.message_type = atmProcess;
                                
                                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                                
                                
                                // send the message to the atm saying its blocked
                                if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                                {
                                    perror("Server: sending message failed.");
                                    exit(1);
                                }

                                else
                                    printf("Server: Account Blocked!");
                            }
                            
                            // otherwise increment the number of failed attempts
                            else
                                accounts[index].numOfFailedAttempts++;

                        }

                        printf("Server: receieved incorrect Pin\n");
                        strcpy(messages.message, "NOT OK");
                    }
                    
										// correct pin
                    else if (correctPin == 1)
                    {
                        printf("Server: receieved correct Pin\n");
                        strcpy(messages.message, "OK");
                    }
                    
                    messages.message_type = atmProcess;
                    msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                    
                    if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                    {
                        perror("Server: sending message failed.");
                        exit(1);
                    }
                    
                    else
                        printf("Server sent: %s\n", messages.message);
                    
                }

                else
                    printf("Server: AccountBlocked\n");
                    
            }

						// if it is a withdraw message
            else if(!strcmp("WITHDRAW", messages.message))
            {
                int index = search(accounts, messages.accountNo);
                
                if (index != -1)
                {
                    if (accounts[index].funds - messages.funds >= 0)
                    {
                        accounts[index].funds -= messages.funds;
                        writeFile(file, accounts);
                        strcpy(messages.message, "Enough");
                    }
                    
                    else
                        strcpy(messages.message, "Not Enough");
                }
                
                else
                {
                    printf("Server: Account does not exist.\n");
                    strcpy(messages.message, "Funds not available");
                }
                
                messages.message_type = atmProcess;
                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                
                if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                {
                    perror("Server: sending message failed.");
                    exit(1);
                }
                
                else
                    printf("Server sent: %s\n", messages.message);
            }
            
						// if it is a request funds message
            else if(!strcmp("REQUEST FUNDS", messages.message))
            {
                int index = search(accounts, messages.accountNo);
                
                if (index != -1)
                {
                    messages.funds = accounts[index].funds;
                    strcpy(messages.message, "Funds available");
                }
                
                else
                {
                    printf("Server: Account does not exist.\n");
                    strcpy(messages.message, "Funds not available");
                }
                
                messages.message_type = atmProcess;
                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                
                if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
                {
                    perror("Server: sending message failed.");
                    exit(1);
                }
                
                else
                    printf("Server sent: %s\n", messages.message);
            }
            
            // if transfer funds request message
            else if(!strcmp("TRANSFER FUNDS", messages.message))
            {
            	// find the index of both accounts
            	int index = search(accounts, messages.accountNo);
            	int index2 = search(accounts, messages.accountNo2);
            	
            	// check if both accounts exist in database
            	if(index != -1 && index2 != -1)
            	{
            		// check funds
            		if (accounts[index].funds - messages.funds >= 0)
            		{
            			accounts[index].funds -= messages.funds;
            			accounts[index2].funds += messages.funds;
            			
            			strcpy(messages.message, "Transfer OK");
            			writeFile(file, accounts);
            		}
            			
            		else
            			strcpy(messages.message, "Transfer Not OK");
            	}
            	
            	else
            		strcpy(messages.message, "Transfer Not OK");
            		
            	
            	messages.message_type = atmProcess;
            	
            	msgLength = sizeof(struct alltypesmessage) - sizeof(long);
              if (msgsnd(dbServerTOatm_message, &messages, msgLength, 0) == -1)
              {
                  perror("Server: sending message failed.");
                  exit(1);
              }
              
              else
                printf("Server sent: %s\n", messages.message);
                
            }
            
						// if it is a update DB message
            else if (!strcmp("Update DB", messages.message))
            {
								// find the appropriate index of the account to update
                int index = search(accounts, messages.accountNo);
                
                if (index != -1)
                {
										// update the account
                    strcpy(accounts[index].accountNo, messages.accountNo);
                    
                    char encTest[3];
                    encrypt(messages.pin,encTest);
                    
                    strcpy(accounts[index].pin, encTest);
                    accounts[index].funds = messages.funds;
                    
										//save to file
                    writeFile(file, accounts);
                }
                
                else
                    printf("\nServer: Account Number does not exist.\n");
            }

            else if (!strcmp("EXIT", messages.message))
            {
                printf("Server: Exiting.\n");
                return;
            }
        }
    }
}


/*******************************************************
 *                                                     *
 *   Encrypts the pin the db editor has provided to    *
 *   be stored in the database.                        *
 *                                                     *
 *   parameter x[]: char array representing the        *
 *                  pin the db editor has provided     *
 *                                                     *
 *   parameter encryption[]: char array representing   *
 *                           the encrypted pin that    *
 *                           the dbserver will store   *
 *                                                     *
 *******************************************************/

void encrypt(char x[], char encryption[])
{    
    int i;
    
    for(i =0; i< 3; i++)
    {
        if(x[i]=='9')
          encryption[i]='0';
        
        else
            encryption[i] = x[i]+1;
    }
    
    encryption[3] = '\0';

}



/*******************************************************
 *                                                     *
 *   Decrypts the pin in the database to compare       *
 *   with the pin provided from atm                    *
 *                                                     *
 *   parameter x[]: char array representing the        *
 *                  pin that exists in the database    *
 *                                                     *
 *   parameter decryption[]: char array representing   *
 *                           the decrypted pin that    *
 *                           is stored in the          *
 *                           database                  *
 *                                                     *
 *******************************************************/

void decrypt(char x[], char decryption[])
{
    int i;
    
    for(i = 0; i < 3; i++)
    {
         if(x[i]=='0')
            decryption[i]='9';
            
        else
            decryption[i] = x[i]-1;
    }
    
    decryption[3] = '\0';
}


/**********************************************************
 *                                                        *
 *   DB editor is responsible for editing the database    *
 *   that stores the account information for a user.      *
 *                                                        *
 *   parameter *arg: void pointer to an argument struct   *
 *                   representing the arguments needed    *
 *                   by the db editor                     *
 *                                                        *
 **********************************************************/

void *dbEditor(void *arg)
{
    char acNo[6];
    char pin[4];
    float funds;
    
    struct args *args1 = (struct args *)arg;
    
    // get values from the arguments passed to pthread_create
    int toDBServer_message = args1 -> toDBServer_message;
    int dbServerProcess = args1 -> dbServerProcess;
    
    
    while (1)
    {
        struct alltypesmessage messages;
        messages.message_type = dbServerProcess;

        printf("Enter account number or X to quit: ");
        scanf("%s", acNo);
        
        // check if user wants to exit
        if(!strcmp(acNo,"X"))
        {
           strcpy(messages.message, "EXIT");
           int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
           
           msgsnd(toDBServer_message, &messages, msgLength, 0);
           
           printf("DBEditor: Exiting.\n");
           pthread_exit(NULL);
        }
        
        printf("Enter pin or X to quit: ");
        scanf("%s", pin);
        
        // check if user wants to exit
        if(!strcmp(pin,"X"))
        {
           strcpy(messages.message, "EXIT");
           int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
           
           msgsnd(toDBServer_message, &messages, msgLength, 0);
           
           printf("DBEditor: Exiting.\n");
           pthread_exit(NULL);
        }
        
        printf("Enter funds: ");
        scanf("%f", &funds);
				
				// create message to send to DBServer
        strncpy(messages.accountNo, acNo, 5);
        messages.accountNo[5] = '\0';
        
        strncpy(messages.pin, pin, 3);
        messages.pin[3] = '\0';
        
        messages.funds = funds;
        
        strcpy(messages.message, "Update DB");
        
        int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
        
        // attempt to send to DBServer
        if (msgsnd(toDBServer_message, &messages, msgLength, 0) == -1)
        {
            perror("DBEditor: sending message failed.");
            exit(1);
        }

        else
            printf("DBEditor: message sent to server\n");
    }
}


/**********************************************************
 *                                                        *
 *   Atm is responsible for asking the user information   *
 *   about the account, and the types of transactions     *
 *   they wish to perform. For each series of prompts     *
 *   the atm will send information to the server and      *
 *   receive information from the server indicating       *
 *   whether to proceed to the next prompt or restart     *
 *   from the beginning.                                  *
 *                                                        *
 *   parameter *arg: void pointer to an argument struct   *
 *                   representing the arguments needed    *
 *                   by the atm                           *
 *                                                        *
 **********************************************************/

void *atm(void *arg)
{
    char acNo[6];
    char pin[4];
    char request[1];
    float funds;
    
    
    struct args *args1 = (struct args *)arg;
    
    // get values from the arguments passed to pthread_create
    int toDBServer_message = args1 -> toDBServer_message;
    int dbServerTOatm_message = args1 -> dbServerTOatm_message;
    int dbServerProcess = args1 -> dbServerProcess;

    
    while (1)
    {
        struct alltypesmessage messages;
        messages.message_type = dbServerProcess;

				// ask user for input
        printf("Enter account number or X to exit: ");
        scanf("%s", acNo);
        
        // check if user wants to exit
        if(!strcmp(acNo,"X\0"))
        {
           strcpy(messages.message, "EXIT");
           int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
          
           msgsnd(toDBServer_message, &messages, msgLength, 0);
           
           printf("ATM: Exiting.\n");
           pthread_exit(NULL);
        }
        
        
        printf("Enter pin or X to exit: ");
        scanf("%s", pin);
        
        // check if user wants to exit
        if(!strcmp(pin,"X\0"))
        {
           strcpy(messages.message, "EXIT");
           int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
           
           msgsnd(toDBServer_message, &messages, msgLength, 0);
           
           printf("ATM: Exiting.\n");
           pthread_exit(NULL);
        }
        
				// create message to send to to dbServer
        strncpy(messages.accountNo, acNo, 5);
        messages.accountNo[5] = '\0';
        
        strncpy(messages.pin, pin, 3);
        messages.pin[3] = '\0';
        
        strcpy(messages.message, "PIN");
        
        
				// send message to dbServer
        int msgLength = sizeof(struct alltypesmessage) - sizeof(long);
        
        if (msgsnd(toDBServer_message, &messages, msgLength, 0) == -1)
        {
            perror("ATM: sending message failed.");
            exit(1);
        }
        
        else
            printf("ATM: message sent to server\n");
        
				// check to receive message
        if(msgrcv(dbServerTOatm_message, &messages, msgLength, 2, 0) == -1)
        {
            perror("ATM: receiving message failed.");
            exit(1);
        }
        
        else
        {
        	  printf("ATM received: %s\n", messages.message);
						
						// OK message received
            if (!strcmp("OK", messages.message))
            {
								// ask user for input such as withdraw or display funds
                printf("Enter a request(W for widthdraw, R for request funds, T for transfer funds): ");
                scanf("%s", request);
                
                // Withdraw request
                if (!strcmp("W", request))
                {
                    printf("Enter funds: ");
                    scanf("%f", &funds);
                    
                    messages.funds = funds;
                    
                    strcpy(messages.message, "WITHDRAW");
                }
                
                // funds requested
                else if (!strcmp("R", request))
                    strcpy(messages.message, "REQUEST FUNDS");
                
                // transfer requested
                else if (!strcmp("T", request))
                {
                		printf("Enter transfer account number: ");
                		scanf("%s", acNo);
                		
                		strcpy(messages.accountNo2, acNo);
                		messages.accountNo2[5] = '\0';
                		
                		printf("Enter funds: ");
                    scanf("%f", &funds);
         						
         						messages.funds = funds;
                	  strcpy(messages.message, "TRANSFER FUNDS");
                }
                
								// create message to send to dbServer
                messages.message_type = dbServerProcess;
                msgLength = sizeof(struct alltypesmessage) - sizeof(long);
                
                
                // send the message to dbServer
                if (msgsnd(toDBServer_message, &messages, msgLength, 0) == -1)
                {
                    perror("ATM: sending message failed.");
                    exit(1);
                }
            		
            		else
                {
                    printf("ATM: request message sent to server\n");
                    
										// check if there is anything in message queue
                    if(msgrcv(dbServerTOatm_message, &messages, msgLength, 2, 0) == -1)
                    {
                        perror("ATM: receiving message failed.");
                        exit(1);
                    }
                    
                    else
                    {
                    	  // funds available message
                        if (!strcmp(messages.message, "Funds available"))
                          printf("ATM: account no: %s funds: %f\n", messages.accountNo, messages.funds);
                        
                        // Withdraw successfull    
                        else if (!strcmp(messages.message, "Enough"))
                          printf("ATM: enough funds.\n");
                        
                        // Withdraw failed
                        else if(!strcmp(messages.message, "Not Enough"))
                          printf("ATM: not enough funds.\n");
                        
                        // Transfer successful
                        else if(!strcmp(messages.message, "Transfer OK"))
                        	printf("ATM: transfer complete.\n");
                        
                        // Transfer failed
                        else if (!strcmp(messages.message, "Transfer Not OK"))
                        	printf("ATM: transfer incomplete.\n");

                    }
                }
            }

            // Account blocked
            else if(!strcmp(messages.message, "Blocked"))
              printf("ATM: this account has been blocked access denied.\n");
            
            // incorrect pin or account number   
            else if(!strcmp(messages.message, "NOT OK"))
            	printf("ATM: either account does not exist or pin is incorrect.\n");
        }
    }
}
